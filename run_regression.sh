#!/usr/bin/env bash
set -euo pipefail
variant=${1:-patched}
[[ "$variant" == baseline || "$variant" == patched ]] || { echo "usage: $0 baseline|patched" >&2;exit 2; }
root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix"
winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
[[ -w /dev/uinput ]] || { echo "/dev/uinput is not writable" >&2;exit 1; }
source="$winebus.original"; [[ "$variant" == patched ]] && source="$winebus.inputfix"
trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT
cp --preserve=all "$source" "$winebus"
make -s
mkdir -p results
prefix="$PWD/prefix-regression-$variant-$(date +%Y%m%d-%H%M%S)"
states="Z:${PWD//\//\\}\\results\\regression-$variant-states.csv"
rumble="Z:${PWD//\//\\}\\results\\regression-$variant-rumble.csv"
./regression_producer >"results/regression-$variant-producer.csv" & producer=$!
sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 WINEPREFIX="$prefix" PROTONPATH="$root" \
  umu-run "$PWD/regression_probe.exe" "$states" "$rumble" 24000
wait "$producer"
./analyze_regression.py "results/regression-$variant-producer.csv" "results/regression-$variant-states.csv" "results/regression-$variant-rumble.csv" | tee "results/regression-$variant-analysis.txt"
