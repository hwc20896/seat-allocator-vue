<template>
  <div class="grid-displayer-container">
    <table class="grid-displayer">
      <tbody>
        <tr v-for="(row, rIdx) in grid" :key="rIdx">
          <td v-for="(cell, cIdx) in row" :key="cIdx" class="grid-cell">
            <GridCell
              :text="cell"
              :color="getCellColor(cell)"
              :is-tagged="taggedRow === rIdx && taggedCol === cIdx"
              :is-swapped="!isShuffling && isCellSwapped(rIdx, cIdx)"
              @click="$emit('cell-click', { row: rIdx, col: cIdx })"
            />
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script setup lang="ts">
import GridCell from './GridCell.vue'

type Grid = string[][]

defineProps<{
  grid: Grid
  isShuffling: boolean
  taggedRow: number | null
  taggedCol: number | null
  getCellColor: (text: string) => string
  isCellSwapped: (row: number, col: number) => boolean
}>()

defineEmits<{
  'cell-click': [{ row: number; col: number }]
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
