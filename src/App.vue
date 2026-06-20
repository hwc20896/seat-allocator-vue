<template>
  <div class="window-container">
    <AppHeader
      :is-grid-loaded="grid.isGridLoaded.value"
      :color-preset-count="colorConfig.colorPresets.value.length"
      :has-custom-config="constraints.hasCustomConfig.value"
      @csv-import="handleCSVImport"
      @csv-export="handleCSVExport"
      @color-import="handleColorImport"
      @clear-colors="handleClearColors"
      @constraints-import="handleConstraintsImport"
      @reset-constraints="handleResetConstraints"
    />

    <main class="centralwidget">
      <div class="top-row-layout">
        <div class="title-group">
          <h1 class="title">座位分配系統</h1>
          <span class="version-badge">v3.0</span>
        </div>

        <PageNavigator
          :page-label="grid.pageLabel.value"
          :current-index="grid.currentIndex.value"
          :total-pages="grid.totalPages.value"
          :is-shuffling="grid.isShuffling.value"
          :is-grid-loaded="grid.isGridLoaded.value"
          :show-original="grid.showOriginal.value"
          @navigate="handleNavigate"
          @toggle-original="handleToggleOriginal"
        />
      </div>

      <SeatGrid
        :grid="renderedGrid"
        :is-shuffling="grid.isShuffling.value"
        :tagged-row="taggedRow"
        :tagged-col="taggedCol"
        :get-cell-color="colorConfig.getCellColor"
        :is-cell-swapped="isCellSwapped"
        @cell-click="handleCellClick"
      />

      <ShuffleButton
        :is-shuffling="grid.isShuffling.value"
        :is-grid-loaded="grid.isGridLoaded.value"
        @shuffle="handleShuffle"
      />
    </main>

    <AppFooter :status-text="statusText" :is-grid-loaded="grid.isGridLoaded.value" />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'

import AppHeader from '@/components/layout/AppHeader.vue'
import AppFooter from '@/components/layout/AppFooter.vue'
import PageNavigator from '@/components/controls/PageNavigator.vue'
import ShuffleButton from '@/components/controls/ShuffleButton.vue'
import SeatGrid from '@/components/grid/SeatGrid.vue'

import { useWasm } from '@/composables/useWasm'
import { useGridShuffle } from '@/composables/useGridShuffle'
import { useColorConfig } from '@/composables/useColorConfig'
import { useConstraintsConfig } from '@/composables/useConstraintsConfig'
import { useFileIO } from '@/composables/useFileIO'

type Grid = string[][]

// ==========================================
// Composable Instances
// ==========================================
const wasm = useWasm()
const grid = useGridShuffle(wasm.wasmModule, wasm.shufflerInstance)
const colorConfig = useColorConfig()
const constraints = useConstraintsConfig()
const fileIO = useFileIO()

// ==========================================
// App-level State
// ==========================================
const statusText = ref('未導入')

// Tagged cell for swap interaction
const taggedCell = ref<{ row: number; col: number } | null>(null)
const taggedRow = computed(() => taggedCell.value?.row ?? null)
const taggedCol = computed(() => taggedCell.value?.col ?? null)

// Choose which grid to render
const renderedGrid = computed<Grid>(() => {
  if (!grid.isGridLoaded.value) return []
  if (grid.showOriginal.value) return grid.originalGrid.value
  return grid.currentGrid.value
})

// ==========================================
// Lifecycle
// ==========================================
onMounted(async () => {
  const success = await wasm.initWasm()
  if (success) {
    statusText.value = '系統就緒，等待導入配置...'
  } else {
    statusText.value = 'WebAssembly 模組載入失敗'
  }
})

// ==========================================
// File Import / Export Handlers
// ==========================================
const handleCSVImport = async (file: File) => {
  if (!wasm.wasmReady.value) return
  try {
    const text = await fileIO.readTextFile(file)
    const parsed = fileIO.parseCSV(text)
    if (parsed.length === 0) return
    const success = grid.loadNewGrid(parsed)
    if (success) {
      taggedCell.value = null
      statusText.value = `已成功導入檔案：${file.name}`
    }
  } catch {
    alert('檔案讀取失敗。')
  }
}

const handleCSVExport = () => {
  if (grid.currentGrid.value.length === 0) return

  if (grid.showOriginal.value) {
    const confirmChoice = confirm('這是原始名單，確定要導出嗎？\n\n建議先執行洗牌操作後再導出。')
    if (!confirmChoice) return
  }

  fileIO.exportCSV(
    grid.currentGrid.value,
    `allocated_seats_page_${grid.currentIndex.value}.csv`,
  )
  statusText.value = `已導出至：allocated_seats_page_${grid.currentIndex.value}.csv`
}

// ==========================================
// Color Configuration Handlers
// ==========================================
const handleColorImport = async (file: File) => {
  try {
    const text = await fileIO.readTextFile(file)
    const success = colorConfig.loadColors(text)
    if (success) {
      statusText.value = `顏色配置載入成功。`
    }
  } catch {
    alert('JSON 顏色配置解析失敗。')
  }
}

const handleClearColors = () => {
  colorConfig.clearColors()
  statusText.value = `顏色配置已卸載。`
}

// ==========================================
// Constraints Configuration Handlers
// ==========================================
const handleConstraintsImport = async (file: File) => {
  try {
    const text = await fileIO.readTextFile(file)
    const success = constraints.loadConstraints(text)
    if (success) {
      statusText.value = `算法約束載入成功：${file.name}。請重新導入座位配置或洗牌。`
    }
  } catch {
    alert('JSON 算法約束檔案格式錯誤。')
  }
}

const handleResetConstraints = () => {
  constraints.resetConstraints()
  statusText.value = `約束已重設。已還原為基礎隨機分配算法。`
}

// ==========================================
// Shuffle Handler
// ==========================================
const handleShuffle = async () => {
  taggedCell.value = null
  const success = await grid.beginShuffleAnimation()
  if (success) {
    statusText.value = `洗牌完成，已生成第 ${grid.currentIndex.value} 次分配結果。`
  }
}

// ==========================================
// Navigation Handlers
// ==========================================
const handleNavigate = (step: number) => {
  grid.navigatePage(step)
  taggedCell.value = null
}

const handleToggleOriginal = () => {
  grid.toggleOriginal()
  taggedCell.value = null
}

// ==========================================
// Cell Interaction (Click to Swap)
// ==========================================
const handleCellClick = ({ row, col }: { row: number; col: number }) => {
  if (grid.showOriginal.value || grid.isShuffling.value) return

  if (!taggedCell.value) {
    taggedCell.value = { row, col }
    statusText.value = `已標記單元格 [${row + 1}, ${col + 1}]，再次點擊其他格子即可進行位置交換。`
  } else if (taggedCell.value.row === row && taggedCell.value.col === col) {
    taggedCell.value = null
    statusText.value = `已取消標記。`
  } else {
    const { row: tRow, col: tCol } = taggedCell.value
    grid.swapCells(tRow, tCol, row, col)
    statusText.value = `已手動交換單元格：[${tRow + 1}, ${tCol + 1}] <-> [${row + 1}, ${col + 1}]`
    taggedCell.value = null
  }
}

const isCellSwapped = (row: number, col: number): boolean => {
  return grid.isCellManuallyModified(row, col)
}
</script>

<style>
/* Modern System Variables - Global scope (must be unscoped for :root) */
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
  width: 100%;
  height: 100%;
  min-width: 800px;
  min-height: 600px;
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
</style>
