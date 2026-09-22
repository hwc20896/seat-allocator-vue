import { afterEach, describe, expect, it, vi } from 'vitest';
import type { MainModule } from '@/assets/wasm/alloc_algo';
import { buildWasmConfigFromJson } from '@/utils/wasmConfig.ts';
import { FakeStringVector } from '@/utils/__tests__/fakeGrid';

const makeModule = (ConfigClass: new () => object): MainModule =>
  ({ ShuffleConfig: ConfigClass, StringVector: FakeStringVector }) as unknown as MainModule;

afterEach(() => {
  vi.restoreAllMocks();
});

describe('buildWasmConfigFromJson', () => {
  it('wasmModule 為 null 時回傳 null', () => {
    expect(buildWasmConfigFromJson(null, '{}')).toBeNull();
  });

  it('JSON 格式錯誤時 warn 並回傳空 cfg', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {}
    const cfg = buildWasmConfigFromJson(makeModule(FakeShuffleConfig), '{bad json');
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect(warnSpy).toHaveBeenCalledWith('Failed to parse constraints JSON', expect.any(Error));
  });

  it('JSON 為 null 時回傳空 cfg', () => {
    class FakeShuffleConfig {}
    const cfg = buildWasmConfigFromJson(makeModule(FakeShuffleConfig), 'null');
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
  });

  it('JSON 為非物件純值時回傳空 cfg', () => {
    class FakeShuffleConfig {}
    const cfg = buildWasmConfigFromJson(makeModule(FakeShuffleConfig), '42');
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
  });

  it('套用所有布林欄位', () => {
    const spies = {
      setAllowFixedPoints: vi.fn(),
      setAllowOriginalNeighbors: vi.fn(),
      setDiagonalsAreNeighbors: vi.fn(),
      setCrossAisleAreNeighbors: vi.fn(),
      setEnableBuddyMatching: vi.fn(),
      setDoBuddyRotate: vi.fn(),
    };
    class FakeShuffleConfig {
      setAllowFixedPoints = spies.setAllowFixedPoints;
      setAllowOriginalNeighbors = spies.setAllowOriginalNeighbors;
      setDiagonalsAreNeighbors = spies.setDiagonalsAreNeighbors;
      setCrossAisleAreNeighbors = spies.setCrossAisleAreNeighbors;
      setEnableBuddyMatching = spies.setEnableBuddyMatching;
      setDoBuddyRotate = spies.setDoBuddyRotate;
    }

    const cfg = buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({
        allowFixedPoints: false,
        allowOriginalNeighbors: true,
        diagonalsAreNeighbors: true,
        crossAisleAreNeighbors: false,
        enableBuddyMatching: true,
        doBuddyRotate: false,
      }),
    );

    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
    expect(spies.setAllowFixedPoints).toHaveBeenCalledWith(false);
    expect(spies.setAllowOriginalNeighbors).toHaveBeenCalledWith(true);
    expect(spies.setDiagonalsAreNeighbors).toHaveBeenCalledWith(true);
    expect(spies.setCrossAisleAreNeighbors).toHaveBeenCalledWith(false);
    expect(spies.setEnableBuddyMatching).toHaveBeenCalledWith(true);
    expect(spies.setDoBuddyRotate).toHaveBeenCalledWith(false);
  });

  it('非布林欄位值被忽略（不呼叫 setter）', () => {
    const setAllowFixedPoints = vi.fn();
    class FakeShuffleConfig {
      setAllowFixedPoints = setAllowFixedPoints;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ allowFixedPoints: 'yes' }),
    );
    expect(setAllowFixedPoints).not.toHaveBeenCalled();
  });

  it('套用合法的 prioritizeBuddyPairPosition', () => {
    const setPrioritize = vi.fn();
    class FakeShuffleConfig {
      setPrioritizeBuddyPairPosition = setPrioritize;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ prioritizeBuddyPairPosition: 'FrontAndBack' }),
    );
    expect(setPrioritize).toHaveBeenCalledWith('FrontAndBack');
  });

  it('非法 prioritizeBuddyPairPosition 被忽略', () => {
    const setPrioritize = vi.fn();
    class FakeShuffleConfig {
      setPrioritizeBuddyPairPosition = setPrioritize;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ prioritizeBuddyPairPosition: 'DiagonalOnly' }),
    );
    expect(setPrioritize).not.toHaveBeenCalled();
  });

  it('套用 customForbiddenPairs', () => {
    const addForbiddenPair = vi.fn();
    class FakeShuffleConfig {
      addForbiddenPair = addForbiddenPair;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({
        customForbiddenPairs: [
          ['A', 'B'],
          ['C', 'D'],
        ],
      }),
    );
    expect(addForbiddenPair).toHaveBeenCalledTimes(2);
    expect(addForbiddenPair).toHaveBeenNthCalledWith(1, 'A', 'B');
    expect(addForbiddenPair).toHaveBeenNthCalledWith(2, 'C', 'D');
  });

  it('customForbiddenPairs 非陣列條目被跳過', () => {
    const addForbiddenPair = vi.fn();
    class FakeShuffleConfig {
      addForbiddenPair = addForbiddenPair;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ customForbiddenPairs: [['A', 'B'], 'oops', [], ['C']] }),
    );
    expect(addForbiddenPair).toHaveBeenCalledTimes(1);
  });

  it('customForbiddenPairs 非陣列時忽略', () => {
    const addForbiddenPair = vi.fn();
    class FakeShuffleConfig {
      addForbiddenPair = addForbiddenPair;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ customForbiddenPairs: 'oops' }),
    );
    expect(addForbiddenPair).not.toHaveBeenCalled();
  });

  it('addForbiddenPair 拋錯時 warn 並繼續處理後續條目', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const addForbiddenPair = vi.fn(() => {
      throw new Error('pair failed');
    });
    class FakeShuffleConfig {
      addForbiddenPair = addForbiddenPair;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({
        customForbiddenPairs: [
          ['A', 'B'],
          ['C', 'D'],
        ],
      }),
    );
    expect(warnSpy).toHaveBeenCalledWith('addForbiddenPair failed', expect.any(Error));
    expect(addForbiddenPair).toHaveBeenCalledTimes(2);
  });

  it('套用全部 constraint 類型（含大小寫不敏感）', () => {
    const spies = {
      forceRow: vi.fn(),
      forbidRow: vi.fn(),
      forceCol: vi.fn(),
      forbidCol: vi.fn(),
      forbidShareRow: vi.fn(),
      forbidShareCol: vi.fn(),
    };
    class FakeShuffleConfig {
      forceRow = spies.forceRow;
      forbidRow = spies.forbidRow;
      forceCol = spies.forceCol;
      forbidCol = spies.forbidCol;
      forbidShareRow = spies.forbidShareRow;
      forbidShareCol = spies.forbidShareCol;
    }

    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({
        constraints: [
          { type: 'forcERow', name: '王小明', rowIdx: 2 },
          { type: 'FORBIDROW', name: '陳小美', rowIdx: 0 },
          { type: 'FORCECOL', name: 'A', colIdx: 1 },
          { type: 'FORBIDCOL', name: 'B', colIdx: 2 },
          { type: 'FORBIDSHAREROW', name1: 'A', name2: 'B' },
          { type: 'FORBIDSHARECOL', name1: 'C', name2: 'D' },
        ],
      }),
    );

    expect(spies.forceRow).toHaveBeenCalledWith('王小明', 2);
    expect(spies.forbidRow).toHaveBeenCalledWith('陳小美', 0);
    expect(spies.forceCol).toHaveBeenCalledWith('A', 1);
    expect(spies.forbidCol).toHaveBeenCalledWith('B', 2);
    expect(spies.forbidShareRow).toHaveBeenCalledWith('A', 'B');
    expect(spies.forbidShareCol).toHaveBeenCalledWith('C', 'D');
  });

  it('未知 constraint 類型 warn 並忽略', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {}
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ constraints: [{ type: 'UNKNOWN_TYPE' }] }),
    );
    expect(warnSpy).toHaveBeenCalledWith('Unknown constraint type', 'UNKNOWN_TYPE');
  });

  it('constraints 含 null 條目時跳過', () => {
    const forceRow = vi.fn();
    class FakeShuffleConfig {
      forceRow = forceRow;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ constraints: [null, { type: 'FORCEROW', name: 'A', rowIdx: 0 }] }),
    );
    expect(forceRow).toHaveBeenCalledTimes(1);
  });

  it('單條 constraint 拋錯時 warn 並繼續', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const forceRow = vi.fn(() => {
      throw new Error('boom');
    });
    const forbidRow = vi.fn();
    class FakeShuffleConfig {
      forceRow = forceRow;
      forbidRow = forbidRow;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({
        constraints: [
          { type: 'FORCEROW', name: 'A', rowIdx: 0 },
          { type: 'FORBIDROW', name: 'B', rowIdx: 1 },
        ],
      }),
    );
    expect(warnSpy).toHaveBeenCalledWith('Applying constraint failed', expect.any(Error));
    expect(forbidRow).toHaveBeenCalledWith('B', 1);
  });

  it('套用 buddyGroups 並釋放 StringVector', () => {
    const setBuddyGroups = vi.fn();
    const deleteSpy = vi.fn();
    class SpyVector extends FakeStringVector {
      delete = deleteSpy;
    }
    class FakeShuffleConfig {
      setBuddyGroups = setBuddyGroups;
    }
    const module = {
      ShuffleConfig: FakeShuffleConfig,
      StringVector: SpyVector,
    } as unknown as MainModule;

    buildWasmConfigFromJson(module, JSON.stringify({ buddyGroups: [['A', 'B'], ['C']] }));

    expect(setBuddyGroups).toHaveBeenCalledTimes(1);
    const [vecA, vecB] = setBuddyGroups.mock.calls[0]!;
    expect([...vecA]).toEqual(['A', 'B']);
    expect([...vecB]).toEqual(['C']);
    expect(deleteSpy).toHaveBeenCalledTimes(2);
  });

  it('buddyGroups 非字串項被跳過', () => {
    const setBuddyGroups = vi.fn();
    // 以 SpyVector 取代真實 delete，避免 finally 釋放後無法檢查內容
    class SpyVector extends FakeStringVector {
      delete = vi.fn();
    }
    class FakeShuffleConfig {
      setBuddyGroups = setBuddyGroups;
    }
    const module = {
      ShuffleConfig: FakeShuffleConfig,
      StringVector: SpyVector,
    } as unknown as MainModule;

    buildWasmConfigFromJson(module, JSON.stringify({ buddyGroups: [['A', 42, 'B'], ['C']] }));
    const [vecA] = setBuddyGroups.mock.calls[0]!;
    expect([...vecA]).toEqual(['A', 'B']);
  });

  it('buddyGroups 空陣列時不呼叫 setBuddyGroups', () => {
    const setBuddyGroups = vi.fn();
    class FakeShuffleConfig {
      setBuddyGroups = setBuddyGroups;
    }
    buildWasmConfigFromJson(makeModule(FakeShuffleConfig), JSON.stringify({ buddyGroups: [] }));
    expect(setBuddyGroups).not.toHaveBeenCalled();
  });

  it('buddyGroups 長度非 2 時 warn 並忽略', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const setBuddyGroups = vi.fn();
    class FakeShuffleConfig {
      setBuddyGroups = setBuddyGroups;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ buddyGroups: [['A']] }),
    );
    expect(warnSpy).toHaveBeenCalledWith(
      'buddyGroups 需為 [groupA, groupB] 或 []',
      expect.anything(),
    );
    expect(setBuddyGroups).not.toHaveBeenCalled();
  });

  it('buddyGroups 子組非陣列時 warn 並忽略', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const setBuddyGroups = vi.fn();
    class FakeShuffleConfig {
      setBuddyGroups = setBuddyGroups;
    }
    buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ buddyGroups: [['A'], 'oops'] }),
    );
    expect(warnSpy).toHaveBeenCalledWith('buddyGroups 兩組均需為字串陣列', expect.anything());
    expect(setBuddyGroups).not.toHaveBeenCalled();
  });

  it('setBuddyGroups 拋錯時 warn 且仍釋放 StringVector', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    const deleteSpy = vi.fn();
    class SpyVector extends FakeStringVector {
      delete = deleteSpy;
    }
    class FakeShuffleConfig {
      setBuddyGroups = vi.fn(() => {
        throw new Error('boom');
      });
    }
    const module = {
      ShuffleConfig: FakeShuffleConfig,
      StringVector: SpyVector,
    } as unknown as MainModule;

    buildWasmConfigFromJson(module, JSON.stringify({ buddyGroups: [['A'], ['B']] }));

    expect(warnSpy).toHaveBeenCalledWith('Applying buddyGroups failed', expect.any(Error));
    expect(deleteSpy).toHaveBeenCalledTimes(2);
  });

  it('setter 拋錯時 warn 並回傳已构建的 cfg', () => {
    const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
    class FakeShuffleConfig {
      setAllowFixedPoints = vi.fn(() => {
        throw new Error('config failed');
      });
    }
    const cfg = buildWasmConfigFromJson(
      makeModule(FakeShuffleConfig),
      JSON.stringify({ allowFixedPoints: true }),
    );
    expect(warnSpy).toHaveBeenCalledWith(
      'Failed to build WASM config from JSON',
      expect.any(Error),
    );
    expect(cfg).toBeInstanceOf(FakeShuffleConfig);
  });
});
