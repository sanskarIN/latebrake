# Original-circuit braking reference boards — Unreal source QA

**Status: source implementation; unverified in Unreal 5.8.** No new
track or official Win64 build is claimed.

LateBrake's single original rectangular four-corner circuit now has
an asset-free proposal for **24 vertical braking-reference bars**
(6 at each turn). An approaching car encounters a group of **three bars
at about 12 metres before the directional gate**, a pair at 8 metres,
and a single bar at 4 metres. The markers stand on the *right-hand
verge* of each clockwise approach and use the same direction order as
the four physical gate traces. They are not exact real-world braking
distances, speed recommendations or physics helpers.

All boards are original procedural transforms built from the existing
Unreal Engine cube shape, with one `UInstancedStaticMeshComponent` per
optional holder actor for low component overhead. There is **no
collision or overlap**, no light shadows or navigation influence.
Marker placement has no impact on the canonical nine collision blocks,
four gate actors, car forces, recovery zones, off-track detection,
timing, AI, medals or SaveGame. A cosmetic mesh/actor failure generates
an optional log warning; it must not block racing or award readiness.

**Portable checks:** C++20 `BrakingReferenceBoardsTests.cpp` checks
all 24 unique placements against the canonical `CircuitLayout.h`
coordinate source, 3/2/1 zone count, correct travel facing/side,
whole-board paved width clearance and fixed 12/8/4m locations.
Python source-binding tests check the *actual Unreal GameMode* creates
a collision-disabled instanced mesh holder and logs success/failure.
Neither suite compiles Unreal Engine.

**Real engine QA before playable claim:**
1. Run genuine UE 5.8 Editor and packaged Game build/UHT.
2. Drive in all four directions and see all 3/2/1 boards in their
   correct approach order without blocking traffic.
3. Check the boards are above and inside the existing asphalt, not
   sunk into it, floating, duplicated or too close to curbs/barriers.
4. Verify the default white material/optional colour parameter is
   visible in actual cooked Win64 and Android rendering.
5. Confirm no frame-time spikes or unwanted collisions and capture
   installed-game screenshots, engine version, commit SHA and logs.
6. Recheck restart, all original gates and career Time Attack/Quick
   Race; this source-only update does not qualify a playable release.

See [release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
