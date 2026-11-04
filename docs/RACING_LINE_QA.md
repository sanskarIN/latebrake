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
  guide radius. The thin 0.8 cm high marks are placed with their undersides
  just **0.25 cm above** the physical asphalt top. That top is derived from
  the real four collision-bearing slabs, not an independent hard-coded Z.
  Their actual in-engine appearance still needs real Win64 evaluation.
- Optional material tint requests teal on straight marks and amber on turns.
  Engine-supplied materials may ignore the `Color` parameter and render
  white. Artwork and legibility must therefore be checked in real Unreal.
- This does **not** affect track physics, AI drive path, road traction,
  authoritative checkpoints, lap times, recovery, race credits, save data,
  existing nine collision-bearing blocks, or the four-gate startup audit.

If the visual actor cannot be allocated, it is discarded and gameplay
authorization stays independent. The actor disables collision and navigation,
overlap generation and dynamic shadows. No game asset downloads are required.

## Optional player visibility

- On a normal, qualified local race, press **L** on the desktop keyboard
  to toggle the **entire** guide on/off. The expanded desktop HUD shows
  `RACING LINE ON/OFF | L TO TOGGLE`; this label is omitted from the
  compact/mobile HUD to preserve driving and checkpoint readability.
- If the optional actor fails to create, the non-compact desktop HUD
  reports `RACING LINE UNAVAILABLE` instead of misleadingly saying OFF.
- The preference begins **on**, persists only while the current world exists,
  and is never written to career/profile saves. It cannot award a lap,
  change driving inputs or alter any checkpoint, credit or result.
- A missing/incomplete guide actor is reported as absent; toggle input
  does nothing rather than creating a fictitious guide.
- Menu, paused game, focus suspension, manual pause, finished race or a
  missing local driver all reject toggle input. The physical rival cannot
  change the local player's guide setting.
- The key and source-level safety conditions are checked by the Python
  Unreal wiring suite. Real keyboard focus behavior and on-screen text
  placement must still be tested in Unreal/Win64.

## Source verification

From the repository root:

```sh
cmake -S Tests/Standalone -B build/racing-line -DCMAKE_BUILD_TYPE=Debug
cmake --build build/racing-line --target latebrake_racing_line_tests
ctest --test-dir build/racing-line -R '^RacingLineLayout$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'UnrealRacingLineBindingTests.py' -v
```

The portable C++20 regression checks all 88 transforms are finite,
on-pavement and forwards, rejects invalid indices, verifies the **full rotated
rectangle** of every stripe lies on a real collision-bearing asphalt slab,
and checks surface offset, exact 48/40 instance budgets, complete-loop
continuity and representative headings. Python verifies the
real Unreal C++ source **refers to** the geometry and prohibits gameplay
mutation in the optional visual function. **Neither suite compiles Unreal.**

## Required Unreal 5.8 / Win64 acceptance before merging as verified art

- [ ] Genuine UnrealHeaderTool and Editor/Game C++ compile, via the authorized
      manual same-repository PR validation gate on an owner-controlled runner.
- [ ] Cook a real engine-authored starter map and install/launch Win64.
- [ ] Visually inspect all four corners from the default camera and the high
      chase preset; no floating/drowned marks, severe Z fighting or unreadable
      curves. Capture screenshots/video with package SHA and settings.
- [ ] In the real Unreal Development log confirm
      `LATEBRAKE_RACING_LINE_READY marks=88/88 straight=48 corner=40 collision=off`.
      On injected optional failures, expect INCOMPLETE and a running race;
      neither log string proves that actual materials rendered correctly.
- [ ] Test the L key before GO, during a valid lap, after pause/resume,
      after a finished race and after restart. Confirm visibility is
      session-local, no SaveGame write occurs, and the optional HUD row
      does not hide timing or checkpoint information.
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
