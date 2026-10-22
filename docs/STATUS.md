# Latebrake engineering status

**Updated:** 2026-10-22
**Latest published milestone:** [v0.5.2 source-only pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.2).
**Next development milestone:** v0.5.3 checkpoint diagnostics (NOT released).
**Playable status:** Not verified; Unreal Engine 5.8 editor/UHT game compilation, cookable original map, installed Windows/Android packages and hardware driving QA remain outstanding.

## v0.5.3 development — deterministic gate diagnostic outcomes

- Added explicit classification for valid gate crossings, invalid samples or velocity, wrong-way/slow motion, oversized frame travel, absent plane crossings, lane misses and height misses.
- Motion trace details distinguish a first baseline, an authorized teleport generation rebase, malformed positions and a legitimate crossing without changing the existing bool scoring method.
- Added strict portable C++20 tests for the four gate directions and diagnostic/boolean parity.
- The diagnostic evaluator is engine-independent. An additional default-off, non-shipping `latebrake.CheckpointDiagnostics` Unreal console variable logs local event/reason transitions, with no network output. **Unreal compilation, console behavior, logging and installed-device validation are unverified.** See [gate diagnostic design](GATE_DIAGNOSTICS.md).

## v0.5.2 published — source-only race integrity

- Genuine bounded forward checkpoint plane crossings with low-FPS swept sampling and four ordered sectors. A 1,200 cm frame-distance cap remains; measured real-world frame-hitch acceptance is pending.
- Car-local teleport generations reject even short recovery/restart shortcuts, and pawn changes rebase samples.
- Invalid finished elapsed/lap times cannot award unrecordable records, credits, medals or completed counters.
- Career replay stages required-car selection and persists before restarting, with save-failure rollback; active career races lock garage mutations.
- Rival lap targets follow the event and Android viewport interruption clears residual smoothed drive input.
- The release workflow passed Windows/Ubuntu/macOS C++20, PowerShell negative packaging policy and Python editor stub tests. **No verified game executable or Android APK/AAB was built or attached.**
- [Release notes](releases/v0.5.2.md) and [release page](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.2).

## GitHub releases (source-only)

- [v0.1.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.1.0) — published pre-release of the original C++ project and racing foundation.
- [v0.1.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.1.1) — published pre-release of racing timing, fastest lap and checkpoint stability.

- [v0.2.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.2.0) — offline circuit/time-attack and experimental rival source.
- [v0.2.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.2.1) — AI steering/recovery source stabilization.
- [v0.3.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.3.0) — garage source foundation.

