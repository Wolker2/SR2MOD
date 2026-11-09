#!/usr/bin/env bash
set -euo pipefail
game="$(realpath "$1")"
dll="$(realpath "$2")"
cd "$(dirname "$0")"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
python3 pe_fixture.py "$game" "$dll" "$work"
gcc -m32 -O2 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
    -fshort-wchar -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 \
    -Wall -Wextra -Werror -I"$work" -c test_core.c -o "$work/test.o"
gcc -m32 -O2 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
    -fshort-wchar -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 \
    -Wall -Wextra -Werror -I"$work" -c test_visual.c -o "$work/visual.o"
gcc -m32 -O2 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
    -fshort-wchar -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 \
    -Wall -Wextra -Werror -I"$work" -c test_save.c -o "$work/save.o"
as --32 start.S -o "$work/start.o"
as --32 "$work/blobs.S" -o "$work/blobs.o"
ld -m elf_i386 -o "$work/test" "$work/start.o" "$work/test.o" "$work/visual.o" "$work/save.o" "$work/blobs.o"
gcc -O2 -no-pie -Wl,-Ttext-segment=0x20000000 bridge.c bridge.S -o "$work/bridge"
for mode in 0 1 2 3 4 5 6 7 8 9 10 11 12; do
    "$work/bridge" "$work/test" "$mode" > "$work/result$mode" || { code=$?; echo "FAIL mode=$mode assertion=$code"; exit "$code"; }
    echo "PASS mode=$mode"
done
for trial in 1 2 3 4; do
    "$work/bridge" "$work/test" 0 > "$work/trial$trial"
done
python3 - "$work" <<'PY'
from pathlib import Path
from statistics import median
import struct
import sys

work = Path(sys.argv[1])
runs = [struct.unpack("<6I", path.read_bytes()) for path in [work / "result0", *sorted(work.glob("trial*"))]]
for index, name in enumerate(("polygon lookup", "faction count", "ranger rating sort")):
    ratios = [times[index * 2] / times[index * 2 + 1] for times in runs]
    print(f"{name}: {median(ratios):.2f}x median in isolated x86 fixtures")
PY
