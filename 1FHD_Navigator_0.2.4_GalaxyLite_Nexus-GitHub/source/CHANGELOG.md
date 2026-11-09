## 0.2.4 GALAXYLITE_TEXT_RESTORE_TEST

- Separate route text owned by Navigator; QuestMarker text never changed.
- No disabling of Navigator label during frame ownership checks.
- Self-healing of cached route label on subsequent native map event.
- Dedicated overlay drawn above QuestMarker panel; no EXE/DLL hooks.

# 0.2.3 GalaxyLite TEXT FIX TEST

- Fixed missing route ETA text after the 0.2.2 frame de-duplication.
- If QuestMarker is present, show ETA in its existing visible `FHDQuestRouteInfo` label and hide the duplicate Navigator frame/label.
- Otherwise use the standalone `FHDNavigatorRouteFrame` and `FHDNavigatorRouteInfo` (label initially enabled; dock foreground layer -4).
- Include shared-label presence in the route state signature, so opening QuestMarker while a route is already selected refreshes ETA.
- Keep native event-based calculation, DLL-free, no EXE offsets or UI hooks.
- QuestMarker's mission-route label can be shared with Navigator; a concurrently shown mission summary may be replaced when manual route recalculates.
- Game playtest required.
