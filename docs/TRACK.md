# Starter circuit specification — v0.4.0

## One canonical course

The active prototype remains **a single rectangular four-sector loop**. This update does not invent a second layout or claim a new track selection UI.

World distances are Unreal centimetres. The drivable road centerlines are at `Y = ±1800` on the east-west straights and `X = ±2800` on the north-south straights. Road half-width is `800 cm`. In order, checkpoints are east/south, east/north, west/north and west/south. Direction checks expect east, north, west and south travel respectively.

`Source/Latebrake/Public/Track/CircuitLayout.h` is the source of truth for those four gates, player/rival start positions and their recovery orientations. Both the Unreal game mode and the rival actor consume these common values. Player laps and recovery penalties have not been rebalanced.

## Visual course furnishings

`TrackDecorations.h` plans 28 white center dashes and 56 non-colliding decorative kerbs/curbs, alternating red and white, all on the existing asphalt. The runtime adds two primitive grandstands outside the drivable road and a visible overhead start/finish banner using only Unreal built-in cubes. Surface finish, materials, lighting, staging and camera angles remain engine-unverified.

Every individual cosmetic mesh currently spawns a separate Unreal static-mesh actor. **This may be expensive on mobile hardware.** A later profile-guided pass should batch repeated markings and scenery with `UInstancedStaticMeshComponent` or `UHierarchicalInstancedStaticMeshComponent`, after the underlying game can be built.

## Course overview map

The pure `CircuitMap.h` helper normalizes finite world locations to a north-up 0–1 coordinate space with clamping for out-of-track positions. In wide desktop HUD layouts, the racing HUD renders a small track loop, player's current position and the next required checkpoint. On compact and mobile viewports it is intentionally omitted to avoid clutter.

## Validation requirements

Portable Windows/Linux/macOS CMake suites check this geometry and math. **They cannot establish** that a UE 5.8 game builds, materials/colors render correctly, that curbs never obstruct driving, or that the minimap avoids all HUD overlap. Real Unreal Editor tests and Win64/Android playtesting are required before a playable/stable release.
