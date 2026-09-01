import { describe, expect, it, vi } from 'vitest';
import { useColorConfig } from './useColorConfig';

describe('useColorConfig', () => {
  it('loadColors 解析 JSON 並建立正則規則', () => {
    const { loadColors, colorPresets, getCellColor } = useColorConfig();
    expect(loadColors('{"^王.*": "#ff0000"}')).toBe(true);
    expect(colorPresets.value).toHaveLength(1);
    expect(getCellColor('王小明')).toBe('#ff0000');
  });

  it('空字串回傳預設顏色', () => {
    const { getCellColor } = useColorConfig();
    expect(getCellColor('')).toBe('#94a3b8');
  });

  it('JSON 格式錯誤時回傳 false 並跳出提示', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const { loadColors } = useColorConfig();
    expect(loadColors('{bad json')).toBe(false);
    expect(alertSpy).toHaveBeenCalled();
  });

  it('無匹配規則時回傳空字串', () => {
    const { loadColors, getCellColor } = useColorConfig();
    loadColors('{"^王.*": "#ff0000"}');
    expect(getCellColor('李小明')).toBe('');
  });

  it('clearColors 清空所有規則', () => {
    const { loadColors, colorPresets, clearColors } = useColorConfig();
    loadColors('{"^王.*": "#ff0000"}');
    clearColors();
    expect(colorPresets.value).toHaveLength(0);
  });
});
