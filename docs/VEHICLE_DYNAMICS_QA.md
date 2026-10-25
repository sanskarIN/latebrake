# Chaos vehicle dynamics readiness (v0.5.45 source development)

> **Unreal Engine 5.8 device QA NOT COMPLETED.** No source, test or
> workflow below certifies a playable Win64/Android game.

## Defect and fix

Previously `ALatebrakeCarPawn::IsRacePhysicsReady()` rejected missing chassis,
missing mesh, disabled simulation, invalid collision, and nonfinite **linear**
position/velocity. It did **not** reject nonfinite **angular** velocity or a
zero/negative/nonfinite chassis mass. `HasLocalPlayerDriver()` and each
`ALatebrakeCheckpoint` use this readiness predicate. A broken Chaos state
could therefore leave active race time and ordered gate eligibility intact.

The production C++ files `Source/Latebrake/Public/Vehicles/VehicleDynamicsReadiness.h`
and `Source/Latebrake/Private/Vehicles/VehicleDynamicsReadiness.cpp` define an
allocation-free, pure C++20 predicate with stable decision reasons:
`ready`, `invalid-mass`, `invalid-angular-velocity`. It requires strictly
positive finite mass and finite angular velocity on all three axes; it does
**not** invent a platform-independent upper mass or angular-speed limit.

The Unreal car now gathers `Chassis->GetMass()` and
`Chassis->GetPhysicsAngularVelocityInDegrees()` and combines that policy
with existing chassis/linear readiness. The player physics tick neutralizes
existing motion and all driver inputs before motor force/torque if either
readiness policy fails. The autonomous rival checks **its own** physics body
before progressing route telemetry. The HUD speed getter returns a finite
zero if a chassis is missing or measured speed is corrupted.

## How to run source-only checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^VehicleDynamicsReadiness$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

The focused CTest compiles the **same production .cpp** as Unreal's module,
links actual `RaceProgress.cpp` and checks every angular axis against NaN,
positive and negative infinity, positive/zero/negative/nonfinite mass,
normal recovery, checkpoint readiness and elapsed-clock freezing. Editor
Python tests inspect real C++ use sites; they do not run a UE editor.

## Required installed-build acceptance

1. Compile the actual UE 5.8 Editor and game target using an authorized
   engine/SDK installation. Observe UE compiler/UHT logs for the new
   `VehicleDynamicsReadiness.cpp` translation unit and pawn/rival bindings.
2. Play a genuine cooked `/Game/Maps/LatebrakeCircuit` on Windows. Confirm
   steering, braking, collisions, normal lap and checkpoint timing with a
   valid positive chassis mass and finite X/Y/Z spin.
3. In a **non-shipping development-only test harness**, explicitly inject
   synthetic missing/bad Chaos readings **into the policy input** rather than
   damaging real engine objects. Confirm that invalid mass/spin makes
   readiness false and disables clock, checkpoint, rival routing and force
   application. Compare with the known-good policy test cases.
4. Check that dropping/removing a car's physical body or disabling
   simulation also blocks scoring. Restoring a healthy vehicle should
   permit only new forward gate samples; no reward for movement missed
   while the physics body was invalid.
5. Check real Android two-finger driving and touch cancellation, pause/focus
   transitions, and framerate after this added readiness query.
6. Attach genuine compiler logs, runnable build identity, screenshots and
   input/physics results to open Android qualification
   [issue #13](https://github.com/sanskarIN/latebrake/issues/13).

**Boundary:** checking mass and angular values cannot prove the car is in
contact with the road or that Unreal/Chaos is stable under all forces.
Actual engine builds, real surface collision, cooked map and installed-game
qualification remain necessary.
