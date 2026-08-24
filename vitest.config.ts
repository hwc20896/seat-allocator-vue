import { fileURLToPath } from 'node:url'
import { mergeConfig, defineConfig, configDefaults } from 'vitest/config'
import viteConfig from './vite.config'

export default mergeConfig(
  viteConfig,
  defineConfig({
    test: {
      environment: 'jsdom',
      exclude: [...configDefaults.exclude, 'e2e/**'],
      root: fileURLToPath(new URL('./', import.meta.url)),
      coverage: {
        provider: 'v8',
        include: ['src/**/*.{ts,vue}'],
        exclude: [
          'src/main.ts',
          'src/App.vue',
          'src/assets/**',
          'src/**/*.d.ts',
          'src/utils/JSONTypes.ts',
        ],
      },
    },
  }),
)
