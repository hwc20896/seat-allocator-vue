//  ==========================================
//  Worker protocol: main thread <-> shuffle.worker
//  Embind handles (Grid/ShuffleConfig) cannot be shared across threads (structured clones are not supported),
//  only raw data is passed: Grid is transmitted as a CSV string, and constraints are transmitted as a JSON string.
//
//  shuffleResult 失敗時，error 欄位混用兩種來源：
//    - ShuffleError（C++ 算法錯誤，embind string enum）
//    - ShuffleWorkerError（worker 端 TS 合成；未真正進入 C++ 或 wasm 呼叫拋例外）
//  ==========================================
import type { ShuffleError, ShuffleReport } from '@/assets/wasm/alloc_algo';

//  As part of the agreed-upon payload, when re-exporting externally, do not need to interact with WASM type files.
export type { ShuffleError, ShuffleReport };

/** Worker 端（非 C++ 算法）判定的錯誤碼。 */
export type ShuffleWorkerError =
  | 'NotReady' // worker 尚未就緒（wasm 模組未載入或尚未 setGrid）
  | 'InvalidConfig' // config JSON 無法構建成 wasm ShuffleConfig
  | 'InternalError'; // wasm 呼叫拋出例外（abort 已另以 fatal 回報）

/** shuffleResult 失敗時 error 欄位的完整取值（C++ 算法錯誤 + worker 端錯誤）。 */
export type ShuffleFailure = ShuffleError | ShuffleWorkerError;

export type ShuffleRequest =
  | { type: 'setGrid'; requestId: number; gridCsv: string }
  | { type: 'shuffle'; requestId: number; configJson: string };

export type ShuffleResponse =
  | { type: 'ready' }
  | { type: 'fatal'; message: string }
  | { type: 'setGridResult'; requestId: number; ok: boolean; reason?: string }
  | {
      type: 'shuffleResult';
      requestId: number;
      success: true;
      gridCsv: string;
      report: ShuffleReport;
    }
  | { type: 'shuffleResult'; requestId: number; success: false; error: ShuffleFailure };

export interface SetGridResult {
  ok: boolean;
  reason?: string;
}

export type ShuffleResult =
  | { success: true; gridCsv: string; report: ShuffleReport }
  | { success: false; error: ShuffleFailure };
