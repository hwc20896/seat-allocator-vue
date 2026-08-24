import { expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import DropDownMenu from '@/components/common/DropDownMenu.vue'

it('渲染 label 與 slot', () => {
  const wrapper = mount(DropDownMenu, {
    props: { label: '文件' },
    slots: { default: '<span>內容</span>' },
  })
  expect(wrapper.text()).toContain('文件')
  expect(wrapper.text()).toContain('內容')
})
