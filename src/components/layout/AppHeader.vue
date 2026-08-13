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
    <DropDownMenu label="顏色設定 (Color)">
      <label class="dropdown-item file-label">
        導入顏色配置
        <span class="key-shortcut-indicator">Ctrl + Shift + C</span>
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
        :disabled="props.colorPresetCount === 0"
        @click="$emit('clear-colors')"
      >
        重設
        <span class="key-shortcut-indicator">Ctrl + Alt + C</span>
      </button>
    </DropDownMenu>

    <!-- Algorithmic Constraints -->
    <DropDownMenu label="算法約束 (Constraints)">
      <label class="dropdown-item file-label">
        導入約束配置
        <span class="key-shortcut-indicator">Ctrl + Shift + K</span>
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
        :disabled="!props.hasCustomConfig"
        @click="$emit('reset-constraints')"
      >
        重設
        <span class="key-shortcut-indicator">Ctrl + Alt + K</span>
      </button>
    </DropDownMenu>
  </header>
</template>

<script setup lang="ts">
import { useTemplateRef } from 'vue'
import DropDownMenu from '@/components/common/DropDownMenu.vue'
import { useKeyboardShortcut } from '@/composables/useKeyboardShortcuts'

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

const gridInputRef = useTemplateRef<HTMLInputElement>('gridInputRef')
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

useKeyboardShortcut('ctrl+i', () => {
  console.debug('ctrl+i triggered')
  gridInputRef.value?.click()
})

useKeyboardShortcut('ctrl+e', () => {
  console.debug('ctrl+e triggered')
  if (!props.isGridLoaded) return
  emit('grid-export')
})

useKeyboardShortcut('ctrl+shift+c', () => {
  console.debug('ctrl+shift+c triggered')
  colorInputRef.value?.click()
})

useKeyboardShortcut('ctrl+alt+c', () => {
  console.debug('ctrl+alt+c triggered')
  if (props.colorPresetCount === 0) return
  emit('clear-colors')
})

useKeyboardShortcut('ctrl+shift+k', () => {
  console.debug('ctrl+shift+k triggered')
  constraintsInputRef.value?.click()
})

useKeyboardShortcut('ctrl+alt+k', () => {
  console.debug('ctrl+alt+k triggered')
  if (!props.hasCustomConfig) return
  emit('reset-constraints')
})
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

.key-shortcut-indicator {
  display: inline-block;
  margin-left: 8px;
  padding: 2px 6px;
  border: 1px solid rgba(79, 70, 229, 0.7);
  border-radius: 4px;
  background-color: #eef2ff;
  color: #4f46e5;
  font-size: 11px;
  font-weight: 500;
}

.dropdown-item[disabled] .key-shortcut-indicator {
  opacity: 0.3;
}
</style>
