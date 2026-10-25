# Rival direct-motion HUD — v0.5.65 source candidate

> This is **source-only development**, not a verified playable UE 5.8
> implementation. No Win64 EXE or Android APK/AAB has been produced or
> installed by these portable source checks.

## What changed

The v0.5.62 validated radar and v0.5.63 close-rival warning show **where**
the real player/rival are and when they remain within 15m. The new
`RivalRelativeMotionTracker` measures the signed difference in **direct
planar separation** from consecutive verified radar frames:

- **CLOSING**: direct distance decreased by at least 2m/s for 0.4s.
- **SEPARATING**: direct distance increased by at least 2m/s for 0.4s.
- **Unavailable**: first reading, low-speed jitter, Time Attack,
  invalid/missing physics, or broken frame/racing eligibility.
- Valid measurements require a 0.01–0.25s sampled frame interval, a
  grounded player/rival marker, finite direct separation and nearby
  opponent within 100m.
- An impossible inferred jump above 90m/s invalidates the prior
  baseline rather than publishing a false closing rate. Motion direction
  reversals immediately clear the previous direction and must pass the
  full 0.4s confirmation period again.
- Rate formatting uses metres per second (m/s) of **changing
  separation**; no opponent telemetry or player speed is invented.
- Compact HUD displays confirmed CLOSING; full HUD displays CLOSING or
  SEPARATING. Neither alert is a collision predictor, longitudinal
  opponent velocity, track gap, classification, or race penalty.
- Both the proximity indicator and relative-motion tracker reuse **one
  production race radar snapshot per actual HUD frame**. Menu, pause,
  finished race, focus-loss, checkpoint-session and player-teleport
  transitions reset the history.

The engine-neutral tracker is never called by authoritative
`RaceProgress`, checkpoint, recovery, physics, garage or save systems.

## Portable validation

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release -R '^(RaceMiniMap|RivalProximityIndicator|RivalRelativeMotion)$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RivalRelativeMotionBindingTests.py' -v
```

The C++20 target links the **actual production**
`Private/Track/RaceMiniMap.cpp` and exercises sign, 0.4s delay,
reversal, invalid/zero/micro/hitch frames, missing or unhealthy
opponent, large discontinuity, jitter threshold, bounded counters and
all explicit reset entrypoints. The Python check binds real HUD data
flow and ensures no scoring/physics mutation was introduced. Standard
CI runs C++ and source-contract checks across Windows/macOS/Ubuntu.

## Unreal/installed-game QA required

- [ ] Install licensed UE 5.8, run UHT and compile Editor, Development
      game and supported Shipping targets.
- [ ] Author and cook the real original four-gate circuit. Verify
      spawned player and opponent physically simulate in gameplay.
- [ ] Sample actual player and opponent actor positions moving together,
      apart, stationary, turning, passing and swapping order.
- [ ] Record the HUD before 0.4s and after sustained 0.4s, including the
      displayed metres/second against captured engine positions.
- [ ] Inject failed rival recovery, successful AI teleport, player paid
      recovery, RESTART, pawn replacement, menu, PAUSE and OS focus-loss;
      no stale closing reading after a discontinuity.
- [ ] Exercise choppy frames (4fps/large hitches), 60fps, and high
      refresh display sampling; watch for false reversal near threshold.
- [ ] Verify desktop 1280×720/1920×1080 and Android
      portrait/landscape/rotation: no overdraw of HUD controls.
- [ ] Preserve logs, compiler/UHT output, executable hashes,
      screenshots and physical device details with the candidate.

Issue #13 remains open for actual Android qualification. Source CI
alone cannot certify engine, cooked content, shipped binaries or
physical device reliability.
