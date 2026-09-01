import { afterEach, describe, expect, it, vi } from 'vitest';
import { nextTick, ref, shallowRef } from 'vue';
import { Position } from '@/utils/Position.ts';
import { useGridShuffle } from './useGridShuffle';
import type { GridShuffler, MainModule, ShuffleConfig } from '@/assets/wasm/alloc_algo';
import { FakeGrid } from '@/utils/__tests__/fakeGrid';

class FakeGridShuffler {
  setConfig = vi.fn();
  setGrid = vi.fn(() => true);
  shuffle = vi.fn(async () => ({ success: true, error: '', data: { tookMUS: 100 } }));
  getGrid = vi.fn(() => new FakeGrid(2, 2, ['a', 'b', 'c', 'd']));
  getShuffledGridCount = vi.fn(() => 0);
  getGridAt = vi.fn(() => new FakeGrid(2, 2, ['a', 'b', 'c', 'd']));
  delete = vi.fn();
}

class FailingSetGridShuffler extends FakeGridShuffler {
  setGrid = vi.fn(() => false);
}

class ThrowingSetGridShuffler extends FakeGridShuffler {
  setGrid = vi.fn(() => {
    throw new Error('boom');
  });
}

class ThrowingSetConfigShuffler extends FakeGridShuffler {
  setConfig = vi.fn(() => {
    throw new Error('x');
  });
}

const makeFakeModule = (shufflerClass: typeof FakeGridShuffler = FakeGridShuffler) => ({
  Grid: FakeGrid,
  GridShuffler: shufflerClass,
  ShuffleConfig: class ShuffleConfig {},
});

const fakeModule = makeFakeModule();

const setup = (getShuffleConfig?: () => ShuffleConfig | null) => {
  const wasmModule = shallowRef<MainModule | null>(null);
  const wasmReady = ref(false);
  const shufflerInstance = shallowRef<GridShuffler | null>(null);
  const api = useGridShuffle(wasmModule, wasmReady, shufflerInstance, getShuffleConfig);
  return { wasmModule, wasmReady, shufflerInstance, ...api };
};

