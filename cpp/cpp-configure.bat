@echo off
cd /d "%~dp0"
emcmake cmake -S . -B ../algo-build -DCMAKE_BUILD_TYPE=Release