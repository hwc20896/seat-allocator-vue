export interface Constraint {
  type: string
  name?: string
  name1?: string
  name2?: string
  rowIdx?: number
  colIdx?: number
}

export interface ImportedConstraint {
  allowFixedPoints: boolean
  allowOriginalNeighbors: boolean
  diagonalsAreNeighbors: boolean
  customForbiddenPairs: [string, string][]
  constraints: Constraint[]
}
