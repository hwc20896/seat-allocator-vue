import { computed, ref, type Ref, type ShallowRef, shallowRef, watch } from 'vue'
import type { PointerOf, ModuleExports, GridShuffler, ShuffleConfig, Grid } from '@/assets/wasm/alloc_algo'
import { swap, Position } from '@/utils/Position.ts'
import { shuffle, cloneDeep } from 'lodash-es'

export function useGridShuffle(
  wasmModule: ShallowRef<PointerOf<ModuleExports>>,
  wasmReady: Ref<boolean>,
  shufflerInstance: ShallowRef<PointerOf<GridShuffler>>,
  getShuffleConfig?: () => PointerOf<ShuffleConfig>,
) {
  const originalGrid = shallowRef<Grid | null>(null)
  const currentGrid = shallowRef<Grid | null>(null)
  const totalPages = ref(0)
  const currentIndex = ref(0)
  const isShuffling = ref(false)
  const showOriginal = ref(false)
  const manuallyModifiedGrids = shallowRef<Record<number, Grid>>({})

  const isGridLoaded = computed(() => !originalGrid.value?.empty())

  watch(
    wasmReady,
    (ready) => {
      if (!ready || !wasmModule.value) return

      originalGrid.value = new wasmModule.value.Grid()
      currentGrid.value = new wasmModule.value.Grid()
    },
    { immediate: true },
  )

  const pageLabel = computed(() => {
    if (!isGridLoaded.value) return '未導入'
    if (showOriginal.value) return '原始列表'
    if (isShuffling.value) return `正在洗牌中... (第 ${currentIndex.value + 1} 次)`
    return `第 ${currentIndex.value} 次分配`
  })

  const loadNewGrid = (grid: Grid) => {
    if (!wasmModule.value) return false

    try {
      if (shufflerInstance.value) {
        shufflerInstance.value.delete()
      }

      let config: PointerOf<ShuffleConfig>
      try {
        config = getShuffleConfig ? getShuffleConfig() : null
      } catch (e) {
        console.warn('getShuffleConfig factory threw', e)
        config = null
      }

      const cfg = config ?? new wasmModule.value.ShuffleConfig()
      shufflerInstance.value = new wasmModule.value.GridShuffler()

      shufflerInstance.value.setConfig(cfg);

      const success = shufflerInstance.value.setGrid(grid)

      console.debug('setGrid done.')
      if (!success) {
        alert('C++ shuffler failed to parse grid dimensions.')
        return false
      }

      originalGrid.value = grid
      currentGrid.value = grid
      manuallyModifiedGrids.value = {}
      totalPages.value = 0
      currentIndex.value = 0
      showOriginal.value = true

      return true
    } catch (e: unknown) {
      console.error(e)
      alert('導入配置失敗，檔案可能含有重複元素。')
      return false
    }
  }

  const getDelayForProgress = (progress: number, minDelay: number, maxDelay: number) => {
    const normalized = Math.sin(progress * Math.PI)
    return maxDelay - normalized * (maxDelay - minDelay)
  }

  const beginShuffleAnimation = async () => {
    if (!shufflerInstance.value || !wasmModule.value || isShuffling.value) return false

    isShuffling.value = true
    showOriginal.value = false

    const shuffleCount = 40
    const minDelay = 50
    const maxDelay = 300

    const getAnimationGrid = (grid: Grid) : Grid => {
      const result = grid.clone();

      const cells = shuffle(result.rawData().filter(cell => cell !== ""));

      let index = 0

      for (let i = 0; i < result.size(); i++){
        if (result.getByIndex(i).length > 0){
          result.setByIndex(i, cells[index++]!)
        }
      }

      return result;
    }

    try {
      console.time("Shuffle Response took")
      const shuffleResult = await shufflerInstance.value.shuffle()
      console.timeEnd('Shuffle Response took')

      if (!shuffleResult.success){
        alert(`Unable to shuffle the grid due to reason: ${shuffleResult.error}`);
        return false
      }
      console.info(`Shuffle done in ${shuffleResult.data.tookMUS / 1000}ms.`);

      let localAnimGrid = originalGrid.value?.clone()

      for (let step = 0; step < shuffleCount; step++) {
        const progress = step / shuffleCount
        const currentDelay = getDelayForProgress(progress, minDelay, maxDelay)

        localAnimGrid = getAnimationGrid(localAnimGrid!)
        currentGrid.value = localAnimGrid

        await new Promise((resolve) => setTimeout(resolve, currentDelay))
      }

      currentGrid.value = shufflerInstance.value.getGrid()
      totalPages.value = shufflerInstance.value.getShuffledGridCount()
      currentIndex.value = totalPages.value

      return true
    } catch (error: unknown) {
      alert('洗牌算法解決失敗！請檢查約束是否互相衝突。')
      console.error(error)
      currentGrid.value = originalGrid.value
      return false
    } finally {
      isShuffling.value = false
    }
  }

  const navigatePage = (step: number) => {
    const target = currentIndex.value + step
    if (target < 1 || target > totalPages.value) return

    currentIndex.value = target

    if (manuallyModifiedGrids.value[target]) {
      currentGrid.value = manuallyModifiedGrids.value[target]
    } else if (shufflerInstance.value) {
      currentGrid.value = shufflerInstance.value.getGridAt(target - 1)
    }
  }

  const toggleOriginal = () => {
    showOriginal.value = !showOriginal.value
  }

  const swapCells = (pos1: Position, pos2: Position) => {
    if (showOriginal.value || isShuffling.value) return

    const gridCopy = currentGrid.value?.clone()

    if (!gridCopy){
      console.error('currentGrid.value is null. cannot swap elements.')
      return
    }

    const swapped = swap(gridCopy, pos1, pos2);

    currentGrid.value = swapped
    manuallyModifiedGrids.value[currentIndex.value] = swapped
  }

  const getCellAt = (pos: Position): string => {
    return currentGrid.value?.getByPos(pos.row, pos.col) || ''
  }

  const isCellManuallyModified = (pos: Position): boolean => {
    if (showOriginal.value || !shufflerInstance.value || currentIndex.value <= 0) {
      return false
    }

    try {
      const pristineGrid = shufflerInstance.value.getGridAt(currentIndex.value - 1)
      return pristineGrid && pristineGrid.getByPos(pos.row, pos.col) !== currentGrid.value?.getByPos(pos.row, pos.col)
    } catch (e) {
      return false
    }
  }

  const applyConfig = async (cfg?: PointerOf<ShuffleConfig>) => {
    if (!wasmModule.value) {
      alert('WebAssembly 模組未就緒，無法套用約束。')
      return false
    }

    const cfgInstance = cfg ?? (getShuffleConfig ? getShuffleConfig() : null)
    if (!cfgInstance) return false

    if (!originalGrid.value || originalGrid.value.empty()) {
      try {
        if (!shufflerInstance.value) {
          shufflerInstance.value = new wasmModule.value.GridShuffler()
        }
        shufflerInstance.value.setConfig(cfgInstance)
        return true
      } catch (e) {
        console.error(e)
        alert('套用約束失敗。')
        return false
      }
    }

    try {
      if (!shufflerInstance.value) {
        shufflerInstance.value = new wasmModule.value.GridShuffler()
        shufflerInstance.value.setGrid(originalGrid.value)
      }

      shufflerInstance.value.setConfig(cfgInstance)

      // 保留已有的打亂結果，但提醒用戶
      if (totalPages.value > 0) {
        alert(
          '約束已套用，但現有的分配結果是基於舊約束產生的，可能不完全滿足新約束。建議重新洗牌以獲得符合新約束的分配結果。',
        )
      }

      return true
    } catch (e) {
      console.error('applyConfig failed', e)
      alert('套用約束失敗。')
      return false
    }
  }

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
  }
}
