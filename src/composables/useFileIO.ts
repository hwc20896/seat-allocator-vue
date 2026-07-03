import * as XLSX from 'xlsx'
import type { Grid } from '@/assets/wasm/alloc_algo'

export function useFileIO() {
  const readTextFile = (file: File): Promise<string> => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader()
      reader.onload = () => resolve(reader.result as string)
      reader.onerror = reject
      reader.readAsText(file)
    })
  }

  const parseCSV = (text: string): Grid => {
    return text
      .split(/\r?\n/)
      .map((row) => row.split(',').map((cell) => cell.trim()))
      .filter((row) => row.length > 0 && row.some((cell) => cell !== ''))
  }

  const generateCSVContent = (grid: Grid): string => {
    return grid.map((row) => row.join(',')).join('\n')
  }

  const parseXLSX = (file: File): Promise<Grid> => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader()
      reader.onload = () => {
        try {
          const data = reader.result as ArrayBuffer
          const wb = XLSX.read(data, { type: 'array' })
          const firstSheetName = wb.SheetNames[0]
          if (!firstSheetName) return resolve([])
          const ws = wb.Sheets[firstSheetName]
          if (!ws) return resolve([])
          const raw: unknown[][] = XLSX.utils.sheet_to_json(ws, { header: 1, raw: false })
          const grid: Grid = raw
            .map((row) =>
              row.map((cell) => (cell === null || cell === undefined ? '' : String(cell).trim()))
            )
            .filter((row) => row.length > 0 && row.some((cell) => cell !== ''))
          resolve(grid)
        } catch (err) {
          reject(err)
        }
      }
      reader.onerror = reject
      reader.readAsArrayBuffer(file)
    })
  }

  const generateXLSXBuffer = (grid: Grid): ArrayBuffer => {
    const ws = XLSX.utils.aoa_to_sheet(grid)
    const wb = XLSX.utils.book_new()
    XLSX.utils.book_append_sheet(wb, ws, 'Sheet1')
    return XLSX.write(wb, { bookType: 'xlsx', type: 'array' }) as ArrayBuffer
  }

  return {
    readTextFile,
    parseCSV,
    parseXLSX,
    generateCSVContent,
    generateXLSXBuffer,
  }
}
