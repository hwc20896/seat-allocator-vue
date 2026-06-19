/// <reference types="vite/client" />
declare module '@/assets/wasm/alloc_algo.js' {
  const Module: (options?: any) => Promise<any>
  export default Module
}
