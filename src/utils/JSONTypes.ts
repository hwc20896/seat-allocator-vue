export interface Constraint {
  type: string;
  name?: string;
  name1?: string;
  name2?: string;
  rowIdx?: number;
  colIdx?: number;
}

/** 搭檔配對的方位偏好（對應 C++ PrioritizeBuddyPairPosition；soft，僅引導搜尋） */
export type BuddyPairPosition = 'LeftAndRight' | 'FrontAndBack' | 'AllAreAcceptable';

export interface ImportedConstraint {
  allowFixedPoints: boolean;
  allowOriginalNeighbors: boolean;
  diagonalsAreNeighbors: boolean;
  customForbiddenPairs: [string, string][];
  constraints: Constraint[];
  crossAisleAreNeighbors: boolean;
  enableBuddyMatching: boolean;
  doBuddyRotate: boolean;
  buddyGroups?: string[][]; //  In algorithm, it is [string[], string[]], which is actually string[][2].
  prioritizeBuddyPairPosition?: BuddyPairPosition; //  In algorithm it is a soft preference only.
}
