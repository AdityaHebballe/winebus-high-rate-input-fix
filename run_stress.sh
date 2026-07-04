#!/usr/bin/env bash
set -euo pipefail

rate=${1:-4000}
case "$rate" in 125|250|500|1000|2000|4000|8000) ;;
  *) echo "usage: $0 125|250|500|1000|2000|4000|8000" >&2;exit 2;;
esac
root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix"
winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
[[ -w /dev/uinput ]] || { echo "/dev/uinput is not writable" >&2;exit 1; }
trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT
cp --preserve=all "$winebus.inputfix" "$winebus"
make -s
mkdir -p results
prefix="$PWD/prefix-stress-${rate}hz-$(date +%Y%m%d-%H%M%S)"
output="Z:${PWD//\//\\}\\results\\stress-${rate}hz-xinput.csv"
./uinput_producer 4 1500 2500 "$rate" >"results/stress-${rate}hz-producer.csv" & producer=$!
sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 WINEPREFIX="$prefix" PROTONPATH="$root" \
  umu-run "$PWD/xinput_probe.exe" 21000 "$output"
wait "$producer"
./analyze.py "results/stress-${rate}hz-producer.csv" "results/stress-${rate}hz-xinput.csv" \
  | tee "results/stress-${rate}hz-analysis.txt"
