#!/usr/bin/env bash
# Build pad-thresh into a flashable .syx with elekloader (GPL-2.0-or-later).
#
#   elekloader/build.sh ELEKLOADER_CHECKOUT STOCK_SYX CORE_ELEMOD [OUT_DIR]
#
# ELEKLOADER_CHECKOUT  an elekloader checkout at tag core-ar2-v0.3 (not main)
# STOCK_SYX            your own Analog-Rytm_MKII_OS1.74.syx (stays on your computer)
# CORE_ELEMOD          core-ar2-0.3.elemod from the core-ar2-v0.3 pre-release
# OUT_DIR              default ./pad-thresh-out
#
# Needs Python 3.9+ and Debian's m68k-linux-gnu binutils and GCC
# (set ELEKLOADER_CROSS for another prefix such as m68k-elf-).
set -euo pipefail
[ $# -ge 3 ] || { sed -n 2,13p "$0"; exit 1; }
HERE="$(cd "$(dirname "$0")" && pwd)"
EL="$(cd "$1" && pwd)"; STOCK="$(realpath "$2")"; CORE="$(realpath "$3")"
OUT="$(mkdir -p "${4:-pad-thresh-out}" && cd "${4:-pad-thresh-out}" && pwd)"
MODDIR="$OUT/moddir"; rm -rf "$MODDIR"; mkdir -p "$MODDIR"
cp "$HERE/mod.json" "$HERE/../src/main.c" "$MODDIR/"
cd "$EL"
export PYTHONDONTWRITEBYTECODE=1
python3 -B -m elekloader.sdk.build "$MODDIR" --stock "$STOCK" --out "$OUT/mod"
ELEMOD="$(ls "$OUT"/mod/pad-thresh-*.elemod | head -1)"
python3 -B -m elekloader.lint "$ELEMOD" --stock "$STOCK" --with "$CORE"
python3 -B -m elekloader.patch --stock "$STOCK" --mod "$CORE" --mod "$ELEMOD" \
    --out "$OUT/pad-thresh.syx" --version PT01
echo "Built $OUT/pad-thresh.syx: send it with Elektron Transfer, then press YES on the unit."
