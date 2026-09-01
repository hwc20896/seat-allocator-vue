// src/utils/Position.test.ts
import { describe, expect, it } from 'vitest'
import { Position, swap } from './Position'
import { FakeGrid } from '@/utils/__tests__/fakeGrid'

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
