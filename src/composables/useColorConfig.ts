import { ref } from 'vue'

export interface ColorPreset {
  pattern: RegExp
  color: string
}

export function useColorConfig() {
  const colorPresets = ref<ColorPreset[]>([])

  const loadColors = (configString: string): boolean => {
    try {
      const data = JSON.parse(configString)
      const tempRules: ColorPreset[] = []

      for (const [pattern, color] of Object.entries(data)) {
        tempRules.push({
          pattern: new RegExp(pattern),
          color: color as string,
        })
      }

      colorPresets.value = tempRules
      return true
    } catch (e) {
      alert('JSON 顏色配置解析失敗。')
      return false
    }
  }

  const getCellColor = (text: string): string => {
    if (!text) return '#94a3b8'
    for (const preset of colorPresets.value) {
      if (preset.pattern.test(text)) {
        return preset.color
      }
    }
    return ''
  }

  const clearColors = () => {
    colorPresets.value = []
  }

  return {
    colorPresets,
    loadColors,
    getCellColor,
    clearColors,
  }
}
