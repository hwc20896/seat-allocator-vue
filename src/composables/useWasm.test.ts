// src/composables/useWasm.test.ts
import { mount } from '@vue/test-utils';
import { defineComponent } from 'vue';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { useWasm } from './useWasm';

const initWasmModuleMock = vi.fn();
vi.mock('@/assets/wasm/alloc_algo.js', () => ({
  default: (...args: unknown[]) => initWasmModuleMock(...args),
}));

describe('useWasm', () => {
  beforeEach(() => {
    initWasmModuleMock.mockReset();
    const { wasmReady, wasmModule, shufflerInstance } = useWasm();
    wasmReady.value = false;
    wasmModule.value = null;
    shufflerInstance.value = null;
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  it('initWasm 成功時 wasmReady 變為 true', async () => {
    initWasmModuleMock.mockResolvedValue({});
    const { initWasm, wasmReady } = useWasm();
    expect(await initWasm()).toBe(true);
    expect(wasmReady.value).toBe(true);
  });

  it('initWasm 失敗時回傳 false', async () => {
    initWasmModuleMock.mockRejectedValue(new Error('load failed'));
    const { initWasm, wasmReady } = useWasm();
    expect(await initWasm()).toBe(false);
    expect(wasmReady.value).toBe(false);
  });

  it('locateFile 對 .wasm 路徑加上 BASE_URL', async () => {
    let capturedLocateFile: ((path: string) => string) | undefined;
    initWasmModuleMock.mockImplementation((options: { locateFile: (p: string) => string }) => {
      capturedLocateFile = options.locateFile;
      return Promise.resolve({});
    });
    const { initWasm } = useWasm();
    await initWasm();
    expect(capturedLocateFile?.('alloc_algo.wasm')).toBe(
      `${import.meta.env.BASE_URL}alloc_algo.wasm`,
    );
    expect(capturedLocateFile?.('other.js')).toBe('other.js');
  });

  it('cleanup 刪除 shuffler 實例', () => {
    const { cleanup, shufflerInstance } = useWasm();
    const deleteSpy = vi.fn();
    shufflerInstance.value = { delete: deleteSpy } as never;
    cleanup();
    expect(deleteSpy).toHaveBeenCalled();
    expect(shufflerInstance.value).toBeNull();
  });

  it('元件 unmount 時自動 cleanup', () => {
    const deleteSpy = vi.fn();
    const { shufflerInstance } = useWasm();
    shufflerInstance.value = { delete: deleteSpy } as never;

    const TestComp = defineComponent({
      setup() {
        useWasm();
        return () => null;
      },
    });
    const wrapper = mount(TestComp);
    wrapper.unmount();

    expect(deleteSpy).toHaveBeenCalled();
    expect(shufflerInstance.value).toBeNull();
  });

  it('cleanup 在 shuffler 為 null 時安全執行', () => {
    const { cleanup, shufflerInstance } = useWasm();
    shufflerInstance.value = null;
    expect(() => cleanup()).not.toThrow();
  });
});
