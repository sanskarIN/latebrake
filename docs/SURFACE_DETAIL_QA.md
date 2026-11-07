# Circuit surface-detail source QA (November 7, 2026)

**Status: draft original Unreal 5.8 C++ source; not a playable build.**

## Purpose

The old controller-edition Win32/GDI executable is a separate **2D preview**
and is not the Unreal game. A font/skin change cannot turn it into a
photorealistic 3D racer. This source feature starts improving the real 3D
course instead of mislabeling that 2D preview as the production game.

The prototype's authoritative physical circuit remains nine actual static
collision blocks and four ordered checkpoint gates. This optional road-surface
pass adds eight continuous **straight-only white edge lines** and 48 narrow
**braking-zone rubber marks** before the four real gates. The 56 thin 3D cube
instances share only two instanced-mesh components and one optional holder
actor. All placements are determined by the same physical collision plan;
there is no misleading curved extra road or off-asphalt mark.

The bottom of each visual cube is at z=-0.95 cm, within 0.05 cm of the
existing asphalt's z=-1 cm collision top. The complete axis-aligned cube
footprint, not merely the center, must fit entirely inside one of the
four collision-bearing asphalt slabs. Invalid, nonfinite, floating, buried
and oversized marks fail closed before instancing.

**Safety boundaries:** no physical collisions, overlaps, navigation,
shadows, scoring, recovery, vehicle acceleration, race events or save changes.
If the mesh, actor, component or complete instance budget is unavailable,
the cosmetic holder is destroyed and the race still proceeds. A structured
ready/incomplete source log distinguishes instantiation success from actual
rendered pixels. Colors are intended white/rubber gray, but stock engine
materials may ignore the requested `Color` parameter; genuine cooked
PBR materials, lighting and real UE visual inspection are still required.

## Automated scope

- Compile `Tests/Standalone/CircuitSurfaceDetailTests.cpp` with strict
  C++20 warnings as errors; confirm all 56 mark rectangles lie fully inside
  actual asphalt, exact counts, heights, opposing sides and invalid inputs.
- Run `Tests/Editor/UnrealCircuitSurfaceDetailBindingTests.py` to verify
  the GameMode calls the real geometry plan and does not grant collision
  or checkpoint authority.
- No tests in ordinary GitHub Actions run the licensed UE5.8/UHT engine.

## Real Windows 3D qualification required

On a private authorized UE 5.8 Windows runner, compile the Editor target,
author and cook the real level, launch the packaged Win64 game. Inspect
marking alignment on the four straights and four braking approaches at
30/60/120 FPS from the chase camera, including night/day lighting,
wet/dry presentation if eventually implemented, car grip and tire
ground contact. Verify boundaries visually and from installed logs:
`LATEBRAKE_SURFACE_DETAIL_READY marks=56/56 groups=2/2 collision=off`.

A cooked asset, real manufacturer-independent 3D car model, realistic
asphalt/grass PBR material, lighting, curved course and installed QA are
**separate unfinished work**. Do not release this source milestone as a
downloadable realistic Windows racing game without that evidence.
