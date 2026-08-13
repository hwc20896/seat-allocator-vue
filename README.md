# 座位分配器（Seat Allocator）

一個在瀏覽器裡執行的座位分配工具。匯入座位排佈（CSV / Excel）後，即可一鍵產生多組符合自訂約束的隨機分配結果，並以網格視覺化呈現。核心演算法以 C++ 撰寫、編譯為 WebAssembly，全部在瀏覽器本機執行——名單與分配結果不會上傳到任何伺服器。

## 功能特色

- **匯入座位排佈**：支援 CSV 與 Excel（.xlsx）格式
- **一鍵洗牌**：以模擬退火演算法產生多組符合約束的隨機分配
- **多頁瀏覽**：每次洗牌可產生多組分配結果，隨時翻頁回顧
- **手動調整**：點選兩個格子即可互換位置，手動修改過的格子會以不同底色標示
- **匯出結果**：將任一頁分配結果存成 Excel 或 CSV
- **顏色標記**：以正則表達式規則為特定座位著色
- **演算法約束**：以 JSON 指定固定位置、禁止鄰座、禁止同排/同列等規則
- **快捷鍵操作**：常用功能皆可用鍵盤完成

## 快速開始

### 線上使用

直接開啟 GitHub Pages 部署的版本：

<https://hwc20896.github.io/seat-allocator-vue/>

### 本機執行

需要 Node.js（版本 `^22.18.0` 或 `>=24.12.0`）：

```sh
npm install
npm run dev
```

開發伺服器預設位在 http://localhost:5173，於瀏覽器開啟即可使用。

## 使用指南

### 1. 匯入座位排佈

- 點擊選單「文件 (File) → 導入座位排佈」（或按 `Ctrl + I`）
- 支援兩種格式：
  - **CSV**：逗號分隔；每個儲存格代表一個座位，內容為人名或標籤，空格留空即可
  - **Excel（.xlsx）**：取第一個工作表；空白列會自動忽略
- 匯入後會顯示原始名單

### 2. 產生分配

- 按「洗牌」按鈕（或按 `Enter`）開始分配；演算法會盡量滿足所有約束，並產生多組結果
- 使用頂部的翻頁器瀏覽「第 1 次分配」「第 2 次分配」……
- 按「原始列表」可隨時切回匯入時的名單

### 3. 手動微調

- 先點一下某個格子將其標記，再點另一個格子，即可交換兩者的位置
- 手動修改過的格子會以不同底色標示

### 4. 匯出

- 「文件 (File) → 導出座位排佈」（或按 `Ctrl + E`）
- 可選擇儲存為 .xlsx 或 .csv
- 若目前停留在原始名單頁，匯出前會先跳出提醒

## 顏色配置

「顏色設定 (Color) → 導入顔色配置」可載入 JSON 檔，格式為「正則表達式 → 顏色」的對應表；名字符合規則的座位會套用該顏色。例如：

```json
{
  "^A-": "#ef4444",
  "^B-": "#3b82f6"
}
```

以「A-」開頭的名字顯示紅色、以「B-」開頭的名字顯示藍色。按 `Ctrl + Alt + C` 可一鍵還原。

## 演算法約束

「算法約束 (Constraints) → 導入約束配置」可載入 JSON 約束，控制洗牌的行為。範例：

```json
{
  "allowFixedPoints": false,
  "allowOriginalNeighbors": true,
  "diagonalsAreNeighbors": false,
  "customForbiddenPairs": [["張三", "李四"]],
  "constraints": [
    { "type": "FORCEROW", "name": "王小明", "rowIdx": 0 },
    { "type": "FORBIDCOL", "name": "陳大文", "colIdx": 2 }
  ]
}
```

| 欄位 | 說明 |
| --- | --- |
| `allowFixedPoints` | 是否允許座位留在原位置（預設 `false`） |
| `allowOriginalNeighbors` | 是否允許與原本的鄰座相鄰（預設 `false`） |
| `diagonalsAreNeighbors` | 斜對角是否視為相鄰（預設 `false`） |
| `customForbiddenPairs` | 禁止相鄰的兩人名單（`[["A", "B"], ...]`） |
| `constraints` | 更細緻的位置約束（見下表） |

`constraints` 支援的型別：

| type | 欄位 | 效果 |
| --- | --- | --- |
| `FORCEROW` | `name`, `rowIdx` | 將某人固定在指定列 |
| `FORBIDROW` | `name`, `rowIdx` | 禁止某人坐在指定列 |
| `FORCECOL` | `name`, `colIdx` | 將某人固定在指定行 |
| `FORBIDCOL` | `name`, `colIdx` | 禁止某人坐在指定行 |
| `FORBIDSHAREROW` | `name1`, `name2` | 兩人不得在同一列 |
| `FORBIDSHARECOL` | `name1`, `name2` | 兩人不得在同一行 |

約束可能互相衝突（例如同時把同一人固定到兩列）；若洗牌失敗，請檢查 JSON 並嘗試放寬約束。修改約束後重新洗牌即可套用。

## 鍵盤快捷鍵

| 快捷鍵 | 功能 |
| --- | --- |
| `Ctrl + I` | 導入座位排佈 |
| `Ctrl + E` | 導出座位排佈 |
| `Ctrl + Shift + C` | 導入顏色配置 |
| `Ctrl + Alt + C` | 重設顏色配置 |
| `Ctrl + Shift + K` | 導入演算法約束 |
| `Ctrl + Alt + K` | 重設演算法約束 |
| `Enter` | 洗牌 |

## 技術架構（簡介）

- 前端：Vue 3 + Vite + TypeScript
- 演算法：C++（模擬退火）編譯為 WebAssembly，於瀏覽器內執行
- 檔案處理：CSV 直接解析；Excel 使用 SheetJS（xlsx）

若你想從原始碼建置、測試或參與開發，請見 [BUILD.md](BUILD.md)。

## License

本專案以 [MIT License](LICENSE) 授權。
