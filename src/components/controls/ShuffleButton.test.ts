import { describe, expect, it } from 'vitest';
import { mount } from '@vue/test-utils';
import ShuffleButton from './ShuffleButton.vue';

describe('ShuffleButton', () => {
  it('點擊時 emit shuffle', async () => {
    const wrapper = mount(ShuffleButton, { props: { isShuffling: false, isGridLoaded: true } });
    await wrapper.find('button').trigger('click');
    expect(wrapper.emitted('shuffle')).toHaveLength(1);
  });

  it('未導入或洗牌中時 disabled', () => {
    const wrapper = mount(ShuffleButton, { props: { isShuffling: false, isGridLoaded: false } });
    expect(wrapper.find('button').attributes('disabled')).toBeDefined();
    wrapper.setProps({ isShuffling: true, isGridLoaded: true });
    expect(wrapper.find('button').attributes('disabled')).toBeDefined();
  });

  it('洗牌中顯示 spinner 與對應文字', () => {
    const wrapper = mount(ShuffleButton, { props: { isShuffling: true, isGridLoaded: true } });
    expect(wrapper.find('.spinner').exists()).toBe(true);
    expect(wrapper.text()).toContain('正在隨機分配洗牌');
  });

  it('Enter 快捷鍵在已導入時 emit shuffle', () => {
    const wrapper = mount(ShuffleButton, { props: { isShuffling: false, isGridLoaded: true } });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', cancelable: true }));
    expect(wrapper.emitted('shuffle')).toHaveLength(1);
    wrapper.unmount();
  });

  it('未導入時 Enter 快捷鍵不 emit', () => {
    const wrapper = mount(ShuffleButton, { props: { isShuffling: false, isGridLoaded: false } });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', cancelable: true }));
    expect(wrapper.emitted('shuffle')).toBeUndefined();
    wrapper.unmount();
  });
});
