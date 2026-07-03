<template>
  <header class="menu-bar">
    <!-- File Operations -->
    <DropDownMenu label="文件 (File)">
      <label class="dropdown-item file-label">
        導入座位排佈
        <span class="key-shortcut-indicator">Ctrl + I</span>
        <input
          ref="gridInputRef"
          type="file"
          accept=".csv, application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
          style="display: none"
          @change="onGridImport"
        />
      </label>
      <button class="dropdown-item" :disabled="!props.isGridLoaded" @click="$emit('grid-export')">
        導出座位排佈
        <span class="key-shortcut-indicator">Ctrl + E</span>
      </button>
    </DropDownMenu>

    <!-- Colors Configuration -->
    <DropDownMenu label="顔色設定 (Color)">
      <label class="dropdown-item file-label">
        導入顔色配置 (JSON)
        <input
          ref="colorInputRef"
          type="file"
          accept=".json"
          style="display: none"
          @change="onColorImport"
        />
      </label>
      <button
        class="dropdown-item"
        :disabled="colorPresetCount === 0"
        @click="$emit('clear-colors')"
      >
        卸載顏色配置
      </button>
    </DropDownMenu>

    <!-- Algorithmic Constraints -->
    <DropDownMenu label="算法約束 (Constraints)">
      <label class="dropdown-item file-label">
        導入約束配置 (JSON)
        <input
          ref="constraintsInputRef"
          type="file"
          accept=".json"
          style="display: none"
          @change="onConstraintsImport"
        />
      </label>
      <button
        class="dropdown-item"
        :disabled="!hasCustomConfig"
        @click="$emit('reset-constraints')"
      >
        重設為默認算法
      </button>
    </DropDownMenu>
  </header>
</template>

<script setup lang="ts">
import { useTemplateRef } from 'vue'
import DropDownMenu from '@/components/common/DropDownMenu.vue'

const props = defineProps<{
  isGridLoaded: boolean
  colorPresetCount: number
  hasCustomConfig: boolean
}>()

const emit = defineEmits<{
  'csv-import': [file: File]
  'xlsx-import': [file: File]
  'grid-export': []
  'color-import': [file: File]
  'clear-colors': []
  'constraints-import': [file: File]
  'reset-constraints': []
}>()

const csvInputRef = useTemplateRef<HTMLInputElement>('csvInputRef')
const xlsxInputRef = useTemplateRef<HTMLInputElement>('xlsxInputRef')
const colorInputRef = useTemplateRef<HTMLInputElement>('colorInputRef')
const constraintsInputRef = useTemplateRef<HTMLInputElement>('constraintsInputRef')

const getFileFromEvent = (event: Event): File | null => {
  const target = event.target as HTMLInputElement
  if (!target.files?.length) return null
  const file = target.files[0]!
  target.value = ''
  return file
}

const onGridImport = (event: Event) => {
  const file = getFileFromEvent(event)
  if (file?.name.endsWith('.csv')) emit('csv-import', file)
  if (file?.name.endsWith('.xlsx')) emit('xlsx-import', file)
}

const onColorImport = (event: Event) => {
  const file = getFileFromEvent(event)
  if (file) emit('color-import', file)
}

const onConstraintsImport = (event: Event) => {
  const file = getFileFromEvent(event)
  if (file) emit('constraints-import', file)
}
</script>

<style scoped>
.menu-bar {
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
  border-bottom: 1px solid var(--border-light);
  display: flex;
  padding: 8px 16px;
  user-select: none;
  z-index: 50;
  backdrop-filter: blur(8px);
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
</style>
