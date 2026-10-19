# Changelog

## v0.2.0 — experimental development (source-only pre-release candidate)

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
