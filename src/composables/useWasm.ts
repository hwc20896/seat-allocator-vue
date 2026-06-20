import { ref, shallowRef, onBeforeUnmount } from 'vue'
import initWasmModule from '@/assets/wasm/alloc_algo.js'
import type { GridShuffler, ModuleExports } from '@/assets/wasm/alloc_algo'

export function useWasm() {
  const wasmModule = shallowRef<ModuleExports | null>(null) //  const auto wasmModule = shallowRef<ModuleExports*>(nullptr);

  const shufflerInstance = shallowRef<GridShuffler | null>(null)
  const wasmReady = ref(false)

  const initWasm = async () => {
    try {
      wasmModule.value = await initWasmModule({
        locateFile: (path: string) => {
          if (path.endsWith('.wasm')) return '/alloc_algo.wasm'
          return path
        },
      })
      wasmReady.value = true
      return true
    } catch (error) {
      console.error('WebAssembly 模組載入失敗:', error)
      return false
    }
  }

  const cleanup = () => {
    if (shufflerInstance.value) {
      shufflerInstance.value.delete()
      shufflerInstance.value = null
    }
  }

  onBeforeUnmount(() => {
    cleanup()
  })

  return {
    wasmModule,
    shufflerInstance,
    wasmReady,
    initWasm,
    cleanup,
  }
}
