# Rival physics-based corner braking — Unreal source QA

**Status:** actual Unreal rival control source enhanced with a physical
braking-distance forecast. This is **not** an installed, UE5.8-compiled,
cooked, playable 3D game or calibrated vehicle.

## Problem and implementation

The pre-existing AI corner anticipator only checked a fixed inbound
2200 cm approach region (22 metres), then used a speed threshold that
was not tied to the newly capped **physical chassis deceleration**.
A rival traveling 3,000–4,200 cm/s can require substantially more
than 22 metres to approach a safe corner speed, particularly after
tire wear, off-track grip loss, impact damage or brake heat fade.

The actual `ALatebrakeRivalCar::Tick` now combines:

- The original verified directional-gate route, waypoint turn planner
  and inbound-only original anticipator.
- A **second earlier speed/distance envelope** for the same four 90°
  circuit turns, effective only up to 5500 cm before a gate and inside
  a narrow 620 cm inbound corridor.
- Current measured real Chaos **planar chassis speed**, the previous
  qualified live tire/surface/damage readings, and the existing
  observed service-brake thermal fade.
- Shared `ArcadeServiceBrakeDeceleration` and
  `SimulationServiceBrakeDeceleration` from the **same force model**
  used by the real player/rival shared pawn. A conservative 76%
  multiplier accommodates the still-unverified Chaos response.
- Target permitted speed from `sqrt(v_corner² + 2 a d)` using an
  approximate **1550 cm/s** turn entry speed and a **550 cm**
  run-through buffer. It requests actual service braking when speed
  exceeds the envelope, rather than turning the car artificially.
- Existing difficulty presets, grip-aware pace adjustment, surface
  damage limits, human-traffic yielding, held-brake hysteresis,
  observed yaw moderation, spin throttle guard, and physical
  gate-crossing/recovery rules remain downstream and authoritative.

Invalid physics, unsupported gate index, unavailable prior qualified
thermal/grip data or nonfinite speed/position fail closed with a safe
brake command; they cannot authorize additional rival propulsion.

## Contact-capacity extension after v0.5.97

The current draft also limits this envelope by an advisory service-brake
capacity estimate from the shared pawn's previous **accepted** drive frame.
That estimate reuses the same spring loads, measured road tangents, garage
profile, tire/surface/damage grip and heat as the actual brake path. It
reserves 35% of available force, accounts for modeled rotor equivalent mass
and subtracts a worst-direction grade allowance from the measured tangent.
Opposing road-point motion cannot count as useful forward braking.

This is an estimate of current source-level capacity, not measured
deceleration, a future-grip prediction or proof that a corner can be made.
It cannot submit Chaos forces. The source still needs real track tuning.

The pawn publishes a monotonic identity only after the whole drive frame
passes its policies and actual commands are sent. Pause, recovery, invalid
physics and garage changes invalidate it. The rival rejects repeated or
pre-teleport identities. A capacity loss is immediate; recovery grows by
at most 360 cm/s³ over fresh accepted observations. The first new estimate
starts at half capacity. This prevents one favorable crest/contact sample
from instantly restoring an optimistic corner forecast.

With this extension active, the target uses the smaller of the original
forecast and current capacity, with a 0.20s plus one-frame reaction-distance
allowance. The inbound window covers the actual incoming segment length;
wrong-lane, crossed-gate and unrelated-road rejection remain. Zero capacity
removes propulsion and keeps a brake request. The standalone legacy API
retains its original default window when no contact ceiling is supplied.

The new source regressions cover 25,000 varied capability frames, grade/SI
references, low load, partial support, spin, receipt ordering and transient
capacity loss/recovery. A 108-case flat prescribed-body harness composes the
actual wheel model and capacity-aware corner envelope across six profiles,
two handling modes, three grip/heat/load conditions and 30/60/120 FPS.
Its motor/rolling-load integration is deliberately a toy harness. It is
not a genuine rival lap, Chaos collision test or installed gameplay result.

This controller does **not** steer the car, teleport the rival, grant a
lap, overwrite the rival route, modify the player's recorded results,
simulate independent wheel contact, claim an overtaking AI, or
replace the mandatory genuine Unreal install/build QA.

## Automated source checks

- `Tests/Standalone/RivalCornerBrakeEnvelopeTests.cpp` tests all four
  physical inbound turn corridors; 3650 cm early braking beyond the old
  static 2200 cm window; clean vs degraded road/tire/brake heat; wrong
  lane, outside region and passed gate; previously requested braking
  and arrived controls; invalid/nonfinite snapshots; and 20,000
  deterministic speed/distance/traction samples.
- `Tests/Editor/UnrealRivalCornerBrakeEnvelopeBindingTests.py`
  verifies the real autonomous pawn uses the qualified previous
  chassis data and keeps future gate/traffic/recovery/SaveGame
  authority separated.

Portable C++20 and Python checks **cannot** establish the actual
rival's track completion rate, path quality or 3D vehicle handling.

## Necessary installed UE5.8 acceptance

1. Compile actual Unreal 5.8/UHT player and rival C++ modules and
   cook the original circuit on an authorized Windows workstation.
2. Measure braking onset, distance, speed and turn exit at all four
   corners in Arcade and Simulation Inspired, including a first lap
   on fresh tires, after worn tires, while damaged, and with hot brakes.
3. Compare multiple frame rates, initial line-up, first-run state
   after restart/focus loss, and motion after any rival recovery.
4. Confirm no phantom braking on the already-crossed gate or wrong
   road shoulder, and that reduced brake grip causes *earlier*, not
   later, braking for the same actual speed.
5. Test all currently supported quick race difficulty settings, player
   vehicles, AI proximity and late-brake traffic interactions; confirm
   verified gates, final race results and career rewards are unchanged.
6. Tune coefficients and behavior from real Chaos frame logs; consider
   genuine contact-wheel physics and multi-line overtaking only after
   this is hardware-verified.

See [Unreal playable release gate
#148](https://github.com/sanskarIN/latebrake/issues/148).
