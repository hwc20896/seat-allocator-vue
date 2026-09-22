import { ref } from 'vue';
import type { MainModule } from '@/assets/wasm/alloc_algo';
import type { ImportedConstraint } from '@/utils/JSONTypes.ts';
import { buildWasmConfigFromJson } from '@/utils/wasmConfig.ts';

/** 預設約束：與 C++ ShuffleConfig 默認值一致（允許原位、允許原本鄰座） */
const DEFAULT_CONFIG_JSON = JSON.stringify({
  allowFixedPoints: true,
  allowOriginalNeighbors: true,
  diagonalsAreNeighbors: false,
  crossAisleAreNeighbors: true,
  enableBuddyMatching: false,
  doBuddyRotate: true,
  customForbiddenPairs: [],
  constraints: [],
  buddyGroups: [],
  prioritizeBuddyPairPosition: 'AllAreAcceptable',
} satisfies ImportedConstraint);

export function useConstraintsConfig() {
  const hasCustomConfig = ref(false);
  const currentConfigJson = ref<string>(DEFAULT_CONFIG_JSON);
  const parsedConfig = ref<ImportedConstraint | null>(null);

  const validateBasicStructure = (obj: ImportedConstraint | null): boolean => {
    if (typeof obj !== 'object' || obj === null) return false;
    if (obj.customForbiddenPairs && !Array.isArray(obj.customForbiddenPairs)) return false;
    if (obj.buddyGroups && !Array.isArray(obj.buddyGroups)) return false;
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
