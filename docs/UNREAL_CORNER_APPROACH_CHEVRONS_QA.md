# Corner approach chevrons — original Unreal circuit QA

**Source-only feature.** These are visible, low-profile arrow-shaped paint
markings **proposed for the existing Unreal Engine 5.8 circuit**.
They have not been visually checked in the licensed Editor, cooked and
installed on Windows, or tested on an Android phone.

## Feature

- Four directional approaches from the authoritative
  `CircuitLayout.h` positions: east, north, west, south.
- Three chevrons per approach, nominally 16m, 11m and 6m from the
  corresponding directional checkpoint plane.
- Two diagonal arms per arrow (24 total pieces). The arrow points
  **forward in the driving direction** on the inside-left portion of
  the paved lane. Braking reference boards remain opposite on the
  right edge of the existing track.
- Each visual bar is a 190 × 19 × 3 cm stock cube instance, above the
  asphalt top, in one non-colliding `UInstancedStaticMeshComponent`.
- A light cyan material parameter is attempted, with the default cube
  material as fallback. Material color/rendering must be verified
  inside Unreal, not assumed from source alone.

These markings are an approximate visual route cue, **not** a racing
line, official turn-in or braking point, recommended safe speed,
collision shape, or evidence of a passed gate. They do not change
car/rival behavior, checkpoints, recovery, physics, AI, starting
eligibility, rewards or saved progress. A missing mesh or partial
visual marker logs `LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE` without
blocking race startup. A complete source-side placement logs
`LATEBRAKE_CORNER_CHEVRONS_READY`; neither log certifies rendering.

## Source tests

- `Tests/Standalone/CornerApproachChevronsTests.cpp`: 24 valid
  deterministic transforms, four gate directions, 3 × 2 per corner,
  sane dimensions, pavement clearance, correct 16/11/6m ordering
  and fail-closed invalid indices.
- `Tests/Editor/UnrealCornerApproachChevronsBindingTests.py`:
  verifies the **actual** Unreal GameMode consumes the pure plan,
  uses a collision-disabled instance component, and reports
  incomplete geometry rather than manipulating race readiness.
- GitHub source CI compiles these tests with strict C++20 on Windows,
  Ubuntu and macOS; Python checks never compile Unreal.

## Real Unreal and device QA (not yet completed)

1. Compile the project in genuine Unreal Engine 5.8/UHT, open the map
   and verify exactly 12 forward-pointing V arrows (24 arm pieces).
2. Drive all four corners; visually check correct direction, spacing,
   leg joining, visibility from the player's actual driving camera,
   no Z-fighting, no sunk or floating markings, no curb overlap.
3. Inspect default material and optional cyan color parameter under
   installed/cooked Win64 and Android rendering and different light.
4. Confirm steering, brake boards, checkpoint acceptance, collisions,
   recovery, AI rival following, start-lights and rewards remain
   unchanged. No spurious overlap or navigation blocking.
5. Record exact commit SHA, real engine version, hardware, startup logs,
   screenshots and frame-time observations.

See [Windows personal playtest](WINDOWS_PERSONAL_PLAYTEST.md)
and [playable release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
