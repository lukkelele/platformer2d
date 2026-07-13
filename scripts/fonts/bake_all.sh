#!/usr/bin/env bash
####################################################################################################
# Bake every .ttf under assets/fonts/ into an MSDF atlas + JSON.
#
# Usage:
#     scripts/fonts/bake_all.sh               # bake all .ttf
#     scripts/fonts/bake_all.sh -x            # skip FontAwesome
#     scripts/fonts/bake_all.sh -s 48 -p 6    # forward size / pxrange to bake.sh
####################################################################################################
set -euo pipefail

SKIP_FONTAWESOME=0
SIZE=""
PXRANGE=""
TOOL=""

while getopts ":xs:p:t:h" opt; do
	case "$opt" in
		x) SKIP_FONTAWESOME=1 ;;
		s) SIZE="$OPTARG" ;;
		p) PXRANGE="$OPTARG" ;;
		t) TOOL="$OPTARG" ;;
		h) sed -n '2,/^$/{/^# /{s/^# \{0,1\}//;p}}' "$0"; exit 0 ;;
		\?) echo "Unknown option: -$OPTARG" >&2; exit 2 ;;
		:)  echo "Option -$OPTARG requires an argument" >&2; exit 2 ;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
FONTS_DIR="$ROOT/assets/fonts"

if [[ ! -d "$FONTS_DIR" ]]; then
	echo "Fonts directory not found: $FONTS_DIR" >&2
	exit 1
fi

FORWARD_ARGS=()
[[ -n "$SIZE"    ]] && FORWARD_ARGS+=( -s "$SIZE" )
[[ -n "$PXRANGE" ]] && FORWARD_ARGS+=( -p "$PXRANGE" )
[[ -n "$TOOL"    ]] && FORWARD_ARGS+=( -t "$TOOL" )

count=0
skipped=0
failed=0

while IFS= read -r -d '' ttf; do
	name="$(basename "$ttf" .ttf)"
	dir="$(dirname "$ttf")"
	rel_dir="${dir#"$ROOT/"}"

	charset="ascii"
	if [[ "$rel_dir" == *FontAwesome* ]]; then
		if [[ "$SKIP_FONTAWESOME" -eq 1 ]]; then
			echo "Skipping (FontAwesome): ${rel_dir}/${name}.ttf"
			skipped=$((skipped + 1))
			continue
		fi
		charset="fontawesome"
	fi

	echo "Baking ${rel_dir}/${name}.ttf (charset=$charset)"
	if bash "$SCRIPT_DIR/bake.sh" -f "$name" -d "$rel_dir" -c "$charset" "${FORWARD_ARGS[@]}"; then
		count=$((count + 1))
	else
		echo "  FAILED: ${rel_dir}/${name}.ttf" >&2
		failed=$((failed + 1))
	fi
done < <(find "$FONTS_DIR" -type f -name '*.ttf' -print0)

echo
echo "Baked:   $count"
echo "Skipped: $skipped"
echo "Failed:  $failed"
[[ $failed -eq 0 ]]

