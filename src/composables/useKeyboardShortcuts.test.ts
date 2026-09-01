import { afterEach, describe, expect, it, vi } from 'vitest';
import { mount } from '@vue/test-utils';
import { defineComponent, h } from 'vue';
import {
  isArrowKey,
  SPECIAL_KEYS,
  useKeyboardShortcut,
  type KeyCombination,
  type ShortcutOptions,
} from './useKeyboardShortcuts';

const wrappers: ReturnType<typeof mount>[] = [];

const mountShortcut = (
  shortcut: KeyCombination | string,
  callback: (e: KeyboardEvent) => void,
  options: ShortcutOptions = {},
) => {
  const wrapper = mount(
    defineComponent({
      setup() {
        useKeyboardShortcut(shortcut, callback, options);
        return () => h('div');
      },
    }),
  );
  wrappers.push(wrapper);
  return wrapper;
};

const press = (init: KeyboardEventInit) => {
  document.dispatchEvent(new KeyboardEvent('keydown', { cancelable: true, ...init }));
};

afterEach(() => {
  vi.restoreAllMocks();
  wrappers.splice(0).forEach((w) => w.unmount());
  document.body.innerHTML = '';
});

describe('useKeyboardShortcut', () => {
  it('字串簡寫 ctrl+o 觸發 callback 並 preventDefault', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+o', callback);
    const event = new KeyboardEvent('keydown', { key: 'o', ctrlKey: true, cancelable: true });
    document.dispatchEvent(event);
    expect(callback).toHaveBeenCalledTimes(1);
    expect(event.defaultPrevented).toBe(true);
  });

  it('缺少修飾鍵時不觸發', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+o', callback);
    press({ key: 'o' });
    expect(callback).not.toHaveBeenCalled();
  });

  it('一般按鍵不區分大小寫', () => {
    const callback = vi.fn();
    mountShortcut('a', callback);
    press({ key: 'A' });
    expect(callback).toHaveBeenCalledTimes(1);
  });

  it('特殊鍵（物件形式）需精確比對', () => {
    const callback = vi.fn();
    mountShortcut({ key: SPECIAL_KEYS.F1 }, callback);
    press({ key: 'F1' });
    press({ key: 'f1' });
    expect(callback).toHaveBeenCalledTimes(1);
  });

  it('在輸入框中不觸發（ignoreInput 預設 true）', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+o', callback);
    const input = document.createElement('input');
    document.body.appendChild(input);
    input.dispatchEvent(new KeyboardEvent('keydown', { key: 'o', ctrlKey: true, bubbles: true }));
    expect(callback).not.toHaveBeenCalled();
  });

  it('ignoreInput: false 時輸入框也會觸發', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+o', callback, { ignoreInput: false });
    const input = document.createElement('input');
    document.body.appendChild(input);
    input.dispatchEvent(new KeyboardEvent('keydown', { key: 'o', ctrlKey: true, bubbles: true }));
    expect(callback).toHaveBeenCalledTimes(1);
  });

  it('target 指定元素之外不觸發', () => {
    const callback = vi.fn();
    const target = document.createElement('button');
    target.id = 'only-here';
    document.body.appendChild(target);
    mountShortcut({ key: 'o', ctrl: true }, callback, { target: '#only-here' });
    press({ key: 'o', ctrlKey: true });
    expect(callback).not.toHaveBeenCalled();
    target.dispatchEvent(new KeyboardEvent('keydown', { key: 'o', ctrlKey: true, bubbles: true }));
    expect(callback).toHaveBeenCalledTimes(1);
  });

  it('preventDefault: false 時不阻擋預設行為', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+o', callback, { preventDefault: false });
    const event = new KeyboardEvent('keydown', { key: 'o', ctrlKey: true, cancelable: true });
    document.dispatchEvent(event);
    expect(callback).toHaveBeenCalledTimes(1);
    expect(event.defaultPrevented).toBe(false);
  });

  it('cleanup 移除 keydown 監聽', () => {
    const callback = vi.fn();
    let api: ReturnType<typeof useKeyboardShortcut> | undefined;
    mount(
      defineComponent({
        setup() {
          api = useKeyboardShortcut('ctrl+o', callback);
          return () => h('div');
        },
      }),
    );
    api!.cleanup();
    press({ key: 'o', ctrlKey: true });
    expect(callback).not.toHaveBeenCalled();
  });

  it('isArrowKey 判斷方向鍵', () => {
    expect(isArrowKey('ArrowUp')).toBe(true);
    expect(isArrowKey('ArrowDown')).toBe(true);
    expect(isArrowKey('a')).toBe(false);
  });

  it('空 key 的字串簡寫不會觸發', () => {
    const callback = vi.fn();
    mountShortcut('ctrl+', callback);
    press({ key: 'o', ctrlKey: true });
    expect(callback).not.toHaveBeenCalled();
  });
});
