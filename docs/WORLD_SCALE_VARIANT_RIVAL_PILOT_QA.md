# Per-circuit measured-velocity rival pilot (source-only)

The six source-only original racing circuits now have a separate
portable, per-rival C++20 advisory controller:
`AI/WorldScaleVariantRivalPilot.h`.

It accepts the actual **selected circuit's independent pace plan**
and an individually authenticated physics observation of one
engine-owned car: actor/session/frame identity, world XY position
and real velocity, heading radians, signed body-forward velocity,
actual frame time, verified road contact and race-green state.

Before suggesting any throttle, the controller checks that measured
world speed agrees with the driver-facing speedometer and that actual
world velocity projected onto the observed body forward direction
agrees with the signed body-forward speed. Neither a synthetic
speedometer nor a FlatSpeedCmps value alone can authorize a car.

The globally projected station is based on the **correct variant's
physical source road and 512-segment pace**, not the legacy track
or another proposed layout. The target steering heading is computed
toward a forward pace lookahead clamped at 650–1700cm, and the
resulting throttle and braking remain within the original conservative
pilot limits. Heading reversal, reverse driving, severe yaw mismatch
and measured overspeed never receive positive propulsion.

Consecutive observations require unchanged real source actor token,
session identity and immutable pace-plan object; strictly increasing
frame number and engine timestamp; actual measured position change
consistent with the trapezoid integration of consecutive world
velocities; and continuous geometric route distance across the
correct lap seam. Invalid physics, missing road contact, fake
speed/velocity mismatch, teleport, swapped actor, stale timestamp,
unqualified race state or plan switch resets the pilot and returns
zero-power braked advice.

## Strict portable QA

```sh
cmake -S Tests/Standalone -B build/six-variant-ai -DCMAKE_BUILD_TYPE=Release
cmake --build build/six-variant-ai --config Release --target latebrake_world_scale_variant_rival_pilot
ctest --test-dir build/six-variant-ai -C Release -R '^WorldScaleVariantRivalPilot$' --output-on-failure
```

The regression drives synthetic **source-geometry-derived** world
velocity at 30/60/120Hz over the start/finish seam on each of the
six different original circuits. It verifies the complete bounded
steering, throttle, braking, speed reference and lookahead; rejects
malformed physics, forged body speed, invalid race epoch, stale clock,
actor replacement, teleport and plan swapping; and checks stationary
aligned launches, actual backwards motion and overspeed braking.

**No installed Unreal/Chaos actor consumes these output commands.**
Real Unreal 5.8 Editor/UHT compilation, physics control integration,
measured 3D tire contact, actual six-car road/vehicle collisions,
cooked original levels, human driving feel, Win64 graphics/hardware,
online sessions and official race control remain unsupported or
unqualified. These source tests certify none of those. This branch
is a draft dependent on PRs #405 and #406, to be kept unmerged until
the owner publishes/audits frozen v0.5.100-test.2 from protected main.
