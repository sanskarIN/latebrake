# Latebrake engineering status

**Updated:** 2026-10-25
**Latest published milestone:** [v0.5.43 source-only pre-release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.43), tagged at tested source commit `8d5cd3d3576877f4efa79f696be6d318d2d95e8a`.
**Next engineering priority:** licensed UE 5.8 Editor/UHT and game compilation, authored cookable circuit map, and installed Windows/Android focus-loss/physics QA with evidence. Historical v0.5.9–v0.5.13 changes were merged without individual release tags.
**Playable status:** Not verified; Unreal Engine 5.8 editor/UHT game compilation, cookable original map, installed Windows/Android packages and hardware driving QA remain outstanding.

## v0.5.43 published source-only pre-release — standalone PAUSE survives rejected world pause

- Detected a distinct gap not covered by v0.5.42's player-driving readiness: a keyboard/gamepad/touch **standalone PAUSE** pressed while the engine was running had no independent fallback state if Unreal's `SetGamePaused(true)` did not actually pause. As a result, the race clock and ordered gates could continue while the player believed PAUSE was requested.
- `ManualRacePauseGuard` now latches first-press intent before the world-pause request and clears only on explicit second press, menu return or accepted restart. All authoritative clock, checkpoint, recovery, rival and pawn-driving consumers respect this latch. PAUSE touch dispatch and HUD remain usable when world pausing fails.
- Source regression tests cover failed PAUSE/UNPAUSE, real race-progress clock behavior, checkpoint session revision, recovery/rival denial and player input readiness. No UE Editor/UHT compiled artifact, Chaos runtime or real Win64/Android installed pause QA is claimed. See issues #31 and #13.
- **Published asset-free SOURCE-ONLY pre-release on 2026-10-25.** The pinned independent workflow passed Windows/macOS/Ubuntu strict C++20 tests, isolated `ManualRacePauseGuard` regressions, Python Unreal source integration and Windows PowerShell policy. Licensed UE/UHT, real pause failure, cooked circuit, playable executables and installed device physics remain unverified.

## v0.5.42 published source-only pre-release — ordinary menu pause and finished-race driving safety

- Identified a gap after the v0.5.39 focus-loss safeguard: when an ordinary menu or manual world pause request fails (or the race is finished while the engine still ticks), player input still flowed through the pawn's force/torque integrator even as GameMode clock and checkpoint scoring were stopped.
- Added portable typed `PlayerDriveReadiness` gating of authoritative GameMode, circuit readiness, menu, engine pause, focus interruption and finished-race state. The pawn now neutralizes driving input and existing linear/angular momentum **before applying any thrust or torque** if those states are blocked. Explicit close/resume/restart makes active racing eligible again.
- Portable C++20 tests cover all 64 combinations; Unreal source-binding tests check real callsite ordering, not engine behavior. Real UE 5.8 Editor/UHT, actual pause-failure/Chaos behavior, cooked map and installed Windows/Android QA remain outstanding.
- **Published asset-free SOURCE-ONLY pre-release on 2026-10-25.** Independent publisher run passed strict Windows/macOS/Ubuntu C++20 tests, isolated `PlayerDriveReadiness` checks, Python source contracts and PowerShell package-policy tests. Real UE 5.8/Chaos pause delivery, authentic cooked map, installed Win64/Android builds and driving QA are still outstanding.

## v0.5.41 published source-only pre-release — real map receipt integrity safeguards

- The prior Editor-generated `starter-map-result.json` established package name and byte length, but not byte equality; a modified map of identical length could satisfy the local receipt. A direct `New-StarterMap.ps1` invocation also did not independently validate engine/project version 5.8.
- Require UE `Build.version`, project EngineAssociation, actual Editor-reported version, versioned SHA256 receipt and exact saved-file hash match before accepting the map. Existing serialized-header, post-authoring source provenance and pre-/post-UAT map snapshot checks remain in place.
- New synthetic PowerShell negative tests and Unreal Python mocks explicitly do not stand in for actual Editor execution; see [MAP_AUTHORING_QA.md](MAP_AUTHORING_QA.md).
- **Published SOURCE-ONLY pre-release on 2026-10-25, with zero compiled game assets.** The independent pinned publisher passed Windows/macOS/Ubuntu portable C++20 checks, Windows synthetic map-authoring tamper cases and Python Editor stub tests. Actual licensed UE 5.8 Editor/UHT, authentic cooked `.umap`, packages and installed device QA remain open.

