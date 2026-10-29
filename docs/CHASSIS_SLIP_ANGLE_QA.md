# Chassis slip-angle telemetry — source development

This optional, *read-only* vehicle telemetry estimates signed **chassis
travel angle relative to its nose** from the actual Chaos rigid-body planar
forward and lateral velocities. It is not tire slip angle, a suspension model,
a racing control assist, a verified drifting system or a safety instrument.

- Positive values mean movement toward the car's right side; negative values
  mean movement toward the left while moving forward.
- Accepted samples require 1–250 ms frames, finite local motion, forward
  speed of at least 300 cm/s and planar speed no greater than 10000 cm/s.
  Reverse, near-stationary, excessive or malformed frames clear the live
  display without manufacturing session peaks.
- On spacious expanded-desktop layouts (at least 28 telemetry rows), the
  live HUD shows a ~120 ms smoothed estimate; smaller desktop windows omit
  this optional line to preserve genuine lap/sector feedback. Finished
  Results show the maximum absolute raw angle and sampled time with at least
  12 degrees of chassis direction difference while traveling forward at
  900 cm/s or faster. These thresholds are *presentation choices*, not
  handling physics or tire-grip validation.
- Restart clears all stats; pause, focus loss, invalid Chaos state and
  teleport remove live readings while preserving valid same-run peaks.
- The meter never changes throttle, steering, braking, race clock, lap
  completion, rewards, credits, records or save-game data. Compact Android
  HUD retains priority for driving controls, genuine timing and route hints.

## Portable verification

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release -R '^ChassisSlipAngleMeter$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'ChassisSlipAngleBindingTests.py' -v
```

Tests cover signed 45-degree motion, temporal smoothing, slow/reverse
rejection, nonfinite/overspeed/long-frame rejection, interruption and
restart behavior, and 20,000 fixed-memory samples. Source integration tests
check that *real* Unreal chassis velocity projections feed the policy and
that the HUD is observational.

## Required real-world acceptance

1. Compile actual licensed UE 5.8 Editor and Game targets with UHT, record
   the compiler, exact engine fingerprint and source commit hash.
2. Cook the owned circuit, package a real Windows executable and physically
   install it. Cross-check telemetry direction on straight, turning, sliding,
   braking and reversing chassis movement in both handling modes.
3. On a genuine Android installation verify compact HUD still prioritizes
   steering, GAS/BRAKE, pause and recovery during landscape/orientation
   changes and OS focus interruption.
4. Check teleport/recovery and paused timing never leave stale live angle.
   Compare displayed angles with recorded world velocity and actor yaw; log
   source/engine/package hashes, observed numeric ranges and any glitches.

Portable CI is **not an Unreal compile, cooked map, executable or device
playtest**. Do not publish this as verified playable functionality until the
existing [game release gate #148](https://github.com/sanskarIN/latebrake/issues/148)
is genuinely satisfied.
