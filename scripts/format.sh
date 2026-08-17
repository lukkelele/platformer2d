#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/.. || exit 1

if [[ $# -eq 0 ]]; then
    dirs=("src" "test")
else
    dirs=("$@")
fi

extensions=("*.h" "*.cpp")

for dir in "${dirs[@]}"; do
    [[ -d "$dir" ]] || continue
    for ext in "${extensions[@]}"; do
        while IFS= read -r -d '' file; do
            clang-format -i "$file"
        done < <(find "$dir" -name "$ext" -print0)
    done
done

