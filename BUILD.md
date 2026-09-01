# BUILD.md

本檔案說明如何在本機或 CI 中建置本專案的前端（npm）與 C++（native / WebAssembly）部分。若你只是想使用這個工具，請見 [README.md](README.md)。

---

## 概覽

- 前端（Vue + Vite）：使用 npm（package.json 已定義指令）
- C++ 演算法：使用 CMake，主要目標是編譯成 WebAssembly（透過 Emscripten），由前端直接載入
    - 演算法本身為 header-only（`cpp/src/*.hpp`），**無第三方 C++ 依賴**
    - `cpp/tests/` 是單元測試工程（GoogleTest），`cpp/benchmark/` 是基準測試工程（Google Benchmark），見下方說明

## 目錄結構（與建置相關）

- `cpp/`：C++ 演算法原始碼與 CMake 工程
  - `src/`：演算法實作（header-only，C++23）
  - `tests/`：單元測試工程（GoogleTest，vcpkg 管理）
  - `benchmark/`：基準測試工程（Google Benchmark，vcpkg 管理）
  - `cpp-configure.bat` / `cpp-configure.sh`：WASM 建置配置腳本
  - `cpp-build.bat` / `cpp-build.sh`：WASM 建置腳本
- `src/assets/wasm/`：WASM 建置產出之一（`alloc_algo.js`，CMake 自動複製）
- `public/`：WASM binary 產出（`alloc_algo.wasm`，CMake 自動複製）
- `algo-build/`：本地與 CI 共用的 CMake 建置目錄（由 CMake preset 指定）

---

## 先決條件

- CMake：主工程、`cpp/tests` 與 `cpp/benchmark` 均需 >= 3.22
- emsdk：編譯 WebAssembly 需要（emscripten）
- （選用）GoogleTest / Google Benchmark：分別用於 `cpp/tests` 單元測試與 `cpp/benchmark` 基準測試，可透過各工程的 vcpkg manifest（`vcpkg.json`）自動安裝

> [!NOTE] 
> 主演算法不依賴任何第三方 C++ 函式庫；
> vcpkg 僅用於測試工程（`cpp/tests` 的 `gtest`、`cpp/benchmark` 的 `benchmark`）。

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

> [!NOTE] 
> 目前 WASM 為單執行緒版本（未啟用 pthread / SharedArrayBuffer）。

---

## C++：Native（選用）

若在未啟用 Emscripten 的環境執行 CMake，工程會改為建置原生執行檔（並印出警告）。此模式僅供演算法除錯使用，產品使用的是 WASM 版本。

```sh
cd cpp
cmake -S . -B cmake-build-native -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-native --parallel
```


## C++：單元測試（選用）

> [!WARNING] 
> 演算法使用 C++23（`std::views::zip`、`std::ranges::to`、`std::expected` 等），需 GCC 14+ 或 Clang 18+ 才能編譯。

`cpp/tests/` 是以 GoogleTest 撰寫的演算法單元測試，依賴由 vcpkg manifest（`cpp/tests/vcpkg.json`）管理。使用專案 preset 建置並以 ctest 執行

```sh
cd cpp/tests 
cmake --preset release # 需先設定 VCPKG_ROOT（見下方 vcpkg 說明） 
cmake --build build-release 
ctest --test-dir build-release --output-on-failure
```

若未使用 preset，可手動指定 vcpkg 工具鏈：

```sh
cd cpp/tests 
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake 
cmake --build build-release --parallel 
ctest --test-dir build-release --output-on-failure
```

---

## C++：基準測試（選用）

`cpp/benchmark/` 是以 Google Benchmark 撰寫的基準測試，可量測演算法在不同規模輸入下的效能。依賴由 vcpkg manifest（`cpp/benchmark/vcpkg.json`）管理，使用專案 preset 建置：

```sh
cd cpp/benchmark 
cmake --preset release # 需先設定 VCPKG_ROOT 
cmake --build build-release --parallel 
./build-release/seat_allocator_vue_algo_benchmark
```

（Windows 執行檔為 `build-release\seat_allocator_vue_algo_benchmark.exe`）

若未使用 preset，可手動指定 vcpkg 工具鏈（與上方單元測試相同方式）；  
`benchmark` 亦可透過 vcpkg 直接安裝（triplet 需與編譯器一致，例如 MSVC 用 `x64-windows`、MinGW 用 `x64-mingw-dynamic`、Linux 用 `x64-linux`）：

```powershell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
vcpkg install benchmark
```

### 輸出 JSON 供分析

預設直接印出結果；要搭配分析腳本請輸出 JSON：
```sh
./build-release/seat_allocator_vue_algo_benchmark --benchmark_out=output.json --benchmark_out_format=json
```

只想跑部分 family（快速迭代，或拆成多個 process 平行加速）可用 `--benchmark_filter`：

```sh
./build-release/seat_allocator_vue_algo_benchmark --benchmark_filter="^BM_Shuffle4.*" --benchmark_out=r4.json --benchmark_out_format=json
```

### 分析腳本

`cpp/benchmark/` 附兩個 Python 分析工具（需 Python 3.10+ 與 `numpy`、`matplotlib`）：

- `benchmark-curve.py`：對每個 family 擬合時間 / 步數的複雜度曲線（冪律），輸出 PNG
- `benchmark-compare.py`：比較自動 / 預設Config / 調參三種 annealing 設定的效率（耗時、步數、錯誤率），輸出對比圖；支援多份 JSON 與 `--run` 直接執行

```sh
python benchmark-curve.py curve.json curve.png --show 
python benchmark-compare.py curve.json --show
```

---

## CI（GitHub Actions）

專案已附 workflow（`.github/workflows/`），可作為 CI 建置的參考：

- `deploy.yml`：push 到 `master` 時自動建置並部署 GitHub Pages，流程為：
  1. 安裝 Emscripten（`emscripten-core/setup-emsdk`，版本 6.0.0）
  2. 在 `cpp/` 以 CMake preset 建置 WASM（`emcmake cmake --preset wasm`，產出目錄為 `algo-build/`，Release 模式）
  3. `npm ci` 安裝前端依賴
  4. `npm run build` 產出 `dist/`
  5. 以 `peaceiris/actions-gh-pages` 將 `dist/` 部署到 GitHub Pages
- `unit-test.yml`：於 PR（`pull_request`）與 push 到 `master` 時執行單元測試，包含兩個並行 job：
  1. 前端：`npm ci` 後執行 `npm run test:unit:ci`（Vitest 單次執行模式）
  2. 演算法：`cpp/tests` 以 vcpkg + CMake preset 建置 GoogleTest，並以 `ctest` 執行

---

## 常見問題排查

- Node 版本不符：執行 `node -v` 確認，使用 nvm / nvm-windows 切換到符合 `engines` 的版本
- 找不到 `emcmake` / `emcc`：確認已執行 `emsdk activate` 與 `emsdk_env`（Windows 為 `emsdk_env.bat`）
- WASM 建置失敗：確認 CMake 偵測到 Emscripten（建置輸出應顯示 "Building for WebAssembly with Emscripten"）
- 基準測試找不到 benchmark：以 `-DCMAKE_PREFIX_PATH` 指向 benchmark 安裝位置，確認 `find_package(benchmark)` 可找到
- 前端畫面顯示「WebAssembly 模組載入失敗」：確認 `src/assets/wasm/alloc_algo.js` 與 `public/alloc_algo.wasm` 都存在，且為同一次建置的產物
- 分析腳本缺少套件：執行 `pip install numpy matplotlib`（建議搭配 venv）
