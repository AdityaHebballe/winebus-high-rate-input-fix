#!/usr/bin/env bash
set -euo pipefail

variant=${1:-}
case "$variant" in
    baseline|patched) ;;
    *) echo "usage: $0 baseline|patched" >&2; exit 2 ;;
esac
trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT

root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix"
winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
case "$variant" in
    baseline) source_binary="$winebus.original" ;;
    patched) source_binary="$winebus.inputfix" ;;
esac

if [[ ! -e /dev/uinput ]]; then
    echo "Missing /dev/uinput. Run: sudo mknod -m 666 /dev/uinput c 10 223" >&2
    exit 1
fi
if [[ ! -w /dev/uinput ]]; then
    echo "/dev/uinput is not writable. Run: sudo chmod 666 /dev/uinput" >&2
    exit 1
fi
if [[ ! -f "$winebus.original" ]]; then
    cp --preserve=all "$winebus" "$winebus.original"
fi
if [[ ! -f "$source_binary" ]]; then
    echo "Missing saved $variant binary: $source_binary" >&2
    exit 1
fi

cp --preserve=all "$source_binary" "$winebus"
make -s
mkdir -p results
prefix="$PWD/prefix-$variant-$(date +%Y%m%d-%H%M%S)"
mkdir -p "$prefix"

echo "variant=$variant winebus_sha256=$(sha256sum "$winebus" | cut -d' ' -f1)"
windows_output="Z:${PWD//\//\\}\\results\\$variant-xinput.csv"
./uinput_producer 5 2000 4000 > "results/$variant-producer.csv" &
producer_pid=$!
sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 \
    WINEPREFIX="$prefix" PROTONPATH="$root" \
    umu-run "$PWD/xinput_probe.exe" 38000 "$windows_output"
wait "$producer_pid"
./analyze.py "results/$variant-producer.csv" "results/$variant-xinput.csv" \
    | tee "results/$variant-analysis.txt"
