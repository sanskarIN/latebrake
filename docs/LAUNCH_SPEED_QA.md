# Launch speed estimate — source QA (unreleased v0.5.76 candidate)

LateBrake adds a **session-only 0–5 km/h forward launch estimate** measured
using the authoritative time since GO. This is *not* a human reaction-time
measurement, a stopwatch-grade crossing timestamp, or a competitive result.
No credits, career awards, best times, saves or vehicle handling depend on it.

## Rules

- A real, qualified 3 / 2 / 1 / GO is mandatory; the **first valid** post-GO
  chassis sample must be below 5 km/h. A first sample already at or above
  5 km/h invalidates the entire attempt rather than allowing a later
  deceleration to masquerade as the initial launch. An observed **below 5
  km/h** chassis-forward sample must precede an observed **5 km/h or more**
  forward sample within 30 seconds of GO.
- Source input is the genuine player's signed *forward* Chaos velocity, **not**
  absolute speed, throttle command, guessed acceleration or rival position.
- Use active race clock minus charged recovery penalties. Invalid frames,
  nonfinite velocity, red lights, pause/menu/focus loss and unqualified physics
  cannot invent a launch. A clock rollback or over-30s threshold invalidates
  an uncaptured estimate. A successful paid recovery invalidates an **uncaptured** launch attempt;
  a previously captured estimate remains unchanged. Failed recovery
  requests cannot erase it.
- An accepted restart clears prior telemetry. A captured observation is
  immutable for the remaining race. No persistent records are written.
- Expanded HUD shows an approximate launch time near the start; Results
  optionally shows the same session estimate. Small screens can clip optional
  telemetry **after** authoritative time/position readouts.

## Next source refinement (v0.5.77, not a playable build)

- For two **consecutive valid** post-GO physics samples separated by at
  most **0.25 seconds**, estimate the first 5 km/h crossing by bounded
  linear interpolation. The estimate must lie between the measured
  samples; it is an approximation, **not reaction time**, an exact sensor
  timestamp or performance-certified acceleration.
- On malformed/nonfinite velocity, pause, focus loss, unqualified frame
  or a longer data gap, **do not interpolate across the interruption**.
  The next physically observed threshold is used conservatively instead.
- Reject a forward 5 km/h crossing reported at the exact timestamp of
  the preceding lower-speed sample; the two readings cannot prove elapsed
  acceleration. An initial already-moving reading still invalidates the
  launch attempt. Verified recovery and race restart keep their existing
  safety semantics.
- The policy is run-local HUD/Results information only. No credits,
  personal bests, career progression, checkpoint gates or tuning depend
  on the interpolated value.

## Automated source checks

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure -R LaunchSpeedMeter
python -m unittest discover -s Tests/Editor -p LaunchSpeedBindingTests.py -v
```

## Still mandatory on an authorized Unreal Engine 5.8 installation

- Check UHT/Editor/game compilation, actual forward Chaos velocity and
  actor tick order; verify that a stationary physical grid start really
  produces the expected first below-threshold sample.
- Test W/gamepad/touch launch, reverse then drive, 3/2/1/GO, pause,
  focus loss, 5-second paid recovery, grid restart and repeated quick races.
- Inspect desktop/mobile HUD overflow and Results detail-page pagination.
  Compare on-screen estimates with recorded physics logs rather than
  describing them as human reaction times.
- Record exact installed Win64/Android package hashes and observed
  device/OS/frame-time evidence before calling any build playable.

This source feature does not close [#148](https://github.com/sanskarIN/latebrake/issues/148)
or [#13](https://github.com/sanskarIN/latebrake/issues/13).
