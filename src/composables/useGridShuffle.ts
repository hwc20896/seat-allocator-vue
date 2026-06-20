import { computed, ref, type ShallowRef } from 'vue'
import type { GridShufflerPointer, ModuleExportsPointer } from '@/assets/wasm/alloc_algo'

type Grid = string[][]

export function useGridShuffle(
  wasmModule: ShallowRef<ModuleExportsPointer, ModuleExportsPointer>,
  shufflerInstance: ShallowRef<GridShufflerPointer, GridShufflerPointer>
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

      const config = new wasmModule.value.ShuffleConfig()
      shufflerInstance.value = new wasmModule.value.GridShuffler(config)

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
      alert('導入配置失敗，檔案可能含有重複元素。')
      console.error(e)
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
      shufflerInstance.value.shuffle()

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

  const swapCells = (row1: number, col1: number, row2: number, col2: number) => {
    if (showOriginal.value || isShuffling.value) return

    const gridCopy = JSON.parse(JSON.stringify(currentGrid.value))

    const temp = gridCopy[row1][col1]
    gridCopy[row1][col1] = gridCopy[row2][col2]
    gridCopy[row2][col2] = temp

    currentGrid.value = gridCopy
    manuallyModifiedGrids.value[currentIndex.value] = gridCopy
  }

  const getCellAt = (row: number, col: number): string => {
    if (!currentGrid.value[row]) return ''
    return currentGrid.value[row][col] || ''
  }

  const isCellManuallyModified = (row: number, col: number): boolean => {
    if (showOriginal.value || !shufflerInstance.value || currentIndex.value <= 0) {
      return false
    }

    try {
      const pristineGrid = shufflerInstance.value.getGridAt(currentIndex.value - 1)
      return (pristineGrid && pristineGrid[row]?.[col] !== currentGrid.value[row]?.[col])
    } catch (e) {
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
  }
}
