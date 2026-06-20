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

  const exportCSV = (grid: Grid, filename: string) => {
    if (grid.length === 0) return

    const csvContent = grid.map((row) => row.join(',')).join('\n')
    const blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' })
    const url = URL.createObjectURL(blob)

    const link = document.createElement('a')
    link.setAttribute('href', url)
    link.setAttribute('download', filename)
    document.body.appendChild(link)
    link.click()
    document.body.removeChild(link)
    URL.revokeObjectURL(url)
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

  const exportXLSX = (grid: Grid, filename: string) => {
    if (grid.length === 0) return

    const ws = XLSX.utils.aoa_to_sheet(grid)
    const wb = XLSX.utils.book_new()
    XLSX.utils.book_append_sheet(wb, ws, 'Sheet1')
    const wb_out = XLSX.write(wb, { bookType: 'xlsx', type: 'array' })
    const blob = new Blob([wb_out], {
      type: 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
    })
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.setAttribute('download', filename)
    document.body.appendChild(link)
    link.click()
    document.body.removeChild(link)
    URL.revokeObjectURL(url)
  }

  return {
    readTextFile,
    parseCSV,
    exportCSV,
    parseXLSX,
    exportXLSX,
  }
}
