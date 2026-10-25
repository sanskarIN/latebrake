# Speed-sensitive steering and rotation readiness — v0.5.48 source development

> These are source changes only. **No licensed UE 5.8 Editor/UHT compilation,
> genuine cooked circuit, Win64 build, Android APK or installed steering
> qualification has been completed.** The current car is an experimental
> rigid-body prototype, not a production suspension/tire model.

## Purpose

The prior steering controller applied the same normalized smoothed steering
torque at all speeds above ~900 cm/s, including near the prototype's highest
road speeds. Steering response at ~200 km/h therefore depended entirely on
Chaos damping, rather than an explicit driver-control curve.

The portable `Vehicles/SpeedSensitiveSteering.h` now shapes the already
smoothed steering command before the car applies yaw torque. It is used
by **all** drive sources — keyboard, gamepad, touch and rival AI — because
the base Unreal car pawn owns the torque integrator.

| Flat planar speed | Arcade multiplier | Simulation Inspired multiplier |
| --- | ---: | ---: |
| 0–900 cm/s (0–32.4 km/h) | 1.00 | 1.00 |
| 3150 cm/s (113.4 km/h) | 0.775 | 0.70 |
| 5400+ cm/s (194.4+ km/h) | 0.55 | 0.40 |

The interpolation is continuous and monotonic. Torque still includes the
existing speed-derived `SteeringAuthority` and reverse-driving direction,
and the controller keeps its established smoothing. The curve is applied
**after** actual chassis speed is read, not from a target speed or HUD
estimate. It is not an automatic lane-keeping or braking system.

Negative/nonfinite planar speed, and nonfinite steering axes, produce
zero torque command. If the Unreal pawn's derived speed, longitudinal or
lateral speed is nonfinite, it neutralizes input and chassis motion before
force/torque application. `RaceVehiclePhysicsReadiness.h` additionally
requires finite physical actor pitch, yaw and roll, so corrupt orientation
cannot generate a forward/right force basis or timed checkpoint scoring.

## Portable verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^(SpeedSensitiveSteering|RaceVehiclePhysicsReadiness)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The portable steering tests cover speed thresholds, interpolation,
monotonicity, both handling modes, input sign/clamp and abnormal numbers.
The existing `RaceVehiclePhysicsReadiness` executable now checks all
three orientation axes with NaN/+Inf/-Inf and actual `RaceProgress`
clock and checkpoint eligibility. Python bindings check Unreal source
integration/order. None of these exercises Unreal Engine or device physics.

## Required installed-game acceptance

1. Compile the real `LatebrakeEditor` and game targets on authorized UE 5.8
   with UHT, warnings and source commit recorded.
2. Author and cook the actual circuit; install a real Win64 candidate.
   Collect steering traces on a collision-bearing drivable car.
3. On a straight and repeatable test track, compare 0–30, ~110 and
   ~195 km/h turn-in at full left/right inputs in **both** handling modes.
   Record target angle, actual yaw rate, lateral drift, recovery from a
   released control, steering reversal and any spin/rollover. Do not infer
   real handling quality from source-only numerical tests.
4. Repeat with reverse steering, aggressive braking, a wall impact,
   a verified player recovery, and rival corner approaches. Confirm that
   the rival still reaches all ordered route targets without an unusual
   recovery/timeout rate after the shared steering change.
5. On physical Android devices test simultaneous steering/throttle/brake
   fingers, landscape viewport changes, touch release, backgrounding,
   manual pause, and both handling modes; check no stale steering torque
   after suspension or restart.
6. In a **development-only** controlled test fixture, inject nonfinite
   planar speed/projection and transform rotation independently. The game
   should neutralize drive commands and block racing clock/checkpoints,
   then resume only when healthy. Never damage engine runtime objects on
   a user's device.

Keep genuine source/engine/map/package hashes, logs, tester/device
identifiers and observed behavior alongside [Android QA issue #13](
https://github.com/sanskarIN/latebrake/issues/13).
A stable or playable release requires the separate complete acceptance
gate; this source feature alone is insufficient.
