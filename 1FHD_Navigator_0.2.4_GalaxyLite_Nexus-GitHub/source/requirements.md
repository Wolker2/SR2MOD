# Requirements and boundaries

- Name: FHD_Navigator. Display approximate travel days when manually plotting
  a route on the stock Galaxy map, including added/removed intermediate points.
- Exact QuestMarker 0.3.12 frame bytes, parent, coordinates, size, font and depth.
- No runtime DLL, fixed game addresses, binary hash lock or executable rewrite.
- Read Distance/PathDesPos and named gs_<ID> markers through UICheckElement.
- Observe clicks through a nonblocking UI GraphButton child. Keep native input.
- Track the complete manual route and cross-check its parsec sum after each edit.
- Do not alter orders, ships, items, fuel, game turns or the native route.
- No global per-turn galaxy scans; work only on map interaction events.
- Scope: manual route edits. Programmatic paths and coincident map pixels cannot
  always be reconstructed and must not be assigned an invented ETA.
- ETA: per-hop hyperspace countdown with active accelerator, plus straight-line
  estimates inside systems. Actual flight accuracy and Windows gameplay need
  a real game test. This is a TEST release.

- Account for fuel, tank capacity, standard black-goo regeneration, accessible
  refueling ports; landing/takeoff per stop and one docked initial takeoff.
- Observer must survive native Galaxy.OnClose clearing Map children.
