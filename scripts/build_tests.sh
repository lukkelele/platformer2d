#!/usr/bin/env bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1

cmake -S . -B build --fresh -DLK_BUILD_TESTS=ON
cmake --build build -j $(nproc)
