import { afterEach, describe, expect, it, vi } from 'vitest';
import { useConstraintsConfig } from './useConstraintsConfig';

afterEach(() => {
  vi.restoreAllMocks();
});

describe('useConstraintsConfig', () => {
  it('loadConstraints 接受合法 JSON', () => {
    const { loadConstraints, hasCustomConfig, parsedConfig } = useConstraintsConfig();
    const ok = loadConstraints('{"allowFixedPoints": true, "constraints": []}');
    expect(ok).toBe(true);
    expect(hasCustomConfig.value).toBe(true);
    expect(parsedConfig.value?.allowFixedPoints).toBe(true);
  });

  it('buildWasmConfig 依 constraints 呼叫對應的 WASM 方法', () => {
    const setAllowFixedPointsSpy = vi.fn();
    const forceRowSpy = vi.fn();

    class FakeShuffleConfig {
      setAllowFixedPoints = setAllowFixedPointsSpy;
      forceRow = forceRowSpy;
    }

    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(
      '{"allowFixedPoints": true, "constraints": [{"type": "FORCEROW", "name": "王小明", "rowIdx": 2}]}',
    );

    const cfg = buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect(setAllowFixedPointsSpy).toHaveBeenCalledWith(true);
    expect(forceRowSpy).toHaveBeenCalledWith('王小明', 2);
  });

  const spies = {
    forbidRow: vi.fn(),
    forceCol: vi.fn(),
    forbidCol: vi.fn(),
    forbidShareRow: vi.fn(),
    forbidShareCol: vi.fn(),
  };

  class FakeShuffleConfig {
    setAllowFixedPoints = vi.fn();
    addForbiddenPair = vi.fn();
    forceRow = vi.fn();
    forbidRow = spies.forbidRow;
    forceCol = spies.forceCol;
    forbidCol = spies.forbidCol;
    forbidShareRow = spies.forbidShareRow;
    forbidShareCol = spies.forbidShareCol;
  }

  it('buildWasmConfig 處理所有 constraint 類型', () => {
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(
      JSON.stringify({
        customForbiddenPairs: [['A', 'B']],
        constraints: [
          { type: 'FORBIDROW', name: '王小明', rowIdx: 0 },
          { type: 'FORCECOL', name: '陳小美', colIdx: 1 },
          { type: 'FORBIDCOL', name: '林小華', colIdx: 2 },
          { type: 'FORBIDSHAREROW', name1: 'A', name2: 'B' },
          { type: 'FORBIDSHARECOL', name1: 'A', name2: 'B' },
          { type: 'UNKNOWN_TYPE' },
        ],
      }),
    );

    const cfg = buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(spies.forbidRow).toHaveBeenCalledWith('王小明', 0);
    expect(spies.forceCol).toHaveBeenCalledWith('陳小美', 1);
    expect(spies.forbidCol).toHaveBeenCalledWith('林小華', 2);
    expect(spies.forbidShareRow).toHaveBeenCalledWith('A', 'B');
    expect(spies.forbidShareCol).toHaveBeenCalledWith('A', 'B');
  });

  it('customForbiddenPairs 含非陣列條目時跳過', () => {
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    expect(loadConstraints('{"customForbiddenPairs": [["A","B"], "oops", []]}')).toBe(true);
    const cfg = buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect((cfg as unknown as FakeShuffleConfig).addForbiddenPair).toHaveBeenCalledTimes(1);
  });

  it('customForbiddenPairs 非陣列時回傳 false 並提示', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const { loadConstraints } = useConstraintsConfig();
    expect(loadConstraints('{"customForbiddenPairs": "oops"}')).toBe(false);
    expect(alertSpy).toHaveBeenCalled();
  });

  it('JSON 格式錯誤時回傳 false 並提示', () => {
    const alertSpy = vi.spyOn(window, 'alert').mockImplementation(() => {});
    const { loadConstraints } = useConstraintsConfig();
    expect(loadConstraints('{bad json')).toBe(false);
    expect(alertSpy).toHaveBeenCalled();
  });

  it('addForbiddenPair 失敗時 warn 並繼續', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      addForbiddenPair = vi.fn(() => {
        throw new Error('pair failed');
      });
      forceRow = vi.fn();
    }
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(JSON.stringify({ customForbiddenPairs: [['A', 'B']], constraints: [] }));
    buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(warnSpy).toHaveBeenCalledWith('addForbiddenPair failed', expect.any(Error));
  });

  it('套用 constraint 失敗時 warn 並繼續', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      forceRow = vi.fn(() => {
        throw new Error('boom');
      });
    }
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(JSON.stringify({ constraints: [{ type: 'FORCEROW', name: 'A', rowIdx: 0 }] }));
    buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(warnSpy).toHaveBeenCalledWith('Applying constraint failed', expect.any(Error));
  });

  it('buildWasmConfig 內部失敗時 warn 但仍回傳 cfg', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      setAllowFixedPoints = vi.fn(() => {
        throw new Error('config failed');
      });
      forceRow = vi.fn();
    }
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(JSON.stringify({ allowFixedPoints: true, constraints: [] }));
    const cfg = buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(warnSpy).toHaveBeenCalledWith(
      'Failed to build WASM config from JSON',
      expect.any(Error),
    );
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
  });

  it('resetConstraints 重置所有狀態', () => {
    const { loadConstraints, resetConstraints, hasCustomConfig, currentConfigJson, parsedConfig } =
      useConstraintsConfig();
    loadConstraints('{"constraints": []}');
    resetConstraints();
    expect(hasCustomConfig.value).toBe(false);
    expect(JSON.parse(currentConfigJson.value)).toEqual({
      allowFixedPoints: true,
      allowOriginalNeighbors: true,
      diagonalsAreNeighbors: false,
      customForbiddenPairs: [],
      constraints: [],
    });
    expect(parsedConfig.value).toBeNull();
  });

  it('非物件 JSON 時回傳 false', () => {
    const { loadConstraints } = useConstraintsConfig();
    expect(loadConstraints('42')).toBe(false);
  });

  it('buildWasmConfig 在 wasmModule 為 null 時回傳 null', () => {
    const { buildWasmConfig } = useConstraintsConfig();
    expect(buildWasmConfig(null)).toBeNull();
  });

  it('buildWasmConfig 在未載入約束時回傳空 cfg', () => {
    class FakeShuffleConfig {}
    const { buildWasmConfig } = useConstraintsConfig();
    const cfg = buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
  });

  it('buildWasmConfig 套用 allowOriginalNeighbors 與 diagonalsAreNeighbors', () => {
    const setAllowOriginalNeighbors = vi.fn();
    const setDiagonalsAreNeighbors = vi.fn();
    class FakeShuffleConfig {
      setAllowOriginalNeighbors = setAllowOriginalNeighbors;
      setDiagonalsAreNeighbors = setDiagonalsAreNeighbors;
    }
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(JSON.stringify({ allowOriginalNeighbors: true, diagonalsAreNeighbors: false }));
    buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(setAllowOriginalNeighbors).toHaveBeenCalledWith(true);
    expect(setDiagonalsAreNeighbors).toHaveBeenCalledWith(false);
  });

  it('constraints 含 null 條目時跳過', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      forceRow = vi.fn();
    }
    const { loadConstraints, buildWasmConfig } = useConstraintsConfig();
    loadConstraints(
      JSON.stringify({ constraints: [null, { type: 'FORCEROW', name: 'A', rowIdx: 0 }] }),
    );
    buildWasmConfig({ ShuffleConfig: FakeShuffleConfig } as never);
    expect(warnSpy).not.toHaveBeenCalled();
  });

  it('buildWasmConfigFromJson 直接以 JSON 字串建構且不依賴 loadConstraints', () => {
    const forceRowSpy = vi.fn<(name: string, rowIdx: number) => void>();
    class FakeShuffleConfig {
      forceRow = forceRowSpy;
    }
    const { buildWasmConfigFromJson, hasCustomConfig } = useConstraintsConfig();
    const cfg = buildWasmConfigFromJson(
      { ShuffleConfig: FakeShuffleConfig } as never,
      JSON.stringify({ constraints: [{ type: 'FORCEROW', name: '王小明', rowIdx: 2 }] }),
    );
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect(forceRowSpy).toHaveBeenCalledWith('王小明', 2);
    expect(hasCustomConfig.value).toBe(false);
  });

  it('buildWasmConfigFromJson 在 JSON 格式錯誤時 warn 並回傳空 cfg', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      forceRow = vi.fn<(name: string, rowIdx: number) => void>();
    }
    const { buildWasmConfigFromJson } = useConstraintsConfig();
    const cfg = buildWasmConfigFromJson({ ShuffleConfig: FakeShuffleConfig } as never, '{bad json');
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect(warnSpy).toHaveBeenCalledWith('Failed to parse constraints JSON', expect.any(Error));
  });

  it('buildWasmConfigFromJson 在 wasmModule 為 null 時回傳 null', () => {
    const { buildWasmConfigFromJson } = useConstraintsConfig();
    expect(buildWasmConfigFromJson(null, '{}')).toBeNull();
  });
});
