# Development & release status

<div class="lb-status-panel">
  <div class="lb-status-title">Source-ready does not mean game-ready</div>
  <p>Automated tests for portable game logic have passed, but the project still lacks verified Unreal Editor/UHT compilation, an authentic cooked circuit map, installed driving QA and official game binaries. This website itself is only documentation.</p>
</div>

## Current milestone

| Item | State |
| --- | --- |
| Latest published GitHub pre-release | [v0.5.98](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.98), experimental **SOURCE-ONLY**, unmerged draft PR #295 commit `bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37`; independently [published and verified](https://github.com/sanskarIN/latebrake/actions/runs/43232929038) with **zero executable attachments** |
| Latest source-development milestone | Last audited main snapshot [`b47091f4`](https://github.com/sanskarIN/latebrake/commit/b47091f49bf541c6b9d5277cf603089339ea2c95) passed portable C++20, Python bindings and docs after rival physics, traffic, pit, drafting and replay fixes. It counted **18,214 qualifying game-source C++ lines**, not the 200,000 target. Additional [PR #366](https://github.com/sanskarIN/latebrake/pull/366) has since merged to verify consecutive ghost motion; do not attribute the older source count to that newer revision. The v0.5.98 release independently captures **unmerged** draft physics rather than active-main changes. |
| Engine targets | Unreal Engine 5.8 + C++20 |
| Cross-platform portable tests | Windows, Ubuntu, macOS GitHub Actions |
| Unreal Editor/UHT | **Not yet verified** |
| Authentic project map cooking | **Not yet verified** |
| Windows playable binary | **Not available / not verified** |
| Android APK and real-device QA | **Not available / not verified** |
| Documentation website | GitHub Pages build, not game validation |

## What needs to happen before a stable release?

1. Compile the real Unreal Editor and game targets on an authorized UE 5.8 installation.
2. Create and cook the project-owned circuit map in Unreal Editor. Verify startup, collision and ordered gates through real runtime logs and driving.
3. Package genuine Windows/Android candidate artifacts. Verify platform structure and source/map identity using the existing scripts.
4. Test installed candidates, including pause, touch, physics, recovery, save loading, checkpoint timing and career progression.
5. Record honest QA evidence for **each exact package** before promoting it to a playable pre-release or stable release.

## Follow real project progress

- [Full engineering changelog and outstanding work](STATUS.md) — detailed version-by-version source notes.
- [Public roadmap](ROADMAP.md) — milestones are planned scope, not release promises.
- [GitHub Actions](https://github.com/sanskarIN/latebrake/actions) — source-level test and documentation workflows.
- [Android hardware qualification issue #13](https://github.com/sanskarIN/latebrake/issues/13).
- [Unreal directional checkpoint qualification issue #31](https://github.com/sanskarIN/latebrake/issues/31).
- [Latest published GitHub source pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.98).

Do not rely on CI badges or web deployment as proof that the game can currently be installed and played.
