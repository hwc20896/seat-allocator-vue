import { computed, ref, type Ref, type ShallowRef, shallowRef, watch } from 'vue';
import type { MainModule, Grid } from '@/assets/wasm/alloc_algo';
import { getShuffleErrorMessage } from '@/utils/shuffleError.ts';
import { swap, Position } from '@/utils/Position.ts';
import { shuffle } from 'lodash-es';
import { buildWasmConfigFromJson } from '@/utils/wasmConfig.ts';
import { ShuffleWorkerUnavailableError, type ShuffleWorkerClient } from './useShuffleWorker';

export function useGridShuffle(
  wasmModule: ShallowRef<MainModule | null>,
  wasmReady: Ref<boolean>,
  shuffleWorker: ShuffleWorkerClient,
  getConfigJson?: () => string,
) {
  const originalGrid = shallowRef<Grid | null>(null);
  const currentGrid = shallowRef<Grid | null>(null);
  const totalPages = ref(0);
  const currentIndex = ref(0);
  const isShuffling = ref(false);
  const showOriginal = ref(false);
  const manuallyModifiedGrids = shallowRef<Record<number, Grid>>({});

  // Worker 每次 shuffle 的結果（等價於 C++ GridShuffler 內部的 shuffleGrids_）
  let shuffledHistory: Grid[] = [];
  // 每次載入新 grid 遞增；非同步結果回來時若 epoch 已變，該結果屬於舊 grid，直接丟棄
  let epoch = 0;

  const isGridLoaded = computed(() => !!originalGrid.value && !originalGrid.value?.empty());

  watch(
    wasmReady,
    (ready) => {
      if (!ready || !wasmModule.value) return;

      originalGrid.value = new wasmModule.value.Grid();
      currentGrid.value = new wasmModule.value.Grid();
    },
    { immediate: true },
  );

  const pageLabel = computed(() => {
    if (!isGridLoaded.value) return '未導入';
    if (showOriginal.value) return '原始列表';
    if (isShuffling.value) return `正在洗牌中... (第 ${currentIndex.value + 1} 次)`;
    return `第 ${currentIndex.value} 次分配`;
  });

  const releaseShuffledHistory = () => {
    for (const grid of shuffledHistory) grid.delete();
    shuffledHistory = [];
  };

  const loadNewGrid = async (grid: Grid): Promise<boolean> => {
    if (!wasmModule.value) return false;

    const myEpoch = ++epoch;
    isShuffling.value = false;
    releaseShuffledHistory();

    try {
      // embind 句柄無法跨執行緒，以 CSV 傳給 Worker 重建
      const { ok, reason } = await shuffleWorker.setGrid(grid.toCSVString());
      console.debug('setGrid done.');

      if (myEpoch !== epoch) return false;

      if (!ok) {
        console.warn('setGrid failed:', reason);
        alert('洗牌引擎無法載入此座位表。');
        return false;
      }

      originalGrid.value = grid;
      currentGrid.value = grid;
      manuallyModifiedGrids.value = {};
      totalPages.value = 0;
      currentIndex.value = 0;
      showOriginal.value = true;

      return true;
    } catch (e: unknown) {
      if (myEpoch === epoch) {
        console.error(e);
        if (e instanceof ShuffleWorkerUnavailableError) {
          alert(`洗牌引擎無法使用，請重新載入頁面。\n原因：${e.reason}`);
        } else {
          alert('導入配置失敗，檔案可能含有重複元素。');
        }
      }
      return false;
    }
  };

  const getDelayForProgress = (progress: number, minDelay: number, maxDelay: number) => {
    const normalized = Math.sin(progress * Math.PI);
    return maxDelay - normalized * (maxDelay - minDelay);
  };

  const beginShuffleAnimation = async (): Promise<boolean> => {
    const module = wasmModule.value;
    if (!module || !isGridLoaded.value || isShuffling.value) return false;

    const myEpoch = epoch;
    isShuffling.value = true;
    showOriginal.value = false;

    const shuffleCount = 40;
    const minDelay = 50;
    const maxDelay = 300;

    const getAnimationGrid = (grid: Grid): Grid => {
      const result = grid.clone();

      const cells = shuffle(Array.from(result.rawData()).filter((cell) => cell !== ''));

      let index = 0;

      for (let i = 0; i < result.size(); i++) {
        if (result.getByIndex(i).length > 0) {
          result.setByIndex(i, cells[index++]!);
        }
      }

      return result;
    };

    try {
      let configJson: string;
      try {
        configJson = getConfigJson ? getConfigJson() : '{}';
      } catch (e) {
        console.warn('getConfigJson factory threw', e);
        configJson = '{}';
      }

      const result = await shuffleWorker.shuffle(configJson);
      if (myEpoch !== epoch) return false;

      if (!result.success) {
        console.warn(`Shuffle failed: ${result.error}`);
        alert(getShuffleErrorMessage(result.error));
        return false;
      }
      console.info(`Shuffle done in ${result.report.tookMUS / 1000}ms.`);

      const resultGrid = module.Grid.fromCSV(result.gridCsv);
      shuffledHistory.push(resultGrid);

      let localAnimGrid = originalGrid.value?.clone() ?? resultGrid;

      for (let step = 0; step < shuffleCount; step++) {
        if (myEpoch !== epoch) return false;

        const progress = step / shuffleCount;
        const currentDelay = getDelayForProgress(progress, minDelay, maxDelay);

        localAnimGrid = getAnimationGrid(localAnimGrid);
        currentGrid.value = localAnimGrid;

        await new Promise((resolve) => setTimeout(resolve, currentDelay));
      }

      currentGrid.value = resultGrid;
      totalPages.value = shuffledHistory.length;
      currentIndex.value = totalPages.value;

      return true;
    } catch (error: unknown) {
      if (myEpoch === epoch) {
        console.error(error);
        if (error instanceof ShuffleWorkerUnavailableError) {
          alert(`洗牌引擎無法使用，請重新載入頁面。\n原因：${error.reason}`);
        } else {
          alert('洗牌算法解決失敗！請檢查約束是否互相衝突。');
        }
        currentGrid.value = originalGrid.value;
      }
      return false;
    } finally {
      if (myEpoch === epoch) {
        isShuffling.value = false;
      }
    }
  };

  const navigatePage = (step: number) => {
    const target = currentIndex.value + step;
    if (target < 1 || target > totalPages.value) return;

    currentIndex.value = target;

    if (manuallyModifiedGrids.value[target]) {
      currentGrid.value = manuallyModifiedGrids.value[target];
    } else {
      const pristineGrid = shuffledHistory[target - 1];
      if (pristineGrid) currentGrid.value = pristineGrid;
    }
  };

  const toggleOriginal = () => {
    showOriginal.value = !showOriginal.value;
  };

  const swapCells = (pos1: Position, pos2: Position) => {
    if (showOriginal.value || isShuffling.value) return;

    const gridCopy = currentGrid.value?.clone();

    if (!gridCopy) {
      console.error('currentGrid.value is null. cannot swap elements.');
      return;
    }

    const swapped = swap(gridCopy, pos1, pos2);

    currentGrid.value = swapped;
    manuallyModifiedGrids.value[currentIndex.value] = swapped;
  };

  const getCellAt = (pos: Position): string => {
    return currentGrid.value?.getByPos(pos.row, pos.col) || '';
  };

  const isCellManuallyModified = (pos: Position): boolean => {
    if (showOriginal.value || currentIndex.value <= 0) {
      return false;
    }

    const pristineGrid = shuffledHistory[currentIndex.value - 1];
    if (!pristineGrid) return false;

    try {
      return (
        pristineGrid.getByPos(pos.row, pos.col) !== currentGrid.value?.getByPos(pos.row, pos.col)
      );
    } catch (e) {
      console.error(e);
      return false;
    }
  };

  const applyConfig = async (configJson?: string): Promise<boolean> => {
    if (!wasmModule.value) {
      alert('WebAssembly 模組未就緒，無法套用約束。');
      return false;
    }

    let json: string;
    try {
      if (configJson !== undefined) {
        json = configJson;
      } else if (getConfigJson) {
        json = getConfigJson();
      } else {
        return false;
      }
    } catch (e) {
      console.warn('getConfigJson factory threw', e);
      return false;
    }

    try {
      // 構建並釋放一次以驗證 JSON 可套用；實際套用發生在 Worker 的每次 shuffle 中
      const cfg = buildWasmConfigFromJson(wasmModule.value, json);
      if (!cfg) return false;
      cfg.delete();
    } catch (e) {
      console.error('applyConfig failed', e);
      alert('套用約束失敗。');
      return false;
    }

    // 保留已有的打亂結果，但提醒用戶
    if (totalPages.value > 0) {
      alert(
        '約束已套用，但現有的分配結果是基於舊約束產生的，可能不完全滿足新約束。建議重新洗牌以獲得符合新約束的分配結果。',
      );
    }

    return true;
  };

  return {
    originalGrid,
    currentGrid,
    totalPages,
    currentIndex,
    isShuffling,
    showOriginal,
    isGridLoaded,
    pageLabel,
    loadNewGrid,
    beginShuffleAnimation,
    navigatePage,
    toggleOriginal,
    swapCells,
    getCellAt,
    isCellManuallyModified,
    applyConfig,
  };
}
