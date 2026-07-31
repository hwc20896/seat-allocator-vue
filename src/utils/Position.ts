import type { Grid } from '@/assets/wasm/alloc_algo'

export class Position{
  constructor(public row: number, public col: number) {}

  toString(baseZero: boolean = false): string {
    if (baseZero){
      return `(${this.row + 1}, ${this.col + 1})`
    }
    return `(${this.row}, ${this.col})`
  }

  equals(other: Position): boolean {
    return this.row === other.row && this.col === other.col
  }
}

export const swap = (grid: Grid, pos1: Position, pos2: Position): Grid => {
  const result = grid.clone();

  const isPositionValid = (pos: Position): boolean => {
    return (
      pos.row >= 0 && pos.row < result.rowCount() && pos.col >= 0 && pos.col < result.colCount()
    )
  }

  if (!isPositionValid(pos1)) {
    throw new RangeError(`Position ${pos1} out of range`);
  }

  if (!isPositionValid(pos2)) {
    throw new RangeError(`Position ${pos2} out of range`);
  }

  const temp = result.getByPos(pos1.row, pos1.col)

  result.setByPos(pos1.row, pos1.col, result.getByPos(pos2.row, pos2.col))

  result.setByPos(pos2.row, pos2.col, temp)

  return result
}
