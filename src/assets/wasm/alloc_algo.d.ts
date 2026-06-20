//  using types: <utiltypes.hpp>

export type ArrayOf<T> = T[];
export type GridOf<T> = T[][];
export type PointerOf<T> = T | null;
export type PairOf<T, U> = [T, U];

export type Grid = GridOf<string>;
export type ForbiddenPairType = PairOf<string, string>;

/*  Defined in @/utils/Position.ts
export interface Position{
  row: number;
  col: number;
}
*/

//  constraints: <constraints.hpp>
export interface ForceRow {name: string, rowIdx: number}

export interface ForceCol {name: string, colIdx: number}

export interface ForbidRow {name: string, rowIdx: number}

export interface ForbidCol {name: string, colIdx: number}

export interface ForbidShareRow{name1: string, name2: string}

export interface ForbidShareCol{name1: string, name2: string}

export type Constraint = ForceRow | ForceCol | ForbidRow | ForbidCol | ForbidShareRow | ForbidShareCol;

export interface ShuffleConfig {
  allowFixedPoints: boolean;
  allowOriginalNeighbors: boolean;
  diagonalsAreNeighbors: boolean;
  customForbiddenPairs: ForbiddenPairType[];
  constraints: Constraint[];

  //  constexpr-able
  setAllowFixedPoints(allow: boolean): void;
  setAllowOriginalNeighbors(allow: boolean): void;
  setDiagonalsAreNeighbors(allow: boolean): void;
  addForbiddenPair(name1: string, name2: string): void;

  forceRow(name: string, rowIdx: number): void;
  forbidRow(name: string, rowIdx: number): void;
  forceCol(name: string, colIdx: number): void;
  forbidCol(name: string, colIdx: number): void;
  forbidShareRow(name1: string, name2: string): void;
  forbidShareCol(name1: string, name2: string): void;
}

export interface ShuffleConfigConstructor{
  new (): ShuffleConfig;
}


//  main algorithm: <algorithm.hpp>
export interface GridShuffler {
  getSize(): number
  setGrid(grid: Grid): boolean
  getOriginalGrid(): Grid
  getGrid(): Grid
  getGridAt(index: number): Grid
  getGridByIndex(index: number): Grid
  shuffle(): void
  delete(): void //  ~GridShuffler();
  validateResult(): boolean
  clearShuffledGrids(): void
  getAllGrids(): ArrayOf<Grid>
}

export interface GridShufflerConstructor {
  new (): GridShuffler
  new (config: ShuffleConfig): GridShuffler
}

export interface ModuleExports {
  ShuffleConfig: ShuffleConfigConstructor
  GridShuffler: GridShufflerConstructor
  shuffleGrid(grid: Grid): Grid
}

export default function Module(): Promise<ModuleExports>
