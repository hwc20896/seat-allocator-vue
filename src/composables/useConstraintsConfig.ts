import { ref } from 'vue'
import type { ModuleExports, PointerOf } from '@/assets/wasm/alloc_algo'

export function useConstraintsConfig() {
  const hasCustomConfig = ref(false)
  const currentConfigJson = ref<string>('{}')
  const parsedConfig = ref<any>(null)

  const validateBasicStructure = (obj: any): boolean => {
    if (typeof obj !== 'object' || obj === null) return false
    if (obj.customForbiddenPairs && !Array.isArray(obj.customForbiddenPairs)) return false
    return !(obj.constraints && !Array.isArray(obj.constraints));

  }

  const loadConstraints = (configString: string): boolean => {
    try {
      const obj = JSON.parse(configString)
      if (!validateBasicStructure(obj)) {
        alert('JSON 算法約束內容不符合預期結構。')
        return false
      }

      currentConfigJson.value = configString
      parsedConfig.value = obj
      hasCustomConfig.value = true
      console.debug('Constraints loaded:', obj)
      return true
    } catch (e) {
      alert('JSON 算法約束檔案格式錯誤。')
      return false
    }
  }

  // Build a WASM ShuffleConfig instance from the parsed JSON.
  // Returns null when wasmModule is not available.
  const buildWasmConfig = (wasmModule: PointerOf<ModuleExports>) => {
    if (!wasmModule) return null

    const cfg = new wasmModule.ShuffleConfig()
    const o = parsedConfig.value
    if (!o) return cfg

    try {
      if (typeof o.allowFixedPoints === 'boolean') cfg.setAllowFixedPoints(o.allowFixedPoints)
      if (typeof o.allowOriginalNeighbors === 'boolean') cfg.setAllowOriginalNeighbors(o.allowOriginalNeighbors)
      if (typeof o.diagonalsAreNeighbors === 'boolean') cfg.setDiagonalsAreNeighbors(o.diagonalsAreNeighbors)

      if (Array.isArray(o.customForbiddenPairs)) {
        for (const p of o.customForbiddenPairs) {
          if (Array.isArray(p) && p.length >= 2) {
            try {
              cfg.addForbiddenPair(String(p[0]), String(p[1]))
            } catch (e) {
              console.warn('addForbiddenPair failed', e)
            }
          }
        }
      }

      console.log(o.constraints)
      if (Array.isArray(o.constraints)) {
        for (const c of o.constraints) {
          if (!c || typeof c.type !== 'string') continue
          try {
            switch (c.type.toUpperCase()) {
              case 'FORCEROW':
                cfg.forceRow(String(c.name), Number(c.rowIdx))
                console.debug(`Found forceRow: name: ${c.name}, rowIdx: ${c.rowIdx}`)
                break
              case 'FORBIDROW':
                cfg.forbidRow(String(c.name), Number(c.rowIdx))
                console.debug(`Found forbidRow: name: ${c.name}, rowIdx: ${c.rowIdx}`)
                break
              case 'FORCECOL':
                cfg.forceCol(String(c.name), Number(c.colIdx))
                console.debug(`Found forceCol: name: ${c.name}, colIdx: ${c.colIdx}`)
                break
              case 'FORBIDCOL':
                cfg.forbidCol(String(c.name), Number(c.colIdx))
                console.debug(`Found forbidCol: name: ${c.name}, colIdx: ${c.colIdx}`)
                break
              case 'FORBIDSHAREROW':
                cfg.forbidShareRow(String(c.name1), String(c.name2))
                console.debug(`Found forbidShareRow: name1: ${c.name1}, name2: ${c.name2}`)
                break
              case 'FORBIDSHARECOL':
                cfg.forbidShareCol(String(c.name1), String(c.name2))
                console.debug(`Found forbidShareCol: name1: ${c.name1}, name2: ${c.name2}`)
                break
              default:
                console.warn('Unknown constraint type', c.type)
                break
            }
          } catch (e) {
            console.warn('Applying constraint failed', e)
          }
        }
      }
    } catch (e) {
      console.warn('Failed to build wasm config from JSON', e)
    }

    return cfg
  }

  const resetConstraints = () => {
    currentConfigJson.value = '{}'
    parsedConfig.value = null
    hasCustomConfig.value = false
  }

  return {
    hasCustomConfig,
    currentConfigJson,
    parsedConfig,
    loadConstraints,
    resetConstraints,
    buildWasmConfig,
  }
}
