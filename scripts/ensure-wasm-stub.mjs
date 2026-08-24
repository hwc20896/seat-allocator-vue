// Ensures the Emscripten-generated module stub exists so Vitest can resolve
// `@/assets/wasm/alloc_algo(.js)` without a prior C++/WASM build (BUILD.md).
// The real file is produced by CMake POST_BUILD and is gitignored; tests mock
// the module, they only need the import to resolve.
import { existsSync, writeFileSync } from 'node:fs'
import { fileURLToPath } from 'node:url'

const target = fileURLToPath(new URL('../src/assets/wasm/alloc_algo.js', import.meta.url))

if (!existsSync(target)) {
  writeFileSync(
    target,
    [
      'export class Grid {}',
      'export default function allocAlgoModule() {',
      "  throw new Error('WebAssembly module is not built; run the C++ Emscripten build first (see BUILD.md)')",
      '}',
      '',
    ].join('\n'),
    'utf-8',
  )
  console.log('[ensure-wasm-stub] created stub: src/assets/wasm/alloc_algo.js')
}
