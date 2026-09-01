import { afterEach, describe, expect, it, vi } from 'vitest';
import { shallowRef } from 'vue';
import * as XLSX from '@e965/xlsx';
import { useFileIO } from './useFileIO';
import { FakeGrid, FakeStringVector } from '@/utils/__tests__/fakeGrid';

vi.mock('@e965/xlsx', async (importOriginal) => {
  const actual = await importOriginal<typeof import('@e965/xlsx')>();
  return {
    ...actual,
    read: vi.fn(actual.read),
  };
});

afterEach(() => {
  vi.mocked(XLSX.read).mockRestore();
  vi.restoreAllMocks();
});

const setup = () => {
  const wasmModule = shallowRef({ Grid: FakeGrid, StringVector: FakeStringVector } as never);
  return { wasmModule, ...useFileIO(wasmModule) };
};

const makeXLSXFile = (rows: unknown[][]): File => {
  const ws = XLSX.utils.aoa_to_sheet(rows);
  const wb = XLSX.utils.book_new();
  XLSX.utils.book_append_sheet(wb, ws, 'Sheet1');
  const buffer = XLSX.write(wb, { bookType: 'xlsx', type: 'array' }) as ArrayBuffer;
  return new File([buffer], 'test.xlsx');
};

describe('useFileIO', () => {
  it('readTextFile 讀取文字內容', async () => {
    const { readTextFile } = setup();
    const file = new File(['hello\nworld'], 'test.csv', { type: 'text/csv' });
    await expect(readTextFile(file)).resolves.toBe('hello\nworld');
  });

  it('parseXLSX 解析工作表為 Grid', async () => {
    const { parseXLSX } = setup();
    const grid = await parseXLSX(
      makeXLSXFile([
        ['A1', 'A2'],
        ['B1', 'B2'],
      ]),
    );
    expect(grid).toBeInstanceOf(FakeGrid);
    expect(grid.rowCount()).toBe(2);
    expect(grid.colCount()).toBe(2);
    expect(grid.getByPos(0, 0)).toBe('A1');
    expect(grid.getByPos(1, 1)).toBe('B2');
  });

  it('parseXLSX 過濾空列與 trim 空白', async () => {
    const { parseXLSX } = setup();
    const grid = await parseXLSX(makeXLSXFile([['  x  '], [], ['y']]));
    expect(grid.rowCount()).toBe(2);
    expect(grid.getByPos(0, 0)).toBe('x');
    expect(grid.getByPos(1, 0)).toBe('y');
  });

  it('parseXLSX 全空內容時回傳空 Grid', async () => {
    const { parseXLSX } = setup();
    const grid = await parseXLSX(makeXLSXFile([['']]));
    expect(grid.rowCount()).toBe(0);
  });

  it('generateXLSXBuffer 產出可讀回的 ArrayBuffer', async () => {
    const { generateXLSXBuffer, parseXLSX } = setup();
    const grid = new FakeGrid(2, 2, ['a', 'b', 'c', 'd']);
    const buffer = generateXLSXBuffer(grid);
    expect(buffer).toBeInstanceOf(ArrayBuffer);
    const parsed = await parseXLSX(new File([buffer], 'out.xlsx'));
    expect(parsed.getByPos(1, 1)).toBe('d');
  });

  it('parseXLSX 讀取失敗時 reject', async () => {
    const { parseXLSX } = setup();
    const error = new Error('read failed');
    const original = globalThis.FileReader;
    globalThis.FileReader = class {
      onload: ((e: ProgressEvent) => void) | null = null;
      onerror: ((e: ProgressEvent) => void) | null = null;
      result: ArrayBuffer | null = null;
      readAsArrayBuffer(): void {
        this.onerror?.(error as unknown as ProgressEvent);
      }
    } as unknown as typeof FileReader;
    try {
      await expect(parseXLSX(new File([new ArrayBuffer(8)], 'bad.xlsx'))).rejects.toBe(error);
    } finally {
      globalThis.FileReader = original;
    }
  });

  it('parseXLSX 解析失敗時 reject', async () => {
    vi.mocked(XLSX.read).mockImplementation(() => {
      throw new Error('corrupt file');
    });
    const { parseXLSX } = setup();
    const file = new File([new Uint8Array([1, 2, 3, 4, 5])], 'bad.xlsx');
    await expect(parseXLSX(file)).rejects.toThrow('corrupt file');
  });
});
