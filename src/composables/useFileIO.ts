import * as XLSX from '@e965/xlsx';
import type { Grid, MainModule } from '@/assets/wasm/alloc_algo';
import type { ShallowRef } from 'vue';

export function useFileIO(wasmModule: ShallowRef<MainModule | null>) {
  const readTextFile = (file: File): Promise<string> => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(reader.result as string);
      reader.onerror = reject;
      reader.readAsText(file);
    });
  };

  const parseXLSX = (file: File): Promise<Grid> => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => {
        try {
          const data = reader.result as ArrayBuffer;
          const wb = XLSX.read(data, { type: 'array' });
          const firstSheetName = wb.SheetNames[0];
          if (!firstSheetName) return resolve(new wasmModule.value!.Grid());
          const ws = wb.Sheets[firstSheetName];
          if (!ws) return resolve(new wasmModule.value!.Grid());
          const raw: unknown[][] = XLSX.utils.sheet_to_json(ws, { header: 1, raw: false });

          const filteredRows = raw.filter(
            (row) =>
              row.length > 0 &&
              row.some((cell) => cell !== null && cell !== undefined && cell !== ''),
          );

          if (filteredRows.length === 0) return resolve(new wasmModule.value!.Grid());

          const rowCount = filteredRows.length;
          const colCount = Math.max(...filteredRows.map((row) => row.length));

          const flatData = new wasmModule.value!.StringVector();
          for (let r = 0; r < rowCount; r++) {
            for (let c = 0; c < colCount; c++) {
              const cell = filteredRows[r]?.[c];
              flatData.push_back(cell === null || cell === undefined ? '' : String(cell).trim());
            }
          }

          resolve(new wasmModule.value!.Grid(rowCount, colCount, flatData));
          flatData.delete();
        } catch (err) {
          reject(err);
        }
      };
      reader.onerror = reject;
      reader.readAsArrayBuffer(file);
    });
  };

  const generateXLSXBuffer = (grid: Grid): ArrayBuffer => {
    const rowCount = grid.rowCount();
    const colCount = grid.colCount();

    const rows: string[][] = [];
    for (let r = 0; r < rowCount; r++) {
      const row: string[] = [];
      for (let c = 0; c < colCount; c++) {
        row.push(grid.getByPos(r, c));
      }
      rows.push(row);
    }

    const ws = XLSX.utils.aoa_to_sheet(rows);
    const wb = XLSX.utils.book_new();
    XLSX.utils.book_append_sheet(wb, ws, 'Sheet1');
    return XLSX.write(wb, { bookType: 'xlsx', type: 'array' }) as ArrayBuffer;
  };

  return {
    readTextFile,
    parseXLSX,
    generateXLSXBuffer,
  };
}
