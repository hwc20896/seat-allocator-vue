<template>
  <div v-if="visible" class="modal-overlay" @click.self="handleCancel">
    <div class="modal-panel" role="dialog" aria-modal="true" aria-label="約束管理">
      <header class="modal-header">
        <h2 class="modal-title">約束管理</h2>
        <button class="close-btn" aria-label="關閉" @click="handleCancel">✕</button>
      </header>

      <div class="modal-body">
        <!-- 座標說明 -->
        <section class="section">
          <div class="section-header">
            <h3 class="section-title">座標說明</h3>
            <button
              class="btn note-toggle"
              :class="{ active: gridNoteActive }"
              aria-label="顯示／隱藏座標說明"
              :aria-expanded="gridNoteActive"
              @click="toggleGridNote"
            >
              ?
            </button>
          </div>
          <div class="guide" v-if="gridNoteActive">
            <div class="guide-row guide-blackboard">⬆ 黑板（前方）</div>
            <div class="guide-row guide-front">第 1 行（最前面，靠黑板）</div>
            <div class="guide-row">第 2 行</div>
            <div class="guide-row guide-back">第 3 行</div>
            <div class="guide-cols">
              <span>第 1 列</span>
              <span>第 2 列</span>
              <span>第 3 列</span>
            </div>
            <p class="guide-note">行：從前到後數；列：從左到右數。索引一律從 1 開始。</p>
          </div>
        </section>
        <!-- 全域設定 -->
        <section class="section">
          <h3 class="section-title">全域設定</h3>
          <label class="switch-row">
            <input v-model="state.allowFixedPoints" type="checkbox" />
            <span>允許座位留在原位</span>
          </label>
          <label class="switch-row">
            <input v-model="state.allowOriginalNeighbors" type="checkbox" />
            <span>允許與原本的鄰座相鄰</span>
          </label>
          <label class="switch-row">
            <input v-model="state.diagonalsAreNeighbors" type="checkbox" />
            <span :title="DIAGONAL_NOTE">
              斜對角視為相鄰
              <span class="hover-hint">懸停查看說明</span>
            </span>
          </label>
        </section>

        <!-- 禁止配對 -->
        <section class="section">
          <div class="section-header">
            <h3 class="section-title">禁止配對</h3>
            <button class="add-btn add-pair" @click="addPair">＋ 新增配對</button>
          </div>
          <div v-for="(pair, index) in state.customForbiddenPairs" :key="index" class="pair-row">
            <input
              v-model="pair[0]"
              class="name-input"
              list="constraint-names"
              placeholder="姓名 A"
            />
            <span class="pair-sep">×</span>
            <input
              v-model="pair[1]"
              class="name-input"
              list="constraint-names"
              placeholder="姓名 B"
            />
            <button class="remove-btn remove-pair" aria-label="刪除配對" @click="removePair(index)">
              ✕
            </button>
          </div>
          <p v-if="state.customForbiddenPairs.length === 0" class="empty-hint">尚未設定禁止配對</p>
        </section>

        <!-- 位置約束 -->
        <section class="section">
          <div class="section-header">
            <h3 class="section-title">位置約束</h3>
            <button class="add-btn add-constraint" @click="addConstraint">＋ 新增約束</button>
          </div>
          <div v-for="(constraint, index) in state.constraints" :key="index" class="constraint-row">
            <select v-model="constraint.type" class="type-select">
              <option v-for="option in CONSTRAINT_TYPES" :key="option.type" :value="option.type">
                {{ option.label }}
              </option>
            </select>
            <template v-if="isPositionConstraint(constraint.type)">
              <input
                v-model="constraint.name"
                class="name-input"
                list="constraint-names"
                placeholder="姓名"
              />
              <input
                v-model.number="constraint.index"
                class="index-input"
                type="number"
                min="1"
                :placeholder="indexLabel(constraint.type)"
              />
            </template>
            <template v-else>
              <input
                v-model="constraint.name1"
                class="name-input"
                list="constraint-names"
                placeholder="姓名 1"
              />
              <input
                v-model="constraint.name2"
                class="name-input"
                list="constraint-names"
                placeholder="姓名 2"
              />
            </template>
            <button
              class="remove-btn remove-constraint"
              aria-label="刪除約束"
              @click="removeConstraint(index)"
            >
              ✕
            </button>
          </div>
          <p v-if="state.constraints.length === 0" class="empty-hint">尚未設定位置約束</p>
        </section>

        <!-- JSON 預覽 -->
        <details class="json-preview">
          <summary>JSON 預覽</summary>
          <textarea readonly class="json-textarea" :value="jsonPreview"></textarea>
          <p class="json-hint">
            註：JSON 中的 rowIdx / colIdx 為 0-based（從 0 開始），與介面顯示的 1-based 索引相差 1。
          </p>
        </details>

        <p v-if="testResult" class="test-result" :class="`test-${testResult.status}`">
          {{ testResult.message }}
        </p>
      </div>

      <datalist id="constraint-names">
        <option v-for="name in names" :key="name" :value="name" />
      </datalist>

      <footer class="modal-footer">
        <button class="btn test-btn" @click="handleTestConstraints">測試現有約束</button>
        <button class="btn export-btn" @click="exportJson">匯出 JSON</button>
        <button class="btn cancel-btn" @click="handleCancel">取消</button>
        <button class="btn reset-btn" @click="handleReset">重設</button>
        <button class="btn primary apply-btn" @click="handleApply">套用</button>
      </footer>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, reactive, ref, watch } from 'vue';
