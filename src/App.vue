<template>
  <div class="window-container">
    <!-- Menu Bar / Header Controls -->
    <header class="menu-bar">
      <!-- File Operations -->
      <div class="menu-item">
        <button class="menu-trigger">文件 (File)</button>
        <div class="dropdown-menu">
          <label class="dropdown-item file-label">
            從 .csv 導入
            <input type="file" accept=".csv" @change="handleCSVImport" style="display: none" />
          </label>
          <button class="dropdown-item" :disabled="!isGridLoaded" @click="handleCSVExport">
            導出為 .csv
          </button>
        </div>
      </div>

      <!-- Colors Configuration -->
      <div class="menu-item">
        <button class="menu-trigger">顔色設定 (Color)</button>
        <div class="dropdown-menu">
          <label class="dropdown-item file-label">
            導入顔色配置 (JSON)
            <input type="file" accept=".json" @change="handleColorImport" style="display: none" />
          </label>
          <button class="dropdown-item" :disabled="colorPresets.length === 0" @click="clearColors">
            卸載顏色配置
          </button>
        </div>
      </div>

      <!-- Algorithmic Constraints -->
      <div class="menu-item">
        <button class="menu-trigger">算法約束 (Constraints)</button>
        <div class="dropdown-menu">
          <label class="dropdown-item file-label">
            導入約束配置 (JSON)
            <input
              type="file"
              accept=".json"
              @change="handleConstraintsImport"
              style="display: none"
            />
          </label>
          <button class="dropdown-item" :disabled="!hasCustomConfig" @click="resetConstraints">
            重設為默認算法
          </button>
        </div>
      </div>
    </header>

    <!-- Main Viewport -->
    <main class="centralwidget">
      <!-- Top Row Layout -->
      <div class="top-row-layout">
        <div class="title-group">
          <h1 class="title">座位分配系統</h1>
          <span class="version-badge">v3.0</span>
        </div>

        <div class="navigation-controls">
          <button
            class="tool-button"
            :disabled="currentIndex <= 1 || isShuffling"
            @click="navigatePage(-1)"
            title="前一頁"
          >
            &lt;
          </button>

          <div class="page-indicator" :class="{ 'original-view': showOriginal }">
            <span class="indicator-dot"></span>
            {{ pageLabel }}
          </div>

          <button
            class="tool-button"
            :disabled="currentIndex >= totalPages || isShuffling"
            @click="navigatePage(1)"
            title="後一頁"
          >
            &gt;
          </button>

          <button
            class="push-button toggle-button"
            :class="{ active: showOriginal }"
            :disabled="!isGridLoaded || isShuffling"
            @click="toggleOriginal"
          >
            {{ showOriginal ? '顯示當前打亂' : '顯示原始列表' }}
          </button>
        </div>
      </div>

      <!-- Grid Displayer (Interactive Table) -->
      <div class="grid-displayer-container">
        <table class="grid-displayer">
          <tbody>
            <tr v-for="(row, rIdx) in renderedGrid" :key="rIdx">
              <td
                v-for="(cell, cIdx) in row"
                :key="cIdx"
                :class="getCellClass(rIdx, cIdx)"
                :style="{ color: getCellColor(cell) }"
                @click="handleCellClick(rIdx, cIdx)"
              >
                <div class="cell-content">
                  <span class="cell-icon">🪑</span>
                  <span class="cell-text">{{ cell || '空位' }}</span>
                </div>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

      <!-- Bottom Layout Button -->
      <div class="bottom-layout">
        <button
          class="shuffle-button"
          :class="{ 'is-active-shuffling': isShuffling }"
          :disabled="!isGridLoaded || isShuffling"
          @click="beginShuffleAnimation"
        >
          <span v-if="isShuffling" class="spinner"></span>
          {{ isShuffling ? '正在隨機分配洗牌...' : '開始隨機分配！' }}
        </button>
      </div>
    </main>

    <!-- Status Bar -->
    <footer class="statusbar">
      <div class="status-content">
        <span class="status-indicator" :class="{ active: isGridLoaded }"></span>
        <span class="status-text">{{ statusText }}</span>
      </div>
    </footer>
  </div>
</template>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref, shallowRef } from 'vue'
import initWasmModule from '@/assets/wasm/alloc_algo.js'

