#!/usr/bin/env bash
####################################################################################################
# Bakes a MSDF font atlas + glyph-metrics JSON from a .ttf using msdf-atlas-gen.
#
# Options:
#     -f <name>     Font base name (no extension)
#     -d <dir>      Directory containing the TTF        [assets/fonts/Roboto]
#     -s <px>       Glyph EM size in atlas pixels       [40]
#     -p <px>       Signed-distance range in pixels     [4]
#     -c <name>     Charset name (ascii|fontawesome)    [ascii]
#                   Resolved as scripts/fonts/charset_<name>.txt
#     -t <path>     Explicit path to msdf-atlas-gen     [auto-detect]
#     -h            Show this help
####################################################################################################
set -euo pipefail

FONT=""
FONT_DIR=""
SIZE=40
PXRANGE=4
CHARSET_NAME="ascii"
TOOL=""

usage() {
	sed -n '2,/^$/{/^# /{s/^# \{0,1\}//;p}}' "$0"
}

while getopts ":f:d:s:p:c:t:h" opt; do
	case "$opt" in
		f) FONT="$OPTARG" ;;
		d) FONT_DIR="$OPTARG" ;;
		s) SIZE="$OPTARG" ;;
		p) PXRANGE="$OPTARG" ;;
		c) CHARSET_NAME="$OPTARG" ;;
		t) TOOL="$OPTARG" ;;
		h) usage; exit 0 ;;
		\?) echo "Unknown option: -$OPTARG" >&2; usage >&2; exit 2 ;;
		:)  echo "Option -$OPTARG requires an argument" >&2; exit 2 ;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# TODO: Verify on Linux
if [[ -z "${TOOL}" ]]; then
	candidates=(
		"${ROOT}/out/build/x64-debug/bin/msdf-atlas-gen"
		"${ROOT}/out/build/x64-release/bin/msdf-atlas-gen"
		"${ROOT}/build/Debug/bin/msdf-atlas-gen"
		"${ROOT}/build/Release/bin/msdf-atlas-gen"
		"${ROOT}/build/bin/msdf-atlas-gen"
	)
	for c in "${candidates[@]}"; do
		if   [[ -x "$c"     ]]; then TOOL="${c}";     break
		elif [[ -x "$c.exe" ]]; then TOOL="${c}.exe"; break
		fi
	done
fi

if [[ -z "${TOOL}" || ! -x "${TOOL}" ]]; then
	echo "msdf-atlas-gen executable not found" >&2
	exit 1
fi

TTF="${ROOT}/${FONT_DIR}/${FONT}.ttf"
PNG="${ROOT}/${FONT_DIR}/${FONT}.msdf.png"
JSON="${ROOT}/${FONT_DIR}/${FONT}.msdf.json"
CHARSET="${SCRIPT_DIR}/charset_${CHARSET_NAME}.txt"

[[ -f "$TTF"     ]] || { echo "Font file not found: $TTF"     >&2; exit 1; }
[[ -f "$CHARSET" ]] || { echo "Charset file not found: $CHARSET" >&2; exit 1; }

echo "Tool:    $TOOL"
echo "Font:    $TTF"
echo "Charset: $CHARSET"
echo "Size:    ${SIZE} px"
echo "PxRange: ${PXRANGE}"
echo "Outputs: $PNG"
echo "         $JSON"
echo

"$TOOL" \
	-font     "$TTF" \
	-charset  "$CHARSET" \
	-type     msdf \
	-format   png \
	-size     "$SIZE" \
	-pxrange  "$PXRANGE" \
	-yorigin  bottom \
	-imageout "$PNG" \
	-json     "$JSON"

echo

