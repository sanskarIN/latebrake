# Development & release status

<div class="lb-status-panel">
  <div class="lb-status-title">Source-ready does not mean game-ready</div>
  <p>Automated tests for portable game logic have passed, but the project still lacks verified Unreal Editor/UHT compilation, an authentic cooked circuit map, installed driving QA and official game binaries. This website itself is only documentation.</p>
</div>

## Current milestone

| Item | State |
| --- | --- |
| Latest published GitHub pre-release | [v0.5.100-test.2](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.100-test.2), experimental **SOURCE-ONLY**, direct tag `79a32648f11888725b1e7e827e222dbc9005aa68`, **zero executable attachments**. Published metadata does not satisfy the strict exact-target and exact-notes gate; no playable game is established. Official v0.5.100 remains separately gated. |
| Latest source-development measurement | [Exact-main audit](https://github.com/sanskarIN/latebrake/actions/runs/45646564285/job/139319032838) at `d79b31b1af59df3870f24cb75b755e0e20c8de28`: **112,808 qualifying production C++ lines in 452 files**, 87,192 remaining to the 200,000-line v0.6.0 target. Count step passed; full CI failed on stale release labels. Portable research source is not proof of native gameplay. |
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
- [Latest published GitHub source pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.100-test.2).

Do not rely on CI badges or web deployment as proof that the game can currently be installed and played.
