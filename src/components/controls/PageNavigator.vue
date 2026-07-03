<template>
  <div class="navigation-controls">
    <button
      class="tool-button"
      :disabled="props.currentIndex <= 1 || props.isShuffling"
      @click="$emit('navigate', -1)"
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
      :disabled="props.currentIndex >= props.totalPages || props.isShuffling"
      @click="$emit('navigate', 1)"
      title="後一頁"
    >
      &gt;
    </button>

    <button
      class="push-button toggle-button"
      :class="{ active: props.showOriginal }"
      :disabled="!props.isGridLoaded || props.isShuffling || props.totalPages === 0"
      @click="$emit('toggle-original')"
    >
      {{ showOriginal ? '顯示當前打亂' : '顯示原始列表' }}
    </button>
  </div>
</template>

<script setup lang="ts">
import { useKeyboardShortcut, SPECIAL_KEYS } from '@/composables/useKeyboardShortcuts'

const props = defineProps<{
  pageLabel: string
  currentIndex: number
  totalPages: number
  isShuffling: boolean
  isGridLoaded: boolean
  showOriginal: boolean
}>()

const emit = defineEmits<{
  navigate: [step: number]
  'toggle-original': []
}>()

useKeyboardShortcut(
  { key: SPECIAL_KEYS.PAGE_UP },
  () => {
    console.debug('Page Up triggered')
    if (props.currentIndex <= 1 || props.isShuffling) return
    emit('navigate', -1)
  },
  { preventDefault: true },
)

useKeyboardShortcut(
  { key: SPECIAL_KEYS.PAGE_DOWN },
  () => {
    console.debug('Page Down triggered')
    if (props.currentIndex >= props.totalPages || props.isShuffling) return
    emit('navigate', 1)
  },
  { preventDefault: true },
)

useKeyboardShortcut(
  { key: SPECIAL_KEYS.HOME },
  () => {
    console.debug('Home Key triggered')
    if (!props.isGridLoaded || props.isShuffling || props.totalPages === 0) return
    emit('toggle-original')
  },
  { preventDefault: true },
)
</script>

<style scoped>
.navigation-controls {
  display: flex;
  align-items: center;
  gap: 8px;
}

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
  0%,
  100% {
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
</style>