import type { FeasibilityReport } from '@/assets/wasm/alloc_algo';
import type { Constraint } from '@/utils/JSONTypes';
import { useKeyboardShortcut } from '@/composables/useKeyboardShortcuts';

const props = defineProps<{
  visible: boolean;
  initialConfig?: string;
  names?: string[];
  checkFeasibility?: (json: string) => FeasibilityReport | null;
}>();

const emit = defineEmits<{
  apply: [json: string];
  cancel: [];
}>();

/** 編輯器內部使用的約束模型：以統一 index 欄位代表 rowIdx / colIdx */
interface EditableConstraint {
  type: string;
  name?: string;
  name1?: string;
  name2?: string;
  index?: number;
}

interface EditorState {
  allowFixedPoints: boolean;
  allowOriginalNeighbors: boolean;
  diagonalsAreNeighbors: boolean;
  customForbiddenPairs: [string, string][];
  constraints: EditableConstraint[];
}

interface TestResult {
  status: 'ok' | 'unsatisfiable' | 'unknown' | 'no-grid';
  message: string;
}

const CONSTRAINT_TYPES = [
  { type: 'FORCEROW', label: '固定行' },
  { type: 'FORBIDROW', label: '禁止行' },
  { type: 'FORCECOL', label: '固定列' },
  { type: 'FORBIDCOL', label: '禁止列' },
  { type: 'FORBIDSHAREROW', label: '禁止同行' },
  { type: 'FORBIDSHARECOL', label: '禁止同列' },
] as const;

/** 「斜對角視為相鄰」的獨立性說明（懸停 tooltip） */
const DIAGONAL_NOTE =
  '此選項獨立於「允許與原本的鄰座相鄰」，並非其附屬開關。\n' +
  '它定義「相鄰」的範圍：僅上下左右，或連同四個斜對角位置。\n' +
  '影響：\n' +
  '  ①「允許與原本的鄰座相鄰」的鄰座判定（原本坐斜對角者也算鄰座）；\n' +
  '  ②「禁止配對」——被禁止配對的兩人坐在彼此的斜對角，同樣視為違反。';

const isPositionConstraint = (type: string): boolean =>
  type === 'FORCEROW' || type === 'FORBIDROW' || type === 'FORCECOL' || type === 'FORBIDCOL';

const indexLabel = (type: string): string =>
  type === 'FORCEROW' || type === 'FORBIDROW' ? '行索引' : '列索引';

const createEmptyState = (): EditorState => ({
  allowFixedPoints: true,
  allowOriginalNeighbors: true,
  diagonalsAreNeighbors: false,
  customForbiddenPairs: [],
  constraints: [],
});

