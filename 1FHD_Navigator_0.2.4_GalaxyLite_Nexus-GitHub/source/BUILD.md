# FHD_Navigator v0.2.4 GalaxyLite text restore

1. `python build.py` builds the runtime mod ZIP.
2. `python tests/verify_galaxylite.py` runs portable static checks.
3. The runtime bundle installs in `Mods/FHD_MODS/FHD_Navigator` and does not include QuestMarker or AModGalaxyLite.
4. The original AModGalaxyLite and QuestMarker packages were also compared using five merge-order static tests. No GUI playtest has been performed.

No DLL or EXE bindings are required by the Navigator runtime.
