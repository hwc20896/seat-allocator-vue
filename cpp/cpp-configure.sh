#!/bin/bash
cd "$(dirname "$0")" || exit
emcmake cmake -S . -B ../algo-build -DCMAKE_BUILD_TYPE=Release