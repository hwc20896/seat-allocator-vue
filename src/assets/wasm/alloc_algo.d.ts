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
  customForbiddenPairs: [string, string][];
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

export interface GridShuffler {
  getSize(): number
  setGrid(grid: string[][]): boolean
  getOriginalGrid(): string[][]
  getGrid(): string[][]
  getGridAt(index: number): string[][]
  getGridByIndex(index: number): string[][]
  shuffle(): void
  delete(): void  //  ~GridShuffler();
  validateResult(): boolean
  clearShuffledGrids(): void
  getAllGrids(): string[][][]
}

export interface GridShufflerConstructor {
  new (): GridShuffler
  new (config: ShuffleConfig): GridShuffler
}

export interface ModuleExports {
  ShuffleConfig: ShuffleConfigConstructor
  GridShuffler: GridShufflerConstructor
  shuffleGrid(grid: string[][]): string[][]
}

export type ModuleExportsPointer = ModuleExports | null
export type GridShufflerPointer = GridShuffler | null

export default function Module(): Promise<ModuleExports>
