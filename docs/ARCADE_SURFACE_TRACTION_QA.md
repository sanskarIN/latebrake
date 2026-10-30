# Arcade road-surface traction — source-only prototype QA

**Source-only, not playable.** This milestone uses an existing Unreal
single-rigid-body vehicle and the existing four-strip authored circuit
footprint. This is an **arcade paved/off-pavement effect**, not a true
wheel/tire contact model, off-road material detection, suspension, grass
collision, gravel particle effect or actual Unreal Engine 5.8 QA.

## Actual vehicle behavior

- Both the player pawn and the existing physical AI rival read the same
  measured `GetActorLocation()` XY coordinates after the authoritative
  race start, complete circuit, valid Chaos body and finite velocity
  gates. No key press, artificial car location or HUD suggestion can
  declare pavement. The classification calls the existing
  `IsOnPavedSurface` road geometry used by the route-advice system.
- It takes **0.15 seconds of continuously observed off-road samples**
  to apply reduced traction. Re-entry requires **0.20 seconds of
  continuously observed pavement** before restoring baseline forces.
  Brief road-edge sensor chatter must not cause a single-frame motor
  or lateral grip snap. Durations are based on accepted finite frame
  intervals of 0 < dt <= 0.10 seconds, not frame counts.
- Confirmed off-pavement driving multiplies the **existing** base motor
  force, active rechargeable nitro addition and qualified slipstream
  addition by **0.78**, while multiplying the **existing** lateral
  damping rate by **0.72**. The existing race-local tire wear/grip
  multiplier still applies to the same bounded lateral resistance.
- On pavement both multipliers are exactly **1.0**, leaving all
  previous acceleration, drafting, nitro, steering and tire behavior
  unchanged. There is **no new braking impulse** and no change to the
  active vehicle speed cap. Existing frame-stable resistance protection
  remains the final authority.
- A malformed/too-large frame, unready driving state, nonfinite or
  implausibly distant XY coordinate fails closed. Unreal neutralizes
  the driver rather than granting a default road acceleration.
  Pause, app deactivation, focus loss, restart and recovery clear
  pending traction state, so no prior off-road sample crosses an
  authorized teleport or race/session boundary.
- The previous off-track HUD warning remains an **independent
  directional advisory**, and may appear before the traction effect
  confirms. It does not score gates or change official timing, career
  credits, records, saved profiles, rival path planning or
  championship results.

## Portable source regression

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^ArcadeSurfaceTraction$' --no-tests=error --output-on-failure
python3 -m unittest discover -s Tests/Editor -p ArcadeSurfaceTractionBindingTests.py -v
```

The C++20 test checks actual authored gate/start-road points, exact
road-strip boundaries, sustained off-road/return intervals, transient
boundary chatter, pause/invalid physics, NaN/Infinity, impossible
positions, 1,000 session resets and **250,000 deterministic source
samples**. Python source-binding tests check measured position and
physics gates precede all motor/boost/lateral forces, no existing
bonus bypasses the same off-pavement multiplier, and both player/rival
pawns own/reset their own state. These tests use no Unreal SDK.

## Mandatory real Unreal and installed-device review

1. Build the *licensed* Unreal Engine 5.8 Editor/UHT and Game targets
   and cook an original circuit map. Do not substitute these portable
   tests for a genuine Win64 EXE or Android APK.
2. Drive both available cars in the four paved straights, near exact
   road edge and progressively across the shoulder. Watch the
   transition at 30, 60 and 120 FPS and in variable frame time, using
   a real car chassis, live forces and real collisions.
3. Compare off-road, on-road and edge-reentry throttle, turning
   response, nitro, slipstream and worn-tire handling. Verify it is
   possible to return to the track without a stuck/reversed force or
   fake collision claim; check rival recovery and checkpoint awards.
4. Test pause, controller unplug, app backgrounding, race restart,
   recovery, losing local player ownership, no landscape collision,
   invalid chassis physics and display scaling in real installed
   Win64/Android sessions. Check actual contact and steering.
5. Record actual engine/build hash, map identity, device, frame-rate,
   reproducible results, runtime logs and candidate package hashes.

**No official installed or playable racing package is produced by this
source-only feature.** The real playable gate is tracked in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148);
Android hardware acceptance remains in
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).
