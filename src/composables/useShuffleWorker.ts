import { onBeforeUnmount, ref } from 'vue';
import type {
  SetGridResult,
  ShuffleRequest,
  ShuffleResponse,
  ShuffleResult,
} from '@/workers/shuffle.protocol';

/** Worker 整體不可用（載入失敗、abort 崩潰或已銷毀）；呼叫端可據此顯示明確錯誤。 */
export class ShuffleWorkerUnavailableError extends Error {
  /** 底層原因（未加英文前綴，適合直接展示） */
  readonly reason: string;

  constructor(reason: string) {
    super(`Shuffle worker unavailable: ${reason}`);
    this.name = 'ShuffleWorkerUnavailableError';
    this.reason = reason;
  }
}

// 真實 Worker 與測試替身的最小共同介面
export interface ShuffleWorkerLike {
  postMessage(message: ShuffleRequest): void;
  terminate(): void;
  onmessage: ((event: MessageEvent<ShuffleResponse>) => void) | null;
  onerror?: ((event: ErrorEvent) => void) | null;
}

export interface UseShuffleWorkerOptions {
  createWorker?: () => ShuffleWorkerLike;
}

/** useGridShuffle 需要的最小後端介面 */
export interface ShuffleWorkerClient {
  setGrid(gridCsv: string): Promise<SetGridResult>;
  shuffle(configJson: string): Promise<ShuffleResult>;
}

type PendingEntry =
  | {
      type: 'setGrid';
      resolve: (result: SetGridResult) => void;
      reject: (reason: Error) => void;
    }
  | {
      type: 'shuffle';
      resolve: (result: ShuffleResult) => void;
      reject: (reason: Error) => void;
    };

export function useShuffleWorker(options: UseShuffleWorkerOptions = {}) {
  const isReady = ref(false);
  const fatalError = ref<string | null>(null);

  let worker: ShuffleWorkerLike | null = null;
  let nextRequestId = 1;
  const pending = new Map<number, PendingEntry>();

  const rejectAllPending = (error: Error) => {
    for (const entry of pending.values()) entry.reject(error);
    pending.clear();
  };

  const handleMessage = (event: MessageEvent<ShuffleResponse>) => {
    const msg = event.data;
    switch (msg.type) {
      case 'ready':
        isReady.value = true;
        return;
      case 'fatal':
        fatalError.value = msg.message;
        isReady.value = false;
        rejectAllPending(new ShuffleWorkerUnavailableError(msg.message));
        return;
      case 'setGridResult': {
        const entry = pending.get(msg.requestId);
        if (!entry) return;
        pending.delete(msg.requestId);
        if (entry.type === 'setGrid') entry.resolve({ ok: msg.ok, reason: msg.reason });
        else entry.reject(new Error('Unexpected setGridResult response'));
        return;
      }
      case 'shuffleResult': {
        const entry = pending.get(msg.requestId);
        if (!entry) return;
        pending.delete(msg.requestId);
        if (entry.type !== 'shuffle') {
          entry.reject(new Error('Unexpected shuffleResult response'));
          return;
        }
        if (msg.success) {
          entry.resolve({ success: true, gridCsv: msg.gridCsv, report: msg.report });
        } else {
          entry.resolve({ success: false, error: msg.error });
        }
        return;
      }
    }
  };

  const handleWorkerError = (event: ErrorEvent) => {
    const message = event.message || 'Worker failed to load';
    fatalError.value = message;
    isReady.value = false;
    rejectAllPending(new ShuffleWorkerUnavailableError(message));
  };

  const createWorker =
    options.createWorker ??
    (() =>
      new Worker(new URL('../workers/shuffle.worker.ts', import.meta.url), { type: 'module' }));

  const getWorker = (): ShuffleWorkerLike => {
    if (!worker) {
      worker = createWorker();
      worker.onmessage = handleMessage;
      worker.onerror = handleWorkerError;
    }
    return worker;
  };

  const registerPending = (requestId: number, entry: PendingEntry): boolean => {
    const fatal = fatalError.value;
    if (fatal !== null) {
      entry.reject(new ShuffleWorkerUnavailableError(fatal));
      return false;
    }
    pending.set(requestId, entry);
    return true;
  };

  const setGrid = (gridCsv: string): Promise<SetGridResult> => {
    const requestId = nextRequestId++;
    return new Promise<SetGridResult>((resolve, reject) => {
      if (!registerPending(requestId, { type: 'setGrid', resolve, reject })) return;
      getWorker().postMessage({ type: 'setGrid', requestId, gridCsv });
    });
  };

  const shuffle = (configJson: string): Promise<ShuffleResult> => {
    const requestId = nextRequestId++;
    return new Promise<ShuffleResult>((resolve, reject) => {
      if (!registerPending(requestId, { type: 'shuffle', resolve, reject })) return;
      getWorker().postMessage({ type: 'shuffle', requestId, configJson });
    });
  };

  const dispose = () => {
    rejectAllPending(new ShuffleWorkerUnavailableError('disposed'));
    worker?.terminate();
    worker = null;
    isReady.value = false;
  };

  getWorker();

  onBeforeUnmount(dispose);

  return {
    isReady,
    fatalError,
    setGrid,
    shuffle,
    dispose,
  };
}
