<template>
  <div class="grid-displayer-container">
    <div v-if="grid && grid.rowCount() > 0" class="grid-displayer-wrapper">
      <div class="blackboard-bar">
        <span class="blackboard-arrow">⬆</span>
        <span>黑板（前方）</span>
      </div>
      <table class="grid-displayer">
        <thead>
          <tr>
            <th class="index-corner" aria-hidden="true"></th>
            <th
              v-for="cIdx in grid?.colCount()"
              :key="`col-${cIdx}`"
              class="index-header col-index"
              scope="col"
              :title="`第 ${cIdx} 列（從左往右數）`"
            >
              {{ cIdx }}
            </th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="rIdx in grid?.rowCount()" :key="`row-${rIdx}`">
            <th
              class="index-header row-index"
              scope="row"
              :title="`第 ${rIdx} 行（從前到後數，第 1 行靠黑板）`"
            >
              {{ rIdx }}
            </th>
            <td v-for="cIdx in grid?.colCount()" :key="`cell-${rIdx}-${cIdx}`" class="grid-cell">
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

.grid-displayer-wrapper {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.blackboard-bar {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 4px 0 2px;
  font-size: 14px;
  font-weight: 700;
  letter-spacing: 1px;
  color: var(--primary);
  flex-shrink: 0;
  user-select: none;
}

.blackboard-arrow {
  font-size: 14px;
}

.index-header {
  padding: 2px 0;
  font-size: 11px;
  font-weight: 600;
  color: var(--text-light);
  text-align: center;
  border: none;
  background: transparent;
  user-select: none;
  white-space: nowrap;
}

.index-corner {
  border: none;
  background: transparent;
}

.grid-displayer {
  flex: 1;
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
