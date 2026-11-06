# Bounded physical yaw steering — Unreal source and QA

**Status: shared Unreal player and AI chassis source changed. NOT
Unreal Engine 5.8 compiled, packaged or hardware playtested.**

## Current driving problem

The original simplified rigid-body LateBrake prototype steered by
applying one essentially constant Z torque times the steering axis.
With high rigid-body angular damping, this is not a proper response
to current rotation and can produce slow turn-in, overshoot or poor
high-speed handling. LateBrake still lacks a real wheel/suspension
contact and tire force model, so it is *not* a complete realistic sim.

## Source improvement

`SpeedAwareYawTorque.h` introduces an engine-independent, fixed-
allocation target-yaw controller for **real Chaos player and rival
chassis movement**:

- Read the same measured signed forward speed, planar speed, actual
  Chaos yaw rate, speed-sensitive steering axis and physics mass as
  the existing real driving tick.
- Estimate target turn rate from bicycle-like wheelbase curvature
  (260cm effective wheelbase and a limited steer angle).
- Bound maximum turn rate by provisional lateral acceleration budgets:
  1176cm/s² arcade and 784cm/s² Simulation Inspired, with a further
  35% reduction after confirmed off-pavement driving.
- Limit Z torque based on mass and a capped steering response, including
  measured-rate feedback and known linear Chaos angular damping.
- **Brake-aware cornering:** the real shared player/rival yaw controller
  now consumes the *same accepted frame's* service/handbrake, opposing-
  pedal and reverse-governor longitudinal deceleration request used by
  `CalculateLateralGripForce`. With exactly the same tire/surface/
  garage/damage grip product, it reduces fresh requested steering yaw
  by the remaining fraction of the existing friction-circle budget:
  `sqrt(1-(bounded_brake/shared_grip)^2)`. No brake keeps the previous
  steering baseline; a fully spent friction circle requests no new turn.
  A car already spinning still receives opposing measured-yaw feedback.
  Front axle support remains an additional final actual-force gate.
  The separate intentional held-handbrake drift torque is unchanged.
- When the car is already rotating too quickly, apply corrective
  opposing torque. Neutral steering damps an unwanted yaw spin.
- Turn direction naturally reverses at physically negative signed
  forward speeds and never requests yaw from steering at zero speed.
- Preserve the previous simple torque specifically for intentional
  **human held-handbrake slides**, but replace its abrupt reverse-speed
  direction switch with a smooth, signed **measured-chassis-speed**
  authority. At zero speed the drift steering torque is zero; rolling
  backwards even at a crawl selects the correct reverse turning direction.
  Authority is proportional to signed longitudinal motion up to ±900 cm/s,
  matching the old full-speed tuning in either direction. There is no
  sudden force-sign jump near −75 cm/s and no wheel/gear teleport.
  Existing speed caps, nitro, brakes, collision and garage power tuning
  remain independent.

**Not** tire physics: body inertia is approximated from source-authored
380×180cm dimensions and actual mass, and the values are a first
driving-tuning estimate until real Unreal playtest measurements.
No teleport, kinematic `SetActorRotation`, score/rival gate rewrites
or new world collisions were introduced.

## Automated tests

- `Tests/Standalone/SpeedAwareYawTorqueTests.cpp`: launch from
  rest, forward right/left turn signs, reverse steering, high-speed
  lateral limit, off-road / simulation differences, yaw overshoot,
  countersteering, neutral spin damping, torque bounds, corrupted
  axes/velocity/mass, invalid frame time, cadence independence.
  New tests cover Arcade/Simulation brake reduction, reverse mirrors,
  fully saturated steering target with spin damping retained, physical
  grip/brake proportion equivalence, nonfinite/out-of-range budgets,
  and 120,000 signed-speed/steer/brake/grip combinations.
- `Tests/Standalone/HandbrakeDrivePolicyTests.cpp` additionally
  tests the exact new continuous signed handbrake yaw law, its ±75 cm/s
  former discontinuity, stationary/reverse/forward behavior, extreme
  input rejection, mirror symmetry and 300,000 finite speed samples.
- `Tests/Editor/UnrealSpeedAwareYawTorqueBindingTests.py` and
  `Tests/Editor/HandbrakeBindingTests.py` check the actual Unreal
  chassis uses the measured signed speed for the human handbrake yaw
  exception; the old hard reverse-sign threshold cannot return, and
  the ordinary measured yaw controller and race authority stay separate.

These are *source-only* checks, not engine/real-drive verification.

## Required actual gameplay acceptance

1. Compile with **real UE 5.8/UHT** and package cooked Win64.
2. Run installed game and drive original four corners in both Arcade
   and Simulation Inspired. Verify adequate steering lock at walking
   pace, progressive cornering at 40–100km/h, reduced steering above
   150km/h, no abrupt yaw or unexpectedly stiff steering.
3. Perform sustained opposite-direction steering, slalom/quick lane
   changes, reverse parking turns, countersteering a physical slide,
   held-handbrake drifts, and AI rival full-lap turn tracing. Drive
   repeated identical corners with no brake, mild service brake and hard
   brake in both modes; compare true yaw rate, lateral slip, stopping
   length and brake-while-turning overshoot. Confirm handbrake slides
   remain intentional and that spin damping still works.
4. Compare yaw-rate telemetry at 30/60/120FPS and after a frame hitch;
   inspect any strong oscillation, excessive correction, unsafe
   lateral jerks, collision-induced rotation and off-road recovery.
5. Calibrate actual chassis tensor, tire physics, downforce, body mass
   and damping after an installed real-engine physics benchmark. Do
   not infer realism merely from passing cross-platform C++ tests.
6. Ensure rewards, checkpoints, pit, recovery, progress and saves
   remain unchanged, with no new false race finishes.

Use the existing [Windows personal playtest build guide](
WINDOWS_PERSONAL_PLAYTEST.md). See
[playable build qualification #148](https://github.com/sanskarIN/latebrake/issues/148).
