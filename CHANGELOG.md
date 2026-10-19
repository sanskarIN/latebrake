# Changelog

## v0.3.0 — garage foundation, source-only preview candidate

### Added
- Three fictional vehicle profiles and asset-free silhouette variants.
- Offline credit-based vehicle unlocking and engine/grip upgrades, with capped deterministic pricing and performance.
- Backward-compatible SaveGame garage ownership, selected profile and upgrade levels.
- Keyboard Q/E/U/I garage shortcuts and HUD profile, unlock and tuning readouts.
- Portable garage C++20 regression tests for purchases, profile sanitization and credit overflow.

### Still outstanding
- Unreal Engine editor/UHT compilation, original 3D vehicle models, real game packages, physical hardware and save serialization tests.

## v0.2.1 — published source-only pre-release

### Improved
- Pure C++20 rival route state tracks approximate waypoint laps and recovery requests after eight seconds at low speed.
- Experimental Unreal rival now uses route progress, recovers in place without resetting tracked lap progress, and applies pilot commands before the base car physics step.
- Basic corner-anticipation braking in the portable rival planner, with invalid negative speed handling.
- Shared deterministic steering slew limiter for both player and AI controls, including engine-independent tests and overflow guard.
- HUD shows approximate rival laps and number of AI recoveries, without claiming competitive race standings.
- Regression tests added for invalid route/sensor values, recovery/reset, corner braking and steering smoothing.

### Not yet verified
- Unreal Engine 5.8 reflection/build, actual driving/rival behavior, packaged Windows/Android binaries and physical device tests.

## v0.2.0 — published source-only pre-release

### Added
- Offline race event definitions for three-lap Circuit and one-lap Time Attack.
- In-game event selection via keyboard `3` and `4`; separate event best-time save fields.
- Portable waypoint-based AI steering, throttle and braking.
- Spawned experimental autonomous rival during circuit mode, removed for Time Attack.
- Player-only checkpoint scoring in shared actor overlap volumes.
- Bounded offline credit rewards for event completion, visible totals and per-event rewards.
- New portable C++20 tests for event catalog, AI steering and credit payouts.

### Still not verified
- Unreal Engine compilation, editor playtesting, vehicle AI cornering, input behavior and any packaged game binaries.
- Rival standings, results accuracy versus opponents and full competitive AI are not implemented.

## v0.1.1 — published source-only pre-release

- Last/current/best lap and sector timings.
- Personal fastest-lap record, checkpoint direction validation, five-second recovery penalty.
- Additional portable C++20 test coverage.
- This version is a **source snapshot**, not a verified playable build.

## v0.1.0 — published source-only pre-release

- Original UE 5.8 C++20 module/targets and input bindings.
- Physics-backed starter car and procedurally constructed circuit.
- Four ordered race checkpoints, lap timer, racing HUD, local best race record.
- Portable C++ tests and Windows/Android packaging preparation scripts.
- No verified Unreal game builds or tested executables/APKs.
