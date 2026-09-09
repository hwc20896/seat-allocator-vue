import { describe, expect, it } from 'vitest';
import * as XLSX from '@e965/xlsx';
import { BUDDY_CSV_TEMPLATE, parseBuddyCSV, parseBuddyRows, readBuddyFile } from './buddyCSV';

describe('parseBuddyCSV', () => {
  it('解析 canonical 格式', () => {
    const r = parseBuddyCSV('A組, 1, 2, 3\nB組, 4, 5');
    expect(r).toEqual({
      ok: true,
      buddyGroups: [
        ['1', '2', '3'],
        ['4', '5'],
      ],
    });
  });

  it('標籤接受 A類／A 且 B 列在前也能正確對應', () => {
    const r = parseBuddyCSV('B類, 6, 7\nA, 1, 2');
    expect(r).toEqual({
      ok: true,
      buddyGroups: [
        ['1', '2'],
        ['6', '7'],
      ],
    });
  });

  it('吸收 BOM、CRLF、全形逗號、分號、Tab 與空格', () => {
    const r = parseBuddyCSV('\uFEFFA組，1；2\t3\r\nB組, 4, 5');
    expect(r).toEqual({
      ok: true,
      buddyGroups: [
        ['1', '2', '3'],
        ['4', '5'],
      ],
    });
  });

  it('支援引號欄位與 "" 跳脫', () => {
    const r = parseBuddyCSV('A組, "王,小明", "李""哥"\nB組, 6');
    expect(r).toEqual({ ok: true, buddyGroups: [['王,小明', '李"哥'], ['6']] });
  });

  it('跳過全空行並忽略尾逗號', () => {
    const r = parseBuddyCSV('\n\nA組, 1, 2,\n\nB組, 3\n');
    expect(r).toEqual({ ok: true, buddyGroups: [['1', '2'], ['3']] });
  });

  it('空檔案回傳錯誤並附範本', () => {
    const r = parseBuddyCSV('');
    expect(r).toEqual({
      ok: false,
      error: expect.stringContaining(BUDDY_CSV_TEMPLATE),
    });
  });

  it('超過兩列回傳錯誤', () => {
    const r = parseBuddyCSV('A組, 1\nB組, 2\nC組, 3');
    expect(r.ok).toBe(false);
  });

  it('兩列同組回傳錯誤', () => {
    const r = parseBuddyCSV('A組, 1\nA類, 2');
    expect(r.ok).toBe(false);
  });

  it('未知標籤回傳錯誤', () => {
    const r = parseBuddyCSV('甲組, 1\nB組, 2');
    expect(r.ok).toBe(false);
  });

  it('跨組重複姓名回傳錯誤', () => {
    const r = parseBuddyCSV('A組, 小明\nB組, 小明');
    expect(r.ok).toBe(false);
    expect(r).toEqual({
      ok: false,
      error: expect.stringContaining('小明'),
    });
  });

  it('任一組為空回傳錯誤', () => {
    const r = parseBuddyCSV('A組\nB組, 1');
    expect(r.ok).toBe(false);
  });
});

const makeXLSXFile = (rows: unknown[][]): File => {
  const ws = XLSX.utils.aoa_to_sheet(rows);
  const wb = XLSX.utils.book_new();
  XLSX.utils.book_append_sheet(wb, ws, 'Sheet1');
  const buffer = XLSX.write(wb, { bookType: 'xlsx', type: 'array' }) as ArrayBuffer;
  return new File([buffer], 'buddies.xlsx');
};

describe('parseBuddyRows', () => {
  it('trim 每格並跳過全空列（含 XLSX 的稀疏陣列）', () => {
    const r = parseBuddyRows([
      ['A組', ' 1 ', null, '2'],
      [null, null],
      ['B組', '3'],
    ]);
    expect(r).toEqual({ ok: true, buddyGroups: [['1', '2'], ['3']] });
  });

  it('數字等非字串 cell 會轉成字串', () => {
    const r = parseBuddyRows([
      ['A組', 101, 102],
      ['B組', 201],
    ]);
    expect(r).toEqual({ ok: true, buddyGroups: [['101', '102'], ['201']] });
  });

  it('語義檢查與 CSV 共用（跨組重複仍報錯）', () => {
    const r = parseBuddyRows([
      ['A組', '小明'],
      ['B組', '小明'],
    ]);
    expect(r.ok).toBe(false);
  });
});

describe('readBuddyFile', () => {
  it('XLSX 與 CSV 同形狀：第一欄標籤、A、B 各一列', async () => {
    const r = await readBuddyFile(
      makeXLSXFile([
        ['A組', '1', '2'],
        ['B組', '3'],
      ]),
    );
    expect(r).toEqual({ ok: true, buddyGroups: [['1', '2'], ['3']] });
  });

  it('XLSX 取第一個工作表，忽略其餘', async () => {
    const ws = XLSX.utils.aoa_to_sheet([
      ['A組', '1'],
      ['B組', '2'],
    ]);
    const wb = XLSX.utils.book_new();
    XLSX.utils.book_append_sheet(wb, ws, '第一頁');
    XLSX.utils.book_append_sheet(wb, XLSX.utils.aoa_to_sheet([['垃圾']]), '第二頁');
    const buffer = XLSX.write(wb, { bookType: 'xlsx', type: 'array' }) as ArrayBuffer;
    const r = await readBuddyFile(new File([buffer], 'buddies.xlsx'));
    expect(r).toEqual({ ok: true, buddyGroups: [['1'], ['2']] });
  });

  it('.csv 副檔名走文字解析', async () => {
    const r = await readBuddyFile(new File(['A組, 1\nB組, 2'], 'buddies.csv'));
    expect(r).toEqual({ ok: true, buddyGroups: [['1'], ['2']] });
  });

  it('損壞的 XLSX 回傳錯誤而非拋出', async () => {
    const r = await readBuddyFile(new File([new Uint8Array([1, 2, 3, 4])], 'bad.xlsx'));
    expect(r.ok).toBe(false);
  });

  it('空工作表回傳「沒有任何內容」錯誤並附範本', async () => {
    const r = await readBuddyFile(makeXLSXFile([]));
    expect(r).toEqual({
      ok: false,
      error: expect.stringContaining(BUDDY_CSV_TEMPLATE),
    });
  });
});
