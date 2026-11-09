#!/usr/bin/env python3
"""Map the two PE files as inert fixtures. Never runs either entry point."""
from pathlib import Path
import struct
import sys


def pe_image(path):
    data = Path(path).read_bytes()
    pe, = struct.unpack_from("<I", data, 0x3C)
    sections, = struct.unpack_from("<H", data, pe + 6)
    optional_size, = struct.unpack_from("<H", data, pe + 20)
    optional = pe + 24
    size, headers = struct.unpack_from("<II", data, optional + 56)
    mapped = bytearray(size)
    mapped[:headers] = data[:headers]
    for index in range(sections):
        row = optional + optional_size + index * 40
        _, rva, raw_size, offset = struct.unpack_from("<IIII", data, row + 8)
        mapped[rva : rva + raw_size] = data[offset : offset + raw_size]
    return mapped, optional


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def zstr(data, offset):
    return bytes(data[offset : data.index(0, offset)]).decode("ascii")


def main():
    game, _ = pe_image(sys.argv[1])
    plugin, optional = pe_image(sys.argv[2])
    target = Path(sys.argv[3])
    target.mkdir(exist_ok=True)
    base = 0x12000000
    delta = base - u32(plugin, optional + 28)
    reloc, reloc_size = struct.unpack_from("<II", plugin, optional + 96 + 5 * 8)
    stop = reloc + reloc_size
    while reloc < stop:
        page, block_size = struct.unpack_from("<II", plugin, reloc)
        for offset in range(reloc + 8, reloc + block_size, 2):
            kind, = struct.unpack_from("<H", plugin, offset)
            if kind:
                assert kind >> 12 == 3
                at = page + (kind & 0xFFF)
                struct.pack_into("<I", plugin, at, (u32(plugin, at) + delta) & 0xFFFFFFFF)
        reloc += block_size
    exports = u32(plugin, optional + 96)
    names_count, functions, names, ordinals = struct.unpack_from("<IIII", plugin, exports + 24)
    definitions = [f"#define DLL_BASE 0x{base:x}u"]
    export_names = set()
    for index in range(names_count):
        name = zstr(plugin, u32(plugin, names + 4 * index))
        ordinal, = struct.unpack_from("<H", plugin, ordinals + 2 * index)
        definitions.append(f"#define {name}_ADDR 0x{base + u32(plugin, functions + 4 * ordinal):x}u")
        export_names.add(name)
    assert export_names == {"XenoPlugin_Initialize", "XenoPlugin_Query"}
    imports = u32(plugin, optional + 104)
    lookup, _, _, dll_name, iat = struct.unpack_from("<IIIII", plugin, imports)
    assert zstr(plugin, dll_name).lower() == "kernel32.dll"
    import_names = set()
    index = 0
    while entry := u32(plugin, lookup + 4 * index):
        name = zstr(plugin, entry + 2)
        definitions.append(f"#define {name}_IAT 0x{base + iat + 4 * index:x}u")
        index += 1
        import_names.add(name)
    assert import_names == {"VirtualProtect", "FlushInstructionCache", "GetModuleHandleExW"}
    assert not any(plugin[imports + 20 : imports + 40])
    (target / "game.bin").write_bytes(game)
    (target / "plugin.bin").write_bytes(plugin)
    (target / "fixture.h").write_text("\n".join(definitions) + "\n")
    (target / "blobs.S").write_text(
        '.section .rodata\n.globl game_data, game_end, plugin_data, plugin_end\n'
        f'game_data: .incbin "{target / "game.bin"}"\ngame_end:\n'
        f'plugin_data: .incbin "{target / "plugin.bin"}"\nplugin_end:\n'
        '.section .note.GNU-stack,"",@progbits\n'
    )


if __name__ == "__main__":
    main()