// Types
type Grid = string[][]
interface ColorPreset {
  pattern: RegExp
  color: string
}

// Emscripten State
const wasmModule = ref<any>(null)
const shufflerInstance = shallowRef<any>(null)

// UI Reactivity
const wasmReady = ref(false)
const isShuffling = ref(false)
const statusText = ref('未導入')
const showOriginal = ref(false)

// Grid Data Management
const originalGrid = ref<Grid>([])
const currentGrid = ref<Grid>([])
const totalPages = ref(0)
const currentIndex = ref(0) // 1-based page index matching C++ currentIndex_

// Swaps & Manual changes
const taggedCell = ref<{ row: number; col: number } | null>(null)
const manuallyModifiedGrids = ref<Record<number, Grid>>({})

// Custom Configurations
const colorPresets = ref<ColorPreset[]>([])
const hasCustomConfig = ref(false)
const currentConfigJson = ref<string>('{}')

// Dynamic Page Labels matching Qt C++ Logic
const pageLabel = computed(() => {
  if (!isGridLoaded.value) return '未導入'
  if (showOriginal.value) return '原始列表'
  return `第 ${currentIndex.value} 次分配`
})

const isGridLoaded = computed(() => originalGrid.value.length > 0)

// Choose which grid model should be displayed
const renderedGrid = computed<Grid>(() => {
  if (!isGridLoaded.value) return []
  if (showOriginal.value) return originalGrid.value
  return currentGrid.value
})

onMounted(async () => {
  try {
    wasmModule.value = await initWasmModule({
      locateFile: (path: string) => {
        if (path.endsWith('.wasm')) return '/alloc_algo.wasm'
        return path
      },
    })
    wasmReady.value = true
    statusText.value = '系統就緒，等待導入配置...'
  } catch (error) {
    statusText.value = 'WebAssembly 模組載入失敗'
    console.error(error)
  }
})

// Clean up memory allocations
onBeforeUnmount(() => {
  if (shufflerInstance.value) {
    shufflerInstance.value.delete()
  }
})

// ==========================================
// File Importing & Exporting Parsing Methods
// ==========================================
const handleCSVImport = (event: Event) => {
  const target = event.target as HTMLInputElement
  if (!target.files?.length) return

  const file = target.files[0]
  const reader = new FileReader()

  reader.onload = () => {
    const text = reader.result as string
    const parsed: Grid = text
      .split('\n')
      .map((row) => row.split(',').map((cell) => cell.trim()))
      .filter((row) => row.length > 0 && row.some((cell) => cell !== ''))

    if (parsed.length === 0) return

    loadNewGrid(parsed, file.name)
  }
  reader.readAsText(file)
}

const handleCSVExport = () => {
  if (currentGrid.value.length === 0) return

  if (showOriginal.value) {
    const confirmChoice = confirm('這是原始名單，確定要導出嗎？\n\n建議先執行洗牌操作後再導出。')
    if (!confirmChoice) return
  }

  const csvContent = currentGrid.value.map((row) => row.join(',')).join('\n')
  const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' })
  const url = URL.createObjectURL(blob)

  const link = document.createElement('a')
  link.setAttribute('href', url)
  link.setAttribute('download', `allocated_seats_page_${currentIndex.value}.csv`)
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)

  statusText.value = `已導出至：allocated_seats_page_${currentIndex.value}.csv`
}

const loadNewGrid = (grid: Grid, filename: string) => {
  if (!wasmReady.value) return

  try {
    if (shufflerInstance.value) {
      shufflerInstance.value.delete()
    }

    const config = new wasmModule.value.ShuffleConfig()
    if (hasCustomConfig.value) {
      const parsedConfig = JSON.parse(currentConfigJson.value)
    }

    shufflerInstance.value = new wasmModule.value.GridShuffler(config)

    const success = shufflerInstance.value.setGrid(grid)
    if (!success) {
      alert('C++ shuffler failed to parse grid dimensions.')
      return
    }

    originalGrid.value = grid
    currentGrid.value = grid
    manuallyModifiedGrids.value = {}
    totalPages.value = 0
    currentIndex.value = 0
    showOriginal.value = true
    resetTagState()

    statusText.value = `已成功導入檔案：${filename}`
  } catch (e: any) {
    alert('導入配置失敗，檔案可能含有重複元素。')
    console.error(e)
  }
}

