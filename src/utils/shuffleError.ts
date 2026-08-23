import type { ShuffleResponse } from '@/assets/wasm/alloc_algo'

export type ShuffleErrorCode = Extract<ShuffleResponse, { success: false }>['error']

export const SHUFFLE_ERROR_MESSAGES: Record<ShuffleErrorCode, string> = {
  EmptyGrid: '尚未載入座位排佈，無法開始分配。',
  Unsatisfiable:
    '無法在滿足所有約束的條件下完成分配：可移動的座位太少，或約束互相衝突。請調整約束設定後再試。',
  MaxAttemptsReached: '已達最大嘗試次數，找不到同時滿足所有約束的分配，請放寬約束後再試。',
}

export const getShuffleErrorMessage = (error: string): string =>
  SHUFFLE_ERROR_MESSAGES[error as ShuffleErrorCode] ??
  `Unable to shuffle the grid due to reason: ${error}`