## v0.5.40 published source-only pre-release — per-lap active clock eligibility

- Found a source-level completion/award consistency gap: a positive overall race time could coexist with a completed zero-duration lap, or a penalty-only lap could look timed despite no successful active clock update. The previous award boundary validated elapsed/best times but not independent timing evidence for **every** completed lap.
- Track `ActiveTimedLapCount` separately from the pre-existing penalty-inclusive `TimedLapCount`. The count increases once per completed lap only after a successful `RaceProgress::Advance` has advanced the actual clock in that lap; reset and rejected deltas clear/retain evidence correctly.
- `FinishTimingEligibility` requires a finished ordered event, a representable elapsed/last/best/slowest clock, and both lap counts equaling required laps. GameMode rejects ineligible finishes **before** transaction construction, awards, credits, record updates or SaveGame I/O; the source HUD and finished insights do not claim a verified result.
- Regression cases include valid multi-lap penalties, zero-time first laps, penalty-only first/final laps, invalid deltas, overlong clocks, restart, and one-gate races. Real Unreal 5.8/UHT, editor map cooking, runtime checkpoint timing, Windows and Android installed QA remain **unverified**.
- **Published SOURCE-ONLY pre-release on 2026-10-25, with zero compiled game assets.** The independent publisher passed the complete Windows/macOS/Ubuntu portable C++20 suite, focused `FinishTimingEligibility` test, Python source-contract tests and packaging-policy fixtures. Genuine Unreal Engine and installed device gameplay remain unverified.

## v0.5.39 published source-only pre-release — neutralize player motion if OS pause fails

- Address a remaining fair-play and input safety hole: v0.5.38 stopped the race clock, checkpoints, recoveries and rivals on application deactivation, but a failed Unreal world pause could leave the *player chassis* simulating momentum and accepting newly delivered inputs while competitive time was stopped.
- Stop held/filtered drive input and zero finite physical linear/angular velocity on the new focus-loss event and during each surviving pawn tick while the guard is latched. This policy intentionally sacrifices momentum continuity on interruption for fair timing; actual Chaos motion still requires testing.
- Fail closed on suspended mobile touch: only a fresh PAUSE tap can intentionally resume; throttle, steering, dragging, recover, restart and career pads cannot dispatch. Physical press identity tracking remains in place.
- The independently gated GitHub release workflow passed full portable C++20 on Windows, macOS and Ubuntu, Python source-contract tests and PowerShell fixture tests, then published the unchanged commit as an asset-free prerelease. The release includes new portable touch-policy C++20 tests, CMake matrix registration and Python source-binding assertions. **Published SOURCE-ONLY prerelease on 2026-10-25, not qualified as a runnable game.** Licensed Unreal 5.8/UHT, real-world physics/OS pause failure and Android tests are still open in #31 and #13.

## v0.5.38 published source-only pre-release — already-paused app focus-loss checkpoint integrity

- Found that `OnFocusLost` returned `false` on a new focus loss if Unreal was already world-paused by its OS. GameMode consequently returned before invalidating the checkpoint motion epoch, despite the race suspension latch activating.
- The guard now returns one of three typed `FocusLossTransition` results. GameMode advances the checkpoint session epoch on **any new active suspension**, while requesting `SetGamePaused` only if needed and ignoring repeated notifications. Foreground still never automatically resumes competitive timing.
- Portable regressions exercise real motion samples through an already-paused interruption, duplicate OS notifications and deliberate recovery; Python source contracts require the actual engine callsite ordering.
- The code remains unqualified in genuine UE 5.8/UHT, the cooked map, Android/Windows OS delivery and installed-device driving. See issues #31 and #13.

## v0.5.37 published source-only pre-release — per-lap fee attribution

- The portable race state records a run-local snapshot of charged fees at an **accepted lap-completing gate**. It exposes already-paid current-lap and last-completed-lap fees without modifying event totals, lap timing, awards or saves.
- The expanded source HUD surfaces `THIS LAP PENALTIES` only when applicable; the results menu can show already-included final-lap penalties. A penalty-free lap following a charged lap is correctly displayed as having zero new fees.
- Added strict C++20 multi-lap and one-gate attribution regression coverage plus Unreal HUD source binding tests. No UE Editor/UHT, map cooking, device render or installed executable was validated by source CI.

## v0.5.36 published source-only pre-release — committed time-penalty telemetry

