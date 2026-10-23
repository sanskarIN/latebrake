# Latebrake roadmap

Milestones are goals, not claims of delivered features. Preserve offline single-player functionality.

| Version | Proposed scope | Gate |
| --- | --- | --- |
| v0.1.0 (pre) | First verified playable car + circuit + lap timing + desktop/mobile input + real packages | Unreal/editor/device validation outstanding |
| v0.1.1 (pre) | Physics and controls stabilization, recovery and saves | Regression fixes |
| v0.2.0 (pre) | Rival driving AI, event definitions, race results | Stable starter circuit |
| v0.2.1 (pre) | Race AI balancing and performance | Instrumented playtests |
| v0.3.0 (pre) | Original multiple cars, garage and tuning | Original licensed art |
| v0.3.1 (pre) | Upgrade/economy/persistence stabilization | Backward-compatible saves |
| v0.4.0 (pre) | First explorable fictional region and traffic | Streaming and UI |
| v0.4.1 (pre) | Experimental visible mobile steering/pedals plus pause/restart/recover touch pads | Engine compile, phone input and layout testing outstanding |
| v0.4.2 (pre, proposed) | Touch cancellation, input remapping and mobile performance pass | Low/mid-range real-device metrics |
| v0.5.0 (published source pre-release) | Two 3-round championships, six challenges, medals, career save v2 and prototype menu source | Three-OS portable CI passed; Unreal and device qualification pending |
| v0.5.1 (source-only pre-release) | Sector timing, career/save integrity, mobile touch input safety, instanced circuit geometry, build smoke guards | Portable Windows/macOS/Ubuntu CI; Unreal editor/map/cook/device qualification still missing |
| v0.5.2 (published source-only pre-release) | Directional swept checkpoints, recovery safety, finish/record integrity, career fairness, rival lap targets and mobile cancellation | Strict three-OS CI passed; Unreal build, original map and hardware playtests still missing |
| v0.5.3 (published source-only pre-release) | Classifiable checkpoint/gate rejection reasons and default-off local QA logging | Strict portable CI passed; real Unreal 5.8 build and installed device acceptance pending |
| v0.5.4 (published source-only pre-release) | Fixed-memory QA counters, bounded ten-second local diagnostics and offline log parser | Three-OS portable tests and policy checks; **not** Unreal/device-qualified |
| v0.5.5 (published source-only pre-release) | Reliable finish scorecard: theoretical sector gap, lap consistency, recent eight-lap history and final-lap trend | Strict three-OS source CI, but genuine Unreal 5.8/UHT and installed UI/device QA remain outstanding |
| v0.5.6 (published source-only pre-release) | Portable collision-bearing circuit plan and fail-closed nine-block / four-gate / 93-scenery startup checks | Strict C++/PowerShell source CI; real UE 5.8 UHT/editor, runtime collision and installed QA pending |
| v0.5.7 (published source-only pre-release) | Verify configured Unreal collision actors and validate player/rival recovery routes against original track/barriers | Strict three-OS portable C++/PowerShell source CI; Unreal 5.8 UHT/editor compile and installed physics/device QA pending |
| v0.5.8 (published source-only pre-release) | Validate expected blocking responses for Pawn, PhysicsBody and WorldDynamic during track actor startup | Strict three-OS C++20 source CI; real Unreal 5.8 UHT/editor compilation and installed physics QA pending |
| v0.5.9 (source development, not released) | Prevent race clock/scoring on incomplete circuit and display blocked-startup warnings | Strict portable RaceProgress-linked C++20 tests; genuine Unreal 5.8 UHT/editor, cooked map and installed HUD/gameplay QA pending |
| v0.5.10 (source development, not released) | Freeze race clock if local player pawn is unavailable; show honest driver-readiness warning | Strict portable RaceProgress-linked tests; licensed engine/editor and installed Win64/Android QA remain pending |
| v0.5.11 (source development, not released) | Isolate each checkpoint motion trail across menu pause/resume, and block awards without an active player | Strict portable four-gate C++ tests; actual UE 5.8 runtime/Win64/Android game QA still required |
| v0.5.12 (source development, not released) | Reject candidate manifest/root symlinks, junction-escaped files, linked archive directories and dirty tracked source | Disposable negative PowerShell fixtures; UE 5.8 UHT/map/cook and device QA still required |
| v0.5.13 (source development, not released) | Bind installed QA to SHA256 of the exact UAT candidate manifest rather than only source commit | Disposable policy negatives + strict three-OS source CI; actual UE/editor/cook and real hardware tests outstanding |
| v0.6.0 (pre) | Evaluated online foundation OR expanded offline modes | Does not harm offline play |
| v0.6.1 (pre) | Multiplayer stabilization OR offline stabilization | Reliable behavior |
| v0.7.0 (pre) | Art, audio, weather, presentation, replay study | Asset/license validation |
| v0.7.1 (pre) | Graphics and Android optimization | Hardware metrics |
| v0.8.0 (pre) | Localization, accessibility, verified Linux portability | Platform qualification |
| v0.8.1 (pre) | Cross-platform fixes | Compatibility matrix |
| v0.9.0 (pre) | Feature-complete beta | Freeze v1 declared scope |
| v0.9.1 (pre) | Release candidate | Security/quality gates |
| v1.0.0 (stable) | Complete tested released game per frozen scope | Win64 and Android real builds |

Playable game releases must include genuine installed packages, actual hardware/gameplay testing, change notes, known issues, and no unsupported promises. Source-only preview archives are **not** playable game artifacts.

### Engine-first roadmap requirement

Future milestones must first secure a real Unreal 5.8 editor/UHT build,
a project-owned cooked map, verified playable Win64 and Android packages,
and hardware evidence. No additional source-only pre-release implies
a playable game. Completing [issue #2](https://github.com/sanskarIN/latebrake/issues/2)
and [issue #13](https://github.com/sanskarIN/latebrake/issues/13)
is a prerequisite for closing the main quality gate.

### Critical work after the v0.5.5 source preview

Create and verify an engine-authored starter `.umap`, compile Unreal
Editor and game targets, cook/assemble a genuine Win64 game candidate,
run installed QA, then follow with Android candidate and mobile controls.
A manual authorized self-hosted runner may execute the new pipeline; the
ordinary CI cannot package Unreal games. No new version is being published
solely because packaging helper scripts now exist.
