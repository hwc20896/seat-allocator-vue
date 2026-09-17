import { describe, expect, it } from 'vitest';
import { extractDisplayName } from './gridUtils';

describe('extractDisplayName', () => {
  it('單行文字原樣回傳', () => {
    expect(extractDisplayName('王小明')).toBe('王小明');
  });
  it('多行文字取第一行有效內容', () => {
    expect(extractDisplayName('王小明 5\nWang Xiaoming')).toBe('王小明 5');
  });
  it('首行為空行時取下一行', () => {
    expect(extractDisplayName('\n\n王小明\nWang')).toBe('王小明');
  });
});
