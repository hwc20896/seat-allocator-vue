import initWasmModule from '@/assets/wasm/alloc_algo.js';
import type { Grid, GridShuffler, MainModule } from '@/assets/wasm/alloc_algo';
import { buildWasmConfigFromJson } from '@/utils/wasmConfig.ts';
import type { ShuffleRequest, ShuffleResponse } from './shuffle.protocol';

//   In DOM, `self` is of type `Window` (incompatible with `postMessage` signatures); assert using a narrow interface.
const scope = self as unknown as {
  onmessage: ((event: MessageEvent<ShuffleRequest>) => void) | null;
  postMessage: (message: ShuffleResponse) => void;
};

let wasmModule: MainModule | null = null;
let shuffler: GridShuffler | null = null;

const bootstrap = async (): Promise<void> => {
  try {
    wasmModule = await initWasmModule({
      locateFile: (path: string) =>
        path.endsWith('.wasm') ? `${import.meta.env.BASE_URL}${path}` : path,
    });
    scope.postMessage({ type: 'ready' });
  } catch (error) {
    console.error('WebAssembly 模組載入失敗 (worker)：', error);
    scope.postMessage({
      type: 'fatal',
      message: error instanceof Error ? error.message : String(error),
    });
  }
};

const readyPromise = bootstrap();

//  Emscripten abort (e.g. OOM) leaves the WASM runtime unusable;
//  report it as fatal so the main thread can stop issuing requests.
const postFatalIfAborted = (error: unknown): boolean => {
  if (!(error instanceof Error) || !error.message.startsWith('Aborted')) return false;
  scope.postMessage({ type: 'fatal', message: error.message });
  return true;
};

const handleSetGrid = (requestId: number, gridCsv: string): void => {
  if (!wasmModule) {
    scope.postMessage({ type: 'setGridResult', requestId, ok: false, reason: 'not-ready' });
    return;
  }

  let grid: Grid | null = null;
  let aborted = false;
  try {
    grid = wasmModule.Grid.fromCSV(gridCsv);
    if (!shuffler) {
      shuffler = new wasmModule.GridShuffler();
    }
    //  In C++, `setGrid` performs a copy-by-value, and the handle is released in the `finally` block.
    const ok = shuffler.setGrid(grid);
    scope.postMessage({ type: 'setGridResult', requestId, ok });
  } catch (error) {
    console.error('setGrid failed:', error);
    aborted = postFatalIfAborted(error);
    if (!aborted) {
      scope.postMessage({
        type: 'setGridResult',
        requestId,
        ok: false,
        reason: error instanceof Error ? error.message : String(error),
      });
    }
  } finally {
    //  釋放 embind 句柄；abort 後 runtime 已不可用，跳過以免二次拋錯
    if (!aborted) grid?.delete();
  }
};

const handleShuffle = (requestId: number, configJson: string): void => {
  if (!wasmModule || !shuffler) {
    //  尚未 setGrid 或模組未就緒（NotReady；EmptyGrid 專指算法收到的網格為空）
    scope.postMessage({ type: 'shuffleResult', requestId, success: false, error: 'NotReady' });
    return;
  }

  const cfg = buildWasmConfigFromJson(wasmModule, configJson);
  if (!cfg) {
    //  config JSON 無法構建成 wasm ShuffleConfig
    scope.postMessage({ type: 'shuffleResult', requestId, success: false, error: 'InvalidConfig' });
    return;
  }

  let aborted = false;
  try {
    shuffler.setConfig(cfg);
    const report = shuffler.shuffle();

    if (!report.success) {
      scope.postMessage({ type: 'shuffleResult', requestId, success: false, error: report.error });
      return;
    }

    scope.postMessage({
      type: 'shuffleResult',
      requestId,
      success: true,
      gridCsv: shuffler.getGrid().toCSVString(),
      report: {
        success: true,
        doneAtAttempt: report.doneAtAttempt,
        doneAtStep: report.doneAtStep,
        tookMUS: report.tookMUS,
        error: report.error,
      },
    });
  } catch (error) {
    console.error('shuffle failed:', error);
    aborted = postFatalIfAborted(error);
    if (!aborted) {
      scope.postMessage({
        type: 'shuffleResult',
        requestId,
        success: false,
        error: 'InternalError',
      });
    }
  } finally {
    //  釋放 embind 句柄；abort 後 runtime 已不可用，跳過以免二次拋錯
    if (!aborted) cfg.delete();
  }
};

const handleRequest = async (request: ShuffleRequest) => {
  await readyPromise;
  switch (request.type) {
    case 'setGrid':
      handleSetGrid(request.requestId, request.gridCsv);
      break;
    case 'shuffle':
      handleShuffle(request.requestId, request.configJson);
      break;
  }
};

//  Serial queue: Asyncify does not allow re-entry into WASM calls during the yield period;
//  Also ensures that setGrid and shuffle, are executed strictly in the order sent by the main thread.
let queue: Promise<void> = readyPromise;

scope.onmessage = (event: MessageEvent<ShuffleRequest>) => {
  queue = queue
    .then(() => handleRequest(event.data))
    .catch((error) => console.error('shuffle.worker request failed:', error));
};
