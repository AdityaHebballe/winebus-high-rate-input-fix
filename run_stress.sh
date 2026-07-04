#!/usr/bin/env bash
set -euo pipefail

rate=${1:-4000}
variant=${2:-patched}
case "$rate" in 125|250|500|1000|2000|4000|8000) ;;
  *) echo "usage: $0 125|250|500|1000|2000|4000|8000" >&2;exit 2;;
esac
case "$variant" in baseline|patched) ;; *) echo "variant must be baseline or patched" >&2;exit 2;; esac
root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix"
winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
[[ -w /dev/uinput ]] || { echo "/dev/uinput is not writable" >&2;exit 1; }
trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT
source="$winebus.inputfix";[[ "$variant" == baseline ]]&&source="$winebus.original"
cp --preserve=all "$source" "$winebus"
make -s
mkdir -p results
prefix="$PWD/prefix-stress-$variant-${rate}hz-$(date +%Y%m%d-%H%M%S)"
stem="stress-$variant-${rate}hz"
output="Z:${PWD//\//\\}\\results\\$stem-xinput.csv"
idle=2500;duration=21000;[[ "$variant" == baseline ]]&&idle=5000&&duration=32000
./uinput_producer 4 1500 "$idle" "$rate" >"results/$stem-producer.csv" & producer=$!
sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 WINEPREFIX="$prefix" PROTONPATH="$root" \
  umu-run "$PWD/xinput_probe.exe" "$duration" "$output"
wait "$producer"
./analyze.py "results/$stem-producer.csv" "results/$stem-xinput.csv" \
  | tee "results/$stem-analysis.txt"
