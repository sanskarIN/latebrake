# Road-return HUD and inbound AI braking QA (v0.5.61)

> **Source development only.** Neither Unreal Engine 5.8 Editor/UHT, a
> cookable binary circuit map, installed Windows/Android packages nor real
> Chaos physics and mobile HUD tests were performed by portable CI.

## Off-track detection and nearest paved road

The HUD now runs `OffTrackIndicator` alongside the existing wrong-way
warning. Its route geometry is shared with
`Source/Latebrake/Public/Track/CircuitLayout.h`:

- Horizontal paved roads: X within ±4000 cm, Y within ±800 cm of the
  north/south road centerlines.
- Vertical paved roads: Y within ±2600 cm, X within ±800 cm of the
  east/west road centerlines.
- `AdviseReturnToCircuit` clamps a position against these four paved
  rectangles, picks the nearest resulting point, and reports
  **direct displacement in meters** plus a normalized world-space
  direction. This is not pathfinding, a guaranteed collision-free
  return route, or a replacement for the player's paid RECOVER function.
- `OffTrackIndicator` requires at least **0.8 seconds** of continuous
  off-pavement samples. It resets immediately on pavement, invalid
  position, menu/pause/focus loss, wrong pawn, new session or teleport.
  A hitch larger than 0.25 seconds cannot fabricate elapsed alert time.
- The HUD renders `OFF TRACK | NEAREST PAVED ROAD ...` (expanded) or
  `OFF TRACK | ROAD ...` (compact). Warnings do not call checkpoint,
  racing results, medal, credits or save APIs.

Tests: `Tests/Standalone/OffTrackAdvisorTests.cpp`, with real
`IsOnPavedSurface` geometry, all four shoulders, diagonal/outside
positions, numerical boundaries, NaN/±Infinity/overflow, resets and
100,000 tick saturation. The integration test binds the actual HUD and
source CMake target.

## Rival corner braking corrected

v0.5.60's speed-based corner anticipation originally used radial
distance alone. A car **past** the target gate or at an incorrect lateral
position could be within the radial window and trigger early braking.
The corrected `AnticipateRivalCorner` now:

1. Requires the rival position to be on the actual paved rectangles.
2. Projects `corner - position` onto the previous-gate→corner segment.
3. Restricts the inbound remaining distance to 440–2200 cm.
4. Restricts cross-track deviation to 620 cm.
5. Preserves the primary pilot's existing steering, stop, collision,
   motor and verified recovery accounting decisions.

The target speed now depends on the **inbound projection** instead of
radial proximity. New tests cover all four corners, aligned approach,
lateral misalignment, driving beyond the gate and off-road cases.

## Verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(OffTrackAdvisor|RivalCornerAnticipation|WrongWayIndicator)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

These checks run only portable C++20 and static Unreal-source integration
contracts, **not actual Unreal movement or compiled HUD**.

## Installed game acceptance gates

1. On authorized UE 5.8, compile game/editor targets and use a serialized,
   cooked, original four-road circuit map. Check source/engine/map hashes.
2. Drive on pavement, all four shoulders and each corner. Confirm no
   warnings on pavement, delayed off-track detection and immediate reset
   after returning to the road. Inspect exact nearest-road direction.
3. Recover/RESTART/PAUSE or background the app while off-road; verify
   the HUD warning clears without changing lap/score/race penalties.
4. On Android compact view, inspect warning clipping/occlusion against
   steering, GAS/BRAKE, navigation and wrong-way hints at multiple sizes.
5. Play against the rival and record steering/brake/speed traces near
   each corner, including a post-gate overshoot and track escape.
   Verify no false automatic lap, paid teleport or finish.
6. Capture reproducible Windows/Android build and gameplay evidence in
   [Android qualification issue #13](https://github.com/sanskarIN/latebrake/issues/13).

No playable or stable release should be claimed until these gates pass.
