import { afterEach, describe, expect, it, vi } from 'vitest';
import { nextTick, ref, shallowRef } from 'vue';
import { Position } from '@/utils/Position.ts';
import { SHUFFLE_WORKER_ERROR_MESSAGES } from '@/utils/shuffleError.ts';
import { useGridShuffle } from './useGridShuffle';
import type { MainModule, ShuffleReport } from '@/assets/wasm/alloc_algo';
import type { SetGridResult, ShuffleResult } from '@/workers/shuffle.protocol';
import type { ShuffleWorkerClient } from './useShuffleWorker';
import { ShuffleWorkerUnavailableError } from './useShuffleWorker';
import { FakeGrid } from '@/utils/__tests__/fakeGrid';

const makeReport = (): ShuffleReport => ({
  success: true,
  doneAtAttempt: 0,
  doneAtStep: 0,
  tookMUS: 100,
  error: 'Unknown',
});

class FakeShuffleWorker implements ShuffleWorkerClient {
  setGrid = vi.fn(async (): Promise<SetGridResult> => ({ ok: true }));
  shuffle = vi.fn(
    async (): Promise<ShuffleResult> => ({
      success: true,
      gridCsv: new FakeGrid(2, 2, ['a', 'b', 'c', 'd']).toCSVString(),
      report: makeReport(),
    }),
  );
}

const makeFakeModule = () => ({
  Grid: FakeGrid,
  ShuffleConfig: class ShuffleConfig {
    delete = vi.fn();
  },
});

const fakeModule = makeFakeModule();

const setup = (getConfigJson?: () => string) => {
  const wasmModule = shallowRef<MainModule | null>(null);
  const wasmReady = ref(false);
  const shuffleWorker = new FakeShuffleWorker();
  const api = useGridShuffle(wasmModule, wasmReady, shuffleWorker, getConfigJson);
  return { wasmModule, wasmReady, shuffleWorker, ...api };
};

const makeGrid = () => new FakeGrid(2, 2, ['A1', 'A2', 'B1', 'B2']);

/** 在假計時器下完整跑一次洗牌動畫 */
const runShuffle = async (ctx: ReturnType<typeof setup>, advance = 20_000) => {
  const promise = ctx.beginShuffleAnimation();
  await vi.advanceTimersByTimeAsync(advance);
  return promise;
};

afterEach(() => {
  vi.restoreAllMocks();
});

