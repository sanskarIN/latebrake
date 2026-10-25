# Rival proximity HUD — v0.5.63 source development

> **Unreleased and source-only.** The C++20 regression matrix checks math and
> binding contracts; it does **not** compile UE 5.8 UHT/Editor, cook the map,
> create a Windows executable or Android package, or establish playable QA.

## Behavior

The v0.5.62 north-up radar displays actual physical player/opponent positions,
but even a readable minimap can be missed during a race. This adds an
**advisory close-rival warning** on both standard desktop and compact/mobile HUD:

- Sample one validated `BuildRaceMiniMapSnapshot` per HUD frame and reuse it
  for the desktop map and the proximity indicator.
- Use measured **planar direct separation**, not along-track race gap,
  collision risk, predicted contact time, AI ranking or championship position.
- Require a healthy player, valid official race state, non-Time-Attack event,
  present physics-ready rival, finite actor position/heading and visible real
  radar markers. A missing rival must not cause a warning.
- Enter only after **0.6 seconds continuously at or within 15m**. Do not
  accumulate time across bad frames (delta <= 0, > 0.25s, NaN/Infinity).
- Once visible, retain the warning while the rival remains within **20m** to
  avoid flashing near the threshold. Clear immediately beyond 20m.
- Reset warning debounce on paused/menu/finished/unverified race, focus loss,
  player replacement, checkpoint session change, player teleport/restart and
  reported verified rival recovery.
- Show `RIVAL CLOSE | …m DIRECT`. This is a situational-awareness cue,
  **not proof of collision**, a penalty, an obstacle alert or race placement.

This policy never changes forces, steering, damage, gate scoring, sector times,
race rewards, garage credit balances or save files.

## Automated source checks

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py' -v
```

`RivalProximityIndicatorTests.cpp` links the production
`Private/Track/RaceMiniMap.cpp` to verify physically derived 10m/19m/20m/21m
boundaries, first-entry debounce, reset/re-arm behavior, Time Attack/rival loss,
invalid player/yaw/off-map positions, zero-distance overlap, corrupted
separation, bad delta, and long-running bounded state.

`RivalProximityBindingTests.py` verifies the actual Unreal HUD snapshots,
cross-viewport notice, missing-rival gate, explicit discontinuity resets and
that the results-menu Race reference is in C++ scope before its first use.

## Required real UE 5.8 / hardware acceptance

- [ ] Open the project in authorized UE 5.8, run UHT and compile Editor and
      Development game targets. Confirm generated-code and HUD linkage.
- [ ] Use the authored and cooked four-gate map, with real Chaos-simulated
      player and opponent; verify exact direct distances with editor debug.
- [ ] Approach within 15m for < 0.6s and then for >= 0.6s. Verify that
      only the latter displays the warning; verify 20m hysteresis/21m clear.
- [ ] Confirm no warning in Time Attack, without a spawned rival, or while
      its body is physically unhealthy.
- [ ] Exercise player and rival recovery, paid teleport, RESTART, pawn swap,
      focus loss, PAUSE/menu/resume and finished race, including failed
      restart placement. No stale warnings or charged actions.
- [ ] Capture desktop 1920×1080, 1280×720, scaled/hidpi and Android compact
      landscape/portrait screenshots. Ensure labels do not cover touch pads.
- [ ] Check real Windows/Android framerate, allocations and thermal load;
      retain engine logs, executable hashes, device model and OS versions.

Do not publish a stable playable release or downloadable build without that
evidence. Existing Android qualification issue #13 remains open.