// ==========================================
// Trigonometric Animation / Shuffling Logic
// ==========================================
const getDelayForProgress = (progress: number, minDelay: number, maxDelay: number) => {
  const normalized = Math.sin(progress * Math.PI)
  return maxDelay - normalized * (maxDelay - minDelay)
}

const beginShuffleAnimation = async () => {
  if (!shufflerInstance.value || isShuffling.value) return

  isShuffling.value = true
  showOriginal.value = false
  resetTagState()

  const shuffleCount = 40
  const minDelay = 50
  const maxDelay = 300

  let localAnimGrid = JSON.parse(JSON.stringify(originalGrid.value))

  for (let step = 0; step < shuffleCount; step++) {
    const progress = step / shuffleCount
    const currentDelay = getDelayForProgress(progress, minDelay, maxDelay)

    localAnimGrid = localAnimGrid.map((row: string[]) => [...row].sort(() => Math.random() - 0.5))
    currentGrid.value = localAnimGrid

    await new Promise((resolve) => setTimeout(resolve, currentDelay))
  }

  try {
    shufflerInstance.value.shuffle()
    const finalResult = shufflerInstance.value.getGrid()

    currentGrid.value = finalResult
    totalPages.value = shufflerInstance.value.getSize()
    currentIndex.value = totalPages.value

    statusText.value = `洗牌完成，已生成第 ${currentIndex.value} 次分配結果。`
  } catch (error: any) {
    alert('洗牌算法解決失敗！請檢查約束是否互相衝突。')
    console.error(error)
  } finally {
    isShuffling.value = false
  }
}

// ==========================================
// Interactive Seat Swapping logic (double-click equivalent)
// ==========================================
const handleCellClick = (row: number, col: number) => {
  if (showOriginal.value || isShuffling.value) return

  if (!taggedCell.value) {
    taggedCell.value = { row, col }
    statusText.value = `已標記單元格 [${row + 1}, ${col + 1}]，再次點擊其他格子即可進行位置交換。`
  } else if (taggedCell.value.row === row && taggedCell.value.col === col) {
    resetTagState()
    statusText.value = `已取消標記。`
  } else {
    const tRow = taggedCell.value.row
    const tCol = taggedCell.value.col

    const gridCopy = JSON.parse(JSON.stringify(currentGrid.value))

    const temp = gridCopy[tRow][tCol]
    gridCopy[tRow][tCol] = gridCopy[row][col]
    gridCopy[row][col] = temp

    currentGrid.value = gridCopy
    manuallyModifiedGrids.value[currentIndex.value] = gridCopy

    statusText.value = `已手動交換單元格：[${tRow + 1}, ${tCol + 1}] <-> [${row + 1}, ${col + 1}]`
    resetTagState()
  }
}

const getCellClass = (row: number, col: number) => {
  const classes = []

  if (taggedCell.value?.row === row && taggedCell.value?.col === col) {
    classes.push('tagged')
  }

  // Defensive check: Do not execute cell comparisons while shuffling is active
  if (isShuffling.value) {
    return classes.join(' ')
  }

  // Check manual modifications against the pristine shuffler grid instance
  if (!showOriginal.value && shufflerInstance.value && currentIndex.value > 0) {
    try {
      // Call the newly bound C++ getGridAt method
      const pristineGrid = shufflerInstance.value.getGridAt(currentIndex.value - 1)
      if (pristineGrid && pristineGrid[row][col] !== currentGrid.value[row][col]) {
        classes.push('swapped')
      }
    } catch (e) {
      // Quietly fall back if index range checks are transiently out of bounds
    }
  }

  return classes.join(' ')
}

const resetTagState = () => {
  taggedCell.value = null
}

// ==========================================
// Dynamic Color Mapping Engine (using Regex)
// ==========================================
const handleColorImport = (event: Event) => {
  const target = event.target as HTMLInputElement
  if (!target.files?.length) return

  const file = target.files[0]
  const reader = new FileReader()

  reader.onload = () => {
    try {
      const data = JSON.parse(reader.result as string)
      const tempRules: ColorPreset[] = []

      for (const [pattern, color] of Object.entries(data)) {
        tempRules.push({
          pattern: new RegExp(pattern),
          color: color as string,
        })
      }

      colorPresets.value = tempRules
      statusText.value = `顏色配置載入成功。`
    } catch (e) {
      alert('JSON 顏色配置解析失敗。')
    }
  }
  reader.readAsText(file)
}

