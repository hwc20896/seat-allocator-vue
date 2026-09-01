import { afterEach, describe, expect, it, vi } from 'vitest';
import { mount, type VueWrapper } from '@vue/test-utils';
import type { FeasibilityReport } from '@/assets/wasm/alloc_algo';
import ConstraintsEditor from './ConstraintsEditor.vue';

const mounted: VueWrapper[] = [];

const mountEditor = (overrides: Record<string, unknown> = {}) => {
  const wrapper = mount(ConstraintsEditor, {
    props: { visible: false, initialConfig: '{}', ...overrides },
  });
  mounted.push(wrapper);
  return wrapper;
};

describe('ConstraintsEditor', () => {
  afterEach(() => {
    mounted.splice(0).forEach((w) => w.unmount());
    document.body.innerHTML = '';
  });

  it('visible=false 時不渲染彈窗', () => {
    const wrapper = mountEditor();
    expect(wrapper.find('.modal-overlay').exists()).toBe(false);
  });

  it('visible=true 時渲染三大區塊與按鈕', () => {
    const wrapper = mountEditor({ visible: true });
    expect(wrapper.text()).toContain('全域設定');
    expect(wrapper.text()).toContain('禁止配對');
    expect(wrapper.text()).toContain('位置約束');
    expect(wrapper.find('.apply-btn').exists()).toBe(true);
    expect(wrapper.find('.cancel-btn').exists()).toBe(true);
    expect(wrapper.find('.export-btn').exists()).toBe(true);
  });

  it('從 initialConfig 初始化開關、配對與約束', () => {
    const wrapper = mountEditor({
      visible: true,
      initialConfig: JSON.stringify({
        allowFixedPoints: true,
        allowOriginalNeighbors: true,
        customForbiddenPairs: [['張三', '李四']],
        constraints: [
          { type: 'forcerow', name: '王小明', rowIdx: 2 },
          { type: 'FORBIDSHARECOL', name1: 'A', name2: 'B' },
        ],
      }),
    });

    const checkboxes = wrapper.findAll('.switch-row input[type="checkbox"]');
    expect((checkboxes[0]!.element as HTMLInputElement).checked).toBe(true);
    expect((checkboxes[1]!.element as HTMLInputElement).checked).toBe(true);
    expect((checkboxes[2]!.element as HTMLInputElement).checked).toBe(false);

    expect(wrapper.findAll('.pair-row')).toHaveLength(1);
    expect(wrapper.findAll('.constraint-row')).toHaveLength(2);

    const selects = wrapper.findAll('.type-select');
    expect((selects[0]!.element as HTMLSelectElement).value).toBe('FORCEROW');
    expect((selects[1]!.element as HTMLSelectElement).value).toBe('FORBIDSHARECOL');
  });

  it('斜對角視為相鄰有獨立性 hover 說明', () => {
    const wrapper = mountEditor({ visible: true });
    const span = wrapper.findAll('.switch-row')[2]!.find('span');
    const title = span!.attributes('title') ?? '';
    expect(title).toContain('獨立於「允許與原本的鄰座相鄰」');
    expect(title).toContain('斜對角');
    expect(title).toContain('禁止配對');
  });

  it('新增與刪除禁止配對', async () => {
    const wrapper = mountEditor({ visible: true });
    expect(wrapper.findAll('.pair-row')).toHaveLength(0);

    await wrapper.find('.add-pair').trigger('click');
    expect(wrapper.findAll('.pair-row')).toHaveLength(1);

    await wrapper.find('.remove-pair').trigger('click');
    expect(wrapper.findAll('.pair-row')).toHaveLength(0);
  });

  it('新增與刪除位置約束', async () => {
    const wrapper = mountEditor({ visible: true });
    expect(wrapper.findAll('.constraint-row')).toHaveLength(0);

    await wrapper.find('.add-constraint').trigger('click');
    expect(wrapper.findAll('.constraint-row')).toHaveLength(1);

    await wrapper.find('.remove-constraint').trigger('click');
    expect(wrapper.findAll('.constraint-row')).toHaveLength(0);
  });

  it('切換約束類型時欄位隨之切換', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.add-constraint').trigger('click');

    // 預設 FORCEROW：姓名 + 行索引（1-based）
    expect(wrapper.findAll('.constraint-row input')).toHaveLength(2);
    expect(wrapper.find('.index-input').attributes('placeholder')).toBe('行索引');

    // 切到 FORBIDSHAREROW：兩個姓名輸入框
    const select = wrapper.find('.type-select');
    await select.setValue('FORBIDSHAREROW');
    expect(wrapper.findAll('.constraint-row input')).toHaveLength(2);
    expect(wrapper.find('.index-input').exists()).toBe(false);

    // 切到 FORCECOL：姓名 + 列索引
    await select.setValue('FORCECOL');
    expect(wrapper.find('.index-input').exists()).toBe(true);
    expect(wrapper.find('.index-input').attributes('placeholder')).toBe('列索引');
  });

  it('套用時 emit 表單內容的 JSON', async () => {
    const wrapper = mountEditor({ visible: true });

    await wrapper.find('.add-pair').trigger('click');
    const pairInputs = wrapper.findAll('.pair-row input');
    await pairInputs[0]!.setValue('張三');
    await pairInputs[1]!.setValue('李四');

    await wrapper.find('.add-constraint').trigger('click');
    const constraintInputs = wrapper.findAll('.constraint-row input');
    await constraintInputs[0]!.setValue('王小明');
    // 介面輸入 1-based：3 = 第 3 行 → JSON 轉回 0-based rowIdx 2
    await constraintInputs[1]!.setValue('3');

    await wrapper.find('.apply-btn').trigger('click');

    const json = wrapper.emitted('apply')![0]![0] as string;
    expect(JSON.parse(json)).toEqual({
      allowFixedPoints: true,
      allowOriginalNeighbors: true,
      diagonalsAreNeighbors: false,
      customForbiddenPairs: [['張三', '李四']],
      constraints: [{ type: 'FORCEROW', name: '王小明', rowIdx: 2 }],
    });
  });

  it('從 initialConfig 的 0-based 索引轉為介面 1-based 顯示', () => {
    const wrapper = mountEditor({
      visible: true,
      initialConfig: JSON.stringify({
        constraints: [
          { type: 'FORCEROW', name: '王小明', rowIdx: 2 },
          { type: 'FORBIDCOL', name: '陳大文', colIdx: 0 },
        ],
      }),
    });
    const values = wrapper
      .findAll('.index-input')
      .map((i) => (i.element as HTMLInputElement).value);
    expect(values).toEqual(['3', '1']);
  });

  it('介面 1-based 索引在套用時轉回 0-based JSON', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.add-constraint').trigger('click');
    await wrapper.find('.index-input').setValue('1');
    await wrapper.find('.apply-btn').trigger('click');
    const json = wrapper.emitted('apply')![0]![0] as string;
    expect(JSON.parse(json).constraints).toEqual([{ type: 'FORCEROW', name: '', rowIdx: 0 }]);
  });

  it('約束類型選單使用行/列定義的標籤', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.add-constraint').trigger('click');
    const labels = wrapper.findAll('.type-select option').map((o) => o.text());
    expect(labels).toEqual(['固定行', '禁止行', '固定列', '禁止列', '禁止同行', '禁止同列']);
  });

  it('座標說明預設隱藏，點 ? 切換顯示／隱藏', async () => {
    const wrapper = mountEditor({ visible: true });
    expect(wrapper.find('.guide').exists()).toBe(false);

    await wrapper.find('.note-toggle').trigger('click');
    expect(wrapper.find('.guide').exists()).toBe(true);
    expect(wrapper.find('.note-toggle').attributes('aria-expanded')).toBe('true');

    await wrapper.find('.note-toggle').trigger('click');
    expect(wrapper.find('.guide').exists()).toBe(false);
    expect(wrapper.find('.note-toggle').attributes('aria-expanded')).toBe('false');
  });

  it('顯示座標說明（黑板在前、行/列定義、索引從 1 開始）', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.note-toggle').trigger('click');
    const text = wrapper.text();
    expect(text).toContain('座標說明');
    expect(text).toContain('黑板（前方）');
    expect(text).toContain('第 1 行（最前面，靠黑板）');
    expect(text).toContain('第 1 列');
    expect(text).toContain('索引一律從 1 開始');
  });

  it('重開彈窗時座標說明重置為關閉', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.note-toggle').trigger('click');
    expect(wrapper.find('.guide').exists()).toBe(true);

    await wrapper.setProps({ visible: false });
    await wrapper.setProps({ visible: true });
    expect(wrapper.find('.guide').exists()).toBe(false);
  });

  it('傳入 names 時姓名輸入框有 datalist 補全', async () => {
    const wrapper = mountEditor({ visible: true, names: ['張三', '李四', '王小明'] });
    const options = wrapper.findAll('#constraint-names option');
    expect(options.map((o) => o.attributes('value'))).toEqual(['張三', '李四', '王小明']);

    // 禁止配對 2 個 + 位置約束 1 個，共 3 個姓名輸入框
    await wrapper.find('.add-pair').trigger('click');
    await wrapper.find('.add-constraint').trigger('click');
    const inputs = wrapper.findAll('.name-input');
    expect(inputs).toHaveLength(3);
    expect(inputs.every((i) => i.attributes('list') === 'constraint-names')).toBe(true);
  });

  it('重設前需確認：取消時不清空', async () => {
    const confirmSpy = vi.spyOn(window, 'confirm').mockReturnValue(false);
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.add-constraint').trigger('click');
    expect(wrapper.findAll('.constraint-row')).toHaveLength(1);

    const resetBtn = wrapper.findAll('.modal-footer button').find((b) => b.text() === '重設')!;
    await resetBtn.trigger('click');
    expect(confirmSpy).toHaveBeenCalledTimes(1);
    expect(wrapper.findAll('.constraint-row')).toHaveLength(1);
  });

  it('重設前需確認：確認後清空', async () => {
    vi.spyOn(window, 'confirm').mockReturnValue(true);
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.add-constraint').trigger('click');
    expect(wrapper.findAll('.constraint-row')).toHaveLength(1);

    const resetBtn = wrapper.findAll('.modal-footer button').find((b) => b.text() === '重設')!;
    await resetBtn.trigger('click');
    expect(wrapper.findAll('.constraint-row')).toHaveLength(0);
  });

  it('測試現有約束：可行時顯示成功訊息', async () => {
    const checkFeasibility = vi.fn<() => FeasibilityReport | null>(() => ({
      status: 0,
      layer: 'ok',
      reason: '',
    }));
    const wrapper = mountEditor({ visible: true, checkFeasibility });
    const testBtn = wrapper
      .findAll('.modal-footer button')
      .find((b) => b.text() === '測試現有約束')!;
    await testBtn.trigger('click');
    expect(checkFeasibility).toHaveBeenCalledTimes(1);
    expect(wrapper.find('.test-result').text()).toContain('約束可滿足');
    expect(wrapper.find('.test-result').classes()).toContain('test-ok');
  });

  it('測試現有約束：無解時顯示原因與判定層', async () => {
    const checkFeasibility = vi.fn<() => FeasibilityReport | null>(() => ({
      status: 1,
      layer: 'domain',
      reason: "元素 'A' 的可放位置不足",
    }));
    const wrapper = mountEditor({ visible: true, checkFeasibility });
    const testBtn = wrapper
      .findAll('.modal-footer button')
      .find((b) => b.text() === '測試現有約束')!;
    await testBtn.trigger('click');
    const text = wrapper.find('.test-result').text();
    expect(text).toContain('約束無法同時滿足');
    expect(text).toContain('可行域檢查');
    expect(text).toContain("元素 'A' 的可放位置不足");
    expect(wrapper.find('.test-result').classes()).toContain('test-unsatisfiable');
  });

  it('測試現有約束：無法判定時提示放寬', async () => {
    const checkFeasibility = vi.fn<() => FeasibilityReport | null>(() => ({
      status: 2,
      layer: 'coloring',
      reason: '超預算',
    }));
    const wrapper = mountEditor({ visible: true, checkFeasibility });
    const testBtn = wrapper
      .findAll('.modal-footer button')
      .find((b) => b.text() === '測試現有約束')!;
    await testBtn.trigger('click');
    expect(wrapper.find('.test-result').text()).toContain('無法在預算內判定');
    expect(wrapper.find('.test-result').classes()).toContain('test-unknown');
  });

  it('測試現有約束：未導入座位排佈時提示', async () => {
    const wrapper = mountEditor({ visible: true, checkFeasibility: vi.fn<() => null>(() => null) });
    const testBtn = wrapper
      .findAll('.modal-footer button')
      .find((b) => b.text() === '測試現有約束')!;
    await testBtn.trigger('click');
    expect(wrapper.find('.test-result').text()).toContain('尚未導入座位排佈');
    expect(wrapper.find('.test-result').classes()).toContain('test-no-grid');
  });

  it('取消按鈕 emit cancel', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.cancel-btn').trigger('click');
    expect(wrapper.emitted('cancel')).toHaveLength(1);
  });

  it('點擊遮罩 emit cancel', async () => {
    const wrapper = mountEditor({ visible: true });
    await wrapper.find('.modal-overlay').trigger('click');
    expect(wrapper.emitted('cancel')).toHaveLength(1);
  });

  it('Escape 快捷鍵在開啟時 emit cancel', () => {
    const wrapper = mountEditor({ visible: true });
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', cancelable: true }));
    expect(wrapper.emitted('cancel')).toHaveLength(1);
  });

  it('Escape 快捷鍵在關閉時不 emit', () => {
    const wrapper = mountEditor();
    document.dispatchEvent(new KeyboardEvent('keydown', { key: 'Escape', cancelable: true }));
    expect(wrapper.emitted('cancel')).toBeUndefined();
  });
});
