# Four-corner Chaos suspension — real vehicle physics development candidate

**Status: source and portable regression candidate only. Not an Unreal Engine
5.8 compiled vehicle, finished physics model, installed Win64 game, licensed
Chaos Vehicles skeletal-wheel implementation or production release.**

## The specific problem being solved

The first LateBrake car was a 3D box-backed rigid body with **pitch and roll
locked**. Four live downward ground queries adjusted arcade traction, but
there was no physical spring/damper force at the corners, no supported body
heave/roll over road-height differences, and the four decorative cylinder
wheels did not follow real ground height.

This change is the start of **one production-quality car-driving foundation**:

- Both human and prototype AI still use the SAME simulated Chaos rigid
  body and original four WorldStatic line traces. The actor ignores itself,
  refuses malformed hits, and samples the **real velocity at each wheel
  anchor** using `GetPhysicsLinearVelocityAtPoint`.
- Each contact has an independently computed **spring and damper**.
  The spring pushes upward at the physical corner using
  `AddForceAtLocation`, allowing body pitch and roll. It does not
  teleport the body or set invented wheel contact positions.
- All forces are scaled to **actual chassis mass and UWorld gravity**.
  At default 1,100 kg and 980 cm/s², four corners at the target 59 cm
  ride distance together counteract the current vehicle's weight.
- Travel is bounded: uncompressed spring reach 82 cm, target 59 cm,
  vertical normal threshold **Z >= 0.68**, no downward spring suction,
  no force from absent/too-far/steep surfaces. The maximum force at
  *each* corner is 70% of the mass's total weight, including heavy
  landings; no speculative 10g forces can escape the policy.
- The four cosmetic spindle meshes now move up/down **independently**
  with sampled vertical travel (between -23 and +25 cm), while their
  existing real-velocity roll and steer animation remains intact.
- During the race's existing countdown, the actual body may settle
  vertically on real springs. Horizontal and yaw input are neutralized;
  no engine/nitro, tire wear, race clock or checkpoint grants are allowed
  until the existing authoritative green phase.
- This does NOT add UE5 Chaos Vehicles plugin, physical wheels, a proper
  tire friction curve, calibrated shock absorbers, aerodynamics, original
  skeletal car assets or professional animation. The original rigid
  box chassis still supplies the collision response.

## Numerical source tests

`Tests/Standalone/SprungChassisDynamicsTests.cpp` compiles the actual
header under strict C++20 warnings-as-errors and verifies:

1. Four-corner equilibrium at five masses (100–15,000 kg).
2. 490 cm/s² alternate real-gravity equilibrium.
3. One-corner bump, asymmetric lift and visual travel.
4. No pulling into road during rebound, bounded damping on impact.
5. Reject unsupported/wall/far samples and extreme frame delays.
6. Reject NaN, infinity, malformed trace distance, normal and gravity.
7. **120,000 independent frame fixtures**, verifying per-corner and total
   force budgets and numerical consistency.

`Tests/Editor/UnrealSprungChassisBindingTests.py` inspects the actual
Unreal pawn for four WorldStatic traces, genuine corner velocity, world
gravity, validated spring output, calls to `AddForceAtLocation`, pre-GO
drive isolation and visual spindle travel. This is **source inspection,
not UHT, a physics trace execution or a real road-driving replay**.

## Mandatory real engine acceptance gate

**Do not merge a candidate as production-ready solely because source CI
is green.** On an authorized Windows Unreal Engine 5.8 workstation:

- [ ] Generate project files; compile UHT, Editor and Win64 Game; investigate
      *all* errors from the new point-velocity and force API calls.
- [ ] Cook the project-owned circuit map and inspect four actual blocking
      WorldStatic road slabs. Capture the exact source revision and engine
      version in build logs.
- [ ] Drive at 30/60/120 FPS with physics substepping and inspect ride height,
      stability and oscillation over 10 minutes; measure actual frame-times.
- [ ] Check stationary startup and 3/2/1/GO grid. The car should settle
      before the start with **no planar pre-start motion** or jump at GO.
- [ ] Test all four straight/corner approaches and raised/road-edge fixtures,
      recording distinct wheel travel, chassis pitch and roll, and true
      support loss. Confirm gravity/force scaling at default map gravity.
- [ ] Drive all six fictional garage cars, both handling presets, AI rival,
      pit stop, hard service braking, collision damage, reverse, spin,
      nitro, recovery, restart, pause and focus loss.
- [ ] Look for wheel intrusion into asphalt or chassis, nonzero thrust
      while unsupported, oscillation at the rest-extension boundary,
      force spikes after hitches, flipped vehicles, steering-torque
      instability, camera clipping and ghost race-clock regressions.
- [ ] Compare before/after lap-time, brake-distance and lateral-slip
      telemetry using the exact same installed package and track fixture.
      Tune spring/damper/vehicle collision geometry **from measurements**.
- [ ] Record genuine screenshots/video, logs, package and source SHA256,
      exact machine GPU/driver/RAM, and known limitations.

## Performance and release scope

This candidate reuses four existing collision traces; no new high-volume
queries or frame allocations are introduced. True Chaos CPU cost, shader
cost, visual impact, platform compatibility and any driving improvement
**cannot** be claimed until the engine runs.

An eventual multi-GB release would contain original cooked 3D tracks,
licensed/generated car meshes, materials, textures, animation, sounds,
localization and optional replay data. **No padding or fake GB assets
should be added:** size is not a measure of gameplay quality.

This is source work on the official UE project; **the separate 200 KB
Win32/GDI 2D preview remains a distinct experiment**. The actual playable
release acceptance remains [GitHub issue #148](https://github.com/sanskarIN/latebrake/issues/148).