const getCellColor = (text: string) => {
  if (!text) return '#94a3b8'
  for (const preset of colorPresets.value) {
    if (preset.pattern.test(text)) {
      return preset.color
    }
  }
  return '' // Fallback to CSS theme variables
}

const clearColors = () => {
  colorPresets.value = []
  statusText.value = `顏色配置已卸載。`
}

// ==========================================
// Constraints Configurations
// ==========================================
const handleConstraintsImport = (event: Event) => {
  const target = event.target as HTMLInputElement
  if (!target.files?.length) return

  const file = target.files[0]
  const reader = new FileReader()

  reader.onload = () => {
    try {
      JSON.parse(reader.result as string)
      currentConfigJson.value = reader.result as string
      hasCustomConfig.value = true
      statusText.value = `算法約束載入成功：${file.name}。請重新導入座位配置或洗牌。`
    } catch (e) {
      alert('JSON 算法約束檔案格式錯誤。')
    }
  }
  reader.readAsText(file)
}

const resetConstraints = () => {
  currentConfigJson.value = '{}'
  hasCustomConfig.value = false
  statusText.value = `約束已重設。已還原為基礎隨機分配算法。`
}

// ==========================================
// Page Navigation
// ==========================================
const navigatePage = (step: number) => {
  const target = currentIndex.value + step
  if (target < 1 || target > totalPages.value) return

  currentIndex.value = target

  // Retrieve current modification or raw grid from history
  if (manuallyModifiedGrids.value[target]) {
    currentGrid.value = manuallyModifiedGrids.value[target]
  } else if (shufflerInstance.value) {
    // Call getGridAt here as well
    currentGrid.value = shufflerInstance.value.getGridAt(target - 1)
  }

  resetTagState()
}

const toggleOriginal = () => {
  showOriginal.value = !showOriginal.value
  resetTagState()
}
</script>

<style>
/* Modern System Variables - Global scope */
:root {
  --primary: #6366f1;
  --primary-hover: #4f46e5;
  --primary-light: #eef2ff;
  --primary-glow: rgba(99, 102, 241, 0.15);

  --bg-dark: #0f172a;
  --bg-panel: #ffffff;
  --bg-page: #f8fafc;
  --bg-control-hover: #f1f5f9;
  --bg-gradient-start: #fafbfc;
  --bg-gradient-end: #f1f5f9;

  --border-light: #e2e8f0;
  --border-medium: #cbd5e1;
  --border-focus: #94a3b8;

  --text-main: #334155;
  --text-dark: #0f172a;
  --text-muted: #64748b;
  --text-light: #94a3b8;

  --shadow-xs: 0 1px 2px rgba(0, 0, 0, 0.04);
  --shadow-sm: 0 2px 4px rgba(0, 0, 0, 0.06);
  --shadow-md: 0 4px 12px -2px rgba(15, 23, 42, 0.1), 0 2px 6px -1px rgba(15, 23, 42, 0.06);
  --shadow-lg: 0 20px 30px -8px rgba(15, 23, 42, 0.12), 0 10px 12px -6px rgba(15, 23, 42, 0.06);
  --shadow-xl: 0 25px 40px -12px rgba(15, 23, 42, 0.18);
  --shadow-glow: 0 0 20px rgba(99, 102, 241, 0.3);

  --transition-fast: 0.15s cubic-bezier(0.4, 0, 0.2, 1);
  --transition-normal: 0.25s cubic-bezier(0.4, 0, 0.2, 1);
  --transition-smooth: 0.35s cubic-bezier(0.4, 0, 0.2, 1);
}
</style>

<style scoped>

.window-container {
  background: linear-gradient(135deg, var(--bg-gradient-start) 0%, var(--bg-gradient-end) 100%);
  border: none;
  border-radius: 0;
  box-shadow: none;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  font-family:
    -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
  color: var(--text-main);
  position: relative;
}

.window-container::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: linear-gradient(90deg, var(--primary) 0%, #8b5cf6 50%, var(--primary) 100%);
  opacity: 0.9;
  z-index: 100;
}

