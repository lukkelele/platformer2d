#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"/../.. || exit 1

BUILD_DIR="build/x64-debug"
cmake --fresh -S . -B $BUILD_DIR
cmake --build $BUILD_DIR --target msdf-atlas-gen-standalone

# TODO: Finish this and verify build dir on linux

