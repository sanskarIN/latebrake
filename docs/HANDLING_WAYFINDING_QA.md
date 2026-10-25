# Corner traction assist and wrong-way HUD — v0.5.53 source development

> **Not a playable release or Unreal-qualified handling.** The two new
> policies are compiled by portable Windows/Linux/macOS C++20 tests and
> source-wired into the Unreal pawn and HUD. They still require a licensed
> UE 5.8 compile, cooked starter circuit and installed Win64/Android QA.

## Feature A: corner traction assist

The car previously applied the same normalized forward motor force
regardless of the ratio of measured sideways to forward speed. On the
early prototype rigid-body chassis, this could sustain accelerator
demand while the car was visibly sliding in a corner.

`Vehicles/CornerTractionAssist.h` now limits **positive** motor
throttle when substantial lateral drift develops on a moving car:

- Full launch authority below 900 cm/s (32.4 km/h) longitudinal speed,
  or below 250 cm/s lateral drift.
- Above the thresholds, a bounded lateral/longitudinal slip proxy
  smoothly scales the **already-smoothed** positive throttle.
- Arcade retains at least **58%** of normalized positive motor demand;
  Simulation Inspired retains at least **45%**. Simulation Inspired
  reaches its stronger intervention earlier.
- All values stay finite and within the incoming throttle range.
  Negative throttle, neutral throttle, reverse manoeuvres and the
  existing explicit BRAKE input are not cut by this assist.
- The actual C++ pawn reads Chaos planar speed, forward projection and
  sideways projection **after** its nonfinite-velocity checks, then
  applies the assist before `Chassis->AddForce`. Since player/touch/
  gamepad/rival share this pawn's physics tick, they share the rule.

This is **not tire simulation or a claim of safe road grip**. It
does not change the physics body, collision response, lateral grip
force, yaw torque, checkpoint policy, garage upgrades or top-speed cap.

## Feature B: nonintrusive wrong-way HUD

`Racing/WrongWayIndicator.h` examines the existing four-gate circuit
and sampled **world-space XY velocity**. It displays a warning only
if the car drives demonstrably against the intended route for at
least **0.8 seconds** on one of the four central straight sections.

The route directions are South straight **east**, East straight
**north**, North straight **west**, West straight **south**.
The advisory intentionally ignores 90-degree turns, outside lanes,
low speeds (below 600 cm/s, roughly 21.6 km/h), sideways drift and
nonfinite position/velocity. Warnings clear immediately after the
driver corrects direction.

In the actual HUD this warning is available on both full and
compact screens:
- `WRONG WAY - RETURN TO CIRCUIT DIRECTION` on normal HUD
- `WRONG WAY - TURN AROUND` in compact/mobile HUD

The warning is reset when the player pawn changes, a checkpoint-session
revision changes, a teleport/restart advances the pawn generation,
the menu opens, the player/circuit becomes ineligible, the race
finishes, or pause/focus interrupts play. Engine deltas larger than
0.25 seconds cannot synthesize a 0.8s warning.

**Important:** This is advisory text, not a penalty or an authoritative
wrong-way disqualification. It never calls `PassCheckpoint`,
`RegisterCheckpoint`, awards, or modifies controls.

## Portable regression verification

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(CornerTractionAssist|WrongWayIndicator|SpeedSensitiveSteering)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`CornerTractionAssistTests.cpp` checks launch/straight authority,
mode-dependent reduction, monotonic slip response, exact bounds,
reverse demands, abnormal input and NaN/inf protection.
`WrongWayIndicatorTests.cpp` checks directions on all four straights,
corner/off-pavement suppression, low-speed/sideways suppression, the
0.8s onset, immediate reset, frame-hitch rejection and finite timer.

`HandlingAndWayfindingBindingTests.py` requires actual pawn motor
integration, HUD session/teleport reset, live physics eligibility and
source CMake registration. It **does not execute Unreal/Chaos**.

## Licensed Unreal + hardware acceptance to complete

1. Compile Editor/UHT and game targets using the actual authorized
   Unreal Engine 5.8 installation. Confirm C++ header types, UHT
   generation and `AHUD::DrawHUD` build correctly.
2. Cook the real serialized original circuit and package genuine Win64
   and Android candidates; capture engine build, source revision,
   circuit-map SHA256 and candidate-manifest identity.
3. Compare side-by-side acceleration on straight, launch from rest,
   hard corner and wet-looking but **non-physical** track sections.
   Confirm no unexpected braking or reverse interference. Compare
   Arcade vs Simulation Inspired at meaningful measured sideways speeds.
4. Drive all four straights both directions. Confirm the HUD appears
   only after sustained wrong-way travel and hides on turns, stop,
   recovery teleport, restart, manual/menu pause and OS deactivation.
5. Repeat on mobile with two-finger driving controls and compact
   viewport; confirm text fits and does not overlap GAS/BRAKE or pause
   pads. Monitor FPS and any repeated allocations or Log spam.
6. Check no changes to authoritative checkpoint acceptance, lap
   scoring, time penalties, medals, garage credits, saved best time
   or rival recovery count. Record on-device evidence in the
   [Android qualification gate](https://github.com/sanskarIN/latebrake/issues/13).

Any failing test blocks a claim of a playable or stable game.
