# BUILD.md

本檔案說明如何在本機或 CI 中建置本專案的前端（npm）與 C++（native / WebAssembly）部分。若你只是想使用這個工具，請見 [README.md](README.md)。

---

## 概覽

- 前端（Vue + Vite）：使用 npm（package.json 已定義指令）
- C++ 演算法：使用 CMake，主要目標是編譯成 WebAssembly（透過 Emscripten），由前端直接載入
  - 演算法本身為 header-only（`cpp/src/*.hpp`），**無第三方 C++ 依賴**
  - `cpp/test/` 是基準測試工程，需要 Google Benchmark（見下方說明）

## 目錄結構（與建置相關）

- `cpp/`：C++ 演算法原始碼與 CMake 工程
  - `src/`：演算法實作（header-only，C++23）
  - `test/`：基準測試工程（需 Google Benchmark）
  - `cpp-configure.bat` / `cpp-configure.sh`：WASM 建置配置腳本
  - `cpp-build.bat` / `cpp-build.sh`：WASM 建置腳本
- `src/assets/wasm/`：WASM 建置產出之一（`alloc_algo.js`，CMake 自動複製）
- `public/`：WASM binary 產出（`alloc_algo.wasm`，CMake 自動複製）
- `algo-build/`：CI 使用的 CMake 建置目錄（由 GitHub Actions 產生）

---

## 先決條件

- Node.js：`^22.18.0` 或 `>=24.12.0`（見 package.json 的 `engines`；建議用 nvm / nvm-windows 管理）
- npm（隨 Node 附帶）
- CMake：主工程和 `cpp/test` 均需 >= 3.22
- emsdk：編譯 WebAssembly 需要（emscripten）
- （選用）Google Benchmark：執行 `cpp/test` 基準測試時需要，可透過 vcpkg 或系統套件管理安裝

> 備註：主演算法不依賴任何第三方 C++ 函式庫；vcpkg 目前僅用於安裝基準測試所需的 `benchmark` 套件。

---

## 前端（npm）

在專案根目錄（含 package.json）執行：

安裝依賴

```sh
npm ci        # CI 和乾淨安裝（推薦）
npm install   # 本地開發（接受 package-lock.json 變動）
```

開發（熱重載）

```sh
npm run dev
```

生產建置（含型別檢查）

```sh
npm run build
```

預覽生產結果

```sh
npm run preview
```

其他指令

```sh
npm run test:unit     # 單元測試（Vitest）
npm run type-check    # 型別檢查（vue-tsc，build 時已自動包含）
npm run lint          # 程式碼檢查（oxlint + eslint）
npm run format        # 格式化（prettier）
```

---

## C++：編譯 WebAssembly（主要目標）

前端實際載入的是 WebAssembly 版演算法，因此一般開發 / 發佈只需建置 WASM。

### 安裝並啟用 emsdk（範例）

Windows (PowerShell)

```powershell
cd C:\path\to\emsdk
.\emsdk install latest
.\emsdk activate latest
.\emsdk_env.bat
```

Linux / macOS

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### 編譯（在 cpp/ 目錄）

使用專案提供的腳本：

```sh
cd cpp
# Windows (PowerShell)：
.\cpp-configure.bat   # 等同 emcmake cmake -S . -B ../algo-build/ -DCMAKE_BUILD_TYPE=Release
.\cpp-build.bat       # 等同 cmake --build ../algo-build/

# Linux / macOS：
./cpp-configure.sh
./cpp-build.sh
```

或手動執行：

```sh
cd cpp
emcmake cmake -S . -B ../algo-build -DCMAKE_BUILD_TYPE=Release
cmake --build ../algo-build --parallel
```

### 建置產出位置

建置完成後，CMake 的 POST_BUILD 步驟會**自動**將產物複製到：

- `src/assets/wasm/alloc_algo.js`（前端 import 的 JS 包裝）
- `public/alloc_algo.wasm`（WASM binary，前端透過 base URL 載入）

無需手動搬檔；執行 `npm run build` 時兩者都會被打包進 `dist/`。

> 備註：目前 WASM 為單執行緒版本（未啟用 pthread / SharedArrayBuffer）。

---

## C++：Native（選用）

若在未啟用 Emscripten 的環境執行 CMake，工程會改為建置原生執行檔（並印出警告）。此模式僅供演算法除錯使用，產品使用的是 WASM 版本。

```sh
cd cpp
cmake -S . -B cmake-build-native -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-native --parallel
```

---

## C++：基準測試（選用）

`cpp/test/` 是以 Google Benchmark 撰寫的基準測試，可量測演算法在不同規模輸入下的效能：

```sh
cd cpp/test
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<benchmark 安裝路徑>
cmake --build cmake-build-release --parallel
./cmake-build-release/seat_allocator_vue_algo_benchmark
```

`benchmark` 可透過 vcpkg 安裝（triplet 依平台選擇，例如 `x64-windows`、`x64-linux`）：

```powershell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
vcpkg install benchmark
```

---

## CI（GitHub Actions）

專案已附 workflow（`.github/workflows/`），可作為 CI 建置的參考：

- `deploy.yml`：push 到 `master` 時自動建置並部署 GitHub Pages，流程為：
  1. 安裝 Emscripten（`emscripten-core/setup-emsdk`，版本 6.0.0）
  2. 在 `algo-build/` 以 `emcmake cmake ../cpp` 建置 WASM（`-DCMAKE_BUILD_TYPE=Release`）
  3. `npm ci` 安裝前端依賴
  4. `npm run build` 產出 `dist/`
  5. 以 `peaceiris/actions-gh-pages` 將 `dist/` 部署到 GitHub Pages
- `test.yml`：測試用 workflow（目前僅為佔位）

---

## 常見問題排查

- Node 版本不符：執行 `node -v` 確認，使用 nvm / nvm-windows 切換到符合 `engines` 的版本
- 找不到 `emcmake` / `emcc`：確認已執行 `emsdk activate` 與 `emsdk_env`（Windows 為 `emsdk_env.bat`）
- WASM 建置失敗：確認 CMake 偵測到 Emscripten（建置輸出應顯示 "Building for WebAssembly with Emscripten"）
- 基準測試找不到 benchmark：以 `-DCMAKE_PREFIX_PATH` 指向 benchmark 安裝位置，確認 `find_package(benchmark)` 可找到
- 前端畫面顯示「WebAssembly 模組載入失敗」：確認 `src/assets/wasm/alloc_algo.js` 與 `public/alloc_algo.wasm` 都存在，且為同一次建置的產物
