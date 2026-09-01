// 共享測試替身：模擬 emscripten 生成的 StringVector 與 Grid。
// 注意：alloc_algo.d.ts 由 --embind-tsd 自動生成，接口變動時請同步此檔。
import type { ClassHandle, Grid, StringVector } from '@/assets/wasm/alloc_algo';

/** 從 StringVector 方法簽名提取 EmbindString 類型（d.ts 未直接導出） */
type EmbindString = Parameters<StringVector['push_back']>[0];

const embindStringToString = (value: EmbindString): string =>
  typeof value === 'string' ? value : new TextDecoder().decode(value);

export class FakeStringVector implements StringVector {
  private items: string[];

  constructor(items: string[] = []) {
    this.items = [...items];
  }

  push_back(_0: EmbindString): void {
    this.items.push(embindStringToString(_0));
  }

  resize(_0: number, _1: EmbindString): void {
    this.items.length = _0;
    for (let i = 0; i < _0; i++) {
      if (this.items[i] === undefined) this.items[i] = embindStringToString(_1);
    }
  }

  size(): number {
    return this.items.length;
  }

  get(_0: number): string | undefined {
    return this.items[_0];
  }

  set(_0: number, _1: EmbindString): boolean {
    if (_0 < 0 || _0 >= this.items.length) return false;
    this.items[_0] = embindStringToString(_1);
    return true;
  }

  // ClassHandle
  isAliasOf(other: ClassHandle): boolean {
    return this === other;
  }

  delete(): void {
    this.items = [];
  }

  deleteLater(): this {
    return this;
  }

  isDeleted(): boolean {
    return false;
  }

  [Symbol.dispose](): void {
    this.delete();
  }

  clone(): this {
    return new FakeStringVector([...this.items]) as this;
  }

  // Iterable<string>
  [Symbol.iterator](): Iterator<string> {
    return this.items[Symbol.iterator]();
  }
}

export class FakeGrid implements Grid {
  private data: string[][];

  constructor(
    rowsOrData: number | string[][] = 0,
    cols = 0,
    flatData: string[] | StringVector = [],
  ) {
    if (typeof rowsOrData === 'number') {
      const items: string[] = Array.isArray(flatData) ? flatData : Array.from(flatData);
      this.data = Array.from({ length: rowsOrData }, (_, r) =>
        Array.from({ length: cols }, (_, c) => items[r * cols + c] ?? ''),
      );
    } else {
      this.data = rowsOrData.map((row) => [...row]);
    }
  }

  getByPos(row: number, col: number): string {
    return this.data[row]?.[col] ?? '';
  }

  getByIndex(index: number): string {
    const colCount = this.colCount() || 1;
    return this.getByPos(Math.floor(index / colCount), index % colCount);
  }

  setByPos(row: number, col: number, value: EmbindString): void {
    const r = this.data[row];
    if (r) r[col] = embindStringToString(value);
  }

  setByIndex(index: number, value: EmbindString): void {
    const colCount = this.colCount() || 1;
    this.setByPos(Math.floor(index / colCount), index % colCount, value);
  }

  rowCount(): number {
    return this.data.length;
  }

  colCount(): number {
    return this.data.reduce((max, r) => Math.max(max, r.length), 0);
  }

  size(): number {
    return this.rowCount() * this.colCount();
  }

  empty(): boolean {
    return this.rowCount() === 0;
  }

  rawData(): StringVector {
    return new FakeStringVector(this.data.flat());
  }

  clone(): Grid {
    return new FakeGrid(this.data.map((row) => [...row]));
  }

  toCSVString(): string {
    return this.data.map((row) => row.join(',')).join('\n');
  }

  // ClassHandle
  isAliasOf(other: ClassHandle): boolean {
    return this === other;
  }

  delete(): void {}

  deleteLater(): this {
    return this;
  }

  isDeleted(): boolean {
    return false;
  }

  [Symbol.dispose](): void {
    this.delete();
  }
}
