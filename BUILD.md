# BUILD.md

本檔案說明如何在本機或 CI 中構建本專案的前端（npm）與 C++（native / WebAssembly）部分。

---

## 概覽
- 前端（Vue + Vite）：使用 npm（package.json 已定義指令）
- C++ 演算法：使用 CMake，支援 native 與 WebAssembly（透過 Emscripten）

---

## 先決條件
- Node.js: 與 package.json 相容的版本：`^22.18.0` 或 `>=24.12.0`（建議使用 nvm / nvm-windows 管理）
- npm（隨 Node 附帶）
- CMake >= 4.3
- vcpkg：用來安裝 C++ 依賴（範例：spdlog、nlohmann-json、OpenXLSX）
- emsdk：若要編譯 WebAssembly（emscripten）版本，需安裝並啟用 emsdk
- 建議：git, Python（部分平台的 build 工具可能需要）

> 備註：vcpkg 的套件名稱可能與 CMake 中使用的 target 名稱略有差異，使用前請在 vcpkg 的 port list 確認正確名稱或 triplet。

---

## 前端（npm）
在專案根目錄（含 package.json）執行：

安裝依賴

```sh
# CI 和乾淨安裝（推薦）
npm ci
# 本地開發（接受 package-lock.json 變動）
npm install
```

開發（熱重載）

```sh
npm run dev
```

生產建置

```sh
npm run build
```

預覽生產結果

```sh
npm run preview
```

測試 / 型別檢查 / Lint / 格式化

```sh
npm run test:unit
npm run type-check
npm run lint
npm run format
```

---

## C++：Native（使用 vcpkg）
假設 vcpkg 已安裝並安裝所需套件（例如：`spdlog`, `nlohmann-json`, `OpenXLSX`）。

Windows (PowerShell) 範例：

```powershell
$env:VCPKG_ROOT = 'C:\path\to\vcpkg'
cmake -B cmake-build-release-native -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build cmake-build-release-native --config Release --parallel
```

Linux / macOS 範例：

```sh
export VCPKG_ROOT=/path/to/vcpkg
cmake -B cmake-build-release-native -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-linux
cmake --build cmake-build-release-native -j
```

註：根據目標平台選擇合適的 vcpkg triplet（例如 x64-windows, x64-linux, x64-osx）。

---

## C++：WebAssembly（使用 Emscripten / emsdk）
若要輸出 `alloc_algo.js` / `alloc_algo.wasm`，需安裝並啟用 emsdk。

安裝與啟用（範例）

Windows (PowerShell)

```powershell
cd C:\path\to\emsdk
.\emsdk install latest
.\emsdk activate latest
# 啟用環境變數
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

編譯專案（在 cpp/ 目錄）

```sh
cd cpp
# 使用 emcmake 把工具鏈包裹起來
emcmake cmake -B cmake-build-release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --parallel
```

備註：
- 若希望同時使用 vcpkg 提供的套件，必須確保那些套件已針對 Emscripten 編譯（需要 Emscripten 的 vcpkg triplet 或手動建立）。
- 建置失敗時常見原因：缺少為 WASM 編譯的第三方函式庫或不相容的原生 API。

---

## 將 WebAssembly 結果放到前端
編譯完成後（例如在 `cmake-build-release`），將輸出檔案複製到前端可服務的位置，例如 `public/wasm/`：

Linux / macOS:

```sh
mkdir -p ../public/wasm
cp cmake-build-release/alloc_algo.js ../public/wasm/
cp cmake-build-release/alloc_algo.wasm ../public/wasm/
```

Windows (PowerShell):

```powershell
md ..\public\wasm -ErrorAction SilentlyContinue
copy cmake-build-release\alloc_algo.js ..\public\wasm\
copy cmake-build-release\alloc_algo.wasm ..\public\wasm\
```

之後前端可透過相對路徑載入 `public/wasm/alloc_algo.js`。

---

## CI 建議
- 使用 `npm ci` 保持 Node 依賴一致
- 若要在 CI 中編譯 C++ 或 WASM：選擇有 emsdk 或能安裝 emsdk 的 runner，或使用預先建立好的 artifacts
- 如果只需前端，CI 可只跑 npm 指令（build/test/lint）

---

## 常見問題排查
- Node 版本不符：確認 `node -v`，使用 nvm 切換
- CMake 找不到第三方套件：確認 `-DCMAKE_TOOLCHAIN_FILE` 已指向 vcpkg，且在 vcpkg 上安裝了相應 package 與 triplet
- WASM 連結錯誤：確認第三方 lib 已為 Emscripten 編譯，或考慮移除/替換不相容的依賴

---