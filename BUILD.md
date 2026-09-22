# BUILD.md

本檔案說明如何在本機或 CI 中建置本專案的前端（npm）與 C++（主工程編譯為 WebAssembly，另有 native 的測試／基準工程）部分。若你只是想使用這個工具，請見 [README.md](README.md)。

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
  - `benchmark/optimizing/`：實驗性超參數調優工程（Optuna，獨立 CMake 工程，見下方說明）
  - `cpp-configure.bat` / `cpp-configure.sh`：WASM 建置配置腳本
  - `cpp-build.bat` / `cpp-build.sh`：WASM 建置腳本
- `src/assets/wasm/`：WASM 建置產出之一（`alloc_algo.js`，CMake 自動複製）
- `public/`：WASM binary 產出（`alloc_algo.wasm`，CMake 自動複製）
- `algo-build/`：本地與 CI 共用的 CMake 建置目錄（由 CMake preset 指定）

---

## 先決條件

- CMake：主工程、`cpp/tests`、`cpp/benchmark` 與 `cpp/benchmark/optimizing` 均需 >= 3.22
- emsdk：編譯 WebAssembly 需要（emscripten）
- （選用）GoogleTest / Google Benchmark：分別用於 `cpp/tests` 單元測試與 `cpp/benchmark` 基準測試，可透過各工程的 vcpkg manifest（`vcpkg.json`）自動安裝

> [!NOTE] 
> 主演算法不依賴任何第三方 C++ 函式庫；
> vcpkg 僅用於測試工程（`cpp/tests` 的 `gtest`、`cpp/benchmark` 的 `benchmark`）與實驗性調優工程（`cpp/benchmark/optimizing` 的 `nlohmann-json`）。

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

> [!NOTE]
> 尚未建置 WASM 時，`npm run test:unit` 會自動產生 `src/assets/wasm/alloc_algo.js` 樁檔，讓測試得以解析 import；`npm run build`（型別檢查 + 打包）則必須先完成 WASM 建置。

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
- `src/assets/wasm/alloc_algo.d.ts`（embind 產生的 TypeScript 型別宣告，會先經 `patch-dts.cmake` 修正再複製，供 IDE／型別檢查使用）

無需手動搬檔；執行 `npm run build` 時 `.js` 與 `.wasm` 會被打包進 `dist/`，`.d.ts` 僅供開發期型別使用、不需打包。

> [!NOTE] 
> 目前 WASM 為單執行緒版本（未啟用 pthread / SharedArrayBuffer）。

---

## C++：Native（已不支援）

主工程 `cpp/` 目前是 **WebAssembly-only**：`wasm-bridge.cpp` 是 embind bridge、沒有 `main()`，且 `cpp/CMakeLists.txt` 在未使用 Emscripten toolchain 時會直接以 `FATAL_ERROR` 中止 configure（錯誤訊息會提示改用 `cmake --preset wasm`），因此已不存在「原生執行檔」模式，也無法在本機直接以 `cmake -S . -B ...` 建置。

如需在本機以原生方式執行／除錯演算法，請改用以下工程：

- `cpp/tests/`：以 GoogleTest 原生執行單元測試（見下方「C++：單元測試」）
- `cpp/benchmark/`：以原生執行基準測試（見「C++：基準測試」）

---

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

### 超參數調優（optimizing，實驗性）

`cpp/benchmark/optimizing/` 是獨立的實驗工程，以 [Optuna](https://optuna.org/) 自動搜尋模擬退火的超參數（`initialTemperature`、`coolingRate`、`maxSteps` 等）。它不是 Google Benchmark 工程：`main.cpp` 編譯出的執行檔會在固定輸入（含搭檔配對等約束）上量測耗時並輸出 JSON，Python 調參腳本以該輸出為目標，反覆呼叫執行檔搜尋參數。

> [!NOTE] 
> 此工程需 C++23（GCC 14+ 或 Clang 18+，與 `cpp/tests` 相同）；MinGW 分支會額外連結 `-lstdc++exp`（GCC 14+ 的 libstdc++ 實驗功能庫）。

目錄內容：

- `main.cpp`：調優用基準程式，支援 `--mode=fixed|dynamic`（固定參數／隨輸入規模調整的自適應公式）與 `--params=<JSON>`，輸出加權平均耗時、平均步數與錯誤率
- `tune-alpha.py` / `fix-tuning.py` / `dynamic-tuning.py`：Optuna 調參腳本（分別探索 alpha 極限、fixed 模式與 dynamic 模式的參數），結果圖存為 PNG
- `pyproject.toml` / `uv.lock`：Python 依賴以 uv 管理（Python >= 3.14，`optuna`、`matplotlib`）
- `vcpkg.json`：C++ 依賴 `nlohmann-json`（preset 透過 `$env{VCPKG_ROOT}` 指定工具鏈）

建置執行檔（需先設定 `VCPKG_ROOT`；編譯器需支援 C++23）：

```sh
cd cpp/benchmark/optimizing
cmake --preset release    # Ninja + vcpkg，產出至 build/
cmake --build build --parallel
```

手動執行（Windows 執行檔為 `build\algo_optimizing.exe`；`--params` 請用單引號包住 JSON，避免 shell 把花括號當成特殊字元）：

```sh
./build/algo_optimizing.exe --mode=fixed --params='{"T0": 5.0, "alpha": 0.999, "maxSteps": 350000}'
./build/algo_optimizing.exe --mode=dynamic --params='{"min_step": 50000, "size_mul": 300, "alpha": 0.99}'
```

執行 Optuna 調參（腳本目前以 Windows 路徑 `./build/algo_optimizing.exe` 呼叫執行檔）：

```sh
uv sync                        # 建立 .venv 並安裝 optuna、matplotlib
uv run python fix-tuning.py         # Fixed 模式（50 trials）
uv run python dynamic-tuning.py     # Dynamic 模式（100 trials）
uv run python tune-alpha.py         # alpha 一維探索（300 trials）
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
- `build.yml`：於 PR（`pull_request`）與手動觸發（`workflow_dispatch`）時驗證完整建置（僅檢查、不部署），步驟與 `deploy.yml` 相同：安裝 Emscripten 並在 `cpp/` 建置 WASM、`npm ci` 後執行 `npm run build`，最後確認 `dist/` 產物（`index.html`、`alloc_algo.wasm`）存在

---

## 常見問題排查

- Node 版本不符：執行 `node -v` 確認，使用 nvm / nvm-windows 切換到符合 `engines` 的版本
- 找不到 `emcmake` / `emcc`：確認已執行 `emsdk activate` 與 `emsdk_env`（Windows 為 `emsdk_env.bat`）
- WASM 建置失敗：確認 CMake 偵測到 Emscripten（建置輸出應顯示 "Building for WebAssembly with Emscripten"）
- 基準測試找不到 benchmark：以 `-DCMAKE_PREFIX_PATH` 指向 benchmark 安裝位置，確認 `find_package(benchmark)` 可找到
- 前端畫面顯示「WebAssembly 模組載入失敗」：確認 `src/assets/wasm/alloc_algo.js` 與 `public/alloc_algo.wasm` 都存在，且為同一次建置的產物
- 分析腳本缺少套件：執行 `pip install numpy matplotlib`（建議搭配 venv）
