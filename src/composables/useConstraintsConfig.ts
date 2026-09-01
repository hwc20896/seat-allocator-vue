import { ref } from 'vue';
import type { MainModule } from '@/assets/wasm/alloc_algo';
import type { ImportedConstraint } from '@/utils/JSONTypes.ts';
import { isBoolean } from 'lodash-es';

/** 預設約束：與 C++ ShuffleConfig 默認值一致（允許原位、允許原本鄰座） */
const DEFAULT_CONFIG_JSON = JSON.stringify({
  allowFixedPoints: true,
  allowOriginalNeighbors: true,
  diagonalsAreNeighbors: false,
  customForbiddenPairs: [],
  constraints: [],
});

export function useConstraintsConfig() {
  const hasCustomConfig = ref(false);
  const currentConfigJson = ref<string>(DEFAULT_CONFIG_JSON);
  const parsedConfig = ref<ImportedConstraint | null>(null);

  const validateBasicStructure = (obj: ImportedConstraint | null): boolean => {
    if (typeof obj !== 'object' || obj === null) return false;
    if (obj.customForbiddenPairs && !Array.isArray(obj.customForbiddenPairs)) return false;
    return !obj.constraints || Array.isArray(obj.constraints);
  };

  const loadConstraints = (configString: string): boolean => {
    try {
      const obj = JSON.parse(configString);
      if (!validateBasicStructure(obj)) {
        alert('JSON 算法約束內容不符合預期結構。');
        return false;
      }

      currentConfigJson.value = configString;
      parsedConfig.value = obj;
      hasCustomConfig.value = true;
      console.debug('Constraints loaded:', obj);
      return true;
    } catch {
      alert('JSON 算法約束檔案格式錯誤。');
      return false;
    }
  };

  // Build a WASM ShuffleConfig instance from a constraints JSON string.
  // Returns null when wasmModule is not available.
  const buildWasmConfigFromJson = (wasmModule: MainModule | null, json: string) => {
    if (!wasmModule) return null;

    const cfg = new wasmModule.ShuffleConfig();

    let o: ImportedConstraint | null;
    try {
      o = JSON.parse(json) as ImportedConstraint;
    } catch (e) {
      console.warn('Failed to parse constraints JSON', e);
      return cfg;
    }
    if (!o) return cfg;

    try {
      if (isBoolean(o.allowFixedPoints)) cfg.setAllowFixedPoints(o.allowFixedPoints);
      if (isBoolean(o.allowOriginalNeighbors))
        cfg.setAllowOriginalNeighbors(o.allowOriginalNeighbors);
      if (isBoolean(o.diagonalsAreNeighbors)) cfg.setDiagonalsAreNeighbors(o.diagonalsAreNeighbors);

      if (Array.isArray(o.customForbiddenPairs)) {
        for (const p of o.customForbiddenPairs) {
          if (Array.isArray(p) && p.length >= 2) {
            try {
              cfg.addForbiddenPair(String(p[0]), String(p[1]));
            } catch (e) {
              console.warn('addForbiddenPair failed', e);
            }
          }
        }
      }

      if (Array.isArray(o.constraints)) {
        for (const c of o.constraints) {
          if (!c) continue;
          try {
            switch (c.type.toUpperCase()) {
              case 'FORCEROW':
                cfg.forceRow(String(c.name), Number(c.rowIdx));
                console.debug(`Found forceRow: name: ${c.name}, rowIdx: ${c.rowIdx}`);
                break;
              case 'FORBIDROW':
                cfg.forbidRow(String(c.name), Number(c.rowIdx));
                console.debug(`Found forbidRow: name: ${c.name}, rowIdx: ${c.rowIdx}`);
                break;
              case 'FORCECOL':
                cfg.forceCol(String(c.name), Number(c.colIdx));
                console.debug(`Found forceCol: name: ${c.name}, colIdx: ${c.colIdx}`);
                break;
              case 'FORBIDCOL':
                cfg.forbidCol(String(c.name), Number(c.colIdx));
                console.debug(`Found forbidCol: name: ${c.name}, colIdx: ${c.colIdx}`);
                break;
              case 'FORBIDSHAREROW':
                cfg.forbidShareRow(String(c.name1), String(c.name2));
                console.debug(`Found forbidShareRow: name1: ${c.name1}, name2: ${c.name2}`);
                break;
              case 'FORBIDSHARECOL':
                cfg.forbidShareCol(String(c.name1), String(c.name2));
                console.debug(`Found forbidShareCol: name1: ${c.name1}, name2: ${c.name2}`);
                break;
              default:
                console.warn('Unknown constraint type', c.type);
                break;
            }
          } catch (e) {
            console.warn('Applying constraint failed', e);
          }
        }
      }
    } catch (e) {
      console.warn('Failed to build WASM config from JSON', e);
    }

    return cfg;
  };

  // Build a WASM ShuffleConfig instance from the currently applied JSON.
  const buildWasmConfig = (wasmModule: MainModule | null) =>
    buildWasmConfigFromJson(wasmModule, currentConfigJson.value);

  const resetConstraints = () => {
    currentConfigJson.value = DEFAULT_CONFIG_JSON;
    parsedConfig.value = null;
    hasCustomConfig.value = false;
  };

  return {
    hasCustomConfig,
    currentConfigJson,
    parsedConfig,
    loadConstraints,
    resetConstraints,
    buildWasmConfigFromJson,
    buildWasmConfig,
  };
}