const makeGrid = () => new FakeGrid(2, 2, ['A1', 'A2', 'B1', 'B2']);

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
    ctx.loadNewGrid(makeGrid());
    expect(ctx.pageLabel.value).toBe('原始列表');
    ctx.toggleOriginal();
    expect(ctx.pageLabel.value).toBe('第 0 次分配');
    ctx.isShuffling.value = true;
    expect(ctx.pageLabel.value).toBe('正在洗牌中... (第 1 次)');
  });

  it('loadNewGrid 成功時設定 grid 並重設狀態', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    const grid = makeGrid();
    expect(ctx.loadNewGrid(grid)).toBe(true);
    expect(ctx.originalGrid.value).toBe(grid);
    expect(ctx.currentGrid.value).toBe(grid);
    expect(ctx.showOriginal.value).toBe(true);
    expect(ctx.totalPages.value).toBe(0);
    expect(ctx.currentIndex.value).toBe(0);
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    expect(shuffler.setConfig).toHaveBeenCalled();
    expect(shuffler.setGrid).toHaveBeenCalledWith(grid);
    expect(alertSpy).not.toHaveBeenCalled();
  });

  it('setGrid 回傳 false 時 alert 並回傳 false', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = makeFakeModule(FailingSetGridShuffler) as never;
    expect(ctx.loadNewGrid(makeGrid())).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('C++ shuffler failed to parse grid dimensions.');
  });

  it('loadNewGrid 拋例外時 alert 導入失敗', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = makeFakeModule(ThrowingSetGridShuffler) as never;
    expect(ctx.loadNewGrid(makeGrid())).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('導入配置失敗，檔案可能含有重複元素。');
  });

  it('getShuffleConfig factory 拋錯時 warn 並改用預設 config', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const ctx = setup(() => {
      throw new Error('factory boom');
    });
    ctx.wasmModule.value = fakeModule as never;
    expect(ctx.loadNewGrid(makeGrid())).toBe(true);
    expect(warnSpy).toHaveBeenCalledWith('getShuffleConfig factory threw', expect.any(Error));
  });

  it('wasm 未就緒時 loadNewGrid 回傳 false', () => {
    const ctx = setup();
    expect(ctx.loadNewGrid(makeGrid())).toBe(false);
  });

  it('navigatePage 越界時不移動', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    ctx.navigatePage(1);
    expect(ctx.currentIndex.value).toBe(0);
    ctx.navigatePage(-1);
    expect(ctx.currentIndex.value).toBe(0);
  });

  it('navigatePage 有手動修改時優先使用', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    shuffler.getGridAt.mockReturnValue(makeGrid());
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.showOriginal.value = false;
    ctx.swapCells(new Position(0, 0), new Position(0, 1));
    expect(ctx.getCellAt(new Position(0, 0))).toBe('A2');

    ctx.navigatePage(1);
    expect(ctx.currentIndex.value).toBe(4);
    expect(ctx.getCellAt(new Position(0, 0))).toBe('A1');

    ctx.navigatePage(-1);
    expect(ctx.currentIndex.value).toBe(3);
    expect(ctx.getCellAt(new Position(0, 0))).toBe('A2');
  });

  it('loadNewGrid 會釋放舊的 shuffler', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    const oldShuffler = new FakeGridShuffler();
    ctx.shufflerInstance.value = oldShuffler as never;
    expect(ctx.loadNewGrid(makeGrid())).toBe(true);
    expect(oldShuffler.delete).toHaveBeenCalled();
  });

  it('beginShuffleAnimation 成功時更新 currentGrid 並結束', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      ctx.loadNewGrid(makeGrid());
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(20_000);
      expect(await promise).toBe(true);
      expect(ctx.isShuffling.value).toBe(false);
      expect(ctx.currentGrid.value).toBeInstanceOf(FakeGrid);
      expect(ctx.totalPages.value).toBe(0);
      expect(ctx.currentIndex.value).toBe(0);
    } finally {
      vi.useRealTimers();
    }
  });

  it('beginShuffleAnimation 演算法失敗時 alert', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      ctx.loadNewGrid(makeGrid());
      const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
      shuffler.shuffle.mockResolvedValue({
        success: false,
        error: 'conflict',
        data: { tookMUS: 1 },
      });
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith('Unable to shuffle the grid due to reason: conflict');
      expect(ctx.isShuffling.value).toBe(false);
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
      ctx.loadNewGrid(makeGrid());
      const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
      shuffler.shuffle.mockRejectedValue(new Error('boom'));
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith('洗牌算法解決失敗！請檢查約束是否互相衝突。');
      expect(ctx.currentGrid.value).toBe(ctx.originalGrid.value);
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

  it('currentGrid 為 null 時 swapCells 記錄錯誤', () => {
    const errorSpy = vi.spyOn(console, 'error').mockImplementation(() => {});
    const ctx = setup();
    ctx.showOriginal.value = false;
    ctx.swapCells(new Position(0, 0), new Position(0, 1));
    expect(errorSpy).toHaveBeenCalledWith('currentGrid.value is null. cannot swap elements.');
  });

  it('isCellManuallyModified 依 pristine 差異判斷', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    shuffler.getGridAt.mockReturnValue(makeGrid());
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.showOriginal.value = false;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);

    ctx.swapCells(new Position(0, 0), new Position(0, 1));
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(true);
  });

  it('showOriginal 或 index 0 時 isCellManuallyModified 為 false', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    ctx.showOriginal.value = true;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
    ctx.showOriginal.value = false;
    ctx.currentIndex.value = 0;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
  });

  it('getGridAt 拋例外時 isCellManuallyModified 回傳 false', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    shuffler.getGridAt.mockImplementation(() => {
      throw new Error('boom');
    });
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.showOriginal.value = false;
    expect(ctx.isCellManuallyModified(new Position(0, 0))).toBe(false);
  });

  it('applyConfig 在 wasm 未就緒時 alert', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    expect(await ctx.applyConfig()).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('WebAssembly 模組未就緒，無法套用約束。');
  });

  it('applyConfig 無 cfg 且無 factory 時回傳 false', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    expect(await ctx.applyConfig()).toBe(false);
  });

  it('applyConfig 空 grid 時只 setConfig', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.wasmReady.value = true;
    await nextTick();
    expect(await ctx.applyConfig({} as never)).toBe(true);
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    expect(shuffler.setConfig).toHaveBeenCalled();
    expect(shuffler.setGrid).not.toHaveBeenCalled();
  });

  it('applyConfig 已有結果時提示重新洗牌', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    ctx.totalPages.value = 3;
    expect(await ctx.applyConfig({} as never)).toBe(true);
    expect(alertSpy).toHaveBeenCalled();
  });

  it('applyConfig 失敗時 alert 套用失敗', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    shuffler.setConfig.mockImplementation(() => {
      throw new Error('x');
    });
    expect(await ctx.applyConfig({} as never)).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('套用約束失敗。');
  });

  it('applyConfig 空 grid 且 setConfig 拋錯時 alert', async () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const ctx = setup();
    ctx.wasmModule.value = makeFakeModule(ThrowingSetConfigShuffler) as never;
    ctx.wasmReady.value = true;
    await nextTick();
    expect(await ctx.applyConfig({} as never)).toBe(false);
    expect(alertSpy).toHaveBeenCalledWith('套用約束失敗。');
  });

  it('applyConfig 有 grid 且無 shuffler 時建立並 setGrid', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    ctx.shufflerInstance.value = null as never;
    expect(await ctx.applyConfig({} as never)).toBe(true);
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    expect(shuffler.setGrid).toHaveBeenCalledWith(ctx.originalGrid.value);
    expect(shuffler.setConfig).toHaveBeenCalled();
  });

  it('beginShuffleAnimation 含空格 grid 時跳過空格', async () => {
    vi.useFakeTimers();
    try {
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      ctx.loadNewGrid(new FakeGrid(3, 3, ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', '']));
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(20_000);
      expect(await promise).toBe(true);
    } finally {
      vi.useRealTimers();
    }
  });

  it('navigatePage 無手動修改且無 shuffler 時不動 currentGrid', () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    ctx.totalPages.value = 5;
    ctx.currentIndex.value = 3;
    ctx.shufflerInstance.value = null as never;
    ctx.navigatePage(1);
    expect(ctx.currentIndex.value).toBe(4);
    expect(ctx.currentGrid.value).toBe(ctx.originalGrid.value);
  });

  it('getCellAt 在無 grid 時回傳空字串', () => {
    const ctx = setup();
    expect(ctx.getCellAt(new Position(0, 0))).toBe('');
  });

  it('applyConfig 使用 getShuffleConfig factory 的結果', async () => {
    const cfg = {};
    const ctx = setup(() => cfg as never);
    ctx.wasmModule.value = fakeModule as never;
    ctx.loadNewGrid(makeGrid());
    expect(await ctx.applyConfig()).toBe(true);
    const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
    expect(shuffler.setConfig).toHaveBeenCalledWith(cfg);
  });

  it('applyConfig 空 grid 且有 shuffler 時不重建', async () => {
    const ctx = setup();
    ctx.wasmModule.value = fakeModule as never;
    ctx.wasmReady.value = true;
    await nextTick();
    const existing = new FakeGridShuffler();
    ctx.shufflerInstance.value = existing as never;
    expect(await ctx.applyConfig({} as never)).toBe(true);
    expect(ctx.shufflerInstance.value).toBe(existing);
    expect(existing.setConfig).toHaveBeenCalled();
  });

  it('beginShuffleAnimation 回傳 Unsatisfiable 時顯示對應提示', async () => {
    vi.useFakeTimers();
    try {
      const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
      const ctx = setup();
      ctx.wasmModule.value = fakeModule as never;
      ctx.loadNewGrid(makeGrid());
      const shuffler = ctx.shufflerInstance.value as unknown as FakeGridShuffler;
      shuffler.shuffle.mockResolvedValue({
        success: false,
        error: 'Unsatisfiable',
        data: { tookMUS: 1 },
      });
      const promise = ctx.beginShuffleAnimation();
      await vi.advanceTimersByTimeAsync(1_000);
      expect(await promise).toBe(false);
      expect(alertSpy).toHaveBeenCalledWith(expect.stringContaining('約束互相衝突'));
    } finally {
      vi.useRealTimers();
    }
  });
});
