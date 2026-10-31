# Gear-aware motor power envelope — source-only gameplay development

**Status:** Engine-neutral C++20 policy wired to the existing Unreal 5.8
rigid-body car; **not** a verified Unreal compile, track cook, Windows EXE,
Android APK, or installed driving experience.

## Gameplay behavior

The existing starter vehicle applies a strong direct chassis force until
it reaches a hard planar speed ceiling. Under sustained throttle the motor
repeatedly requests full power immediately before that ceiling, leading to
a sharper transition at high speed than a typical arcade drivetrain.

The new `GearAwareMotorPowerFactor` applies a **smooth, non-increasing
torque envelope** as actual signed forward chassis speed approaches the
corresponding gear limit:

| Handling | Forward fade begins | Forward force reaches zero | Reverse fade begins | Reverse force reaches zero |
| --- | ---: | ---: | ---: | ---: |
| Arcade | 82% of live forward cap | 100% of live forward cap | 74% of 1,350 cm/s | 1,350 cm/s |
| Simulation Inspired | 82% of live forward cap | 100% of live forward cap | 74% of 1,000 cm/s | 1,000 cm/s |

- The *live forward cap* includes garage top-speed tuning and the existing
  ramped nitro speed-cap bonus. Reverse retains its own handling-specific
  limit independent of garage tuning.
- A continuous smoothstep curve transitions between 1.0 and 0.0, with zero
  slope at both ends. This changes **motor propulsion only**; it does not
  change top-speed values or the separate hard velocity safety limits.
- Engine braking and direction changes are not tapered: forward throttle
  while rolling backward, and reverse throttle while rolling forward,
  retain their previous behavior. Zero throttle does not create force.
- Baseline motor power, local-human nitro, and qualified-rival drafting all
  receive **the same extra non-increasing factor**. The earlier combined
  boost limit of 40% extra cannot be bypassed.
- Both human and physically controlled AI rival share the base pawn, so
  neither can bypass the envelope during a qualified racing frame.
- Lateral damping, steering, vertical Chaos velocity, checkpoint timing,
  collision damage, tire wear, awards, credits and save data are untouched.

This is **not** a simulated engine torque curve, gearbox, transmission,
traction-control unit, ABS system or Chaos tire/suspension solver.

## Portable qualification

Run standalone tests with the existing CMake/CTest configuration:

```text
ctest --test-dir <configured-CMake-build-dir> --output-on-failure -R GearAwareMotorPower
```

The production policy runs **200,000 deterministic samples** covering
both modes, positive and negative measured speeds, garaged forward caps,
live boost allocations, reversed controls, C1-continuous and monotonic
limits, NaN/infinite inputs, zero-throttle and near-zero demands.

`Tests/Editor/GearAwareMotorPowerBindingTests.py` guards the actual
`ALatebrakeCarPawn::Tick` call order, protects all three production
`AddForce` branches, and ensures ordinary braking still uses the
untapered driver demand.

## Still required on real hardware

1. Compile real Unreal Engine **5.8** Editor/UHT and game targets on an
   authorized Windows workstation and cook the project-owned circuit.
2. Confirm full standing-start traction and low-speed reverse remain
   responsive, and positive motor demand tapers continuously near the
   actual forward speed ceiling in both handling modes.
3. Test nitro/drafting overlap and garage upgrades; verify that added
   acceleration never jumps near the cap, including after nitro is released.
4. Test brakes, coast, opposite throttle, handbrake slides, reverse gear,
   pause/focus loss and rival recovery.
5. Run separate installed Windows and Android sessions with true traces
   and performance recordings, following
   [playable qualification gate #148](
   https://github.com/sanskarIN/latebrake/issues/148).

**No gameplay, actual Unreal builds or release binaries were verified by
these source tests.**
