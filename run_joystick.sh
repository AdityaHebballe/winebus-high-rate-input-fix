#!/usr/bin/env bash
set -euo pipefail
root="$HOME/.local/share/Steam/compatibilitytools.d/Proton-CachyOS-InputFix";winebus="$root/files/lib/wine/x86_64-unix/winebus.so"
[[ -w /dev/uinput ]]||{ echo "/dev/uinput is not writable" >&2;exit 1; };trap 'cp --preserve=all "$winebus.inputfix" "$winebus"' EXIT
cp --preserve=all "$winebus.inputfix" "$winebus";make -s;mkdir -p results
prefix="$PWD/prefix-joystick-$(date +%Y%m%d-%H%M%S)";output="Z:${PWD//\//\\}\\results\\joystick-winmm.csv"
./uinput_producer 1 5000 3000 1000 generic >results/joystick-producer.csv & producer=$!;sleep 1
PROTON_PREFER_SDL=1 PROTONFIXES_DISABLE=1 GAMEID=0 WINEPREFIX="$prefix" PROTONPATH="$root" umu-run "$PWD/winmm_probe.exe" "$output" 15000
wait "$producer";./analyze_joystick.py results/joystick-producer.csv results/joystick-winmm.csv | tee results/joystick-analysis.txt
