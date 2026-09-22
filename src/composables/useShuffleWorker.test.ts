import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { defineComponent } from 'vue';
import { mount } from '@vue/test-utils';
import {
  ShuffleWorkerUnavailableError,
  useShuffleWorker,
  type ShuffleWorkerLike,
} from './useShuffleWorker';
import type { ShuffleReport, ShuffleRequest, ShuffleResponse } from '@/workers/shuffle.protocol';

class FakeWorker implements ShuffleWorkerLike {
  postMessage = vi.fn<(message: ShuffleRequest) => void>();
  terminate = vi.fn();
  onmessage: ((event: MessageEvent<ShuffleResponse>) => void) | null = null;
  onerror: ((event: ErrorEvent) => void) | null = null;

  emit(message: ShuffleResponse): void {
    this.onmessage?.({ data: message } as MessageEvent<ShuffleResponse>);
  }

  emitError(message: string): void {
    this.onerror?.({ message } as ErrorEvent);
  }
}

const makeReport = (): ShuffleReport => ({
  success: true,
  doneAtAttempt: 0,
  doneAtStep: 0,
  tookMUS: 100,
  error: 'Unknown',
});

const setup = () => {
  const fakeWorker = new FakeWorker();
  const api = useShuffleWorker({ createWorker: () => fakeWorker });
  return { fakeWorker, ...api };
};

beforeEach(() => {
  // useShuffleWorker 註冊 onBeforeUnmount；無元件上下文時 Vue 會警告，此處靜音
  vi.spyOn(console, 'warn').mockImplementation(() => {});
});

afterEach(() => {
  vi.restoreAllMocks();
});

describe('useShuffleWorker', () => {
  it('建構時即建立 Worker', () => {
    const createWorker = vi.fn(() => new FakeWorker());
    useShuffleWorker({ createWorker });
    expect(createWorker).toHaveBeenCalledTimes(1);
  });

  it('ready 訊息後 isReady 為 true', () => {
    const { fakeWorker, isReady } = setup();
    expect(isReady.value).toBe(false);
    fakeWorker.emit({ type: 'ready' });
    expect(isReady.value).toBe(true);
  });

  it('setGrid 發送請求並以 requestId 配對回應', async () => {
    const { fakeWorker, setGrid } = setup();
    const promise = setGrid('A,B\nC,D\n');

    expect(fakeWorker.postMessage).toHaveBeenCalledWith({
      type: 'setGrid',
      requestId: 1,
      gridCsv: 'A,B\nC,D\n',
    });

    fakeWorker.emit({ type: 'setGridResult', requestId: 1, ok: true });
    await expect(promise).resolves.toEqual({ ok: true });
  });

  it('setGrid 失敗時回傳 reason', async () => {
    const { fakeWorker, setGrid } = setup();
    const promise = setGrid('A,B');
    fakeWorker.emit({ type: 'setGridResult', requestId: 1, ok: false, reason: 'boom' });
    await expect(promise).resolves.toEqual({ ok: false, reason: 'boom' });
  });

  it('shuffle 成功時回傳 gridCsv 與 report', async () => {
    const { fakeWorker, shuffle } = setup();
    const report = makeReport();
    const promise = shuffle('{}');

    expect(fakeWorker.postMessage).toHaveBeenCalledWith({
      type: 'shuffle',
      requestId: 1,
      configJson: '{}',
    });

    fakeWorker.emit({ type: 'shuffleResult', requestId: 1, success: true, gridCsv: 'X,Y', report });
    await expect(promise).resolves.toEqual({ success: true, gridCsv: 'X,Y', report });
  });

  it('shuffle 失敗時回傳 error', async () => {
    const { fakeWorker, shuffle } = setup();
    const promise = shuffle('{}');
    fakeWorker.emit({
      type: 'shuffleResult',
      requestId: 1,
      success: false,
      error: 'Unsatisfiable',
    });
    await expect(promise).resolves.toEqual({ success: false, error: 'Unsatisfiable' });
  });

  it('多個請求依序遞增 requestId 且互不干擾', async () => {
    const { fakeWorker, setGrid, shuffle } = setup();
    const first = setGrid('A,B');
    const second = shuffle('{}');

    expect(fakeWorker.postMessage).toHaveBeenNthCalledWith(1, {
      type: 'setGrid',
      requestId: 1,
      gridCsv: 'A,B',
    });
    expect(fakeWorker.postMessage).toHaveBeenNthCalledWith(2, {
      type: 'shuffle',
      requestId: 2,
      configJson: '{}',
    });

    // 刻意先回第二個，確認配對不依賴回應順序
    fakeWorker.emit({ type: 'shuffleResult', requestId: 2, success: false, error: 'NotReady' });
    fakeWorker.emit({ type: 'setGridResult', requestId: 1, ok: true });

    await expect(first).resolves.toEqual({ ok: true });
    await expect(second).resolves.toEqual({ success: false, error: 'NotReady' });
  });

  it('fatal 訊息會 reject 進行中的請求，且後續請求直接失敗', async () => {
    const { fakeWorker, setGrid, fatalError, isReady } = setup();
    fakeWorker.emit({ type: 'ready' });
    const pending = setGrid('A,B');

    fakeWorker.emit({ type: 'fatal', message: 'wasm load failed' });

    const error = await pending.catch((e: unknown) => e);
    expect(error).toBeInstanceOf(ShuffleWorkerUnavailableError);
    expect((error as ShuffleWorkerUnavailableError).reason).toBe('wasm load failed');
    expect(fatalError.value).toBe('wasm load failed');
    expect(isReady.value).toBe(false);
    await expect(setGrid('C,D')).rejects.toBeInstanceOf(ShuffleWorkerUnavailableError);
  });

  it('worker onerror 時設定 fatalError', () => {
    const { fakeWorker, fatalError } = setup();
    fakeWorker.emitError('script load failed');
    expect(fatalError.value).toBe('script load failed');
  });

  it('dispose 會 terminate 並 reject 進行中的請求', async () => {
    const { fakeWorker, setGrid, dispose, isReady } = setup();
    fakeWorker.emit({ type: 'ready' });
    const pending = setGrid('A,B');

    dispose();

    expect(fakeWorker.terminate).toHaveBeenCalledTimes(1);
    await expect(pending).rejects.toThrow('disposed');
    expect(isReady.value).toBe(false);
  });

  it('未知 requestId 的回應被忽略', async () => {
    const { fakeWorker, setGrid } = setup();
    const promise = setGrid('A,B');

    fakeWorker.emit({ type: 'setGridResult', requestId: 999, ok: false, reason: 'stale' });
    fakeWorker.emit({ type: 'setGridResult', requestId: 1, ok: true });

    await expect(promise).resolves.toEqual({ ok: true });
  });

  it('元件 unmount 時自動 terminate', () => {
    const fakeWorker = new FakeWorker();
    const TestComp = defineComponent({
      setup() {
        useShuffleWorker({ createWorker: () => fakeWorker });
        return () => null;
      },
    });
    const wrapper = mount(TestComp);
    wrapper.unmount();
    expect(fakeWorker.terminate).toHaveBeenCalled();
  });
});
