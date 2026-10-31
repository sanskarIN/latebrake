# Rival yaw-rate steering moderation — source-only gameplay QA

**Scope:** The actual physical Unreal racing rival now uses its measured
Chaos chassis angular velocity to moderate over-rotation in high-speed
turns. This source change is **not** a licensed UE 5.8/UHT compile,
a verified racing game, a cooked original circuit, a Windows EXE or an
Android APK.

## Motivation

The existing `PlanRivalTurn` computes a desired heading and steering
input from the next physical circuit waypoint. The base car applies
real chassis yaw torque. While cornering quickly, the rival may already
be rotating in the desired direction, yet keep requesting nearly the
same steering torque until its heading catches up. That can contribute
to overshoot and unnecessary correction.

`ModerateRivalYawRate` provides one conservative feedback step:
*reduce* desired steering only when the chassis's real world-Z angular
velocity has the **same sign** as requested steering, and both yaw
rate and planar speed are elevated. It does **not** add opposite
steering, steer around other cars, or change waypoint selection.

## Actual C++ pawn integration

The qualified `ALatebrakeRivalCar::Tick` runs:

1. Authoritative race/start/physics/circuit gates and real chassis
   position/speed observation.
2. Waypoint steering, corner anticipation, tire grip adaptation, and
   confirmed off-pavement/impact pace adjustment.
3. Real player-traffic braking and short following-brake hold.
4. **Real `Chassis->GetPhysicsAngularVelocityInRadians().Z` sampling**
   and `ModerateRivalYawRate` application to the final pilot command.
5. Existing `SetAutonomousControls`, which routes through the normal
   player/rival base pawn force/steering and speed budgets.

## Conservative tuning

- At or below **950 cm/s** actual planar speed, steering remains unchanged.
- At or below **0.30 rad/s** yaw, steering remains unchanged.
- Between 950–3150 cm/s and 0.30–1.50 rad/s, attenuation rises
  continuously as both measurements increase.
- Maximum attenuation is **65%**, retaining at least 35% of the
  previously requested steering magnitude for high-speed correction.
- Opposite-direction measured yaw retains 100% of requested corrective
  steering. Neutral input never creates countersteer.
- Existing brake decisions, throttle, waypoint arrival, and real
  player-traffic policy are copied unchanged.
- Invalid physics readiness, malformed/nonfinite measurements, frame
  hitches >0.10 s or invalid pilot commands fail closed to neutral
  steering, zero throttle and a brake request.

This is **not** a Chaos vehicle steering actuator, physical stability
control, real suspension, a per-wheel tire model or a promise of better
real-world lap times.

## Portable regression and source bindings

The standalone `RivalYawRateSteeringTests.cpp` checks:

- Same-direction left/right yaw moderation, symmetry and sign retention.
- Full steering at launches, ordinary yaw, and when correcting
  opposite-direction rotation.
- No generated steering from a neutral command; no changes to braking,
  throttle, or arrived-waypoint decisions.
- Finite input ranges, invalid chassis samples, and failed readiness.
- Monotone yaw attenuation and **200,000 deterministic motion samples**
  preserving all existing pilot decisions except steering magnitude.

The editor-source bindings ensure the real rival takes the yaw sample
from its physical Chaos chassis **after** corner and traffic policies
and **before** the existing `SetAutonomousControls` call.

## Real engine/hardware acceptance still required

1. Build UE 5.8 Editor/UHT and the game target with valid owned assets,
   and author/cook the original `LatebrakeCircuit` map.
2. Drive an actual rival through left/right hairpins and straights at
   low and high speed. Compare real chassis yaw/angular velocity,
   overshoot and lap progress against the earlier build.
3. Verify a recovering or spinning rival retains enough directional
   steering to get back on route; ensure no oscillation or indefinite
   braking with tire, dirt, and collision damage systems.
4. Test traffic following, collisions, focus loss, pause, startup,
   restart, recovery teleport, and malformed-physics fail-closed paths.
5. Capture installed Windows and Android traces and qualification
   evidence under [playable-release gate #148](
   https://github.com/sanskarIN/latebrake/issues/148).

Portable source tests alone do not certify real installed driving.
