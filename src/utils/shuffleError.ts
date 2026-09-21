import type { ShuffleError } from '@/assets/wasm/alloc_algo';
import type { ShuffleWorkerError } from '@/workers/shuffle.protocol';

export const SHUFFLE_ERROR_MESSAGES: Record<ShuffleError, string> = {
  EmptyGrid: '尚未載入座位排佈，無法開始分配。',
  Unsatisfiable:
    '無法在滿足所有約束的條件下完成分配：可移動的座位太少，或約束互相衝突。請調整約束設定後再試。',
  MaxAttemptsReached: '已達最大嘗試次數，找不到同時滿足所有約束的分配，請放寬約束後再試。',
  //  C++ 端目前沒有實際回傳 Unknown 的路徑；此為 embind 對應的防御性值
  Unknown: '發生未預期的錯誤，請查看瀏覽器 Console 以取得更多資訊。',
};

export const SHUFFLE_WORKER_ERROR_MESSAGES: Record<ShuffleWorkerError, string> = {
  NotReady: '洗牌引擎尚未就緒，請稍候再試。',
  InvalidConfig: '約束設定無法轉換為洗牌引擎的配置，請檢查約束設定後再試。',
  InternalError: '洗牌引擎內部發生未預期的錯誤，請查看瀏覽器 Console。',
};

export const getShuffleErrorMessage = (error: string): string =>
  SHUFFLE_ERROR_MESSAGES[error as ShuffleError] ??
  SHUFFLE_WORKER_ERROR_MESSAGES[error as ShuffleWorkerError] ??
  `Unable to shuffle the grid due to reason: ${error}`;
