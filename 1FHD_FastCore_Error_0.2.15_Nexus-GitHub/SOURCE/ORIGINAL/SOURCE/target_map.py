#!/usr/bin/env python3
"""Verify the reference EXE and regenerate the C guard table."""
import hashlib
from pathlib import Path
import struct
import sys

TARGET_SHA256 = "e43a4b15503c705eebe2d8ce5778c2a2fb090db515a3fd86d319e5c5aa692ebf"
BASE = 0x400000
GUARDS = {
    "RECOVERY_SCRIPT_TURN_CODE": (0x64A12F, 0x64A150),
    "RECOVERY_SCRIPT_VM": (0x46CB84, 0x46CC02),
    "CLEANUP_MISSILES_OPEN_SCENE": (0x7E2619, 0x7E2640),
    "CLEANUP_MISSILES_REFRESH_SCENE": (0x7E27C1, 0x7E27E8),
    "RECOVERY_MISSILE_SCENE_FACTORY": (0x782B4B, 0x782B6C),
    "COUNTABLE_UNIT_SIZE": (0x707680, 0x707718),
    "COUNTABLE_MERGE": (0x7079AC, 0x7079E3),
    "COUNTABLE_PICKUP_NEAR": (0x7FC536, 0x7FC554),
    "COUNTABLE_PICKUP_PULL": (0x7FC884, 0x7FC89F),
    "COUNTABLE_PICKUP_NATIVE": (0x7F4F5C, 0x7F4FBF),
    "CLEANUP_SAVED_WORLD": (0x7D2DFF, 0x7D2E4A),
    "CLEANUP_SAVED_IMAGE": (0x7D384D, 0x7D3879),
    "CLEANUP_NATIVE_IMAGE_LOAD": (0x800250, 0x8002EF),
    "CLEANUP_COMBAT_SHOT": (0x7F98BD, 0x7F98DE),
    "CLEANUP_COMBAT_CONTINUE": (0x7FA075, 0x7FA084),
    "BRIDGE_HIDE": (0x558634, 0x5586E4),
    "BRIDGE_SELECT": (0x5586EB, 0x55874C),
    "RECOVERY_MESSAGE_CALL": (0x5E64D8, 0x5E64E8),
    "RECOVERY_MESSAGE_LOAD": (0x7A5540, 0x7A5644),
    "RECOVERY_GI_SUFFIX": (0x7CC2A4, 0x7CC2C0),
    "RECOVERY_CLEAR_WIDE": (0x405D18, 0x405D2D),
    "RECOVERY_SCENE_FACTORY": (0x782914, 0x782A43),
    "RECOVERY_SHIP_FRAME": (0x6C3C14, 0x6C3C32),
    "RECOVERY_STATION_VISUAL": (0x6C43DF, 0x6C441C),
    "RECOVERY_SHIP_VISUAL": (0x6C4474, 0x6C44A0),
    "RECOVERY_PLANET_LOAD": (0x72E155, 0x72E1B0),
    "RECOVERY_PLANET_FACTORY": (0x782914, 0x782A23),
    "RECOVERY_GRAPH_CONFIG": (0x786584, 0x7865FD),
    "RECOVERY_GRAPH_LOOKUP": (0x7865F3, 0x78660B),
    "RECOVERY_SCRIPT_LOOP": (0x7D2B7E, 0x7D2C1E),
    "RECOVERY_SCRIPT_SERIALIZATION": (0x64F04C, 0x64F738),
    "RECOVERY_SCRIPT_STATE": (0x64F738, 0x6501DE),
    "RECOVERY_CACHE_PATH": (0x4D1934, 0x4D19D4),
    "RECOVERY_SCRIPT_FINISH": (0x7D4340, 0x7D435A),
    "RECOVERY_SCRIPT_FINISH_ORIGINAL": (0x7D2064, 0x7D2068),
    "ART_RECOVERY_LOAD": (0x70F818, 0x70F8FA),
    "ART_RECOVERY_CONFIG": (0x70F4F4, 0x70F535),
    "ART_RECOVERY_BITMAP": (0x70E6F0, 0x70E7C7),
    "RECOVERY_EXTRA_MODULE": (0x6F764C, 0x6F7770),
    "RECOVERY_READ_MODULE": (0x6F7310, 0x6F7387),
    "WARMUP_CODE": (0x5BF384, 0x5BF3FB),
    "POLYGON_CONTAINS": (0x7CF22C, 0x7CF2F1),
    "POLYGON_FIND": (0x7CF328, 0x7CF357),
    "POLYGON_ASSIGN": (0x7CF358, 0x7CF38D),
    "POLYGON_PLACEMENT": (0x7E4B60, 0x7E4B7D),
    "POLYGON_SEED": (0x7E3D96, 0x7E3DA7),
    "POLYGON_GROWTH": (0x7E3F74, 0x7E3F87),
    "FACTION_COUNT": (0x7EAD18, 0x7EAD6C),
    "FACTION_TLIST_GET": (0x416368, 0x41638F),
    "FACTION_WIDESTRING_COMPARE": (0x406044, 0x4060C5),
    "FACTION_RATIO": (0x7F0B70, 0x7F0BB9),
    "RANGER_RATING": (0x7EAB14, 0x7EAC66),
    "AVERAGE_WEALTH": (0x7EA7E0, 0x7EA986),
    "AVERAGE_STRENGTH": (0x7EA988, 0x7EAAEF),
}

