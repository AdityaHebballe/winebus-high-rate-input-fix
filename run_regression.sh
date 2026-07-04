#!/usr/bin/env bash
set -euo pipefail
variant=${1:-patched}
rate=${2:-1000}
[[ "$variant" == baseline || "$variant" == patched ]] || { echo "usage: $0 baseline|patched" >&2;exit 2; }
root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix"
winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
[[ -w /dev/uinput ]] || { echo "/dev/uinput is not writable" >&2;exit 1; }
source="$winebus.original"; [[ "$variant" == patched ]] && source="$winebus.inputfix"
trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT
cp --preserve=all "$source" "$winebus"
make -s
mkdir -p results
suffix="$variant";[[ "$rate" != 1000 ]]&&suffix="$variant-${rate}hz"
prefix="$PWD/prefix-regression-$suffix-$(date +%Y%m%d-%H%M%S)"
states="Z:${PWD//\//\\}\\results\\regression-$suffix-states.csv"
rumble="Z:${PWD//\//\\}\\results\\regression-$suffix-rumble.csv"
./regression_producer "$rate" >"results/regression-$suffix-producer.csv" & producer=$!
sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 WINEPREFIX="$prefix" PROTONPATH="$root" \
  umu-run "$PWD/regression_probe.exe" "$states" "$rumble" 24000
wait "$producer"
./analyze_regression.py "results/regression-$suffix-producer.csv" "results/regression-$suffix-states.csv" "results/regression-$suffix-rumble.csv" | tee "results/regression-$suffix-analysis.txt"
