#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
output="${1:-./FHD_FastCore_Error.XenoPlugin.dll}"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
flags=(-m32 -O2 -std=c11 -ffreestanding -fno-builtin -fno-pic -fno-pie
       -fno-stack-protector -fno-asynchronous-unwind-tables -fno-ident
       -fshort-wchar -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2
       -mno-sse -mno-mmx -Wall -Wextra -Werror)
for source in fast_core fast_paths resource_guard save_recovery import_stub; do
    gcc "${flags[@]}" -c "$source.c" -o "$work/$source.elf"
    python3 elf_to_coff.py "$work/$source.elf" "$work/$source.obj"
done
ld -m i386pe --shared --entry 0 --no-leading-underscore --kill-at \
    --out-implib "$work/kernel32.a" -o "$work/kernel32_stub.dll" \
    kernel32.def "$work/import_stub.obj"
ld -m i386pe --shared --entry 0 --no-leading-underscore --no-insert-timestamp \
    --enable-reloc-section --dynamicbase --nxcompat --subsystem windows --strip-all \
    -o "$output" fast_core.def "$work/fast_core.obj" "$work/fast_paths.obj" "$work/resource_guard.obj" "$work/save_recovery.obj" "$work/kernel32.a"
