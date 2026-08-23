import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'
import GridCell from './GridCell.vue'

const baseProps = {
  text: '',
  color: '#000',
  isTagged: false,
  isSwapped: false,
  isShuffling: false,
  isCurrentlyOriginal: false,
}

describe('GridCell', () => {
  it('有文字時顯示🪑圖示與文字', () => {
    const wrapper = mount(GridCell, { props: { ...baseProps, text: '王小明' } })
    expect(wrapper.find('.cell-icon').exists()).toBe(true)
    expect(wrapper.text()).toContain('王小明')
  })

  it('空字串時顯示「空位」且沒有圖示', () => {
    const wrapper = mount(GridCell, { props: baseProps })
    expect(wrapper.find('.cell-icon').exists()).toBe(false)
    expect(wrapper.text()).toContain('空位')
  })

  it('isShuffling 為 true 時加上 shuffling class', () => {
    const wrapper = mount(GridCell, { props: { ...baseProps, isShuffling: true } })
    expect(wrapper.find('.cell-content').classes()).toContain('shuffling')
  })

  it('點擊時 emit click 事件', () => {
    const wrapper = mount(GridCell, { props: baseProps })
    wrapper.trigger('click')
    expect(wrapper.emitted('click')).toHaveLength(1)
  })

  it('isTagged 為 true 時加上 tagged class', () => {
    const wrapper = mount(GridCell, { props: { ...baseProps, isTagged: true } })
    expect(wrapper.find('.cell-content').classes()).toContain('tagged')
  })

  it('isSwapped 為 true 時加上 swapped class', () => {
    const wrapper = mount(GridCell, { props: { ...baseProps, isSwapped: true } })
    expect(wrapper.find('.cell-content').classes()).toContain('swapped')
  })

  it('isCurrentlyOriginal 為 true 時加上 original-view class', () => {
    const wrapper = mount(GridCell, { props: { ...baseProps, isCurrentlyOriginal: true } })
    expect(wrapper.find('.cell-content').classes()).toContain('original-view')
  })
})
