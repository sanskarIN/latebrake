# Development & release status

<div class="lb-status-panel">
  <div class="lb-status-title">Source-ready does not mean game-ready</div>
  <p>Automated tests for portable game logic have passed, but the project still lacks verified Unreal Editor/UHT compilation, an authentic cooked circuit map, installed driving QA and official game binaries. This website itself is only documentation.</p>
</div>

## Current milestone

| Item | State |
| --- | --- |
| Latest published GitHub pre-release | [v0.5.97](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.97), experimental source only, unmerged physics PR #295 commit `65ef6f2f192081b64f8256867403f40882cca462`; **zero executable attachments** |
| Latest source-development milestone | **v0.5.95 source-only:** original Unreal 3D starter-circuit straight-edge paint and 48 braking-zone rubber marks, all geometry bounded to the real asphalt collision blocks; optional Editor-authored PBR material tooling and SHA256-pinned candidate build source checks. [Draft PR #313](https://github.com/sanskarIN/latebrake/pull/313) remains unmerged; real authored materials, UE 5.8 Editor/UHT compile, Windows executable, 3D lighting and installed driving are still **unverified**. [Exact publisher verification](https://github.com/sanskarIN/latebrake/actions/runs/42339222627): **171 CTests on each Windows/Ubuntu/macOS and 621 Python source tests**, plus PowerShell policies and docs. Previous source-only snapshot: v0.5.94. |
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
- [Latest published GitHub source pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.95).

Do not rely on CI badges or web deployment as proof that the game can currently be installed and played.
