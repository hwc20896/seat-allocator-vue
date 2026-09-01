import { describe, expect, it } from 'vitest';
import { mount } from '@vue/test-utils';
import SeatGrid from './SeatGrid.vue';
import GridCell from './GridCell.vue';
import { Position } from '@/utils/Position.ts';

const makeMiniGrid = () =>
  ({
    rowCount: () => 2,
    colCount: () => 2,
    getByPos: (r: number, c: number) => ['A1', 'A2', 'B1', 'B2'][r * 2 + c] ?? '',
  }) as never;

const mountSeatGrid = (grid: unknown) =>
  mount(SeatGrid, {
    props: {
      grid: grid as never,
      isShuffling: false,
      taggedRow: null,
      taggedCol: null,
      getCellColor: () => '#fff',
      isCellSwapped: () => false,
      isCurrentlyOriginal: true,
    },
  });

describe('SeatGrid', () => {
  it('渲染 rowCount x colCount 個 GridCell', () => {
    const wrapper = mountSeatGrid(makeMiniGrid());
    expect(wrapper.findAllComponents(GridCell)).toHaveLength(4);
  });

  it('顯示 1-based 行/列索引與黑板標記', () => {
    const wrapper = mountSeatGrid(makeMiniGrid());
    const rowIndices = wrapper.findAll('.row-index').map((el) => el.text());
    const colIndices = wrapper.findAll('.col-index').map((el) => el.text());
    expect(rowIndices).toEqual(['1', '2']);
    expect(colIndices).toEqual(['1', '2']);
    expect(wrapper.find('.blackboard-bar').text()).toContain('黑板');
  });

  it('點擊 cell 時 emit cell-click 帶 Position', async () => {
    const wrapper = mountSeatGrid(makeMiniGrid());
    await wrapper.findAllComponents(GridCell)[0]!.trigger('click');
    const payload = wrapper.emitted('cell-click')![0]![0] as Position;
    expect(payload).toBeInstanceOf(Position);
    expect(payload.row).toBe(0);
    expect(payload.col).toBe(0);
  });

  it('grid 為 null 時不渲染 cell、索引與黑板標記', () => {
    const wrapper = mountSeatGrid(null);
    expect(wrapper.findAllComponents(GridCell)).toHaveLength(0);
    expect(wrapper.find('.blackboard-bar').exists()).toBe(false);
    expect(wrapper.find('.row-index').exists()).toBe(false);
  });

  it('傳遞 tagged / swapped / currentlyOriginal props 給 GridCell', async () => {
    const wrapper = mountSeatGrid(makeMiniGrid());
    await wrapper.setProps({
      taggedRow: 1,
      taggedCol: 0,
      isCellSwapped: (pos: Position) => pos.row === 0 && pos.col === 1,
      isCurrentlyOriginal: false,
    });
    const cells = wrapper.findAllComponents(GridCell);
    expect(cells[2]!.props('isTagged')).toBe(true);
    expect(cells[0]!.props('isTagged')).toBe(false);
    expect(cells[1]!.props('isSwapped')).toBe(true);
    expect(cells[0]!.props('isSwapped')).toBe(false);
    expect(cells[0]!.props('isCurrentlyOriginal')).toBe(false);
  });

  it('taggedCol 不匹配或洗牌中時 prop 為 false', async () => {
    const wrapper = mountSeatGrid(makeMiniGrid());
    await wrapper.setProps({
      taggedRow: 0,
      taggedCol: 5,
      isShuffling: true,
      isCellSwapped: () => true,
    });
    const cells = wrapper.findAllComponents(GridCell);
    cells.forEach((c) => {
      expect(c.props('isTagged')).toBe(false);
      expect(c.props('isSwapped')).toBe(false);
    });
  });
});
