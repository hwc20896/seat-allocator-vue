import { afterEach, describe, expect, it, vi } from 'vitest'
import { mount, type VueWrapper } from '@vue/test-utils'
import AppHeader from './AppHeader.vue'

const mounted: VueWrapper[] = []

const mountHeader = (overrides: Record<string, unknown> = {}) => {
  const wrapper = mount(AppHeader, {
    props: { isGridLoaded: false, colorPresetCount: 0, hasCustomConfig: false, ...overrides },
  })
  mounted.push(wrapper)
  return wrapper
}


afterEach(() => {
  vi.restoreAllMocks()
  mounted.splice(0).forEach((w) => w.unmount())
  document.body.innerHTML = ''
})

describe('AppHeader', () => {
  it('導入 .csv 檔案時 emit csv-import', async () => {
    const wrapper = mountHeader()
    const input = wrapper.find('input[type="file"]')
    const file = new File(['a,b'], 'test.csv', { type: 'text/csv' })
    Object.defineProperty(input.element, 'files', { value: [file], configurable: true })
    await input.trigger('change')
    expect(wrapper.emitted('csv-import')![0]![0]).toBe(file)
  })

  it('導入 .xlsx 檔案時 emit xlsx-import', async () => {
    const wrapper = mountHeader()
    const input = wrapper.find('input[type="file"]')
    const file = new File(['x'], 'test.xlsx')
    Object.defineProperty(input.element, 'files', { value: [file], configurable: true })
    await input.trigger('change')
    expect(wrapper.emitted('xlsx-import')![0]![0]).toBe(file)
  })

  it('未導入 grid 時匯出按鈕 disabled，導入後點擊 emit grid-export', async () => {
    const wrapper = mountHeader()
    const exportBtn = wrapper
      .findAll('button.dropdown-item')
      .find((b) => b.text().includes('導出'))!
    expect(exportBtn.attributes('disabled')).toBeDefined()
    await wrapper.setProps({ isGridLoaded: true })
    await wrapper
      .findAll('button.dropdown-item')
      .find((b) => b.text().includes('導出'))!
      .trigger('click')
    expect(wrapper.emitted('grid-export')).toHaveLength(1)
  })

  it('Ctrl+E 快捷鍵在已導入時 emit grid-export', async () => {
    const wrapper = mountHeader({ isGridLoaded: true })
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'e', ctrlKey: true, cancelable: true }))
    expect(wrapper.emitted('grid-export')).toHaveLength(1)
  })

  it('重設顏色按鈕在無顏色時 disabled', async () => {
    const wrapper = mountHeader()
    const resetBtn = wrapper.findAll('button.dropdown-item').find((b) => b.text().includes('重設'))!
    expect(resetBtn.attributes('disabled')).toBeDefined()
    await wrapper.setProps({ colorPresetCount: 3 })
    expect(
      wrapper
        .findAll('button.dropdown-item')
        .find((b) => b.text().includes('重設'))!
        .attributes('disabled'),
    ).toBeUndefined()
  })

  it('導入顏色配置時 emit color-import', async () => {
    const wrapper = mountHeader()
    const inputs = wrapper.findAll('input[type="file"]')
    const file = new File(['{}'], 'colors.json')
    Object.defineProperty(inputs[1]!.element, 'files', { value: [file], configurable: true })
    await inputs[1]!.trigger('change')
    expect(wrapper.emitted('color-import')![0]![0]).toBe(file)
  })

  it('導入約束配置時 emit constraints-import', async () => {
    const wrapper = mountHeader()
    const inputs = wrapper.findAll('input[type="file"]')
    const file = new File(['{}'], 'constraints.json')
    Object.defineProperty(inputs[2]!.element, 'files', { value: [file], configurable: true })
    await inputs[2]!.trigger('change')
    expect(wrapper.emitted('constraints-import')![0]![0]).toBe(file)
  })

  it('點擊重設顏色按鈕 emit clear-colors', async () => {
    const wrapper = mountHeader({ colorPresetCount: 3 })
    const resetButtons = wrapper.findAll('button.dropdown-item').filter((b) => b.text().includes('重設'))
    await resetButtons[0]!.trigger('click')
    expect(wrapper.emitted('clear-colors')).toHaveLength(1)
  })

  it('點擊重設約束按鈕 emit reset-constraints', async () => {
    const wrapper = mountHeader({ hasCustomConfig: true })
    const resetButtons = wrapper.findAll('button.dropdown-item').filter((b) => b.text().includes('重設'))
    await resetButtons[1]!.trigger('click')
    expect(wrapper.emitted('reset-constraints')).toHaveLength(1)
  })

  it('Ctrl+I 快捷鍵觸發檔案輸入 click', () => {
    const clickSpy = vi.spyOn(HTMLInputElement.prototype, 'click').mockImplementation(() => {})
    mountHeader()
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'i', ctrlKey: true, cancelable: true }))
    expect(clickSpy).toHaveBeenCalledTimes(1)
  })

  it('Ctrl+Shift+C 快捷鍵觸發顏色輸入 click', () => {
    const clickSpy = vi.spyOn(HTMLInputElement.prototype, 'click').mockImplementation(() => {})
    mountHeader()
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'c', ctrlKey: true, shiftKey: true, cancelable: true }))
    expect(clickSpy).toHaveBeenCalledTimes(1)
  })

  it('Ctrl+Shift+K 快捷鍵觸發約束輸入 click', () => {
    const clickSpy = vi.spyOn(HTMLInputElement.prototype, 'click').mockImplementation(() => {})
    mountHeader()
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'k', ctrlKey: true, shiftKey: true, cancelable: true }))
    expect(clickSpy).toHaveBeenCalledTimes(1)
  })

  it('Ctrl+Alt+C 快捷鍵：無顏色不 emit，有顏色 emit clear-colors', () => {
    const wrapper = mountHeader()
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'c', ctrlKey: true, altKey: true, cancelable: true }))
    expect(wrapper.emitted('clear-colors')).toBeUndefined()
    const wrapper2 = mountHeader({ colorPresetCount: 3 })
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'c', ctrlKey: true, altKey: true, cancelable: true }))
    expect(wrapper2.emitted('clear-colors')).toHaveLength(1)
  })

  it('Ctrl+Alt+K 快捷鍵：無自訂約束不 emit，有自訂 emit reset-constraints', () => {
    const wrapper = mountHeader()
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'k', ctrlKey: true, altKey: true, cancelable: true }))
    expect(wrapper.emitted('reset-constraints')).toBeUndefined()
    const wrapper2 = mountHeader({ hasCustomConfig: true })
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'k', ctrlKey: true, altKey: true, cancelable: true }))
    expect(wrapper2.emitted('reset-constraints')).toHaveLength(1)
  })

  it('檔案輸入無檔案時不 emit 任何事件', async () => {
    const wrapper = mountHeader()
    const inputs = wrapper.findAll('input[type="file"]')
    await inputs[0]!.trigger('change')
    await inputs[1]!.trigger('change')
    await inputs[2]!.trigger('change')
    expect(wrapper.emitted('csv-import')).toBeUndefined()
    expect(wrapper.emitted('xlsx-import')).toBeUndefined()
    expect(wrapper.emitted('color-import')).toBeUndefined()
    expect(wrapper.emitted('constraints-import')).toBeUndefined()
  })
})
