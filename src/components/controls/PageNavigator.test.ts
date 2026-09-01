import { describe, expect, it } from 'vitest';
import { mount } from '@vue/test-utils';
import PageNavigator from './PageNavigator.vue';

const mountNav = (overrides: Partial<Record<string, unknown>> = {}) =>
  mount(PageNavigator, {
    props: {
      pageLabel: '第 3 次分配',
      currentIndex: 3,
      totalPages: 5,
      isShuffling: false,
      isGridLoaded: true,
      showOriginal: false,
      ...overrides,
    },
  });

describe('PageNavigator', () => {
  it('顯示 pageLabel', () => {
    expect(mountNav().text()).toContain('第 3 次分配');
  });

  it('前一頁/後一頁按鈕 emit navigate', async () => {
    const wrapper = mountNav();
    await wrapper.findAll('button.tool-button')[0]!.trigger('click');
    await wrapper.findAll('button.tool-button')[1]!.trigger('click');
    expect(wrapper.emitted('navigate')).toEqual([[-1], [1]]);
  });

  it('第一頁時前一頁 disabled', () => {
    const wrapper = mountNav({ currentIndex: 1 });
    expect(wrapper.findAll('button.tool-button')[0]!.attributes('disabled')).toBeDefined();
  });

  it('最後一頁時後一頁 disabled', () => {
    const wrapper = mountNav({ currentIndex: 5 });
    expect(wrapper.findAll('button.tool-button')[1]!.attributes('disabled')).toBeDefined();
  });

  it('切換原始列表按鈕 emit toggle-original', async () => {
    const wrapper = mountNav({ totalPages: 3 });
    await wrapper.find('button.push-button').trigger('click');
    expect(wrapper.emitted('toggle-original')).toHaveLength(1);
  });

  it('未導入時切換按鈕 disabled', () => {
    const wrapper = mountNav({ isGridLoaded: false });
    expect(wrapper.find('button.push-button').attributes('disabled')).toBeDefined();
  });

  it('showOriginal 時按鈕文字為顯示當前打亂', () => {
    const wrapper = mountNav({ showOriginal: true });
    expect(wrapper.text()).toContain('顯示當前打亂');
  });

  it('PageUp 快捷鍵 emit navigate -1', () => {
    const wrapper = mountNav({ currentIndex: 3 });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'PageUp', cancelable: true }));
    expect(wrapper.emitted('navigate')).toEqual([[-1]]);
  });

  it('第一頁時 PageUp 快捷鍵不 emit', () => {
    const wrapper = mountNav({ currentIndex: 1 });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'PageUp', cancelable: true }));
    expect(wrapper.emitted('navigate')).toBeUndefined();
  });

  it('PageDown 快捷鍵 emit navigate 1', () => {
    const wrapper = mountNav({ currentIndex: 3, totalPages: 5 });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'PageDown', cancelable: true }));
    expect(wrapper.emitted('navigate')).toEqual([[1]]);
  });

  it('Home 快捷鍵 emit toggle-original', () => {
    const wrapper = mountNav({ totalPages: 3 });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Home', cancelable: true }));
    expect(wrapper.emitted('toggle-original')).toHaveLength(1);
  });
});
