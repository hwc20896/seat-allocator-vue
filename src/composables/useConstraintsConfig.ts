import { ref } from 'vue'

export function useConstraintsConfig() {
  const hasCustomConfig = ref(false)
  const currentConfigJson = ref<string>('{}')

  const loadConstraints = (configString: string): boolean => {
    try {
      JSON.parse(configString)
      currentConfigJson.value = configString
      hasCustomConfig.value = true
      return true
    } catch (e) {
      alert('JSON 算法約束檔案格式錯誤。')
      return false
    }
  }

  const resetConstraints = () => {
    currentConfigJson.value = '{}'
    hasCustomConfig.value = false
  }

  return {
    hasCustomConfig,
    currentConfigJson,
    loadConstraints,
    resetConstraints,
  }
}
