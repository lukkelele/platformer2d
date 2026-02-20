#!/usr/bin/env bash
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1

echo "[platformer2d] Running setup"

echo "[platformer2d] Building glad"
./scripts/build_glad.sh

glad_ret=$?
if [ "$glad_ret" -eq 0 ]; then
  echo "[platformer2d] Successfully built glad"
else
  echo "[platformer2d] Failed to build glad (return ${glad_ret})"
  exit 1
fi

echo "[platformer2d] Setup complete"