/* Classical menu headers - Modernized */
.menu-bar {
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
  border-bottom: 1px solid var(--border-light);
  display: flex;
  padding: 8px 16px;
  user-select: none;
  z-index: 50;
  backdrop-filter: blur(8px);
}

.menu-item {
  position: relative;
  margin-right: 8px;
}

.menu-trigger {
  background: none;
  border: none;
  padding: 8px 14px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  color: var(--text-muted);
  border-radius: 8px;
  transition: var(--transition-fast);
  position: relative;
}

.menu-trigger:hover {
  background-color: var(--bg-control-hover);
  color: var(--text-dark);
  transform: translateY(-1px);
}

.menu-trigger::after {
  content: '';
  position: absolute;
  bottom: 2px;
  left: 50%;
  width: 0;
  height: 2px;
  background: var(--primary);
  transition: var(--transition-fast);
  transform: translateX(-50%);
}

.menu-trigger:hover::after {
  width: 60%;
}

.dropdown-menu {
  display: none;
  position: absolute;
  top: calc(100% + 6px);
  left: 0;
  background-color: #ffffff;
  border: 1px solid var(--border-light);
  box-shadow: var(--shadow-lg);
  z-index: 100;
  min-width: 220px;
  border-radius: 10px;
  overflow: hidden;
  padding: 6px;
  backdrop-filter: blur(12px);
}

