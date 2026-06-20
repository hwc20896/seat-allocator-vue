<template>
  <div class="cell-content" :class="contentClass" :style="cellStyle" @click="$emit('click')">
    <span class="cell-icon">🪑</span>
    <span class="cell-text">{{ text || '空位' }}</span>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

const props = defineProps<{
  text: string
  color: string
  isTagged: boolean
  isSwapped: boolean
}>()

defineEmits<{
  click: []
}>()

const contentClass = computed(() => {
  const classes: string[] = []
  if (props.isTagged) classes.push('tagged')
  if (props.isSwapped) classes.push('swapped')
  if (!props.text) classes.push('empty-element')
  return classes
})

const cellStyle = computed(() => ({
  color: props.color,
}))
</script>

<style scoped>
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

/* Empty state */
.cell-content:has(.cell-text:empty) {
  background-color: rgba(241, 245, 249, 0.5);
  border: 1px dashed var(--border-medium);
  box-shadow: none;
}

/* Hover */
.cell-content:hover {
  transform: translateY(-3px) scale(1.02);
  box-shadow: var(--shadow-lg);
  border-color: var(--primary);
}

.cell-content:hover::before {
  opacity: 1;
}

.cell-content:hover .cell-icon {
  opacity: 1;
  filter: grayscale(0);
  transform: scale(1.1);
}

/* Tagged state */
.cell-content.tagged {
  background: linear-gradient(135deg, #fef9c3 0%, #fef3c7 100%) !important;
  border: 2px solid #f59e0b !important;
  transform: scale(0.95);
  animation: pulse-tag 2s ease-in-out infinite;
  box-shadow: 0 0 20px rgba(245, 158, 11, 0.3);
}

.cell-content.tagged .cell-text {
  color: #92400e;
  font-weight: 700;
}

@keyframes pulse-tag {
  0%,
  100% {
    box-shadow:
      0 0 0 0px rgba(245, 158, 11, 0.4),
      0 0 20px rgba(245, 158, 11, 0.2);
  }
  50% {
    box-shadow:
      0 0 0 8px rgba(245, 158, 11, 0.15),
      0 0 30px rgba(245, 158, 11, 0.3);
  }
}

/* Swapped state */
.cell-content.swapped {
  background: linear-gradient(135deg, #fee2e2 0%, #fecaca 100%) !important;
  border-color: #f87171 !important;
  border-style: solid;
  box-shadow: 0 0 15px rgba(248, 113, 113, 0.25);
}

.cell-content.swapped .cell-text {
  color: #991b1b;
  font-style: italic;
  font-weight: 700;
}

.cell-content.swapped .cell-text::after {
  content: ' ✎';
  font-size: 11px;
  vertical-align: super;
  font-style: normal;
  opacity: 0.8;
}

.cell-content.empty-element {
  opacity: 0.7;
}
</style>
