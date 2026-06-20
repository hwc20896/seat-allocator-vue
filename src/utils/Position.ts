import type {GridOf} from '@/assets/wasm/alloc_algo'

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

/*
template <class T>
T[][] swap(T[][] grid, Position pos1, Position pos2){
  std::swap(grid[pos1.row][pos1.col], grid[pos2.row][pos2.col]);
  return grid;
}
 */
export const swap = <T>(grid: GridOf<T>, pos1: Position, pos2: Position): GridOf<T> => {
  const isPositionValid = (pos: Position): boolean => {
    const row = grid[pos.row]
    return row !== undefined && pos.col >= 0 && pos.col < row.length
  }

  if (!isPositionValid(pos1)) throw new RangeError(`Position ${pos1} out of range`)

  if (!isPositionValid(pos2)) throw new RangeError(`Position ${pos2} out of range`)

  const temp = grid[pos1.row]![pos1.col]!
  grid[pos1.row]![pos1.col] = grid[pos2.row]![pos2.col]!
  grid[pos2.row]![pos2.col] = temp

  return grid
}