.menu-item:hover .dropdown-menu {
  display: block;
  animation: slideDown var(--transition-fast);
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-2px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.dropdown-item {
  display: block;
  width: 100%;
  text-align: left;
  background: none;
  border: none;
  padding: 10px 14px;
  font-size: 13px;
  color: var(--text-main);
  cursor: pointer;
  border-radius: 8px;
  transition: var(--transition-fast);
  margin-bottom: 2px;
}

.dropdown-item:hover:not([disabled]) {
  background: linear-gradient(135deg, var(--primary-light) 0%, #dbeafe 100%);
  color: var(--primary) !important;
  transform: translateX(4px);
  box-shadow: var(--shadow-xs);
}

.dropdown-item[disabled] {
  color: #cbd5e1;
  cursor: not-allowed;
}

.file-label {
  margin: 0;
}

/* Central Widget viewport */
.centralwidget {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 24px 28px;
  gap: 18px;
  overflow: hidden;
  background: transparent;
}

.top-row-layout {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 15px;
  flex-shrink: 0;
}

.title-group {
  display: flex;
  align-items: center;
  gap: 10px;
}

.title {
  font-size: 24px;
  font-weight: 700;
  background: linear-gradient(135deg, var(--text-dark) 0%, var(--primary) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  letter-spacing: -0.5px;
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.05));
}

.version-badge {
  background: linear-gradient(135deg, var(--bg-control-hover) 0%, #e0e7ff 100%);
  color: var(--primary);
  font-size: 11px;
  font-weight: 600;
  padding: 3px 10px;
  border-radius: 8px;
  border: 1px solid var(--border-light);
  box-shadow: var(--shadow-xs);
  letter-spacing: 0.5px;
}

.navigation-controls {
  display: flex;
  align-items: center;
  gap: 8px;
}

/* Page navigator indicators */
.page-indicator {
  min-width: 170px;
  height: 42px;
  font-size: 13px;
  font-weight: 600;
  border: 1px solid var(--border-light);
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
  color: var(--text-main);
  transition: var(--transition-fast);
  box-shadow: var(--shadow-xs);
  letter-spacing: 0.3px;
}

.indicator-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: linear-gradient(135deg, var(--primary) 0%, #8b5cf6 100%);
  display: inline-block;
  box-shadow: 0 0 8px var(--primary-glow);
  animation: pulse-dot 2s ease-in-out infinite;
}

@keyframes pulse-dot {
  0%, 100% {
    transform: scale(1);
    opacity: 1;
  }
  50% {
    transform: scale(1.15);
    opacity: 0.8;
  }
}

.page-indicator.original-view {
  background-color: #fffbeb;
  border-color: #fde68a;
  color: #b45309;
}

.page-indicator.original-view .indicator-dot {
  background-color: #d97706;
}

.tool-button {
  width: 42px;
  height: 42px;
  font-size: 15px;
  font-weight: bold;
  color: var(--text-muted);
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
  border: 1px solid var(--border-light);
  border-radius: 10px;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: var(--transition-fast);
  box-shadow: var(--shadow-xs);
}

.tool-button:hover:not(:disabled) {
  background: linear-gradient(135deg, var(--bg-control-hover) 0%, #e0e7ff 100%);
  color: var(--primary);
  border-color: var(--border-focus);
  transform: translateY(-2px);
  box-shadow: var(--shadow-md);
}

.tool-button:active:not(:disabled) {
  transform: translateY(0);
}

.tool-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
  background-color: var(--bg-page);
}

.push-button {
  height: 42px;
  padding: 0 18px;
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
  border: 1px solid var(--border-light);
  border-radius: 10px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  color: var(--text-muted);
  transition: var(--transition-fast);
  box-shadow: var(--shadow-xs);
}

.push-button:hover:not(:disabled) {
  background: linear-gradient(135deg, var(--bg-control-hover) 0%, #e0e7ff 100%);
  color: var(--primary);
  border-color: var(--border-focus);
  transform: translateY(-1px);
  box-shadow: var(--shadow-sm);
}

.push-button.active {
  background: linear-gradient(135deg, var(--primary-light) 0%, #c7d2fe 100%);
  border-color: var(--primary);
  color: var(--primary);
  font-weight: 600;
  box-shadow: 0 0 12px var(--primary-glow);
}

/* Table Layout Styling - Grid Separation Structure */
.grid-displayer-container {
  flex: 1;
  border: 1px solid var(--border-light);
  background: linear-gradient(135deg, var(--bg-page) 0%, #f1f5f9 100%);
  border-radius: 12px;
  overflow: auto;
  padding: 16px;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.03);
}

.grid-displayer-container::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}
.grid-displayer-container::-webkit-scrollbar-track {
  background: transparent;
}
.grid-displayer-container::-webkit-scrollbar-thumb {
  background: var(--border-medium);
  border-radius: 4px;
}

.grid-displayer {
  width: 100%;
  height: 100%;
  border-collapse: separate;
  border-spacing: 12px;
  table-layout: fixed;
}

.grid-displayer td {
  padding: 0;
  vertical-align: middle;
  border: none;
  width: auto;
}

/* Modern Card Layout inside cells */
.cell-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  width: 100%;
  height: 100%;
  min-height: 90px;
  background: linear-gradient(135deg, #ffffff 0%, #fafbfc 100%);
  border: 1px solid var(--border-light);
  border-radius: 10px;
  box-shadow: var(--shadow-sm);
  transition: var(--transition-normal);
  cursor: pointer;
  user-select: none;
  position: relative;
  overflow: hidden;
}

.cell-content::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  background: linear-gradient(90deg, transparent 0%, var(--primary) 50%, transparent 100%);
  opacity: 0;
  transition: var(--transition-fast);
}

/* Ensure consistent cell sizing regardless of content */
.cell-content > * {
  flex-shrink: 0;
}

.cell-icon {
  font-size: 18px;
  margin-bottom: 6px;
  opacity: 0.6;
  transition: var(--transition-normal);
  filter: grayscale(0.3);
}

.cell-text {
  font-size: 14px;
  font-weight: 600;
  color: var(--text-dark);
  letter-spacing: 0.3px;
}

/* Filled VS Empty Visual Classes */
.grid-displayer td.is-empty .cell-content {
  background-color: rgba(241, 245, 249, 0.5);
  border: 1px dashed var(--border-medium);
  box-shadow: none;
}

.grid-displayer td.is-empty .cell-text {
  color: #94a3b8;
  font-weight: 400;
}

.grid-displayer td.is-empty .cell-icon {
  opacity: 0.35;
}

.grid-displayer td.is-filled:hover .cell-content {
  transform: translateY(-3px) scale(1.02);
  box-shadow: var(--shadow-lg);
  border-color: var(--primary);
}

.grid-displayer td.is-filled:hover .cell-content::before {
  opacity: 1;
}

.grid-displayer td.is-filled:hover .cell-icon {
  opacity: 1;
  filter: grayscale(0);
  transform: scale(1.1);
}

/* Tagged / Marked State */
.grid-displayer td.tagged .cell-content {
  background: linear-gradient(135deg, #fef9c3 0%, #fef3c7 100%) !important;
  border: 2px solid #f59e0b !important;
  transform: scale(0.95);
  animation: pulse-tag 2s ease-in-out infinite;
  box-shadow: 0 0 20px rgba(245, 158, 11, 0.3);
}

.grid-displayer td.tagged .cell-text {
  color: #92400e;
  font-weight: 700;
}

@keyframes pulse-tag {
  0%, 100% {
    box-shadow: 0 0 0 0px rgba(245, 158, 11, 0.4), 0 0 20px rgba(245, 158, 11, 0.2);
  }
  50% {
    box-shadow: 0 0 0 8px rgba(245, 158, 11, 0.15), 0 0 30px rgba(245, 158, 11, 0.3);
  }
}

/* Swapped Highlight State */
.grid-displayer td.swapped .cell-content {
  background: linear-gradient(135deg, #fee2e2 0%, #fecaca 100%) !important;
  border-color: #f87171 !important;
  border-style: solid;
  box-shadow: 0 0 15px rgba(248, 113, 113, 0.25);
}

.grid-displayer td.swapped .cell-text {
  color: #991b1b;
  font-style: italic;
  font-weight: 700;
}

.grid-displayer td.swapped .cell-text::after {
  content: ' ✎';
  font-size: 11px;
  vertical-align: super;
  font-style: normal;
  opacity: 0.8;
}

/* Bottom Action Row */
.bottom-layout {
  display: flex;
  justify-content: center;
  align-items: center;
  flex-shrink: 0;
  padding: 4px 0;
}

.shuffle-button {
  width: 340px;
  height: 56px;
  font-size: 17px;
  font-weight: 700;
  background: linear-gradient(135deg, var(--primary) 0%, #8b5cf6 50%, var(--primary-hover) 100%);
  background-size: 200% 200%;
  color: #ffffff;
  border: none;
  border-radius: 12px;
  cursor: pointer;
  box-shadow: 0 6px 20px rgba(99, 102, 241, 0.35), 0 0 0 1px rgba(99, 102, 241, 0.1);
  transition: var(--transition-normal);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  letter-spacing: 0.5px;
  position: relative;
  overflow: hidden;
}

.shuffle-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: 0.5s;
}

.shuffle-button:hover:not(:disabled)::before {
  left: 100%;
}

.shuffle-button:hover:not(:disabled) {
  box-shadow: 0 8px 28px rgba(99, 102, 241, 0.45), 0 0 0 2px rgba(99, 102, 241, 0.15);
  transform: translateY(-2px);
  background-position: 100% 0;
}

.shuffle-button:active:not(:disabled) {
  transform: translateY(0);
  box-shadow: 0 4px 16px rgba(99, 102, 241, 0.3);
}

.shuffle-button:disabled {
  background: #cbd5e1;
  color: #64748b;
  cursor: not-allowed;
  box-shadow: none;
}

/* Shuffling spinner animation */
.spinner {
  width: 20px;
  height: 20px;
  border: 3px solid rgba(255, 255, 255, 0.3);
  border-top-color: #ffffff;
  border-radius: 50%;
  animation: rotate 0.7s linear infinite;
  box-shadow: 0 0 10px rgba(255, 255, 255, 0.3);
}

@keyframes rotate {
  to {
    transform: rotate(360deg);
  }
}

/* Status Bar */
.statusbar {
  background: linear-gradient(to top, var(--bg-page) 0%, #ffffff 100%);
  border-top: 1px solid var(--border-light);
  padding: 10px 20px;
  font-size: 12px;
  color: var(--text-muted);
  flex-shrink: 0;
  box-shadow: 0 -2px 8px rgba(0, 0, 0, 0.03);
}

.status-content {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-indicator {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background-color: var(--border-medium);
  transition: var(--transition-fast);
}

.status-indicator.active {
  background: linear-gradient(135deg, #10b981 0%, #34d399 100%);
  box-shadow: 0 0 10px rgba(16, 185, 129, 0.6), 0 0 20px rgba(16, 185, 129, 0.3);
  animation: status-pulse 2s ease-in-out infinite;
}

@keyframes status-pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}

.status-text {
  font-weight: 500;
  letter-spacing: 0.3px;
}
</style>