- Added allocation-free count and total for successfully charged race time penalties in the genuine engine-independent `RaceProgress`. The audited increment matches the representable amount actually added to the race clock; invalid/undercharged/saturated fees cannot mutate any accounting.
- Expanded HUD and results menu show the penalty total **already included** in the race clock, split and lap times. No extra scoring charges or profile/save fields were introduced.
- New strict C++20 `TimePenaltyAccounting` test and Python Unreal-HUD callsite checks cover recovery, finish, reset, malformed charges, numerical edge cases and on-screen source integration.
- Source CI does not prove UE 5.8/UHT editor compilation, authentic cooked maps, installed Windows/Android driving or actual HUD appearance. Those remain the release gates.

## v0.5.35 published source-only pre-release — app focus-loss race integrity

- OS deactivation/background previously cleared pawn input but did not
  itself guarantee the authoritative game race clock was paused.
  A platform could continue ticking while the player could not drive.
- Introduced `AppFocusRaceGuard`: once gameplay loses focus, timing,
  ordered checkpoint scoring, paid recovery, and autonomous rival driving
  all reject updates even if Unreal's `SetGamePaused` fails or the OS
  resumes an already-paused world. Foreground notifications never lift
  the latch; only a deliberate player resume, menu return or accepted
  restart does so.
- Runtime Pawn lifecycle bindings delegate deactivation/background to
  the GameMode. All real consumers (GameMode tick, gate actor, recovery,
  rival) observe the latch. Session revision invalidates pre-loss gate
  motion on an active deactivation.
- Added strict portable C++20 real-`RaceProgress.cpp` tests and Python
  source-binding contracts. No real Android/Windows OS delivery, UHT/UE
  5.8 compilation, device pause/render or installed gameplay is claimed.
- Follow-up hardware scenarios are documented in
  [Touch controls](TOUCH_CONTROLS.md) and remain covered by issue #13.

## v0.5.34 published source-only pre-release — release workflow integrity

- Source-only pre-release v0.5.32 initially hit a malformed YAML CTest
  filter before any tag was created. The workflow was repaired and retested.
- New standard-library `SourceReleaseWorkflowPolicyTests.py` checks
  modern source-only release workflow structure: exactly one of each required
  job, three-platform C++ matrix, valid isolated CTest filter, immutable
  three-job checkout SHA and matching publisher SHA, proper notes/version
  identity, predecessor gate and explicit asset-free pre-release policy.
- Mutation fixtures ensure that malformed regex quoting, duplicated
  release jobs, inconsistent source SHAs, wrong tag/notes path, and
  missing source-only warnings fail closed before a new source release.
- It is a **structural source check**, not a full YAML parser or proof of
  Unreal Editor builds. Existing GitHub Actions remain the real workflow
  semantic/execution check.

## v0.5.33 published source-only pre-release — live sector target

- A new portable `LiveSectorTarget.h` compares the current unfinished
  sector only with a previously measured positive split of the SAME gate.
  The UI shows time **until** the previous best or positive overtime once
  the current split takes longer. It never claims an achieved improvement
  before the gate is crossed.
- Expanded Unreal HUD source binds to this shared engine-neutral policy.
  Cross-platform real-`RaceProgress.cpp` C++20 tests and editor-source
  binding regressions cover start, improving sector, missing reference,
  exact best, slow sector, restart, finish and invalid clocks.
- Genuine UE 5.8 Editor/UHT compilation, rendered HUD checks and installed
  Win64/Android driving QA are **not** established by source CI.

## v0.5.32 published source-only pre-release — current-lap sector potential

