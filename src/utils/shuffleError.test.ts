import { describe, expect, it } from 'vitest';
import { getShuffleErrorMessage } from '@/utils/shuffleError.ts';

describe('getShuffleErrorMessage', () => {
  it('EmptyGrid 回傳對應訊息', () => {
    expect(getShuffleErrorMessage('EmptyGrid')).toContain('尚未載入座位排佈');
  });

  it('Unsatisfiable 回傳對應訊息', () => {
    expect(getShuffleErrorMessage('Unsatisfiable')).toContain('約束互相衝突');
  });

  it('MaxAttemptsReached 回傳對應訊息', () => {
    expect(getShuffleErrorMessage('MaxAttemptsReached')).toContain('放寬約束');
  });

  it('未知錯誤回傳 fallback 原文', () => {
    expect(getShuffleErrorMessage('conflict')).toBe(
      'Unable to shuffle the grid due to reason: conflict',
    );
  });
});