const toEditable = (c: Constraint): EditableConstraint => {
  const type = String(c.type ?? '').toUpperCase();
  if (isPositionConstraint(type)) {
    const rowBased = type === 'FORCEROW' || type === 'FORBIDROW';
    const idx = rowBased ? c.rowIdx : c.colIdx;
    return { type, name: c.name, index: typeof idx === 'number' ? idx + 1 : undefined };
  }
  return { type, name1: c.name1, name2: c.name2 };
};

const toConstraint = (c: EditableConstraint): Constraint => {
  if (isPositionConstraint(c.type)) {
    const rowBased = c.type === 'FORCEROW' || c.type === 'FORBIDROW';
    const idx = typeof c.index === 'number' ? c.index - 1 : undefined;
    return rowBased
      ? { type: c.type, name: c.name, rowIdx: idx }
      : { type: c.type, name: c.name, colIdx: idx };
  }
  return { type: c.type, name1: c.name1, name2: c.name2 };
};

const parseConfig = (json: string): EditorState => {
  try {
    const obj = JSON.parse(json) as Record<string, unknown>;
    const rawConstraints = obj.constraints as unknown[];
    return {
      allowFixedPoints: typeof obj.allowFixedPoints === 'boolean' ? obj.allowFixedPoints : true,
      allowOriginalNeighbors:
        typeof obj.allowOriginalNeighbors === 'boolean' ? obj.allowOriginalNeighbors : true,
      diagonalsAreNeighbors:
        typeof obj.diagonalsAreNeighbors === 'boolean' ? obj.diagonalsAreNeighbors : false,
      customForbiddenPairs: Array.isArray(obj.customForbiddenPairs)
        ? obj.customForbiddenPairs
            .filter((p): p is [unknown, unknown] => Array.isArray(p) && p.length >= 2)
            .map((p) => [String(p[0]), String(p[1])] as [string, string])
        : [],
      constraints: Array.isArray(rawConstraints)
        ? rawConstraints
            .filter((c): c is Constraint => !!c && typeof c === 'object')
            .map(toEditable)
        : [],
    };
  } catch {
    return createEmptyState();
  }
};

const state = reactive<EditorState>(parseConfig(props.initialConfig ?? '{}'));
const testResult = ref<TestResult | null>(null);
const gridNoteActive = ref(false);

const toggleGridNote = () => {
  gridNoteActive.value = !gridNoteActive.value;
};

watch(
  () => props.visible,
  (visible) => {
    if (visible) {
      Object.assign(state, parseConfig(props.initialConfig ?? '{}'));
      testResult.value = null;
      gridNoteActive.value = false;
    }
  },
);

const toJson = (): string =>
  JSON.stringify(
    {
      allowFixedPoints: state.allowFixedPoints,
      allowOriginalNeighbors: state.allowOriginalNeighbors,
      diagonalsAreNeighbors: state.diagonalsAreNeighbors,
      customForbiddenPairs: state.customForbiddenPairs.map((pair) => [pair[0], pair[1]]),
      constraints: state.constraints.map(toConstraint),
    },
    null,
    2,
  );

const jsonPreview = computed(() => toJson());

const addPair = () => {
  state.customForbiddenPairs.push(['', '']);
};

const removePair = (index: number) => {
  state.customForbiddenPairs.splice(index, 1);
};

const addConstraint = () => {
  state.constraints.push({ type: 'FORCEROW', name: '', index: 1 });
};

const removeConstraint = (index: number) => {
  state.constraints.splice(index, 1);
};

const handleApply = () => {
  emit('apply', toJson());
};

const handleCancel = () => {
  emit('cancel');
};

const handleReset = () => {
  if (!window.confirm('確定要清空所有約束設定嗎？此操作無法復原。')) return;
  testResult.value = null;
  Object.assign(state, createEmptyState());
};

const LAYER_LABELS: Record<string, string> = {
  domain: '可行域檢查',
  matching: '完美匹配檢查',
  coloring: '著色檢查',
  ok: '',
};

