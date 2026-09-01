import type { ShuffleError } from '@/assets/wasm/alloc_algo';

export const SHUFFLE_ERROR_MESSAGES: Record<ShuffleError, string> = {
  EmptyGrid: '尚未載入座位排佈，無法開始分配。',
  Unsatisfiable:
    '無法在滿足所有約束的條件下完成分配：可移動的座位太少，或約束互相衝突。請調整約束設定後再試。',
  MaxAttemptsReached: '已達最大嘗試次數，找不到同時滿足所有約束的分配，請放寬約束後再試。',
  Unknown: '未知錯誤。也許你應該高興才對，因為目前還沒有一個可以走到要回傳Unknown的狀況。',
};

export const getShuffleErrorMessage = (error: string): string =>
  SHUFFLE_ERROR_MESSAGES[error as ShuffleError] ??
  `Unable to shuffle the grid due to reason: ${error}`;
