# Latebrake engineering status

**Updated:** 2026-10-19
**Current code milestone:** v0.2.0 experimental source development.
**Playable status:** Not verified; no Unreal Engine 5.8 editor build or Windows/Android runtime package has been tested.

## GitHub releases (source-only)

- [v0.1.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.1.0) — published pre-release of the original C++ project and racing foundation.
- [v0.1.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.1.1) — published pre-release of racing timing, fastest lap and checkpoint stability.

These tags are source previews. They are **not downloadable playable game installers**.

## v0.2.0 implementation on the feature branch

- Portable offline event catalog: three-lap Circuit Race and one-lap Time Attack, both using the starter four-checkpoint circuit.
- Keyboard bindings `3` and `4` switch events and restart the active race.
- Separate personal-best race records by event; backward-compatible SaveGame field for Time Attack.
- Portable waypoint pilot computes steering, throttle and braking from the target, heading and speed.
- Experimental autonomous rival pawn follows four circuit waypoints in Circuit Race; the rival is removed in Time Attack.
- Player-only checkpoint registration prevents the rival from advancing the player's lap count.
- Offline event completion rewards, recovery-aware clean bonus, accumulated and bounded credits.
- Portable C++20 tests for event configuration, pilot inputs, lap rules, gate validation and credits.

## Successfully verified

- GitHub Actions builds and executes the portable C++20 test suite on Windows, Ubuntu and macOS for earlier v0.2.0 development commits.
- Check the newest branch-head workflow before tagging a pre-release.
- GitHub CI does **not** have Unreal Engine source/SDK/tooling available.

## Not implemented or not qualified

- Unreal 5.8 compilation, Unreal Header Tool generation, in-editor driving/playtesting, performance profiling or packaged hardware testing.
- A cooked project-owned map, original finished vehicle model, Chaos Vehicles wheel/suspension rig, optimized Android touch controls and tested phone lifecycle behavior.
- Race-aware overtaking, full AI opponent ranking/finished times, traffic, vehicle upgrades, garage, championships and multiplayer.
- Tested save migrations, touch inputs, actual vehicle/rival cornering, AI avoidance, pause/resume and HUD at varied viewports.
- Windows EXE, Android APK/AAB or Linux native binary **not available**.

## Next acceptance gates

1. Install licensed UE 5.8 + platform SDKs on a supported workstation and compile all Unreal-targeted C++.
2. Test event switching, Time Attack/circuit result separation, rival spawn/despawn, waypoint motion and player-only gate scoring.
3. Verify credit rewards paid exactly once, personal-best saves/migration, recovery penalties and race restarts.
4. Author a cookable .umap and validate real Win64 and Android packages/device playtests.
5. Investigate Chaos Vehicles suspension and steering; replace provisional rigid-body mechanics after measurement.
6. Publish **stable v1.0.0 only** after complete declared release scope and tested Win64/Android artifacts.

Track the first playable-release gates in [issue #2](https://github.com/sanskarIN/latebrake/issues/2).