- [v0.3.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.3.1) — garage and HUD source hardening.
- [v0.3.2](https://github.com/sanskarIN/latebrake/releases/tag/v0.3.2) — driving input and cache safety.
- [v0.4.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.4.0) — circuit decorations and north-up minimap.
- [v0.4.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.4.1) — experimental Android control HUD and touch reliability.
- [v0.5.0](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.0) — career, two championship series, challenges, offline progression and source-level development menu.
- [v0.5.1](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.1) — split timing, save/launch safety, mobile touch guards, batched circuit scenery and stricter build checks. **Source archive only; no installed game.**

- [v0.5.2](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.2) — checkpoint and race integrity fixes; source-only, no playable build.\n\nThese tags are source previews. They are **not downloadable playable game installers**.

## v0.4.1 published source work (NOT engine verified)

- New portable touch-pad rectangles shared by the Android HUD and input hit-testing, for steering, gas, brake, pause, restart and recovery.
- Touch capture starts only with presses on visible pads; nonfinite input is neutralized, pedal drags through the gap release both pedals and moves cannot capture an unrelated finger.
- Touch pause action is bound to work when the game is paused; restarting while paused unpauses before race reset.
- Compact HUD prioritizes timing and vehicle status over upgrade pricing, leaving space for large touch pads.
- Added engine-independent C++20 tests for responsive layout, pad separation, steering values and invalid coordinates; standalone smoke testing performed with GCC 14, not Unreal.
- **Outstanding:** compile and playtest the Unreal touch dispatcher, pause behavior and HUD on Android hardware; validate lifecycles and accessibility; do not imply a playable APK.

## v0.4.0 changes

- Centralized race circuit reference data for player spawn, gate checkpoints, recovery and rival AI waypoints; reduces divergence among duplicated literals.
- Added pure C++20 decoration plan and runtime primitive road marking, curb, spectator stand and banner source code.
- Added nonblocking cosmetic geometry (still one static-mesh actor per piece; future instanced-mesh optimization needed).
- Added north-up minimap projection and experimental desktop HUD circuit visual, player position and next gate.
- Expanded standalone C++20 tests for course consistency, decoration placement, and invalid/outside positions.
- No new drivable track variant, Unreal Editor validation, cookable original map, or playable binary claims.

## v0.3.2 development branch

- Pure C++20 input neutralization for nonfinite/overflowing values, touch scaling and smoothed throttle commands.
- Player/AI throttle inputs are sanitized; pause and recovery clear analog and touch controls.
- Simulated car tick ignores nonfinite frame time and resets corrupt/nonfinite physics linear velocity.
- The game mode caches selected garage tuning and only updates when selected vehicle/upgrade levels/pawn change.
- Failed SaveGame writes set an explicit HUD warning.
- Added portable tests for input safety and garage tuning cache keys.

This does not constitute an Unreal 5.8 engine build, an instrumented driving test, or a ready-to-install game package.

## v0.3.1 changes

- Corrupt negative garage mask now resets ownership to Arrow, not all prototypes.
- Garage model exposes next affordable unlock, next owned car and upgrade prices; game-mode actions provide feedback to the HUD.
- HUD text layout adapts to viewport width/height and offers condensed information for small screens, though Unreal render tests are still required.
- Saved race times are checked for invalid/malformed values; race completion counter saturates instead of overflowing.
- Portable C++20 tests cover malformed saves, feedback-related garage model queries, responsive HUD metrics and record bounds.

## v0.3.0 added in this development branch

- Original fictional starter garage profiles, with different force/grip/speed multipliers and primitive-only cabin silhouettes.
- Offline earned-credit unlocks and 3-level Engine/Grip upgrades with saturation and validation.
- SaveGame fields to persist selected car, owned vehicles and upgrade levels (legacy fallback).
- HUD and keyboard controls for garage operations (Q/E/U/I).
- New C++20 garage economy/progression regression tests.
- Does not include authored complete car models, wheel suspension, a production garage menu, or any tested game packages.

## v0.2.1 improvements (merged and source preview published)

- Deterministic rival waypoint-lap and stalled-car state tracker with bounded recovery count.
- Integrates self-recovery after eight seconds of sustained near-zero speed, preserving route progress.
- Moves AI planning ahead of its own base-car physics tick; additional steering smoothing for both human and autonomous inputs.
- Starts braking earlier near a waypoint; rejects negative speed readings.
- HUD now labels route laps explicitly as approximate AI telemetry, not scored race placement.
- Adds pure C++20 CTest targets for route progress and steering filter.

## v0.2.0 implementation (merged and source-preview published)

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
- Check the newest branch-head workflow before tagging a pre-release; tests cannot exercise Unreal gameplay symbols.
- GitHub CI does **not** have Unreal Engine source/SDK/tooling available.

## Not implemented or not qualified

- Unreal 5.8 compilation, Unreal Header Tool generation, in-editor driving/playtesting, performance profiling or packaged hardware testing.
- A cooked project-owned map, original finished vehicle model, Chaos Vehicles wheel/suspension rig, production-ready mobile controls and tested phone lifecycle behavior.
- Race-aware overtaking, full AI opponent ranking/finished times, traffic, production garage/championship UX and multiplayer. Prototype upgrades, garage logic and championship source already exist; they still need actual engine playtesting.
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

## v0.5.0 published source pre-release audit (2026-10-20)

| Subsystem | Source state | Remaining gate |
| --- | --- | --- |
| Race & checkpoint clock | Four ordered gates, lap timing, slowest-lap tracking | UE/physics playtest |
| Career catalog | Stable IDs, 12 events, prerequisite graph | Real UI and balance QA |
| Championships | Two three-round cups, medal points and once-only bonuses | UE integration playtest |
| Challenges | Six portable validation rules incl. clean/precision/consistency/Kestrel/endurance | Device timing and vehicle QA |
| Garage | Existing Arrow/Kestrel/Phantom saved state preserved, bounded credits | Actual save migration |
| SaveGame v2 | Career medals/selection + unit preference appended; future schema write guard | Unreal serialization, interruption/rollback tests |
| Menu/UI | 10-page asset-free paused Canvas menu, career selection, speed unit toggle, device touch actions | Editor compile, viewport/accessibility and UMG work |
| Mobile lifecycle | Deactivation/background/foreground/unpossess cleanup source | Physical Android cancel/focus-loss tests |
| CI | Portable CMake/CTest across Windows, macOS, Ubuntu | Does not compile Unreal |
| Packaging | No verified game binaries or project-owned cookable map | UE installed/cook/package/hardware evidence |

Issues [#2](https://github.com/sanskarIN/latebrake/issues/2) and
[#13](https://github.com/sanskarIN/latebrake/issues/13) are **still open**;
source CI cannot close them. The current in-container environment has a
C++ toolchain but **no Unreal Engine 5.8 installation**, and the GitHub
connection cannot manufacture validated game packages.

The published v0.5.0 tag is an immutable **source-only pre-release** at merge commit `ad17b47886b8d3f9633299f74d85b4949aa1b5a8`. Release and core CI each passed Windows, Ubuntu and macOS portable tests; PowerShell syntax validation also passed. GitHub contains no attached executable assets. These checks do not validate the Unreal runtime or physical device behavior.

## v0.5.1 source-release build readiness — no executable yet

- Default game map and cooker now target `/Game/Maps/LatebrakeCircuit`.
  The actual Unreal-generated `.umap` **does not exist in the repo yet**.
- Removed unused ChaosVehicles plugin and game-module dependency; this
  project currently uses a physically simulated rigid-body prototype.
- Added Unreal Engine 5.8 UBT Editor compilation helper, Editor-only
  Python map-authoring script, PowerShell wrapper and UAT BuildCookRun
  pipeline for real Win64/Android candidate packages.
- Added platform-specific package shape/hashes + self-declared installed
  QA verification and negative test fixtures; Windows CI checks syntax.
- Added a manual self-hosted UE 5.8 Actions workflow for candidate artifacts,
  with no automatic release attachment and no claim that a runner is
  connected.
- **Still unverified:** real UHT/UBT compilation, map creation, cooking,
  installation, Windows/Android physics/menu/save QA, device performance
  and game binaries. Issues #2 and #13 remain open.
- **v0.5.0 remains an immutable source-only snapshot**, and v0.5.1 is also a source-only prerelease. Neither version adds playable EXE/APK assets.

See [actual packaging procedure](EXECUTABLE_BUILD_PIPELINE.md).

### v0.5.1 source changes since v0.5.0

- Fixed editor-map existence checking that erroneously queried the
  `EditorAssetLibrary` API for a **level**; Unreal documents that API as
  excluding levels. Existing levels are now preserved and loaded.
- Added seven isolated Unreal Editor script flow tests using disposable
  fake engine objects, **not** engine assets.
- Candidate packaging refuses nonempty output folders, unreadable or
  forged `.umap` headers and missing real engine/map prerequisites.
- Added UNVERIFIED build provenance manifest (source commit, engine/map
  and per-file SHA256); no gameplay success is inferred from hashes.
- Fixed pause key handling so an open development menu closes before
  the game resumes. Real Unreal focus/lifecycle behavior still requires QA.
- No Win64 EXE or Android APK has yet been produced or attached.
  Issues #2 and #13 remain open.

### Additional pre-release engine-readiness work — no executable yet

- Android runtime source now includes a dedicated local PlayerController for
  game-only input and custom touch UI, connected to RaceGameMode.
- UE map-authoring now demands a **fresh successful editor save receipt**,
  rather than trusting process exit code and preexisting map alone; failure
  paths are covered by engine-free stub tests.
- The runtime circuit emits a **4/4 checkpoint launch marker**; an independent
  PowerShell script can check a real installed-game log after Unreal/packaging
  actually runs. CI deliberately tests only failure cases with synthetic text.
- These changes have **not** been compiled against UE 5.8 or device tested;
  v0.5.1 is still a source-only pre-release, with issues #2 and
  #13 open pending actual game binaries and QA.

### v0.5.1 — source-level SaveGame failure recovery improvements

- `ProfileWriteCheckpoint` captures the previous garage, career medals,
  completed-race count and best race/lap/time-attack times before committing
  a result. An unsuccessful `SaveGameToSlot` now restores the session's
  previous values rather than leaving unsaved credits and unlocks usable.
- Garage purchases, active vehicle selection, engine/grip upgrades and
  MPH/KMH preference likewise restore prior values on a failed write.
- Portable tests cover retryable first-completion rewards and
  championship bonus after a deliberately failed save.
- **This is not a crash-safe atomic filesystem transaction.** No genuine
  UE SaveGame serialization, disk-failure simulation, hardware QA or
  executable has run here. v0.5.1 is also source-only and issues #2/#13
  remain open.

### v0.5.1 held-touch input stabilization (source only)

- Engine-independent `TouchDriveState` now owns two-finger driving input;
  the Unreal pawn routes press/move/release and lifecycle/menu clears
  through it rather than duplicating mutable finger flags.
- Control safety: second/third fingers cannot steal controls; dragging
  from telemetry cannot activate pedals; pedal neutral gaps, vertical
  steering departures, and invalid coordinates neutralize input.
- A standalone pause suppresses new gas/brake/steering capture until
  an explicit resume. Held control HUD states share the physics input
  source. Strict portable C++20 tests cover the state machine.
- The tests can pass on all three OSs without the Unreal SDK, **but this
  is not proof of Android input delivery or a working Unreal executable**.
  The same real map/engine/platform build blockers remain open.

### v0.5.1 mobile viewport input safety (source only)

- Touch viewport dimensions are now observed before each Android car
  physics tick, and within every touch event. A size/orientation change
  cancels existing steering and pedal ownership immediately.
- Physical finger tracking is maintained across geometry changes to avoid
  duplicate action firing and stale-control reacquisition until release.
- Disposable portable C++20 tests cover rotate, resize, invalid viewport,
  delayed movement, duplicate press and genuine re-press behavior.
- This does not prove Android touch-event dispatch, frame performance or
  packaged-game playability; keep engine/device qualification issues open.

### v0.5.1 runtime startup smoke gate (source only)

- The future real installed-game log verifier now requires **both** the
  authentic four-gate startup marker and one complete, earlier batched
  scenery marker (`7` groups and `93` instances). A missing, duplicate,
  malformed or explicitly incomplete scene cannot pass the gate.
- Disposable CI parser fixtures exercise only **rejections**. They are not
  runnable Unreal evidence, renderer benchmarks or validated gameplay.

### v0.5.1 instanced course visuals (source only)

- Single prototype circuit decoration plan now groups 93 static cosmetic
  cube placements into seven color/style batches under one Unreal actor
  using `UInstancedStaticMeshComponent`, rather than 93 separate actors.
- Existing asphalt/ground/barrier collision is preserved separately;
  decorative collision, overlaps, navigation and shadow casting are disabled.
- Portable C++20 regression tests compare every proposed batched
  placement against the existing stripe/curb layout and verify group counts.
- Startup logs distinguish complete vs incomplete instance creation.
  **UE 5.8 UHT/Editor compilation and renderer/phone profiling are not
  verified**; no FPS or verified packaged build is claimed.

### v0.5.1 progression and recovery safety (source only)

- A career event now stages its event ID and any required owned vehicle
  inside an in-memory profile checkpoint, persists that selection, and
  **only then** resets/reconfigures the active race. Failed SaveGame writes
  reject the launch and restore the previous profile without switching races.
- Player RECOVER charges an entire 5-second penalty and increments its
  recovery counter before repositioning; a finished race, unchargeable
  clock or saturated counter cannot grant an uncharged teleport.
- Portable C++20 tests check invalid/locked event selection, missing required
  vehicle, fail/retry/commit semantics and recovery count/time overflow.
- There is still no UE 5.8 engine build or real Win64/Android installation.
  These assertions are engine-facing **source integration**, not hardware QA.

### v0.5.1 race split timing (source only)

- Portable `RaceProgress` now tracks the fastest strictly positive split
  for each ordered sector and compares a fresh split only with the **previous**
  best of that identical sector. Negative delta means faster.
- A run-local theoretical lap becomes available once every sector has a
  valid best. It is labelled as theoretical, not a completed personal best,
  and is cleared on restart/event switch instead of being written to save.
- The non-compact HUD renders these diagnostics. A separate strict C++20
  SectorTiming regression target covers ordering, time penalties, signed
  deltas, reset and numeric bounds.
- No Unreal Editor/UI testing or actual installed gameplay was performed.
  See [sector timing acceptance tests](SECTOR_TIMING.md).

### v0.5.1 release integrity work — no executable published

- A candidate's post-UAT SHA256 file manifest is now verified **immediately**
  after packaging, not just written and uploaded without a check.
- The manifest verifier requires exact file count, paths, sizes and hashes.
  It rejects stale/missing, extra or modified archive files, unsafe relative
  paths, dirty tracked source trees and wrong project/platform metadata.
- Both platform-specific QA gates require a manifest and human installed
  test evidence with `build_commit` equal to the archive source commit.
- The dual-platform release script delegates to both full platform gates
  and rejects different source commits or Unreal versions.
- CI uses only short-lived synthetic text files for integrity tests. **No**
  actual Unreal Engine build, cooked level, executable, APK or installed
  game was validated or published by these checks.

## Unreleased v0.5.2 event-specific rival lap targets

- The source-level rival pilot now configures waypoint count and required
  route laps from the selected player race rather than remaining fixed at
  the three-lap quick-race default through championship event changes.
- Existing and newly spawned rivals are reinitialized per event; career
  replays clear the rival's previous route/stuck/recovery state.
- The desktop HUD labels the rival's **approximate** completed/expected
  waypoint loops to avoid misrepresenting a short career event.
- The cross-platform portable route tests exercise all current
  opponent-enabled career events, replays and corrupt route dimensions.
- Still **source-only**. No genuine Unreal engine compile, gameplay,
  Windows executable or Android APK verified. Published v0.5.2
  source-only pre-release is unchanged.
