// composables/useKeyboardShortcut.ts
import { onMounted, onUnmounted } from 'vue'

export const SPECIAL_KEYS = {
  // 方向鍵
  ARROW_UP: 'ArrowUp',
  ARROW_DOWN: 'ArrowDown',
  ARROW_LEFT: 'ArrowLeft',
  ARROW_RIGHT: 'ArrowRight',

  F1: 'F1',
  F2: 'F2',
  F3: 'F3',
  F4: 'F4',
  F5: 'F5',
  F6: 'F6',
  F7: 'F7',
  F8: 'F8',
  F9: 'F9',
  F10: 'F10',
  F11: 'F11',
  F12: 'F12',

  ESC: 'Escape',
  TAB: 'Tab',
  SPACE: ' ',
  ENTER: 'Enter',
  BACKSPACE: 'Backspace',
  DELETE: 'Delete',
  HOME: 'Home',
  END: 'End',
  PAGE_UP: 'PageUp',
  PAGE_DOWN: 'PageDown',
  INSERT: 'Insert',
} as const

export type SpecialKey = (typeof SPECIAL_KEYS)[keyof typeof SPECIAL_KEYS]

export interface KeyCombination {
  key: string | SpecialKey
  ctrl?: boolean
  shift?: boolean
  alt?: boolean
  meta?: boolean // Mac Command 鍵
}

export interface ShortcutOptions {
  /** 是否在輸入框中禁用（避免干擾使用者的打字） */
  ignoreInput?: boolean
  /** 是否阻止預設行為 */
  preventDefault?: boolean

  target?: string
}

export function useKeyboardShortcut(
  shortcut: KeyCombination | string,
  callback: (event: KeyboardEvent) => void,
  options: ShortcutOptions = {},
) {
  const { ignoreInput = true, preventDefault = true, target } = options

  // 解析快捷鍵，支援字串簡寫如 'ctrl+o'
  const parsed = ((s: KeyCombination | string): KeyCombination => {
    if (typeof s === 'string') {
      const parts = s.toLowerCase().split('+')
      return {
        ctrl: parts.includes('ctrl'),
        shift: parts.includes('shift'),
        alt: parts.includes('alt'),
        meta: parts.includes('meta'),
        key: parts[parts.length - 1] || '',
      }
    }
    return s
  }) (shortcut)

  const handler = (event: KeyboardEvent) => {
    // 檢查是否在輸入框中
    if (ignoreInput) {
      const tag = (event.target as HTMLElement)?.tagName?.toLowerCase()
      const isInput = ['input', 'textarea', 'select'].includes(tag)
      const isContentEditable = (event.target as HTMLElement)?.isContentEditable
      if (isInput || isContentEditable) return
    }

    // 檢查特定目標元素
    if (target) {
      const targetEl = document.querySelector(target)
      if (targetEl && !targetEl.contains(event.target as Node)) return
    }

    // 檢查組合鍵
    const ctrlMatch = (parsed.ctrl ?? false) === event.ctrlKey
    const shiftMatch = (parsed.shift ?? false) === event.shiftKey
    const altMatch = (parsed.alt ?? false) === event.altKey
    const metaMatch = (parsed.meta ?? false) === event.metaKey

    // 檢查主鍵（不區分大小寫，但特殊按鍵需要精確比對）
    let keyMatch: boolean

    // 如果是特殊按鍵（方向鍵、F1等），精確比對
    const specialKeyValues = Object.values(SPECIAL_KEYS)
    if (specialKeyValues.includes(parsed.key as SpecialKey)) {
      keyMatch = parsed.key === event.key
    } else {
      // 一般按鍵不區分大小寫
      keyMatch = parsed.key.toLowerCase() === event.key.toLowerCase()
    }

    if (ctrlMatch && shiftMatch && altMatch && metaMatch && keyMatch) {
      if (preventDefault) {
        event.preventDefault()
      }
      callback(event)
    }
  }

  onMounted(() => {
    document.addEventListener('keydown', handler)
  })

  onUnmounted(() => {
    document.removeEventListener('keydown', handler)
  })

  return {
    cleanup: () => {
      document.removeEventListener('keydown', handler)
    },
    handler,
  }
}

export const isArrowKey = (key: string): boolean => {
  return (
    [
      SPECIAL_KEYS.ARROW_UP,
      SPECIAL_KEYS.ARROW_DOWN,
      SPECIAL_KEYS.ARROW_LEFT,
      SPECIAL_KEYS.ARROW_RIGHT,
    ] as SpecialKey[]
  ).includes(key as SpecialKey)
}
