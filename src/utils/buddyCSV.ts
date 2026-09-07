import * as XLSX from '@e965/xlsx';

export const BUDDY_CSV_TEMPLATE = 'A組, 姓名1, 姓名2, …\nB組, 姓名1, 姓名2, …';

export interface BuddyCSVSuccess {
  ok: true;
  buddyGroups: [string[], string[]];
}

export interface BuddyCSVFailure {
  ok: false;
  error: string;
}

export type BuddyCsvResult = BuddyCSVSuccess | BuddyCSVFailure;

/** 與 C++ Grid::fromCSVString 同語法：支援引號、"" 跳脫、CRLF/LF、跳過全空行 */
const parseCSVRows = (text: string): string[][] => {
  const rows: string[][] = [];
  let row: string[] = [];
  let field = '';
  let inQuotes = false;
  let i = 0;
  const pushField = () => {
    row.push(field);
    field = '';
  };
  const endRow = () => {
    if (row.length > 0 || field !== '') {
      row.push(field);
      rows.push(row);
    }
    row = [];
    field = '';
  };

  while (i < text.length) {
    const c = text[i];
    if (inQuotes) {
      if (c === '"') {
        if (text[i + 1] === '"') {
          field += '"';
          i += 2;
        } else {
          inQuotes = false;
          i++;
        }
      } else {
        field += c;
        i++;
      }
    } else if (c === '"') {
      inQuotes = true;
      i++;
    } else if (c === ',' || c === '，' || c === ';' || c === '；' || c === '\t') {
      pushField();
      i++;
    } else if (c === '\r' || c === '\n') {
      if (c === '\r' && text[i + 1] === '\n') i += 2;
      else i++;
      if (row.length === 0 && field === '') continue;
      endRow();
    } else {
      field += c;
      i++;
    }
  }
  if (row.length > 0 || field !== '') endRow();
  return rows;
};

const toGroupIndex = (label: string): 0 | 1 | undefined => {
  const t = label.toUpperCase();
  if (t === 'A' || t === 'A類' || t === 'A組') return 0;
  if (t === 'B' || t === 'B類' || t === 'B組') return 1;
  return undefined;
};

const failWithTemplate = (message: string): BuddyCSVFailure => ({
  ok: false,
  error: `${message}\n\n預期格式（第一欄為組別，A、B 各一列）：\n${BUDDY_CSV_TEMPLATE}`,
});

/** 語義核心：接受任何來源（CSV 或 XLSX）的二維陣列，產出 A／B 兩組名單 */
export const parseBuddyRows = (rawRows: unknown[][]): BuddyCsvResult => {
  const rows = rawRows
    .map((row) =>
      row.map((cell) => (cell === null || cell === undefined ? '' : String(cell).trim())),
    )
    .filter((row) => row.some((cell) => cell !== ''));

  if (rows.length === 0) return failWithTemplate('檔案沒有任何內容。');
  if (rows.length !== 2)
    return failWithTemplate(`預期恰好 2 列（A 組一列、B 組一列），實際 ${rows.length} 列。`);

  const groups: [string[], string[]] = [[], []];
  const seen = new Set<0 | 1>();

  for (const [rowIndex, row] of rows.entries()) {
    const label = row[0] ?? '';
    const group = toGroupIndex(label);
    if (group === undefined)
      return failWithTemplate(
        `第 ${rowIndex + 1} 列的第一欄「${label}」無法辨識。第一欄須為 A組／A類／A 或 B組／B類／B。`,
      );
    if (seen.has(group))
      return failWithTemplate(
        `第 ${rowIndex + 1} 列與另一列同為 ${group === 0 ? 'A' : 'B'} 組；A、B 各需一列。`,
      );
    seen.add(group);
    groups[group] = row.slice(1).filter((name) => name !== '');
  }

  if (groups[0].length === 0 || groups[1].length === 0)
    return failWithTemplate('A、B 兩組都至少需要一名成員。');

  const overlap = groups[0].filter((name) => groups[1].includes(name));
  if (overlap.length > 0)
    return failWithTemplate(
      `以下姓名同時出現在 A、B 兩組，每人只能歸屬一組：${overlap.join('、')}。`,
    );

  return { ok: true, buddyGroups: groups };
};

/** CSV 文字入口：語法層（parseCSVRows）＋語義層（parseBuddyRows） */
export const parseBuddyCSV = (text: string): BuddyCsvResult =>
  parseBuddyRows(parseCSVRows(text.replace(/^\uFEFF/, '')));

/** XLSX 入口：取第一個工作表轉二維陣列後進語義層；任何失敗都回傳錯誤而非拋出 */
export const parseBuddyXLSX = (file: File): Promise<BuddyCsvResult> =>
  new Promise((resolve) => {
    const reader = new FileReader();
    reader.onload = () => {
      try {
        const wb = XLSX.read(reader.result as ArrayBuffer, { type: 'array' });
        const firstSheetName = wb.SheetNames[0];
        const ws = firstSheetName ? wb.Sheets[firstSheetName] : undefined;
        const raw: unknown[][] = ws ? XLSX.utils.sheet_to_json(ws, { header: 1, raw: false }) : [];
        resolve(parseBuddyRows(raw));
      } catch {
        resolve({ ok: false, error: '無法解析 XLSX 檔案，請確認是有效的 Excel 檔案。' });
      }
    };
    reader.onerror = () => resolve({ ok: false, error: '讀取檔案失敗，請確認檔案可讀取。' });
    reader.readAsArrayBuffer(file);
  });

/** 依副檔名分流：XLSX／XLS 走工作表解析，其餘視為 CSV 文字 */
export const readBuddyFile = (file: File): Promise<BuddyCsvResult> =>
  /\.(xlsx|xls)$/i.test(file.name)
    ? parseBuddyXLSX(file)
    : readFileAsText(file).then(parseBuddyCSV, () => ({
        ok: false,
        error: '讀取檔案失敗，請確認檔案可讀取。',
      }));

export const readFileAsText = (file: File): Promise<string> =>
  new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result as string);
    reader.onerror = () => reject(reader.error ?? new Error('讀檔失敗'));
    reader.readAsText(file);
  });
