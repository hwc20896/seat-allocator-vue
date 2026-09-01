import { expect, it } from 'vitest';
import { mount } from '@vue/test-utils';
import AppFooter from '@/components/layout/AppFooter.vue';

it('顯示狀態文字與 active class', () => {
  const wrapper = mount(AppFooter, { props: { statusText: '已導入', isGridLoaded: true } });
  expect(wrapper.text()).toContain('已導入');
  expect(wrapper.find('.status-indicator').classes()).toContain('active');
});
