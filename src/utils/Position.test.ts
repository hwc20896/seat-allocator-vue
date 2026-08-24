// src/utils/Position.test.ts
import { describe, expect, it } from 'vitest'
import type { Grid } from '@/assets/wasm/alloc_algo'
import { Position, swap } from './Position'

class FakeGrid implements Grid {
  private data: string[][]

  constructor(data: string[][]) {
    this.data = data
  }

  clone(): Grid {
    return new FakeGrid(this.data.map((row) => [...row]))
  }

  rowCount(): number {
    return this.data.length
  }

  colCount(): number {
    return this.data[0]?.length ?? 0
  }

  getByPos(row: number, col: number): string {
    return this.data[row]?.[col] ?? ''
  }

  setByPos(row: number, col: number, value: string): void {
    const targetRow = this.data[row]
    if (targetRow) targetRow[col] = value
  }

  getByIndex(idx: number): string {
    const cols = this.colCount()
    return this.getByPos(Math.floor(idx / cols), idx % cols)
  }

  setByIndex(idx: number, value: string): void {
    const cols = this.colCount()
    this.setByPos(Math.floor(idx / cols), idx % cols, value)
  }

  size(): number {
    return this.rowCount() * this.colCount()
  }

  empty(): boolean {
    return this.data.length === 0
  }

  rawData(): string[] {
    return this.data.flat()
  }

  toCSVString(): string {
    return this.data.map((row) => row.join(',')).join('\n')
  }
}

describe('Position', () => {
  it('toString 支援 baseZero 參數', () => {
    expect(new Position(0, 0).toString()).toBe('(0, 0)')
    expect(new Position(0, 0).toString(true)).toBe('(1, 1)')
  })

  it('equals 比較行列是否相同', () => {
    expect(new Position(1, 2).equals(new Position(1, 2))).toBe(true)
    expect(new Position(1, 2).equals(new Position(2, 1))).toBe(false)
  })
})

describe('swap', () => {
  it('交換兩個位置的值並回傳新 grid，不修改原 grid', () => {
    const grid = new FakeGrid([
      ['A', 'B'],
      ['C', 'D'],
    ])
    const result = swap(grid, new Position(0, 0), new Position(1, 1))
    expect(result.getByPos(0, 0)).toBe('D')
    expect(result.getByPos(1, 1)).toBe('A')
    expect(grid.getByPos(0, 0)).toBe('A')
  })

  it('位置超出範圍時拋出 RangeError', () => {
    const grid = new FakeGrid([['A', 'B']])
    expect(() => swap(grid, new Position(0, 0), new Position(5, 5))).toThrow(RangeError)
  })

  it('pos1 超出範圍時拋出 RangeError', () => {
    const grid = new FakeGrid([['A', 'B']])
    expect(() => swap(grid, new Position(5, 5), new Position(0, 0))).toThrow(RangeError)
  })
})
