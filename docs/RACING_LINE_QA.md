# Original circuit racing line — source candidate

**Status: Unreal source implementation only, NOT an installed or visually
verified game.** This guide describes a cosmetic reference line for the
existing single original counter-clockwise circuit. It neither provides a genuine
racing-school driving recommendation nor replaces authored production track
materials, track layouts or car art.

## What changed

- The real Unreal `ALatebrakeRaceGameMode::BuildCircuit` requests one
  optional guide actor, made from **two instanced static mesh components**:
  long dashed straight references and shorter, tangent-aligned corner marks.
- Exactly **88 planned cube instances** cover four sections, each containing
  12 straight and 10 corner dashes. The layout comes from the same
  `CircuitLayout.h` road coordinates as the actual circuit and gates.
- The guide is inset 250 cm towards the centre of the circuit to avoid the
  existing central road stripes. Four circular quarter-turns use a 650 cm
  guide radius. Ground elevation is based on the existing asphalt slabs;
  actual installed visual alignment has not yet been measured.
- Optional material tint requests teal on straight marks and amber on turns.
  Engine-supplied materials may ignore the `Color` parameter and render
  white. Artwork and legibility must therefore be checked in real Unreal.
- This does **not** affect track physics, AI drive path, road traction,
  authoritative checkpoints, lap times, recovery, race credits, save data,
  existing nine collision-bearing blocks, or the four-gate startup audit.

If the visual actor cannot be allocated, it is discarded and gameplay
authorization stays independent. The actor disables collision and navigation,
overlap generation and dynamic shadows. No game asset downloads are required.

## Source verification

From the repository root:

```sh
cmake -S Tests/Standalone -B build/racing-line -DCMAKE_BUILD_TYPE=Debug
cmake --build build/racing-line --target latebrake_racing_line_tests
ctest --test-dir build/racing-line -R '^RacingLineLayout$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'UnrealRacingLineBindingTests.py' -v
```

The portable C++20 regression checks all 88 transforms are finite,
on-pavement and forwards, rejects invalid indices, and verifies straight and
corner counts and representative heading positions. Python verifies the
real Unreal C++ source **refers to** the geometry and prohibits gameplay
mutation in the optional visual function. **Neither suite compiles Unreal.**

## Required Unreal 5.8 / Win64 acceptance before merging as verified art

- [ ] Genuine UnrealHeaderTool and Editor/Game C++ compile, via the authorized
      manual same-repository PR validation gate on an owner-controlled runner.
- [ ] Cook a real engine-authored starter map and install/launch Win64.
- [ ] Visually inspect all four corners from the default camera and the high
      chase preset; no floating/drowned marks, severe Z fighting or unreadable
      curves. Capture screenshots/video with package SHA and settings.
- [ ] Check 720p/1080p/high-DPI visibility, colorblind distinguishability,
      material fallback, frame times and shadows on tested hardware.
- [ ] Drive in both directions, stop over marks, collide with outer walls,
      recover and restart; cosmetics must not alter road contact, steering,
      rival path, checkpoints or rewards.
- [ ] Repeat a complete race and time attack with missing optional mesh
      or forced guide allocation failure; no false circuit-ready state,
      missing race clock or startup crash.

Unverified geometry source does not create a new Windows EXE, Android APK,
multi-GB art pack, professionally authored car model or stable release.
Actual playable qualification remains in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148).
