<template>
  <div class="grid-displayer-container">
    <table class="grid-displayer">
      <tbody>
        <tr v-for="rIdx in grid?.rowCount()" :key="rIdx">
          <td v-for="cIdx in grid?.colCount()" :key="cIdx" class="grid-cell">
            <GridCell
              :text="grid?.getByPos(rIdx - 1, cIdx - 1) || ''"
              :color="getCellColor(grid?.getByPos(rIdx - 1, cIdx - 1) || '')"
              :is-tagged="taggedRow === rIdx - 1 && taggedCol === cIdx - 1"
              :is-swapped="!isShuffling && isCellSwapped(new Position(rIdx - 1, cIdx - 1))"
              :is-currently-original="isCurrentlyOriginal"
              :is-shuffling="isShuffling"
              @click="$emit('cell-click', new Position(rIdx - 1, cIdx - 1))"
            />
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script setup lang="ts">
import GridCell from './GridCell.vue'
import type { Grid } from '@/assets/wasm/alloc_algo'
import { Position } from '@/utils/Position.ts'

defineProps<{
  grid: Grid | null
  isShuffling: boolean
  taggedRow: number | null
  taggedCol: number | null
  getCellColor: (text: string) => string
  isCellSwapped: (position: Position) => boolean
  isCurrentlyOriginal: boolean
}>()

defineEmits<{
  'cell-click': [Position]
}>()
</script>

<style scoped>
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

.grid-displayer td.grid-cell {
  padding: 0;
  vertical-align: middle;
  border: none;
  width: auto;
}
</style>
