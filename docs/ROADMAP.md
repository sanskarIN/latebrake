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
| v0.5.14 (published source-only pre-release) | Preflight authentic UAT output structure for Win64 EXE/cooked payloads and Android APK contents before archiving | PowerShell disposable rejection fixtures; actual Unreal and target-hardware QA still required |
| v0.5.15 (published source-only pre-release) | Unified race session epoch on successful restart and menu return, no Quick Race double reset | Standalone real-gate C++ regressions, then real Unreal 5.8/hardware follow-up |
| v0.5.16 (published source-only pre-release) | Shared 16-finger touch identity contract | Cross-platform touch policy tests; physical Android QA #13 remains open |
| v0.5.17 (published source-only pre-release) | Live race pace projection in expanded HUD from valid recent lap history | Portable real RaceProgress tests; Unreal HUD and timing device QA pending |
| v0.5.18 (published source-only pre-release) | Rival AI is gated by player race readiness, circuit, menu/pause and finished state | Real C++20 RaceProgress regression; Unreal physics QA pending |
| v0.5.19 (published source-only pre-release) | Finite rival recovery budget prevents uncounted AI teleports | Portable real route stress regression; installed Unreal QA pending |
| v0.5.20 (published source-only pre-release) | Prevent duplicate and backdated laps in bounded recent history | Strict ring-buffer source regression; UE gameplay QA outstanding |
| v0.5.21 (published source-only pre-release) | Neutralize captured Android touch input on invalid viewport press/move regardless of finger identity | Cross-platform touch regression; hardware Android rotation/focus QA outstanding |
| v0.5.22 (published source-only pre-release) | Neutralize nonfinite press from an existing driving finger without affecting others | Portable touch ownership and malformed-event regression; installed Android QA required |
| v0.5.23 (source development, not released) | Wire malformed duplicate touch safety through the actual Unreal input callback, preserve one-shot action presses and isolate held controls | Portable admission/regression tests and three-OS CI; real Android press-event QA #13 remains open |
| v0.5.24 (source development, not released) | Reject untracked C++/config and modified tracked source before/after editor map authoring, before UAT packaging and candidate manifest writing; pin the original source HEAD throughout the candidate build | Synthetic Git policy tests on Windows CI; genuine Unreal builds/QA still pending |
| v0.5.25 (source development, not released) | Retain one circuit map SHA256 throughout Editor→UAT→manifest and require Win64/Android map identity equality | Disposable PowerShell negative tests and strict CI; actual UE build and installed hardware playtests still required |
| v0.5.26 (source development, not released) | Reject paid player recovery while the circuit is incomplete, the player is missing, the race/menu is paused or the race is finished | Real RaceProgress-linked C++20 regression matrix; Unreal gameplay/device recovery QA still required |
| v0.5.27 (source development, not released) | Require an actual simulated, collision-bearing, finite player chassis before racing time/gates/recovery; rebase checkpoint traces on physics loss | Real RaceProgress-linked three-OS C++20 tests; genuine Unreal Chaos and device QA still pending |
| v0.5.28 (source development, not released) | Process Android touch release callbacks even during paused/menu worlds so held finger slots re-arm only after physical lift | Three-OS portable C++20 lifecycle plus Python engine-source binding policy; UE and real Android QA still pending |
| v0.5.29 (source development, not released) | Clear filtered motor/steering commands on touch lift, invalid coordinates and move from GAS to neutral/BRAKE; preserve independent finger ownership | Three-OS portable C++20 tests plus engine-source callback contract; physical Android/Chaos QA remains open |
| v0.5.30 (published source-only pre-release) | Reset swept checkpoint baselines on invalid physics velocity; require a new finite sample before awarding a genuine forward gate | Four-gate real RaceProgress C++20 regression; UE 5.8 and installed driving QA still required |
| v0.5.31 (published source-only pre-release) | Surface invalid-velocity checkpoint discontinuities in opt-in local summary logging; parse extended and archived QA logs without conflating types | C++20 category tests, Python parser/source contracts, and actual Unreal log/device evidence still required |
| v0.5.32 (published source-only pre-release) | Show non-compact HUD current-lap potential from positive sector bests and difference to actual best lap; no reward or record change | Portable real RaceProgress tests; UE HUD rendering and installed Win64/Android qualification still required |
| v0.5.33 (published source-only pre-release) | Show current-sector time remaining until measured same-sector best or time already over that best in the expanded HUD; strict portable and source-binding regression | Source C++20 and Unreal-HUD binding checks; UE 5.8 editor/cook and installed-device verification outstanding |
| v0.5.34 (published source-only pre-release) | Guard modern source pre-release workflows against malformed CTest filters, duplicated jobs and mismatched immutable source/tag identity | Python standard-library negative fixtures in CI; GitHub workflow semantic execution and Unreal/game builds remain separate gates |
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
### Recent source-only milestones

| Version | Source scope | Qualification outstanding |
| --- | --- | --- |
| v0.5.44 (published source-only pre-release) | Bind Unreal build/cook candidate and Win64/Android pair to exact 5.8 Build.version SHA256 in new schema-2 manifests | Synthetic PowerShell version mutations and manifest pair checks; real UE binary, packaging and installed-device QA pending |
| v0.5.43 (published source-only pre-release) | Add independently latched manual PAUSE fallback so rejected Unreal world pause cannot leave clock, gate, AI, recovery or physical controls active | C++20 standalone cross-boundary tests and Python callsite checks; real Unreal failed-pause/Android QA still required |
| v0.5.42 (published source-only pre-release) | Unify player driving readiness across menu/world pause, focus guard, unavailable circuit and finished results so no force accrues with stopped clock | Pure C++20 readiness matrix and Unreal source contracts; real UE 5.8 pause-failure physics and installed package QA needed |
| v0.5.41 (published source-only pre-release) | Bind genuine UE 5.8 authored map receipt to exact SHA256 saved bytes and reject same-size modified files in independent QA policy | Windows synthetic tamper cases plus Python Unreal stubs passed; real UE map authoring/cook and installed device QA pending |
| v0.5.40 (published source-only pre-release) | Require active clock advancement on every completed lap before any award or final insight; reject zero-duration and penalty-only laps | Real RaceProgress C++20 regressions and source-binding checks; Unreal 5.8, cooked map and installed package QA pending |
| v0.5.39 (published source-only pre-release) | Neutralize player momentum/inputs when OS focus-loss pause fails and allow only fresh PAUSE touches to resume | Three-OS portable policy tests plus Unreal source contracts; Unreal physics and real-device qualification remain outstanding |
| v0.5.38 (published source-only pre-release) | Ensure every new OS focus-loss suspension invalidates checkpoint motion even if Unreal was already paused; retain manual resume | Strict portable C++20 plus Unreal-source binding checks; actual Unreal/Android/Win64 interruption and crossing QA outstanding |
| v0.5.37 (published source-only pre-release) | Attribute charged recovery fees to current and last completed lap only at ordered lap boundaries; show actual attribution on non-compact HUD | Strict three-OS C++20 and Editor binding contracts, plus required future Unreal/device rendering QA |
| v0.5.36 (published source-only pre-release) | Track only successfully charged recovery/time fees atomically; show already-included total on HUD/results and verify source integration | Strict C++20 and Python HUD contracts; installed Unreal and device rendering/physics QA remains open |
| v0.5.35 (published source-only pre-release) | Suspend player time, checkpoint scoring, recoveries and rival driving on app focus loss independently of a successful engine world pause; explicitly resume after player action | C++20 and source bindings; genuine Unreal 5.8 and installed Win64/Android focus-loss QA still required |
