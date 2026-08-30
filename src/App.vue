<template>
  <div class="window-container">
    <AppHeader
      :is-grid-loaded="grid.isGridLoaded.value"
      :color-preset-count="colorConfig.colorPresets.value.length"
      :has-custom-config="constraints.hasCustomConfig.value"
      @csv-import="handleCSVImport"
      @xlsx-import="handleXLSXImport"
      @grid-export="handleGridExport"
      @color-import="handleColorImport"
      @clear-colors="handleClearColors"
      @constraints-import="handleConstraintsImport"
      @reset-constraints="handleResetConstraints"
      @open-constraints-editor="isConstraintsEditorOpen = true"
    />

    <main class="centralwidget">
      <div class="top-row-layout">
        <div class="title-group">
          <h1 class="title">座位分配系統</h1>
          <span class="version-badge">v{{ appVersion }}</span>
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
        :is-currently-original="isOriginal"
        @cell-click="handleCellClick"
      />

      <ShuffleButton
        :is-shuffling="grid.isShuffling.value"
        :is-grid-loaded="grid.isGridLoaded.value"
        @shuffle="handleShuffle"
      />
    </main>

    <AppFooter :status-text="statusText" :is-grid-loaded="grid.isGridLoaded.value" />

    <ConstraintsEditor
      :visible="isConstraintsEditorOpen"
      :initial-config="constraints.currentConfigJson.value"
      :names="allNames"
      :check-feasibility="checkFeasibility"
      @apply="handleConstraintsApply"
      @cancel="isConstraintsEditorOpen = false"
    />
  </div>
</template>

<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'

import AppHeader from '@/components/layout/AppHeader.vue'
import AppFooter from '@/components/layout/AppFooter.vue'
import PageNavigator from '@/components/controls/PageNavigator.vue'
import ShuffleButton from '@/components/controls/ShuffleButton.vue'
import SeatGrid from '@/components/grid/SeatGrid.vue'
import ConstraintsEditor from '@/components/constraints/ConstraintsEditor.vue'

import { useWasm } from '@/composables/useWasm'
import { useGridShuffle } from '@/composables/useGridShuffle'
import { useColorConfig } from '@/composables/useColorConfig'
import { useConstraintsConfig } from '@/composables/useConstraintsConfig'
import { useFileIO } from '@/composables/useFileIO'
import { useKeyboardShortcut } from '@/composables/useKeyboardShortcuts'
import { useUnsavedChangesGuard } from '@/composables/useUnsavedChangesGuard'

import type { FeasibilityReport, Grid, PointerOf } from '@/assets/wasm/alloc_algo'
import { Position } from '@/utils/Position.ts'

// ==========================================
// Composable Instances
// ==========================================
const wasm = useWasm()
const constraints = useConstraintsConfig()
const grid = useGridShuffle(wasm.wasmModule, wasm.wasmReady, wasm.shufflerInstance, () =>
  constraints.buildWasmConfig(wasm.wasmModule.value),
)
const colorConfig = useColorConfig()
const fileIO = useFileIO(wasm.wasmModule)
const unsavedGuard = useUnsavedChangesGuard()

const appVersion = __APP_VERSION__

// ==========================================
// App-level State
// ==========================================
const statusText = ref('未導入')
const isOriginal = ref(false)
const isConstraintsEditorOpen = ref(false)

// Tagged cell for swap interaction
const taggedCell = ref<PointerOf<Position>>(null)
const taggedRow = computed(() => taggedCell.value?.row ?? null)
const taggedCol = computed(() => taggedCell.value?.col ?? null)

// Choose which grid to render
const renderedGrid = computed<Grid | null>(() => {
  const module = wasm.wasmModule.value
  if (!module) return null
  if (!grid.isGridLoaded.value) return new module.Grid()
  if (grid.showOriginal.value && grid.originalGrid.value) return grid.originalGrid.value
  return grid.currentGrid.value
})

// Unique names from the imported seating chart, for constraint editor autocomplete
const allNames = computed(() => {
  const original = grid.originalGrid.value
  if (!original || original.empty()) return []
  return [...new Set(original.rawData().filter((name) => name.trim() !== ''))]
})

// Test whether the given constraints JSON is satisfiable against the imported grid
const checkFeasibility = (json: string): FeasibilityReport | null => {
  const module = wasm.wasmModule.value
  const original = grid.originalGrid.value
  if (!module || !original || original.empty()) return null
  const cfg = constraints.buildWasmConfigFromJson(module, json)
  if (!cfg) return null
  return module.checkFeasibility(original, cfg, true, 50_000)
}

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
    const parsed = wasm.wasmModule.value!.Grid.fromCSV(text)
    if (parsed.empty()) return
    const success = grid.loadNewGrid(parsed)
    if (!success) return
    taggedCell.value = null
    statusText.value = `已成功導入檔案：${file.name}`
    unsavedGuard.markDirty()
  } catch (e) {
    alert('檔案讀取失敗。')
    console.error(e)
  }
}

const handleXLSXImport = async (file: File) => {
  if (!wasm.wasmReady.value) return
  try {
    const parsed = await fileIO.parseXLSX(file)
    if (parsed.empty()) return
    const success = grid.loadNewGrid(parsed)
    if (!success) return
    taggedCell.value = null
    statusText.value = `已成功導入檔案：${file.name}`
    unsavedGuard.markDirty()
  } catch (e) {
    alert('檔案讀取失敗。')
    console.error(e)
  }
}