- [PR #85](https://github.com/sanskarIN/latebrake/pull/85) added portable
  `CurrentLapPotential.h`, using only the actual `RaceProgress` current-lap
  duration and the best positive measured splits **from the same race**.
  Completed sectors are already included in elapsed lap time; the current
  sector can only add its unspent best remainder. No speculative
  negative-duration time is subtracted.
- The non-compact Unreal HUD shows **LAP POTENTIAL (BEST SPLITS)** and its
  signed difference to the *achieved* within-run best. It hides the line
  before a timed lap, after finishing/restarting, for missing or zero
  reference splits, and for invalid or out-of-bound clocks.
- Added a strict standalone `CurrentLapPotentialTests.cpp` target linked
  to real game `RaceProgress.cpp`. Ubuntu, Windows and macOS CI, release
  script policy, editor-script checks and documentation build passed on
  PR #85 before merge. See [sector timing](SECTOR_TIMING.md).
- This is **not** a playable release, Unreal compile, engine-rendered
  HUD verification or device test. v0.5.32 is now the latest **public**
  source-only tag; genuine engine/Win64/Android qualification remains open.

## v0.5.31 source development — local invalid-velocity QA visibility

- The v0.5.30 checkpoint trace rejects nonfinite velocity, but
  non-shipping mode-2 local QA summaries did not display the
  `InvalidVelocity` rejection bucket. Real QA inspectors could not
  distinguish a corrupt physics measurement from unclassified
  diagnostic samples using aggregated logs alone.
- The actual `ALatebrakeCheckpoint` 10-second summary appends
  `invalid-velocity=<count>` when diagnostics mode 2 is enabled.
  It reads the existing bounded `GateDiagnosticCounters` category;
  no position coordinates, per-frame buffers, network traffic or
  shipping logging are added.
- `AnalyzeCheckpointQA.py` now accepts both the prior summary format
  without this trailing field (assigning zero for missing history) and
  the new format with a validated nonnegative UINT32 counter. It rejects
  malformed or repeated fields and counts above the sample budget.
- Added standalone C++20 bucket regressions, Python legacy/new parser
  tests, and a source-level contract for Unreal's local log format.
  **These are synthetic/source tests, not evidence of installed Unreal
  runtime logging or real Android diagnostics.**
- The full checklist is in
  [bounded checkpoint QA summaries](GATE_QA_SUMMARIES.md).
  No playable game packages or stable release are yet qualified.

## v0.5.30 source development — checkpoint corrupt-velocity discontinuity

- Detected a race-integrity gap in `CheckpointMotionTrace::SampleDetailed`:
  a NaN/infinite velocity sample could be rejected while **its finite
  position was still retained** as the baseline for the next gate
  crossing. A later finite reading could falsely bridge that corrupt
  physics interval.
- Reject any nonfinite horizontal velocity **before** capturing a
  baseline; reset the trace and report `GateRejected/InvalidVelocity`
  without giving `RaceProgress` an ordered gate. The next entirely
  valid sample establishes a new baseline only; a subsequent real
  forward crossing can count. Position, session and teleport guards
  remain unchanged.
- New C++20 `CheckpointVelocityContinuityTests.cpp` links original
  `RaceProgress.cpp` and checks all four gates, NaN/±infinity in
  velocity, first-frame invalidity, unchanged session epochs, per-gate
  diagnostic counts and no ghost lap completion.
- Technical qualification steps are at
  [checkpoint velocity QA](CHECKPOINT_VELOCITY_QA.md); genuinely
  compiling/cooking UE 5.8 and testing installed Win64/Android
  packages remains **outstanding**. This is source-only development,
  not a new public release or binary.

## v0.5.29 source development — remove residual motor and steering input

- Identified a mismatch between touch ownership and physics output:
  `TouchReleased` and a nonfinite `ApplyTouch` canceled driving fingers
  but left already-smoothed throttle or steering values untouched.
  Dragging an owned GAS finger into the neutral gap similarly allowed
  an old motor command to decay over subsequent physics frames.
- Added shared portable `TouchResidualSafety.h`:
  `CancelTouchOwnerAndOutput` drops the released/invalid finger's
  ownership and its respective filtered command, while
  `MoveTouchWithResidualSafety` cuts residual gas on a move into
  neutral/BRAKE, steering on exit from its active vertical band, and
  both commands on an unusable viewport.
- Wired the actual Unreal Pawn's `TouchReleased` and `ApplyTouch`
  to these helpers. Independent held fingers and the physical
  `TouchPressTracker` action latch remain intact until genuine release.
- Added strict three-OS portable C++20 regressions and Python
  source-binding checks; none compile Unreal or validate Android
  touch/physics on real hardware.
- **Unreleased source development**; genuine UE 5.8 UHT/editor, cooked
  original map and device QA are still required under issues #13/#31.

## v0.5.28 source development — paused multi-touch release admission

- Android regression identified: `IE_Pressed` was bound with
  `bExecuteWhenPaused = true`, but `IE_Released` used the default.
  A finger lifted during a paused menu/session could remain marked
  as physically held by `TouchPressTracker`, suppressing subsequent
  legitimate presses with the same finger ID.
- Bind the existing `TouchReleased` callback to execute while paused.
  The callback releases only touch drive ownership and its matching
  physically-held tracker slot; it never fires discrete menu/race actions.
  No additional reset on menu entry is allowed, which would incorrectly
  re-arm a physically still-held finger.
- New strict portable C++20 lifecycle test exercises simultaneous gas,
  steering and PAUSE fingers across pause, release and new presses.
  A Python source-level binding contract verifies the actual Unreal
  callback wiring, and has negative source mutation fixtures.
- Remains **source-only**: needs UE 5.8 UHT/input binding compilation
  and Android device pause/menu/touch event delivery checks under #13.

## v0.5.27 source development — actual player chassis readiness

- Investigation: `HasLocalPlayerDriver()` previously returned true for
  any possessed `ALatebrakeCarPawn`, even if the underlying physics
  chassis was missing its mesh, not simulating, not blocking WorldStatic
  or had nonfinite location/velocity. The race clock could accumulate
  ghost time despite an undrivable vehicle.
- Added portable `RaceVehiclePhysicsSnapshot` and a predicate enforcing
  all required chassis conditions. The actual pawn now exposes
  `IsRacePhysicsReady()`, used by local player clock/gate readiness and
  by charged recoveries.
- Engine checkpoint actors clear their motion baselines and local QA
  samples when a car loses physical readiness, so restored physics
  does not interpolate an invalid old trajectory into a gate award.
- New strict C++20 tests use original `RaceProgress.cpp` for
  missing/invalid chassis, nonfinite 3D physics state, frozen clock
  while broken, resumed active time and finished-race behavior.
- Still **source-only**. UE 5.8 compilation, real rigid body collision,
  editor map cook and installed Windows/Android QA have not occurred.

## v0.5.26 source development — paid recovery session guard

- Review identified a gameplay mismatch: `RecoverPlayer` used to allow
  a paid teleport during an incomplete circuit startup or while the
  game/menu was paused, even though checkpoint scoring and the race
  clock were explicitly suspended in those states.
- `TryChargePlayerRecoveryIfReady` reuses the real player scoring
  readiness contract before the existing atomic five-second recovery
  fee. The engine-facing `RecoverPlayer` checks active driver identity,
  track readiness, both pause conditions and race completion before
  charging and teleporting.
- New strict portable tests link original `RaceProgress.cpp` and
  check every circuit/menu/driver readiness combination, denied-state
  immutability, legitimate charges, restart, finish and counter/
  floating-point boundary failure.
- Unreal 5.8 compilation, actual installed game behavior, mobile
  pause/recovery input and collision QA remain **unverified**.
  No new release tag or executable is implied by this work.

## v0.5.25 source development — original cooked map identity

- Earlier pre-release packaging recorded the circuit map's SHA256 after
  UAT, but could not prove the *same bytes* were present before cooking.
  The one-command candidate builder now snapshots Editor-authored map
  bytes and verifies them before/after UAT and at manifest creation.
- `Test-CandidateMapSnapshot.ps1` fails on missing, implausibly small,
  reparse-point, corrupt or changed map bytes. Disposable policy fixtures
  test same-size mutation, invalid SHA256 and restoration.
- The dual-platform manual release gate also compares the map's SHA256
  between Win64 and Android candidate manifests. A matching Git SHA
  and engine version alone are insufficient when the map was generated
  separately. Synthetic paired-manifest tests exercise this policy.
- These are source/build identity improvements only. No genuine UE
  compilation, packaged game, hardware driving QA or official
  v0.5.25 release has been verified.

## v0.5.24 source development — source-to-binary provenance gate

- A preexisting package manifest gate checked only tracked changes.
  Untracked new `.cpp`, `.h` or `.ini` source could affect a genuine
  Unreal build while the manifest still claimed an unchanged Git `HEAD`.
- Added `Test-SourceProvenance.ps1` to reject modified, staged and
  unexpected untracked source. Only the anticipated real Editor-authored
  `Content/Maps/LatebrakeCircuit.umap` path can be untracked, and its
  SHA256 is recorded separately by the candidate manifest writer.
- A follow-up source revision pin fixes another handoff ambiguity: a
  clean but *different* HEAD after a long Editor/UAT task cannot pass as
  the exact same candidate. Build-PlayableCandidate passes its initial
  40-character commit through all source checks; portable Git policy
  regressions cover matching, incorrect, malformed and blank pins.
- Invoked before the expensive Editor build, after level authoring, when
  invoking UAT directly, and before writing the final candidate manifest.
- Added temporary Git policy fixtures under Windows CI (not generated game
  content). Actual UE 5.8 Editor/UHT, cooked map, installed packages
  and hardware QA remain blocked on a licensed runner/device.

## v0.5.23 source development — close malformed duplicate press dispatch gap

- Investigation: the real pawn's `TouchPressed` previously returned as soon as
  `TouchPressTracker::Press` rejected an already-down finger. This bypassed
  v0.5.22's `TouchDriveState::Press` malformed duplicate handling: the
  portable state test passed, but the engine event path never reached it.
- New engine-independent `AdmitTouchPress` classifies events before returning:
  the same held finger's NaN/Inf repeated press cancels its own driving
  ownership while staying physically down (no duplicate PAUSE/RESTART/RECOVER).
  Legitimate duplicates and unrelated captured fingers remain unaffected.
- The Unreal pawn calls the shared policy and immediately zeroes its
  smoothed steering/throttle output if that specific held owner is canceled.
  First presses still reach viewport handling, including malformed first presses.
- Expanded portable `TouchPressTrackerTests.cpp` checks the exact integrated
  admission policy and multi-touch isolation. These tests are not Unreal
  event binding, packaged Android input, or installed-game QA.
- No v0.5.23 release tag, executable, APK or AAB is implied by this work.
  Real-device qualification remains open in [issue #13](https://github.com/sanskarIN/latebrake/issues/13).


## v0.5.15 source development — one checkpoint epoch for all gameplay transitions

- `RaceSessionEpoch` now tracks pause, accepted race restart, and
  menu-to-gameplay transitions independently of individual actor ticks.
- `CloseMenuForRace` consolidates menu dismissal and input clearing for
  successful Quick Race, career, championships, challenges and results replay.
- The Quick Race menu avoids restarting twice when event selection
  already restarts the authoritative race. A direct restart action
  also closes an open menu after success, not after failed career replay.
- Strict C++20 portable regression exercises real directional
  checkpoint crossings and RaceProgress before/after every transition.
- Pending actual UE 5.8/UHT/editor and hardware race qualification.

## v0.5.14 merged source — reject malformed UAT package outputs

- New Win64/Android candidate structure script checks payload sizes,
  Windows MZ headers, cooked asset presence, APK ZIP contents and
  AndroidManifest.xml/native or dex content.
- The real UAT packaging stage calls this preflight BEFORE the archive
  manifest writer and temporary unverified artifact upload.
- Negative fixtures check malformed files, not game output.
- Genuine UE 5.8 editor/UHT/cook and on-device racing qualification
  still require an authorized Windows Unreal workstation.
  [Build instructions](EXECUTABLE_BUILD_PIPELINE.md).

## v0.5.13 merged source — exact package/QA identity

- The platform release gate now requires a 64-character SHA256 of
  `Latebrake-candidate-manifest.json` in each installed-device QA
  report. Matching only the source commit no longer qualifies a
  different package produced by a second Unreal cook.
- `Build-PlayableCandidate.ps1` prints the exact digest after the
  archive manifest passes its closed-world hash/file-list verifier.
- Both Win64 and Android QA templates and negative policy tests
  cover the new identity requirement. Synthetic tests do **not**
  represent human playtesting or authentic Unreal binaries.
- Real UE 5.8 compilation, cooked Win64/Android candidates and
  installed hardware testing remain outstanding.
  [Build/QA instructions](EXECUTABLE_BUILD_PIPELINE.md).

## v0.5.12 merged source — reject linked candidate directories

- Verified an archive-integrity gap: checking only each final file for
  reparse-point attributes was insufficient when the file's *parent
  directory* is a symlink or Windows junction outside the archive.
- Manifest verification now rejects the manifest/root links and every
  linked parent component before hashing a listed payload; empty linked
  directories in the archive are rejected too.
- The manifest writer refuses source with dirty tracked files, links and
  empty archive payloads before recording SHA256 digests.
- Disposable local negative fixtures cover a junction outside the
  candidate root, with no synthetic game binary or gameplay claim.
- Pending: licensed UE 5.8 editor, map, UAT packages, real installed
  Win64/Android QA. See [build pipeline](EXECUTABLE_BUILD_PIPELINE.md).

## v0.5.11 merged source — never bridge checkpoint samples across pause

- New allocation-free `CheckpointSessionTrace` resets the motion baseline
  on menu and Escape/gamepad pause/resume session-revision changes,
  independently of teleport
  revisions and pawn swaps. No gate can earn a score from two samples
  separated by a menu pause.
- Unreal checkpoint actors reset their diagnostic windows on session
  changes and refuse sampling during blocked/paused/menu-open states.
- GameMode scoring now requires a valid local player driver as well as
  a ready unpaused circuit; race clock similarly treats an open menu
  as paused even if the world pause call fails.
- Four-gate real `RaceProgress` portable C++20 tests exercise
  interrupted and uninterrupted forward crossings, plus reset,
  corrupt coordinates and large/wrapped revisions.
- UE 5.8 UHT/editor compile and installed Win64/Android resume/gate QA
  remain outstanding. [Required acceptance](CHECKPOINT_SESSION_QA.md).

## v0.5.10 merged source — avoid ghost time without a player car

- GameMode only advances actual `RaceProgress` while a valid local
  `ALatebrakeCarPawn` is available through the player controller.
- An otherwise-complete course can now show a distinct HUD warning
  when the player pawn is missing. Time resumes on player return
  instead of inventing elapsed time while no driver could race.
- Engine-neutral `PlayerRaceClockReadiness` tests cover loss, return,
  pause, invalid time, finish and reset with original race source.
- Unreal UHT/editor compile, real possessed-pawn behavior and
  installed-game HUD/device verification remain pending.

## v0.5.9 merged source — clock cannot run on an incomplete circuit

- New portable `AdvanceRaceClockIfReady` uses real `RaceProgress`
  and refuses time accumulation when the circuit readiness audit
  failed, the world is paused, the race finished or the frame time
  is malformed.
- GameMode `Tick` now uses this policy rather than unconditionally
  advancing the elapsed race clock. The public
  `IsCircuitReadyForGameplay` source accessor allows the HUD and
  menu to display an explicit track-not-ready warning.
- A strict C++20 test links the actual original `RaceProgress.cpp`
  and checks positive advancement, rejected gates, pause/resume,
  invalid durations, finished race freeze and restart isolation.
- No real UE 5.8 editor/game compilation, actual circuit cooking
  or installed Win64/Android QA exists for v0.5.9.
  [Clock-startup QA design](RACE_CLOCK_READINESS_QA.md).

## v0.5.8 published source-only — verify collision channel blocking

- Original engine-neutral `CollisionResponsePolicy` now requires
  Pawn, PhysicsBody and WorldDynamic responses to be **Block**.
- After setting `BlockAll`, the engine-facing `SpawnBlock` checks
  each actual Unreal mesh component channel and destroys Actors that
  would still Ignore or Overlap important vehicle/dynamic channels.
- This integrates with the previous nine-piece startup audit: a
  misconfigured road/barrier cannot contribute to successful course
  construction or enable race checkpoint rewards.
- Portable C++20 `CollisionResponsePolicyTests.cpp` covers each
  rejected channel and positive paths. C++/CTest cannot verify
  actual real-game rigid body contacts.
- v0.5.8 is published as a source-only pre-release, without actual game executables.
  [Collision response QA](COLLISION_RESPONSE_QA.md).

## v0.5.7 published source-only — actual mesh-configuration and spawn preflight

- New `CollisionBlockSetup` policy disallows startup credit when an
  Unreal actor exists but its mesh component is absent, the expected
  Cube is not bound, blocking collision is disabled or scaling failed.
- The engine-facing `SpawnBlock` code destroys rejected actors and
  reports `LATEBRAKE_COLLISION_SETUP_FAILED`; the existing source
  startup audit still requires all nine full block configurations.
- New engine-independent `CircuitSpawnSafety` ensures the player and
  rival starts plus all recovery targets are finite, on planned paved
  road and outside conservative perimeter barrier clearances.
  `LATEBRAKE_SPAWN_LAYOUT_BLOCKED` prevents the course from entering
  ready state if future geometry edits invalidate a spawn.
- New strict portable tests and synthetic smoke-parser negatives
  guard these policies, but neither compiles Unreal Editor/UHT nor
  confirms that a real vehicle collides with the road on hardware.
- **v0.5.7 is published as source only; no verified playable build exists.**
  [Source and real-game QA design](COLLISION_SETUP_QA.md).

## v0.5.6 source-only preview — fail-closed circuit geometry startup

- The engine-independent `CircuitCollisionPlan` defines nine original
  ground/road/barrier transforms from shared world coordinates, preserving
  the earlier prototype's actual positions and scales.
- A new `CircuitStartupAudit` requires all nine collision block
  creations, four unique checkpoint indices and exact seven ISM groups /
  93 instances before marking the course ready.
- Unreal GameMode tracks attempted/created block Actors, logs
  `LATEBRAKE_GEOMETRY_READY collision=9/9` or
  `LATEBRAKE_GEOMETRY_INCOMPLETE`, and does not allow race checkpoint
  progression on incomplete startup.
- The runtime log parser requires complete, single-instance scenery,
  geometry and four-gate ready markers in source emission order.
  Disposable fixtures validate parser grammar only, **not real Unreal
  physics or actual game launch**.
- Portable C++20 tests check counts, duplicate gates, partial Actor
  creation, invalid plan indices, road alignment and missing scenery.
  This feature has not been compiled with UE 5.8 or tested on real
  Windows/Android machines.
- A source-only v0.5.6 GitHub pre-release exists; no playable EXE/APK exists. See the
  [startup audit design](CIRCUIT_STARTUP_AUDIT.md).

## v0.5.5 published source-only — player-facing finish scorecard

- `RecentTimedLaps` retains a fixed eight-entry true timed-lap history,
  resets on restart and excludes invalid/zero laps. Results menu and
  non-compact finished HUD can compare the latest two measured laps
  with a signed +slower/-faster delta.
- New engine-neutral `BuildFinishedRaceInsights` uses genuine
  `RaceProgress` and the already shared bounded finish-time policy.
- Shows an achieved best lap against this run's theoretical split sum,
  plus a fastest-to-slowest measured lap spread only with two or more
  positive timed laps. Instant zero-duration gates do not produce fake
  lap consistency data.
- Results-menu and non-compact finished-HUD source now render those
  values if a valid finished race exists; no reward, save, career or
  checkpoint behavior changes.
- Strict C++20 tests cover actual ordered lap progression and clock
  edge cases. **Real Unreal 5.8 compilation, HUD readability and
  installed Windows/Android playtest are not verified.**
- v0.5.5 is a source-only pre-release, not an installed game. [Design](FINISH_INSIGHTS.md) and [release notes](releases/v0.5.5.md).

## v0.5.4 published source-only — bounded checkpoint QA summaries

- New engine-independent fixed-size `GateDiagnosticCounters` tracks total samples, true crossings, baseline/recovery events and first classified gate rejection reasons.
- Shared `GateDiagnosticWindow` holds bounded ten-second frame time and
  counters; invalid/nonfinite durations cannot poison summaries and one
  long hitch cannot create an entire QA interval.
- New `scripts/AnalyzeCheckpointQA.py` validates and aggregates user-supplied
  runtime-log text offline, with explicit unverified-provenance warnings.
  Synthetic parser unit tests run alongside the Unreal editor-script stubs.
- Counts saturate rather than wrap and can be merged/reset without heap allocations, GPS coordinates, player names or network services.
- Non-shipping Unreal console mode `latebrake.CheckpointDiagnostics 2` adds a ten-second aggregated per-gate summary instead of continuous frame logging. Modes 0 (silent) and 1 (event transitions) remain.
- The standalone regression test proves real motion-trace decisions, merge arithmetic, saturation, reset, invalid categories and recovery isolation. **The Unreal logging code is not engine-compiled or hardware-qualified.**
- **v0.5.4 is a GitHub source-only pre-release; no EXE, Android APK or installed game qualification is available.** [QA summary design](GATE_QA_SUMMARIES.md).

## v0.5.3 published — deterministic gate diagnostic outcomes

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

- [v0.5.2](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.2) — checkpoint and race integrity fixes; source-only, no playable build.\n\n- [v0.5.3](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.3) — directional rejection reasons and opt-in local Unreal diagnostic log source; **source-only**, no executable.\n\n- [v0.5.4](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.4) — bounded QA counters and local log inspection. **Source-only; no playable build.**

- [v0.5.5](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.5) — genuine timed-lap and within-run theoretical finish insights, source-only. **No executable.**

- [v0.5.6](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.6) — original collision plan and audited startup marker grammar, **source-only**.

- [v0.5.7](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.7) — collision component setup and planned safe spawn/recovery checks, **source-only**.

- [v0.5.8](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.8) — component Pawn/PhysicsBody/WorldDynamic blocking policy, **source-only**.

These tags are source previews. They are **not downloadable playable game installers**.

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
