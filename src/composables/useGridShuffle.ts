import { computed, ref, type ShallowRef } from 'vue'
import type { PointerOf, ModuleExports, GridShuffler, Grid, ShuffleConfig } from '@/assets/wasm/alloc_algo'
import {swap, Position} from '@/utils/Position.ts'

export function useGridShuffle(
  wasmModule: ShallowRef<PointerOf<ModuleExports>>,
  shufflerInstance: ShallowRef<PointerOf<GridShuffler>>,
  // optional factory that returns a populated ShuffleConfig (or null)
  getShuffleConfig?: () => PointerOf<ShuffleConfig>,
) {
  const originalGrid = ref<Grid>([])
  const currentGrid = ref<Grid>([])
  const totalPages = ref(0)
  const currentIndex = ref(0)
  const isShuffling = ref(false)
  const showOriginal = ref(false)
  const manuallyModifiedGrids = ref<Record<number, Grid>>({})

  const isGridLoaded = computed(() => originalGrid.value.length > 0)

  const pageLabel = computed(() => {
    if (!isGridLoaded.value) return '未導入'
    if (showOriginal.value) return '原始列表'
    return `第 ${currentIndex.value} 次分配`
  })

  const loadNewGrid = (grid: Grid) => {
    if (!wasmModule.value) return false

    try {
      if (shufflerInstance.value) {
        shufflerInstance.value.delete()
      }

      let config: PointerOf<ShuffleConfig> = null
      try {
        config = getShuffleConfig ? getShuffleConfig() : null
      } catch (e) {
        console.warn('getShuffleConfig factory threw', e)
        config = null
      }

      const cfg = config ?? new wasmModule.value.ShuffleConfig()
      shufflerInstance.value = new wasmModule.value.GridShuffler(cfg)

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
    if (!shufflerInstance.value || isShuffling.value) return false

    isShuffling.value = true
    showOriginal.value = false

    const shuffleCount = 40
    const minDelay = 50
    const maxDelay = 300

    let localAnimGrid = JSON.parse(JSON.stringify(originalGrid.value))

    for (let step = 0; step < shuffleCount; step++) {
      const progress = step / shuffleCount
      const currentDelay = getDelayForProgress(progress, minDelay, maxDelay)

      localAnimGrid = wasmModule.value?.shuffleGrid(localAnimGrid)
      currentGrid.value = localAnimGrid

      await new Promise((resolve) => setTimeout(resolve, currentDelay))
    }

    try {
      shufflerInstance.value?.shuffle()

      currentGrid.value = shufflerInstance.value.getGrid()
      totalPages.value = shufflerInstance.value.getSize()
      currentIndex.value = totalPages.value

      return true
    } catch (error: unknown) {
      alert('洗牌算法解決失敗！請檢查約束是否互相衝突。')
      console.error(error)
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

    const gridCopy = JSON.parse(JSON.stringify(currentGrid.value)) as Grid

    const swapped = swap(gridCopy, pos1, pos2);

    currentGrid.value = swapped
    manuallyModifiedGrids.value[currentIndex.value] = swapped
  }

  const getCellAt = (pos: Position): string => {
    if (!currentGrid.value[pos.row]) return ''
    return currentGrid.value[pos.row]?.[pos.col] || ''
  }

  const isCellManuallyModified = (pos: Position): boolean => {
    if (showOriginal.value || !shufflerInstance.value || currentIndex.value <= 0) {
      return false
    }

    try {
      const pristineGrid = shufflerInstance.value.getGridAt(currentIndex.value - 1)
      return pristineGrid && pristineGrid[pos.row]?.[pos.col] !== currentGrid.value[pos.row]?.[pos.col]
    } catch (e) {
      return false
    }
  }

  const applyConfig = (cfg?: PointerOf<ShuffleConfig>, options: { preserveManual?: boolean } = { preserveManual: true }) => {
    // Apply a new ShuffleConfig to the native shuffler while trying to preserve UI state
    if (!wasmModule.value) {
      alert('WebAssembly 模組未就緒，無法套用約束。')
      return false
    }

    // If there's no original grid yet, just replace the shuffler instance for future loads
    if (!originalGrid.value || originalGrid.value.length === 0) {
      try {
        const cfgInstance = cfg ?? (getShuffleConfig ? getShuffleConfig() : null)
        const newShuffler = cfgInstance ? new wasmModule.value.GridShuffler(cfgInstance) : new wasmModule.value.GridShuffler(new wasmModule.value.ShuffleConfig())
        if (shufflerInstance.value) {
          try { shufflerInstance.value.delete() } catch (e) { console.warn('failed to delete old shuffler', e) }
        }
        shufflerInstance.value = newShuffler
        return true
      } catch (e) {
        console.error(e)
        alert('套用約束失敗。')
        return false
      }
    }

    const prevShuffler = shufflerInstance.value
    const prevTotal = totalPages.value
    const prevIndex = currentIndex.value
    const prevManual = JSON.parse(JSON.stringify(manuallyModifiedGrids.value))

    try {
      const cfgInstance = cfg ?? (getShuffleConfig ? getShuffleConfig() : null)
      const newShuffler = cfgInstance ? new wasmModule.value.GridShuffler(cfgInstance) : new wasmModule.value.GridShuffler(new wasmModule.value.ShuffleConfig())

      const success = newShuffler.setGrid(originalGrid.value)
      if (!success) {
        alert('C++ shuffler 無法解析座位配置。')
        try { newShuffler.delete() } catch {}
        return false
      }

      // If there were generated pages before, regenerate with new config so indices remain meaningful
      if (prevTotal > 0) {
        try { newShuffler.shuffle() } catch (e) { console.warn('newShuffler.shuffle() failed', e) }
      }

      const newSize = newShuffler.getSize()

      // Replace native instance only after success to avoid losing previous state on failure
      shufflerInstance.value = newShuffler
      if (prevShuffler) {
        try { prevShuffler.delete() } catch (e) { console.warn('failed to delete prev shuffler', e) }
      }

      totalPages.value = newSize
      currentIndex.value = Math.min(prevIndex, newSize)

      if (options.preserveManual) {
        const newManual: Record<number, Grid> = {}
        for (const kStr of Object.keys(prevManual)) {
          const k = Number(kStr)
          if (k >= 1 && k <= newSize) newManual[k] = prevManual[k]
        }
        manuallyModifiedGrids.value = newManual
      } else {
        manuallyModifiedGrids.value = {}
      }

      // Update currentGrid to reflect new shuffler state (or stay original)
      if (showOriginal.value) {
        currentGrid.value = originalGrid.value
      } else if (currentIndex.value > 0) {
        try {
          currentGrid.value = shufflerInstance.value.getGridAt(currentIndex.value - 1)
        } catch (e) {
          console.warn('getGridAt failed after applying config', e)
          currentGrid.value = originalGrid.value
        }
      } else {
        currentGrid.value = originalGrid.value
      }

      return true
    } catch (e) {
      console.error('applyConfig failed', e)
      // Attempt to restore previous state
      if (!shufflerInstance.value && prevShuffler) shufflerInstance.value = prevShuffler
      totalPages.value = prevTotal
      currentIndex.value = prevIndex
      manuallyModifiedGrids.value = prevManual
      alert('套用約束失敗，已還原至先前狀態。')
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