const handleGridExport = async () => {
  if (grid.currentGrid.value?.empty()) return

  if (grid.showOriginal.value) {
    const confirmChoice = confirm('這是原始名單，確定要導出嗎？\n\n建議先執行洗牌操作後再導出。')
    if (!confirmChoice) return
  }

  const defaultFileName = `allocated_seats_page_${grid.currentIndex.value}`

  if (!window.showSaveFilePicker) {
    alert('您的瀏覽器不支援另存新檔功能，請更新瀏覽器或使用預設下載。')
    return
  }

  try {
    const fileHandle = await window.showSaveFilePicker({
      suggestedName: defaultFileName,
      types: [
        {
          description: 'Excel 活頁簿',
          accept: {
            'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet': ['.xlsx'],
          },
        },
        {
          description: 'CSV (逗號分隔)',
          accept: {
            'text/csv': ['.csv'],
          },
        },
      ],
    })

    const actualFileName = fileHandle.name
    const writable = await fileHandle.createWritable()

    if (actualFileName.endsWith('.xlsx')) {
      const excelData = fileIO.generateXLSXBuffer(grid.currentGrid.value!)
      await writable.write(excelData)
      statusText.value = `已成功匯出 Excel：${actualFileName}`
    } else if (actualFileName.endsWith('.csv')) {
      const csvData = grid.currentGrid.value!.toCSVString()
      await writable.write(csvData)
      statusText.value = `已成功匯出 CSV：${actualFileName}`
    }

    await writable.close()
    unsavedGuard.markClean()
  } catch (err: unknown) {
    if (err instanceof Error && err.name === 'AbortError') {
      statusText.value = '已取消導出。'
    } else {
      console.error('導出失敗：', err)
      statusText.value = '導出失敗，請檢查權限或控制台錯誤。'
    }
  }
}

// ==========================================
// Color Configuration Handlers
// ==========================================
const handleColorImport = async (file: File) => {
  try {
    const text = await fileIO.readTextFile(file)
    const success = colorConfig.loadColors(text)
    if (!success) return
    statusText.value = `顏色配置載入成功。`
    unsavedGuard.markDirty()
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
      statusText.value = `算法約束載入成功：${file.name}。`

      // Try to apply constraints immediately if wasm is ready
      if (wasm.wasmReady.value && typeof grid.applyConfig === 'function') {
        const cfg = constraints.buildWasmConfig(wasm.wasmModule.value)
        const applied = grid.applyConfig(cfg)
        if (await applied) {
          statusText.value += ' 已套用約束。'
        } else {
          statusText.value += ' 套用約束失敗，請重新導入座位配置或手動洗牌。'
        }
      } else {
        statusText.value += ' 請重新導入座位配置或洗牌以套用新約束。'
      }
    }
  } catch {
    alert('JSON 算法約束檔案格式錯誤。')
  }
}

const handleResetConstraints = () => {
  constraints.resetConstraints()
  statusText.value = `約束已重設。已還原為基礎隨機分配算法。`
}

const handleConstraintsApply = async (json: string) => {
  const success = constraints.loadConstraints(json)
  if (!success) return

  statusText.value = `約束已更新。`

  // Apply constraints immediately if wasm is ready
  if (wasm.wasmReady.value && typeof grid.applyConfig === 'function') {
    const cfg = constraints.buildWasmConfig(wasm.wasmModule.value)
    const applied = await grid.applyConfig(cfg)
    if (applied) {
      statusText.value += ' 已套用約束。'
    } else {
      statusText.value += ' 套用約束失敗，請重新導入座位配置或手動洗牌。'
    }
  } else {
    statusText.value += ' 請重新導入座位配置或洗牌以套用新約束。'
  }

  isConstraintsEditorOpen.value = false
}

// ==========================================
// Shuffle Handler
// ==========================================
const handleShuffle = async () => {
  taggedCell.value = null
  const success = await grid.beginShuffleAnimation()
  if (!success) return
  statusText.value = `洗牌完成，已生成第 ${grid.currentIndex.value} 次分配結果。`
  unsavedGuard.markDirty()
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
  isOriginal.value = !isOriginal.value
}

// ==========================================
// Cell Interaction (Click to Swap)
// ==========================================
const handleCellClick = (position: Position) => {
  if (grid.showOriginal.value || grid.isShuffling.value) return

  if (!taggedCell.value) {
    taggedCell.value = position
    statusText.value = `已標記單元格 ${position.toString(true)}，再次點擊其他格子即可進行位置交換。`
  } else if (taggedCell.value?.equals(position)) {
    taggedCell.value = null
    statusText.value = `已取消標記。`
  } else {
    const tPos = taggedCell.value
    grid.swapCells(tPos, position)
    statusText.value = `已手動交換單元格：${tPos} ⟺ ${position}`
    taggedCell.value = null
    unsavedGuard.markDirty()
  }
}

const isCellSwapped = (pos: Position): boolean => {
  return grid.isCellManuallyModified(pos)
}

useKeyboardShortcut('enter', () => {
  console.debug('Enter pressed')
  handleShuffle()
})
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
