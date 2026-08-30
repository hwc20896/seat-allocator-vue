//  using types: Algorithm.Utils

export type PointerOf<T> = T | null

export type ForbiddenPairType = [string, string]

//  constraints: Algorithm.Constraints
export interface ForceRow {
  name: string
  rowIdx: number
}

export interface ForceCol {
  name: string
  colIdx: number
}

export interface ForbidRow {
  name: string
  rowIdx: number
}

export interface ForbidCol {
  name: string
  colIdx: number
}

export interface ForbidShareRow {
  name1: string
  name2: string
}

export interface ForbidShareCol {
  name1: string
  name2: string
}

export type Constraint =
  | ForceRow
  | ForceCol
  | ForbidRow
  | ForbidCol
  | ForbidShareRow
  | ForbidShareCol

export class Grid {
  constructor()

  constructor(row: number, col: number, data: string[])

  getByPos(row: number, col: number): string
  getByIndex(idx: number): string
  setByPos(row: number, col: number, value: string): void
  setByIndex(idx: number, value: string): void
  rowCount(): number
  colCount(): number
  size(): number
  empty(): boolean
  rawData(): string[]
  clone(): Grid

  static fromCSV(csvString: string): Grid

  toCSVString(): string
}

//  Shuffle Config: Algorithm.Configs
export interface ShuffleConfig {
  allowFixedPoints: boolean
  allowOriginalNeighbors: boolean
  diagonalsAreNeighbors: boolean
  customForbiddenPairs: ForbiddenPairType[]
  constraints: Constraint[]

  //  constexpr-able
  setAllowFixedPoints(allow: boolean): void
  setAllowOriginalNeighbors(allow: boolean): void
  setDiagonalsAreNeighbors(allow: boolean): void
  addForbiddenPair(name1: string, name2: string): void

  forceRow(name: string, rowIdx: number): void
  forbidRow(name: string, rowIdx: number): void
  forceCol(name: string, colIdx: number): void
  forbidCol(name: string, colIdx: number): void
  forbidShareRow(name1: string, name2: string): void
  forbidShareCol(name1: string, name2: string): void
}

export interface ShuffleConfigConstructor {
  new (): ShuffleConfig
}

//  Algorithm Config: Algorithm.Configs
export interface AnnealingConfig {
  initialTemperature: number
  coolingRate: number
  maxSteps: number
  maxAttempts: number
}

export interface PenaltyWeights {
  fixedPoint: number
  absolutePosition: number
  originalNeighbor: number
  customForbidden: number
  forbidShare: number
}

//  Main Algorithm: Algorithm.Shuffler
export interface ResultType {
  doneAtAttempt: number
  doneAtStep: number
  tookMUS: number
}

export type ShuffleResponse =
  | { success: true; data: ResultType }
  | { success: false; error: 'EmptyGrid' | 'MaxAttemptsReached' | 'Unsatisfiable' }

export interface GridShuffler {
  delete(): void
  getShuffledGridCount(): number
  setGrid(grid: Grid): bool
  setConfig(cfg: ShuffleConfig): void
  setAnnealingConfig(cfg: AnnealingConfig): void
  setPenaltyWeights(cfg: PenaltyWeights): void
  getGrid(): Grid
  getGridAt(idx: number): Grid
  shuffle(): Promise<ShuffleResponse>
  validateResult(): boolean
  clearShuffledGrids(): void
}

export interface GridShufflerConstructor {
  new (): GridShuffler
}

export interface FeasibilityReport {
  status: number
  layer: string
  reason: string
}

export function checkFeasibility(
  grid: Grid,
  cfg: ShuffleConfig,
  checkForbidShare: boolean,
  coloringNodeBudget: number,
): FeasibilityReport

export interface ModuleExports {
  ShuffleConfig: ShuffleConfigConstructor
  GridShuffler: GridShufflerConstructor
  Grid: typeof Grid
  checkFeasibility: typeof checkFeasibility
}

export default function Module(): Promise<ModuleExports>