describe('useGridShuffle', () => {
  it('wasmReady 變 true 時建立空 Grid', async () => {
    const ctx = setup();
    expect(ctx.isGridLoaded.value).toBe(false);
    ctx.wasmModule.value = fakeModule as never;
    ctx.wasmReady.value = true;
    await nextTick();
    expect(ctx.originalGrid.value).toBeInstanceOf(FakeGrid);
    expect(ctx.currentGrid.value).toBeInstanceOf(FakeGrid);
    expect(ctx.isGridLoaded.value).toBe(false);
  });

  it('pageLabel 依狀態顯示', async () => {
    const ctx = setup();
    expect(ctx.pageLabel.value).toBe('未導入');
    ctx.wasmModule.value = fakeModule as never;
    ctx.wasmReady.value = true;
    await nextTick();
    await ctx.loadNewGrid(makeGrid());
    expect(ctx.pageLabel.value).toBe('原始列表');
    ctx.toggleOriginal();
    expect(ctx.pageLabel.value).toBe('第 0 次分配');
    ctx.isShuffling.value = true;
    expect(ctx.pageLabel.value).toBe('正在洗牌中... (第 1 次)');
  });

  it('loadNewGrid 成功時設定 grid、傳送 CSV 並重設狀態', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    const grid = makeGrid();
    expect(await ctx.loadNewGrid(grid)).toBe(true);
    expect(ctx.originalGrid.value).toBe(grid);
    expect(ctx.currentGrid.value).toBe(grid);
    expect(ctx.showOriginal.value).toBe(true);
    expect(ctx.totalPages.value).toBe(0);
    expect(ctx.currentIndex.value).toBe(0);
    expect(ctx.shuffleWorker.setGrid).toHaveBeenCalledWith(grid.toCSVString());
    expect(alertSpy).not.toHaveBeenCalled();
  });

  it('setGrid 回傳 false 時 alert 並回傳 false', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.shuffleWorker.setGrid.mockResolvedValue({ ok: false, reason: 'bad' });
    expect(await ctx.loadNewGrid(makeGrid())).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('洗牌引擎無法載入此座位表。');
    expect(ctx.originalGrid.value).toBeNull();
  });

  it('loadNewGrid 拋例外時 alert 導入失敗', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.shuffleWorker.setGrid.mockRejectedValue(new Error('boom'));
    expect(await ctx.loadNewGrid(makeGrid())).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('導入配置失敗，檔案可能含有重複元素。');
  });

  it('loadNewGrid 遇引擎不可用時顯示引擎真實原因', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.shuffleWorker.setGrid.mockRejectedValue(new ShuffleWorkerUnavailableError('Aborted(OOM)'));
    expect(await ctx.loadNewGrid(makeGrid())).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('洗牌引擎無法使用，請重新載入頁面。\n原因：Aborted(OOM)');
  });

  it('併發 loadNewGrid 時過期的結果被丟棄', async () => {
    let resolveFirst!: (value: SetGridResult) => void;
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.shuffleWorker.setGrid.mockImplementationOnce(
      () =>
        new Promise<SetGridResult>((resolve) => {
          resolveFirst = resolve;
        }),
    );

    const first = ctx.loadNewGrid(makeGrid());
    const second = ctx.loadNewGrid(makeGrid());
    resolveFirst({ ok: true });

    expect(await first).toBe(false);
    expect(await second).toBe(true);
  });

  it('wasm 未就緒時 loadNewGrid 回傳 false', async () => {
    const ctx = setup();
    expect(await ctx.loadNewGrid(makeGrid())).toBe(false);
  });

  it('navigatePage 越界時不移動', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    await ctx.loadNewGrid(makeGrid());
    ctx.navigatePage(1);
    expect(ctx.currentIndex.value).toBe(0);
    ctx.navigatePage(-1);
    expect(ctx.currentIndex.value).toBe(0);
  });

  it('navigatePage 有手動修改時優先使用', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());

      await runShuffle(ctx);
      await runShuffle(ctx);
      expect(ctx.totalPages.value).toBe(2);
      expect(ctx.currentIndex.value).toBe(2);

      ctx.swapCells(new Position(0, 0), new Position(0, 1));
      expect(ctx.getCellAt(new Position(0, 0))).toBe('b');

      ctx.navigatePage(-1);
      expect(ctx.currentIndex.value).toBe(1);
      expect(ctx.getCellAt(new Position(0, 0))).toBe('a');

      ctx.navigatePage(1);
      expect(ctx.currentIndex.value).toBe(2);
      expect(ctx.getCellAt(new Position(0, 0))).toBe('b');
    } finally {
      vi.useRealTimers();
    }
  });

  it('loadNewGrid 會釋放舊的結果 grid', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());

      const fromCsvSpy = vi.spyOn(FakeGrid, 'fromCSV');
      await runShuffle(ctx);

      const created = fromCsvSpy.mock.results[0]?.value as FakeGrid;
      const deleteSpy = vi.spyOn(created, 'delete');

      await ctx.loadNewGrid(makeGrid());
      expect(deleteSpy).toHaveBeenCalled();
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 成功時更新 currentGrid 並結束', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(20_000);
      expect(await promise).toBe(true);
      expect(ctx.isShuffling.value).toBe(false);
      expect(ctx.currentGrid.value).toBeInstanceOf(FakeGrid);
      expect(ctx.totalPages.value).toBe(1);
      expect(ctx.currentIndex.value).toBe(1);
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 傳入最新 configJson', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup(() => '{"constraints":[]}');
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      expect(await runShuffle(ctx)).toBe(true);
      expect(ctx.shuffleWorker.shuffle).toHaveBeenCalledWith('{"constraints":[]}');
    } finally {
      vi.useRealTimers();
    }
  });

  it('getConfigJson factory 拋錯時 warn 並改用預設 config', async () => {
    vi.useFakeTimers();
    try {
      const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
      const ctx = setup(() => {
        throw new Error('factory boom');
      });
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      expect(await runShuffle(ctx)).toBe(true);
      expect(warnSpy).toHaveBeenCalledWith('getConfigJson factory threw', expect.any(Error));
      expect(ctx.shuffleWorker.shuffle).toHaveBeenCalledWith('{}');
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 遇 worker 錯誤碼 NotReady 時顯示對應提示', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      ctx.shuffleWorker.shuffle.mockResolvedValue({ success: false, error: 'NotReady' });

      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith(SHUFFLE_WORKER_ERROR_MESSAGES.NotReady);
      expect(ctx.isShuffling.value).toBe(false);
      expect(ctx.totalPages.value).toBe(0);
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 回傳 Unsatisfiable 時顯示對應提示', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      ctx.shuffleWorker.shuffle.mockResolvedValue({ success: false, error: 'Unsatisfiable' });

      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith(expect.stringContaining('約束互相衝突'));
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 拋例外時 alert 並還原 grid', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      ctx.shuffleWorker.shuffle.mockRejectedValue(new Error('boom'));

      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith('洗牌算法解決失敗！請檢查約束是否互相衝突。');
      expect(ctx.currentGrid.value).toBe(ctx.originalGrid.value);
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 遇引擎不可用時顯示引擎真實原因', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      ctx.shuffleWorker.shuffle.mockRejectedValue(
        new ShuffleWorkerUnavailableError('Aborted(OOM)'),
      );

      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith(
        '洗牌引擎無法使用，請重新載入頁面。\n原因：Aborted(OOM)',
      );
    } finally {
      vi.useRealTimers();
    }
  });

  it('isShuffling 時 beginShuffleAnimation 不重入', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.isShuffling.value = true;
    expect(await ctx.beginShuffleAnimation()).toBe(false);
  });

  it('動畫期間載入新 grid 會丟棄舊結果', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());

      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(10);
      await ctx.loadNewGrid(makeGrid());
      await vi.advanceTimersByTimeAsync(30_000);

      expect(await promise).toBe(false);
      expect(ctx.isShuffling.value).toBe(false);
      expect(ctx.totalPages.value).toBe(0);
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 含空格 grid 時跳過空格', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(new FakeGrid(3, 3, ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', '']));
      expect(await runShuffle(ctx)).toBe(true);
    } finally {
      vi.useRealTimers();
    }
  });

  it('currentGrid 為 null 時 swapCells 記錄錯誤', () => {
    const errorSpy = vi.spyOn(console, 'error').mockImplementation(() => {});
    const ctx = setup();
    ctx.showOriginal.value = false;
    ctx.swapCells(new Position(0, 0), new Position(0, 1));
    expect(errorSpy).toHaveBeenCalledWith('currentGrid.value is null. cannot swap elements.');
  });

  it('isCellManuallyModified 依 pristine 差異判斷', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      expect(await runShuffle(ctx)).toBe(true);

      expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);

      ctx.swapCells(new Position(0, 0), new Position(0, 1));
      expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(true);
    } finally {
      vi.useRealTimers();
    }
  });

  it('showOriginal 或 index 0 時 isCellManuallyModified 為 false', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    await ctx.loadNewGrid(makeGrid());
    ctx.showOriginal.value = true;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
    ctx.showOriginal.value = false;
    ctx.currentIndex.value = 0;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
  });

  it('無對應歷史時 isCellManuallyModified 回傳 false', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    await ctx.loadNewGrid(makeGrid());
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.showOriginal.value = false;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
  });

  it('navigatePage 無手動修改且無歷史時不動 currentGrid', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    await ctx.loadNewGrid(makeGrid());
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.navigatePage(1);
    expect(ctx.currentIndex.value).toBe(4);
    expect(ctx.currentGrid.value).toBe(ctx.originalGrid.value);
  });

  it('getCellAt 在無 grid 時回傳空字串', () => {
    const ctx = setup();
    expect(ctx.getCellAt(new Position(0, 0))).toBe('');
  });

  it('applyConfig 在 wasm 未就緒時 alert', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    expect(await ctx.applyConfig()).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('WebAssembly 模組未就緒，無法套用約束。');
  });

  it('applyConfig 無 configJson 且無 factory 時回傳 false', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    expect(await ctx.applyConfig()).toBe(false);
  });

  it('applyConfig 以 configJson 構建並釋放 cfg', async () => {
    const deleteSpy = vi.fn();
    class FakeShuffleConfig {
      delete = deleteSpy;
    }
    const ctx = setup();
    ctx.wasmModule.value = { Grid: FakeGrid, ShuffleConfig: FakeShuffleConfig } as never;
    expect(await ctx.applyConfig('{}')).toBe(true);
    expect(deleteSpy).toHaveBeenCalledTimes(1);
  });

  it('applyConfig 已有結果時提示重新洗牌', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      await ctx.loadNewGrid(makeGrid());
      expect(await runShuffle(ctx)).toBe(true);

      expect(await ctx.applyConfig('{}')).toBe(true);
      expect(alertSpy).toHaveBeenCalledWith(expect.stringContaining('建議重新洗牌'));
    } finally {
      vi.useRealTimers();
    }
  });

  it('applyConfig 構建失敗時 alert 套用失敗', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    class ThrowingShuffleConfig {
      constructor() {
        throw new Error('x');
      }
    }
    const ctx = setup();
    ctx.wasmModule.value = { Grid: FakeGrid, ShuffleConfig: ThrowingShuffleConfig } as never;
    expect(await ctx.applyConfig('{}')).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('套用約束失敗。');
  });

  it('applyConfig 使用 getConfigJson factory 的結果', async () => {
    const forceRowSpy = vi.fn();
    class FakeShuffleConfig {
      forceRow = forceRowSpy;
      delete = vi.fn();
    }
    const ctx = setup(() =>
      JSON.stringify({ constraints: [{ type: 'FORCEROW', name: 'A', rowIdx: 0 }] }),
    );
    ctx.wasmModule.value = { Grid: FakeGrid, ShuffleConfig: FakeShuffleConfig } as never;
    expect(await ctx.applyConfig()).toBe(true);
    expect(forceRowSpy).toHaveBeenCalledWith('A', 0);
  });
});
