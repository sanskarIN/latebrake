# Development & release status

<div class="lb-status-panel">
  <div class="lb-status-title">Source-ready does not mean game-ready</div>
  <p>Automated tests for portable game logic have passed, but the project still lacks verified Unreal Editor/UHT compilation, an authentic cooked circuit map, installed driving QA and official game binaries. This website itself is only documentation.</p>
</div>

## Current milestone

| Item | State |
| --- | --- |
| Latest published GitHub pre-release | [v0.5.93](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.93), **experimental source only**, immutable unmerged PR #295 commit `9dd2b3236f25b242387233c988255881dd7aab59`; **zero executable attachments** |
| Latest source-development milestone | **v0.5.93 source-only:** unmerged [PR #295](https://github.com/sanskarIN/latebrake/pull/295) improves signed longitudinal opposite-pedal recovery during sideways slides, engine heat when motor force is blocked, physically delivered slipstream HUD readouts and non-shipping force telemetry with a hash-bound Win64 playtest inspector. The **exact source** passed 184 CTests each on Windows/Ubuntu/macOS and **674 Python tests**. The separate PR merge-preview passed 679 Python tests by adding five main-branch publisher-policy tests; see the [clarification](releases/v0.5.93-test-count-clarification.md). **UE5.8/UHT compilation, authentic cooking and installed Windows/Android driving remain unverified.** Previous pre-release v0.5.92 was also source-only. |
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
- [Latest published GitHub source pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.93).

Do not rely on CI badges or web deployment as proof that the game can currently be installed and played.