VISUAL_CALLS = {
    "AWARD_TEXT": (0x7CC250, (0x742E12, 0x742E57)),
    "STATION_BACKGROUND": (0x55754C, (0x55854B, 0x56E7F5, 0x56E985)),
    "STATION_IMAGE": (0x499014, (0x558564, 0x56E80E, 0x56E99E)),
    "STATION_SHOPS": (0x499014, (0x50E490, 0x50E4DF, 0x67F27B, 0x67F2BE)),
    "STATION_HANGAR_INFO": (0x499014, (0x5FF0BB, 0x5FF0FE, 0x58C9F9, 0x58CA3C)),
    "ITEM_ICONS": (0x494834, (0x58E994, 0x5978B9)),
    # The equipped-module overlay calls belong to the FHDUU compatibility
    # surface and are excluded from the native graphics patches.
    "SHOP_ITEM_IMAGES": (0x499014, (0x68025C, 0x682458, 0x6839D1)),
    "SHIP_EQUIPMENT_IMAGES": (0x499014, (0x6A9F57, 0x6AAC8A)),
    "SHIP_HOLD_IMAGES": (0x499014, (0x6AB18B, 0x6AB1F6, 0x6AB3E5, 0x6AB453)),
    "SHIP_DETAIL_IMAGES": (0x499014, (0x6BAC1E, 0x6BACEC, 0x6BD674)),
    "SCANNER_ITEM_IMAGES": (0x499014, (0x518251, 0x518B2B, 0x518F43, 0x51A315)),
    "ITEM_ANIMATIONS": (0x496454, (0x68033A, 0x6AA0E1, 0x5183EE)),
    "SHIP_ITEM_CURSORS": (0x4C5D78, (0x6AD877, 0x6AD911)),
    "AWARD_STRIPS": (0x494834, (0x5D530C, 0x6A7309, 0x515D5B)),
    "AWARD_HINTS": (0x494834, (0x5D3FA9, 0x6A77A7, 0x516250)),
    "AWARD_DISPLAY_PATHS": (0x5D07C4, (0x5D0E0E, 0x5D0E55, 0x5D12F3, 0x5D14FC)),
    "CAPTAIN_SHIP": (0x499014, (0x6A5899,)),
    "CAPTAIN_SHIP_ANIM": (0x496454, (0x6A590A,)),
    "CAPTAIN_NEW_GAME": (0x499014, (0x5C73AC,)),
    "CAPTAIN_NEW_GAME_ANIM": (0x496454, (0x5C7474,)),
    "CAPTAIN_TALK": (0x499014, (0x5EAA4B,)),
    "CAPTAIN_TALK_ANIM": (0x496454, (0x5EAB0A,)),
    "CAPTAIN_SCANNER": (0x499014, (0x5150CE,)),
    "CAPTAIN_SCANNER_ANIM": (0x496454, (0x515147,)),
    "CAPTAIN_HANGAR": (0x499014, (0x5FF3DA,)),
    "CAPTAIN_HANGAR_ANIM": (0x496454, (0x5FF442,)),
    "CAPTAIN_MARKET": (0x499014, (0x50DDEB, 0x50DF67)),
    "CAPTAIN_MARKET_ANIM": (0x496454, (0x50DE6E, 0x50DFD8)),
    "CAPTAIN_RATING": (0x499014, (0x5D6550, 0x5D761D, 0x5D783F, 0x5D7A5E)),
    "CAPTAIN_RATING_ANIM": (0x496454, (0x5D6645, 0x5D7682, 0x5D78A4, 0x5D7AC3)),
    "CAPTAIN_SCORE": (0x499014, (0x68EE82,)),
    "CAPTAIN_SCORE_ANIM": (0x496454, (0x68EF42,)),
}


def main():
    executable = Path(sys.argv[1]).read_bytes()
    if hashlib.sha256(executable).hexdigest() != TARGET_SHA256:
        raise SystemExit("Rangers.exe SHA-256 does not match the supported executable")
    pe_offset, = struct.unpack_from("<I", executable, 0x3C)
    section_count, = struct.unpack_from("<H", executable, pe_offset + 6)
    optional_size, = struct.unpack_from("<H", executable, pe_offset + 20)
    section_table = pe_offset + 24 + optional_size
    sections = []
    for i in range(section_count):
        _, rva, raw_size, offset = struct.unpack_from(
            "<IIII", executable, section_table + 40 * i + 8
        )
        sections.append((BASE + rva, raw_size, offset))

    def read_span(start, stop):
        for address, raw_size, offset in sections:
            if address <= start < stop <= address + raw_size:
                return executable[offset + start - address : offset + stop - address]
        raise ValueError(f"unmapped executable address {start:#x}")

    def guard(start, stop):
        fingerprint = 2166136261
        for byte in read_span(start, stop):
            fingerprint = ((fingerprint ^ byte) * 16777619) & 0xFFFFFFFF
        return f"{{0x{start:x}u, {stop - start}u, 0x{fingerprint:08x}u}}"

    print("/* Generated by target_map.py for PE timestamp 6AC53B97. */")
    for name, (start, stop) in GUARDS.items():
        print(f"#define {name}_GUARD {guard(start, stop)}")
    for name, (target, sites) in VISUAL_CALLS.items():
        entries = []
        for site in sites:
            call = read_span(site, site + 5)
            assert call[0] == 0xE8 and site + 5 + struct.unpack_from("<i", call, 1)[0] == target
            entries.append(f"{{0x{site:x}u, {guard(site - 9, site + 9)}}}")
        print(f"#define {name}_CALLS " + "{" + ", ".join(entries) + "}")


if __name__ == "__main__":
    main()
