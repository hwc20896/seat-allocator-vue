/// <reference types="vite/client" />
declare const __APP_VERSION__: string
declare module '@/assets/wasm/alloc_algo.js' {
  //  eslint-disable-next-line @typescript-eslint/no-explicit-any
  const Module: (options?: any) => Promise<any>;
  export default Module;
}