const handleTestConstraints = () => {
  if (!props.checkFeasibility) return;

  const report = props.checkFeasibility(toJson());
  if (report === null) {
    testResult.value = { status: 'no-grid', message: '尚未導入座位排佈，無法測試約束。' };
    return;
  }

  const layer = typeof report.layer === 'string' ? report.layer : '';

  if (report.status === 0) {
    testResult.value = { status: 'ok', message: '約束可滿足，存在可行解。' };
  } else if (report.status === 1) {
    const layerNote = LAYER_LABELS[layer] ? `（${LAYER_LABELS[layer]}）` : '';
    testResult.value = {
      status: 'unsatisfiable',
      message: `約束無法同時滿足${layerNote}：${report.reason}`,
    };
  } else {
    testResult.value = {
      status: 'unknown',
      message: `無法在預算內判定，建議放寬約束後再試。${report.reason}`,
    };
  }
};

const exportJson = () => {
  const blob = new Blob([toJson()], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = 'constraints.json';
  anchor.click();
  URL.revokeObjectURL(url);
};

// Escape 關閉彈窗（輸入框中不觸發，避免干擾打字）
useKeyboardShortcut({ key: 'Escape' }, () => {
  if (props.visible) emit('cancel');
});
</script>

<style scoped>
.modal-overlay {
  position: fixed;
  inset: 0;
  z-index: 1000;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  background: rgba(15, 23, 42, 0.45);
  backdrop-filter: blur(4px);
  animation: fadeIn var(--transition-fast);
}

.modal-panel {
  width: 580px;
  max-width: 100%;
  max-height: 85vh;
  display: flex;
  flex-direction: column;
  background: var(--bg-panel);
  border-radius: 14px;
  box-shadow: var(--shadow-xl);
  overflow: hidden;
  animation: slideUp var(--transition-normal);
}

.modal-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px 20px;
  border-bottom: 1px solid var(--border-light);
  background: linear-gradient(to bottom, #ffffff 0%, var(--bg-page) 100%);
}

.modal-title {
  margin: 0;
  font-size: 17px;
  font-weight: 700;
  color: var(--text-dark);
}

.close-btn {
  border: none;
  background: none;
  font-size: 14px;
  color: var(--text-muted);
  cursor: pointer;
  width: 28px;
  height: 28px;
  border-radius: 8px;
  transition: var(--transition-fast);
}

.close-btn:hover {
  background: var(--bg-control-hover);
  color: var(--text-dark);
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 16px 20px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.section {
  display: flex;
  flex-direction: column;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.section-title {
  margin: 0;
  font-size: 13px;
  font-weight: 600;
  color: var(--text-muted);
  letter-spacing: 0.5px;
}

.switch-row {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 13px;
  color: var(--text-main);
  cursor: pointer;
  padding: 6px 8px;
  border-radius: 8px;
  transition: var(--transition-fast);
}

.switch-row:hover {
  background: var(--bg-control-hover);
}

.guide {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 10px 12px;
  margin: 10px;
  border: 1px solid var(--border-light);
  border-radius: 10px;
  background: var(--bg-page);
  font-size: 12px;
}

.guide-row {
  padding: 2px 8px;
  border-radius: 6px;
  color: var(--text-muted);
}

.guide-blackboard {
  background: #e0e7ff;
  color: var(--primary);
  font-weight: bold;
  text-align: center;
}

.guide-front {
  background: #f1f5f9;
  color: var(--text-dark);
  font-weight: 600;
}

.guide-back {
  background: #f8fafc;
  color: var(--text-light);
}

.guide-cols {
  display: flex;
  justify-content: space-around;
  padding: 2px 8px;
  color: var(--text-muted);
  border-top: 1px dashed var(--border-light);
}

.guide-note {
  margin: 4px 0 0;
  color: var(--text-light);
  font-size: 11px;
}

.switch-row input[type='checkbox'] {
  accent-color: var(--primary);
  width: 16px;
  height: 16px;
  cursor: pointer;
}

.hover-hint {
  margin-left: 30px;
  font-size: 11px;
  color: var(--text-light);
}

.pair-row,
.constraint-row {
  display: flex;
  align-items: center;
  gap: 8px;
}

.name-input,
.index-input,
.type-select {
  border: 1px solid var(--border-medium);
  border-radius: 8px;
  padding: 7px 10px;
  font-size: 13px;
  color: var(--text-main);
  background: #ffffff;
  outline: none;
  transition:
    border-color var(--transition-fast),
    box-shadow var(--transition-fast);
}

.name-input:focus,
.index-input:focus,
.type-select:focus {
  border-color: var(--primary);
  box-shadow: 0 0 0 3px var(--primary-glow);
}

.name-input {
  flex: 1;
  min-width: 0;
}

.index-input {
  width: 72px;
}

.type-select {
  width: 130px;
}

.pair-sep {
  color: var(--text-light);
  font-size: 13px;
  flex-shrink: 0;
}

.remove-btn {
  border: none;
  background: none;
  color: var(--text-light);
  cursor: pointer;
  width: 28px;
  height: 28px;
  border-radius: 8px;
  font-size: 12px;
  flex-shrink: 0;
  transition: var(--transition-fast);
}

.remove-btn:hover {
  background: #fee2e2;
  color: #dc2626;
}

.btn.reset-btn {
  border-color: #fca5a5;
  color: #dc2626;
}

.btn.reset-btn:hover {
  border-color: #dc2626;
  background: #fee2e2;
  color: #dc2626;
}

.btn.test-btn {
  border-color: #65a30d;
  color: #155724;
}

.btn.test-btn:hover {
  border-color: #155724;
  background: #dcfce7;
  color: #155724;
}

/* 座標說明切換鈕：圓形小按鈕，開啟時高亮 */
.btn.note-toggle {
  width: 26px;
  height: 26px;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  font-size: 13px;
  font-weight: 700;
  line-height: 1;
  border-color: var(--border-medium);
  color: var(--text-muted);
}

.btn.note-toggle:hover,
.btn.note-toggle.active {
  border-color: var(--primary);
  color: var(--primary);
  background: var(--primary-light);
}

.add-btn {
  border: 1px dashed var(--border-medium);
  background: none;
  color: var(--primary);
  font-size: 12px;
  font-weight: 600;
  padding: 5px 12px;
  border-radius: 8px;
  cursor: pointer;
  transition: var(--transition-fast);
}

.add-btn:hover {
  border-color: var(--primary);
  background: var(--primary-light);
}

.empty-hint {
  margin: 0;
  font-size: 12px;
  color: var(--text-light);
}

.json-preview summary {
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  color: var(--text-muted);
  user-select: none;
}

.json-textarea {
  width: 100%;
  height: 140px;
  margin-top: 8px;
  resize: vertical;
  font-family: 'Cascadia Code', Consolas, 'Courier New', monospace;
  font-size: 12px;
  line-height: 1.5;
  color: var(--text-main);
  background: var(--bg-page);
  border: 1px solid var(--border-light);
  border-radius: 8px;
  padding: 10px;
  outline: none;
}

.json-hint {
  margin: 6px 0 0;
  font-size: 11px;
  color: var(--text-light);
}

.test-result {
  margin: 0;
  padding: 8px 12px;
  border-radius: 8px;
  font-size: 12px;
  font-weight: 600;
  line-height: 1.5;
}

.test-ok {
  background: #dcfce7;
  color: #15803d;
}

.test-unsatisfiable {
  background: #fee2e2;
  color: #b91c1c;
}

.test-unknown {
  background: #fef9c3;
  color: #a16207;
}

.test-no-grid {
  background: #f1f5f9;
  color: var(--text-muted);
}

.modal-footer {
  display: flex;
  justify-content: flex-start;
  gap: 10px;
  padding: 14px 20px;
  border-top: 1px solid var(--border-light);
  background: var(--bg-page);
}

.modal-footer .export-btn {
  margin-right: auto;
}

.btn {
  border: 1px solid var(--border-medium);
  background: #ffffff;
  color: var(--text-main);
  font-size: 13px;
  font-weight: 600;
  padding: 8px 18px;
  border-radius: 8px;
  cursor: pointer;
  transition: var(--transition-fast);
}

.btn:hover {
  border-color: var(--primary);
  color: var(--primary);
}

.btn.primary {
  border: none;
  background: linear-gradient(135deg, var(--primary) 0%, #8b5cf6 100%);
  color: #ffffff;
}

.btn.primary:hover {
  filter: brightness(1.08);
  transform: translateY(-1px);
  box-shadow: var(--shadow-md);
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(12px) scale(0.98);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}
</style>
