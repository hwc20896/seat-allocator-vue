import { afterEach, describe, expect, it } from 'vitest'
import { mount } from '@vue/test-utils'
import { defineComponent, h, type Ref } from 'vue'
import { useUnsavedChangesGuard } from './useUnsavedChangesGuard'

interface GuardApi {
  hasUnsavedChanges: Ref<boolean>
  markDirty: () => void
  markClean: () => void
}

const wrappers: ReturnType<typeof mount>[] = []

const mountGuard = (): GuardApi => {
  let api!: GuardApi
  const wrapper = mount(
    defineComponent({
      setup() {
        api = useUnsavedChangesGuard()
        return () => h('div')
      },
    }),
  )
  wrappers.push(wrapper)
  return api
}

const fireBeforeUnload = (): Event => {
  const event = new Event('beforeunload', { cancelable: true })
  window.dispatchEvent(event)
  return event
}

afterEach(() => {
  wrappers.splice(0).forEach((w) => w.unmount())
  document.body.innerHTML = ''
})

describe('useUnsavedChangesGuard', () => {
  it('初始狀態為無未儲存變更', () => {
    const api = mountGuard()
    expect(api.hasUnsavedChanges.value).toBe(false)
  })

  it('markDirty 後標記為有未儲存變更', () => {
    const api = mountGuard()
    api.markDirty()
    expect(api.hasUnsavedChanges.value).toBe(true)
  })

  it('markClean 後回到無未儲存變更', () => {
    const api = mountGuard()
    api.markDirty()
    api.markClean()
    expect(api.hasUnsavedChanges.value).toBe(false)
  })

  it('無未儲存變更時 beforeunload 不阻擋', () => {
    mountGuard()
    const event = fireBeforeUnload()
    expect(event.defaultPrevented).toBe(false)
  })

  it('有未儲存變更時 beforeunload 會 preventDefault', () => {
    const api = mountGuard()
    api.markDirty()
    const event = fireBeforeUnload()
    expect(event.defaultPrevented).toBe(true)
  })

  it('unmount 後移除 beforeunload 監聽', () => {
    const api = mountGuard()
    api.markDirty()
    wrappers.splice(0).forEach((w) => w.unmount())
    const event = fireBeforeUnload()
    expect(event.defaultPrevented).toBe(false)
  })
})
