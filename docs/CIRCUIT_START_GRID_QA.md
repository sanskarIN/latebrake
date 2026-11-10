# Six-slot original-circuit starting grid — source and UE QA

**Source-only draft.** This candidate defines one existing human slot, the
existing physical rival slot, and four additional *future* slots. Only the
current player and **one AI rival** are instantiated by the real GameMode.
Do not describe this candidate as six-car multiplayer, six active rivals,
verified physical spacing or a playable release.

## Implemented in source

- One shared fixed grid array in original Unreal-centimetre coordinates
  reuses the **unchanged** PlayerStart and RivalStart as slots 0 and 1.
  The current GameMode obtains the live player and one rival start poses
  from these two shared slots, including retry/restart. No game mode,
  race clock, checkpoint, recovery, save, garage or opponent count changes.
- Four **future** positions are arranged two-abreast behind the existing
  pair, still on the physical four-asphalt-slab south straight.
- A fail-closed, allocation-free portable preflight uses stock chassis
  dimensions 380 x 180 cm plus 45 cm per side. Four padded chassis corners
  must be on the original paved surface; rotated-rectangle separating-axis
  checks must clear the original physical barriers and other proposed slots.
- Invalid slots, NaN/infinite coordinates and impossible rotations are
  rejected; touching safety margins are conservatively rejected.
- The new tests exercise the actual geometry and native GameMode's
  source bindings. This is not an Unreal collision query.

## Run local portable verification

    cmake -S Tests/Standalone -B Tests/Standalone/build
    cmake --build Tests/Standalone/build --parallel
    ctest --test-dir Tests/Standalone/build -R '^CircuitStartGrid$' --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p 'CircuitStartGridBindingTests.py' -v

## Required engine qualification before any full-grid expansion

1. Build and run a genuine UE5.8 Editor/UHT/Game target on the exact branch
   head. Open the real owned circuit and verify stock car shell/ground contact.
2. Inspect the entire padded 6-slot footprint against **actual** cooked
   road, physical barriers, height, lighting, camera sight lines and starts.
   Source geometry is NOT proof of installed actor clearance.
3. In a separate future PR, add extra physical AI only with a per-opponent
   identity for gate observation, independent route timers, safe per-car
   recovery destinations, rival-rival traffic braking and bounded CPU cost.
   Do **not** count unspawned candidate slots as racing participants.
4. Once a real multi-AI build exists, validate six grid launches at 30/60/
   120 FPS and all directions (restart, reverse, spin, support loss, crashes)
   in installed Win64. Record real footage and worst-frame data.

The latest published source-only version is still v0.5.97.
Keep [playability gate #148](https://github.com/sanskarIN/latebrake/issues/148)
open until its actual licensed-engine and installed-package gates pass.
This branch does not change the pinned v0.5.98 physics candidate or merged
main. It must remain a draft until qualified.
