# Changelog

## Unreleased v0.5.20 — Strict recent timed-lap order

- The fixed-size recent completed lap buffer refuses duplicate or
  backdated lap identifiers while preserving valid increasing records.
- Correct ring-buffer oldest/newest ordering survives rejected writes
  even after wrapping. Race reset remains a fresh timing session.
- Actual portable RecentTimedLaps regression covers malformed IDs,
  successful wrap/eviction and clean reset. No Unreal runtime,
  engine build or hardware QA has been performed.


## Unreleased v0.5.19 — Counted rival recovery budget

- `RivalRouteProgress` refuses new teleport requests after its finite
  session allowance, rather than continuing untracked AI recoveries.
- Every accepted AI recovery request has an observable count. New
  racing attempts correctly reset the count and allowance.
- Existing RivalRouteTests stress the full 100,000-request boundary,
  the next rejected request, later stuck states and clean reset.
- Real Unreal Engine driving/collision and device QA are pending.


## Unreleased v0.5.18 — Autonomous rival readiness parity

- `RivalDriveReadiness.h` disallows driving when circuit startup,
  local driver, game focus, event selection or player finish is invalid.
- Unreal rival pawn now applies braking instead of acceleration when
  an unready circuit, open menu or paused/finished race is observed.
- Standalone C++20 regression links real `RaceProgress.cpp` and tests
  event, pause, circuit, local player, route finish and restart.
- These are source tests, not real Unreal 5.8 or hardware qualification.


## Unreleased v0.5.17 — Measured lap pace forecast in expanded HUD

- `LiveRacePaceProjection.h` derives an estimated total finish clock
  from up to three most recent valid completed timed laps.
- The active race HUD displays a clearly estimated lap reference and
  finish time, never an official measured result or career payout.
- Rejects estimates before one valid lap, after finish, across restart,
  and on invalid clocks or seven-day record boundary overflow.
- Strict C++20 tests link actual `RaceProgress.cpp` and cover rolling
  history, slow current laps, zero-time laps and restart.
- Real Unreal 5.8 runtime and visual/hardware QA are outstanding.


## Unreleased v0.5.16 — Consistent physical touch ownership range

- Portable `TouchFingerRange.h` gives physical touch tracking and the
  lower-level driving state the exact same 16-finger bounds.
- Rejects untrackable positive and negative inputs while preserving the
  global invalid-viewport cancellation of previously held controls.
- Cross-platform boundary regressions check ownership, movement, release
  and invalid viewport behavior. Android device QA remains outstanding.


## Unreleased v0.5.15 — Complete checkpoint session transition isolation

- Adds portable C++20 `RaceSessionEpoch` for explicitly revisioned
  pause/resume, accepted race restart, and menu-return-to-race transitions.
- Replaces the GameMode's unstructured revision counter. All successful
  menu-to-gameplay closures clear held car inputs, unpause and invalidate
  pre-menu gate positions through one `CloseMenuForRace` method.
- Quick Race avoids an unnecessary duplicate restart when changing away from
  a career or other race type.
- Every accepted restart invalidates checkpoint traces even if the
  teleport/recovery was not observed by a checkpoint tick. Direct
  restart actions now close an open menu too, preventing an unpaused
  but permanently menu-blocked race clock; failed career replay
  still leaves the results menu and race untouched.
- Regression tests use real ordered RaceProgress and a directional
  checkpoint to reject phantom scoring across pauses, menu returns and
  restarts while preserving normal forward crossings.
- Unreal 5.8 UHT/editor compilation and installed driving/device QA remain
  unverified; this is not a playable release.


## Unreleased v0.5.14 — UAT candidate package preflight

- `Build-PreRelease.ps1` now invokes a standalone
  `Test-UATPackageStructure.ps1` immediately after a successful
  authentic Unreal BuildCookRun operation. Expected extension or
  zero exit code alone cannot greenlight an unverified archive.
- Win64 candidate preflight checks MZ headers and minimum sizes of
  Latebrake executable(s) and requires substantive cooked .pak/.ucas
  payloads. Android preflight checks ZIP header/structure, one
  nonempty AndroidManifest.xml and compiled dex/native content.
- Rejects linked/reparse package nodes before inspecting payloads.
- Disposable negative PowerShell fixtures test missing, tiny, padded
  and structurally invalid outputs; no playable binaries are made.
- This is **structural source safety only**. Even a passing preflight
  cannot certify a genuine engine build or installed gameplay.


## Unreleased v0.5.13 — Bind installed-game QA to exact candidate bytes

- Platform QA now requires `candidate_manifest_sha256` matching the
  SHA256 of the **exact** candidate manifest supplied to the verifier.
  The manifest already fingerprints all package payloads, Unreal map,
  source commit and engine version.
- Source commit alone was insufficient: different Unreal cook/package
  runs from the same commit can produce different installable games.
  Prior installed-device QA cannot be reused for an unrelated candidate.
- Both Windows and Android QA templates include the required fingerprint.
  The build pipeline prints the digest after manifest validation, but
  does **not** automatically claim that installed gameplay was tested.
- Disposable PowerShell policy tests reject missing, malformed, wrong
  and stale fingerprints for a second manifest with the same commit.
  A matching digest still requires a genuine executable and playtest.
- **No real Unreal 5.8 UHT/editor build, cooked game or device
  qualification has been performed. No playable game is released.**


## Unreleased v0.5.12 — Candidate archive link/traversal integrity

- Hardened `Test-CandidateManifest.ps1` against symlinks/junctions
  in the manifest itself, archive root and **every parent directory**
  of a listed payload. Checking only the final file was insufficient:
  a path through a directory junction could resolve outside the archive.
- Rejects even empty unlisted linked directories during the complete
  manifest-vs-archive file-list check.
- `Write-CandidateManifest.ps1` refuses dirty tracked source,
  linked/junction package entries and empty payload files *before*
  fingerprinting rather than producing an unusable integrity record.
- New disposable offline policy fixtures exercise file paths through
  an external Windows junction and a linked folder absent from the
  manifest; neither contains a real Unreal build.
- Unreal Engine 5.8 UHT/editor compile, real cooked map, installed
  Win64/Android builds and device evidence remain **unverified**.
  No playable release or new version tag is being published.


## Unreleased v0.5.11 — Pause-safe checkpoint session traces (source development)

- Added portable `CheckpointSessionTrace.h` to discard pre-pause
  gate positions before the first post-resume positional sample.
  Per-pawn teleport revision checks remain independently enforced.
- Menu pause/resume and standalone Escape/gamepad pause transitions
  increment an explicit session revision shared with all four Unreal
  checkpoint actors. Diagnostics do not
  mix counts across these different motion sessions.
- A missing player car, unready circuit, paused world or open menu
  cannot advance checkpoint scoring; timing follows the same block.
- New strict C++20 four-gate regression uses real `RaceProgress.cpp`
  and validates the otherwise-plausible crossing across a pause,
  legitimate post-resume progression, corrupt samples, pawn reset,
  teleport changes and revision wrap.
- **Not released**: licensed UE 5.8 editor/UHT/packaging and physical
  Win64/Android gate, menu and HUD qualification are still missing.

## Unreleased v0.5.10 — Local player readiness guard (source development)

- New portable `PlayerRaceClockReadiness.h` refuses to advance the
  real `RaceProgress` timer without a valid local player pawn, even
  if the original circuit passed every source-side startup check.
- Unreal GameMode now uses one player-pawn validity probe for both
  its timing gate and HUD warnings; it shows `PLAYER CAR NOT READY`
  when the track exists but a local player has not spawned/returned.
- New strict C++20 `PlayerRaceClockReadinessTests.cpp` links actual
  race progression and checks driver absence, return, pause, startup
  rejection, malformed time, finish and restart.
- **Not released or Unreal-verified**; editor/UHT compile and installed
  Win64/Android player-spawn and HUD QA remain outstanding.

## Unreleased v0.5.9 — Fail-closed race clock on incomplete startup

- New portable `RaceClockReadiness.h` advances the actual
  `RaceProgress` clock only when the entire original circuit has
  passed its source startup audit, the game is unpaused, the race is
  still running, and frame duration is finite and positive.
- Updated `LatebrakeRaceGameMode.cpp` to call the shared policy
  rather than advancing race time unconditionally on every tick.
  Incomplete collision geometry, unchecked collision channels,
  unsafe planned spawns and missing checkpoints can no longer
  accumulate a deceptive result clock.
- Shared `CanScoreCheckpoint` now checks startup readiness, pause and
  finished state before any overlap can advance checkpoints or awards;
  the clock uses the identical eligibility rule.
- Added `IsCircuitReadyForGameplay()` source accessor and HUD/menu
  warnings for a blocked/incomplete course.
- New strict C++20 `RaceClockReadinessTests.cpp` links the actual
  game race-progress source and checks blocked startup, world pause,
  malformed time, running/resumed race, real finish, completed race
  freeze and restart safety. Registered in standalone CMake.
- **Not released**; no real Unreal Editor/UHT/game C++ compile,
  cooked map, installed Win64/Android build or gameplay observation.


## v0.5.8 — Collision-channel response integrity (SOURCE-ONLY pre-release, 2026-10-22)

- New `CollisionResponsePolicy.h` classifies three critical blocking
  collision responses for pawns, simulated physics bodies and dynamic
  world objects. Merely setting `QueryAndPhysics` is not sufficient
  if the profile allows Ignore or Overlap on those channels.
- Unreal GameMode now checks the actual `UStaticMeshComponent`
  channel responses and destroys/rejects improperly configured
  collision-bearing track Actors, preserving v0.5.6/v0.5.7 strict
  startup gating.
- Added `CollisionResponsePolicyTests.cpp` with positive, each
  failure, multi-failure and stable local diagnostic name cases.
- Included explicit Unreal headers for the engine channel enums and
  mesh-component calls rather than relying on transitive includes.
- Published source-only, not a playable game. No packaged game binaries,
  Unreal Engine 5.8 UHT/editor compile or installed-game collision
  evidence exists for this milestone.


## v0.5.7 — Collision actor configuration and start-position preflight (SOURCE-ONLY pre-release, 2026-10-22)

- New `CollisionBlockSetup.h` classifies missing Actor, absent mesh
  component, wrong mesh, nonblocking collision, invalid source plan
  or unapplied transform, without depending on Unreal headers.
- `LatebrakeRaceGameMode.cpp` now checks actual created block mesh
  configuration after applying the built-in Cube and
  `QueryAndPhysics`; rejected Actors are destroyed and counted as
  startup failures rather than silently contributing to a ready course.
- New `CircuitSpawnSafety.h` validates player/rival starts and
  recovery positions against the shared original circuit, paved
  surface and conservative XY clearance from planned perimeter barriers.
  An unsafe planned spawn blocks race-ready state.
- New portable C++20 `CollisionBlockSetupTests.cpp` and
  `CircuitSpawnSafetyTests.cpp` test all expected failures, each
  planned collision block, recovery route, invalid coordinates and
  unsafe barrier verges.
- The runtime PowerShell log verifier rejects
  `LATEBRAKE_COLLISION_SETUP_FAILED` or
  `LATEBRAKE_SPAWN_LAYOUT_BLOCKED` even when other counters claim
  success. Synthetic negative regression fixtures cover both failures.
- **Published as source-only GitHub pre-release**; licensed UE 5.8 UHT/editor/game C++
  compilation and installed Win64/Android physics/controls remain
  unverified, and no playable v0.5.7 game executable exists.


## v0.5.6 — Circuit startup geometry integrity (SOURCE-ONLY pre-release, 2026-10-22)

- Created **`CircuitCollisionPlan.h`**, an original engine-independent
  source of all nine collision-bearing ground, road and barrier transforms
  and material styles. Unreal GameMode uses this plan rather than an
  unrelated set of nine hard-coded actor placements.
- Created **`CircuitStartupAudit.h`**, a bounded, allocation-free
  startup policy requiring exactly 9/9 collision-bearing block spawns,
  four distinct checkpoint indices, and all 7/93 scenery
  groups/instances. Missing or duplicate actors fail closed.
- Refactored `LatebrakeRaceGameMode.cpp/.h` to emit an explicit
  `LATEBRAKE_GEOMETRY_READY collision=9/9` marker only on source-level
  construction success and refuse progress/awards on incomplete circuit.
  Rendering/collision physics are still unverified.
- Updated `Verify-RuntimeStartupLog.ps1` to demand a single, ordered
  scenery -> collision-geometry -> startup marker sequence and reject
  missing, repeated, out-of-order or incomplete construction.
- Added `CircuitStartupAuditTests.cpp` and
  `CircuitCollisionPlanTests.cpp` as strict portable C++20 tests.
  Added offline positive grammar and negative synthetic log fixtures
  to PowerShell packaging-policy regression; fixture acceptance never
  constitutes real Unreal playability evidence.
- **Published source-only; no playable game.** No Unreal Editor/UHT/game compile, author-cooked map,
  hardware driving QA or downloadable game package is claimed.


## v0.5.5 — Race finish insights (SOURCE-ONLY pre-release, 2026-10-22)

- Retain the last eight genuine positive timed laps in an allocation-free
  ring with physical completed-lap indices, bounded access and clear
  restart semantics. The finish screen can show last-versus-prior lap
  improvement (negative delta = faster) without inventing previous
  laps in a time attack.
- Added strict recent-history ring tests for invalid values, capacity,
  wraparound, empty lookup and post-reset isolation, plus integration
  assertions using real ordered RaceProgress events.
- Count strictly positive finite timed laps independently of ordered
  gate/lap completion; an instant t=0 lap is never a pace observation.
- Add portable `FinishedRaceInsights` computing an achieved best lap,
  last timed lap, within-run theoretical split sum, positive gap to the
  theoretical lap, and fastest-to-slowest lap spread only with at least
  two genuinely timed laps.
- Reject invalid and unrecordable clocks using the same existing
  finished-race record policy. No SaveGame schema, career medals, race
  rewards or gate acceptance rules are changed.
- Add result-menu and non-compact finished-HUD source display, plus
  strict actual-RaceProgress C++20 tests for three laps, one lap,
  zero-duration laps, restarts and seven-day record limits.
- This is a **source-only GitHub pre-release**, not a playable game. No Unreal Editor/UHT compile,
  real gameplay QA or Windows/Android executable is claimed.


## v0.5.4 — Bounded checkpoint QA counters (SOURCE-ONLY pre-release, 2026-10-22)

- Extracted the diagnostic summary timer into engine-neutral
  `GateDiagnosticWindow`, with deterministic invalid-duration,
  frame-hitch-capping and reset regression coverage.
- Added an offline diagnostic log summarizer and negative Python parser
  tests, deliberately marking all parsed log text as unauthenticated and
  gameplay-unverified.
- Added `GateDiagnosticCounters` with fixed-size, allocation-free per-reason
  counts, saturating arithmetic, merge and reset for bounded QA windows.
- Captures crossing, initial baseline, teleport rebase, invalid samples
  and geometry rejection reasons without actor coordinates or player identity.
- Added non-shipping `latebrake.CheckpointDiagnostics 2` source integration:
  one per-gate summary about every ten seconds; default remains silent,
  mode 1 continues logging interesting events.
- A strict portable C++20 regression uses actual motion decisions and
  proves safe saturation and isolation after reset. Unreal Editor/UHT
  compilation and installed 30/60 FPS device measurements are outstanding.
- **Published as a GitHub source-only pre-release; no playable game package or device QA exists.**


## v0.5.3 — Explainable checkpoint diagnostics (SOURCE-ONLY pre-release, 2026-10-22)

- Added allocation-free, Unreal-independent gate rejection classifications:
  invalid index, malformed samples/velocity, below-threshold/wrong-way
  movement, over-limit sweeps, no directional crossing, lateral misses
  and excessive crossing height.
- Exposed initial baseline/teleport rebase/invalid position and regular
  gate scoring outcomes in the checkpoint trace. Existing boolean game
  behavior and v0.5.2 scoring limits are intentionally unchanged.
- Added strict portable C++20 regression tests for four directional gates,
  diagnostic reasons and short-teleport behavior. Unreal editor/device
  compilation and real driving instrumentation remain unverified.
- Released [v0.5.3](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.3) as an immutable source-only pre-release after guarded three-OS CTest and packaging-policy checks; no playable EXE/APK was built. The opt-in non-shipping Unreal logger remains engine-unverified.

## v0.5.2 — Race Integrity & Checkpoint Safety (SOURCE-ONLY pre-release, 2026-10-22)

- Published [source-only release v0.5.2](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.2) after separately gated three-OS portable C++20,
  negative packaging-policy and Unreal automation stub tests.
- Qualified *portable source* for bounded swept forward plane crossings
  on four ordered gates, motion-generation teleport rebasing and safe
  low-frame-rate sampling within a 1,200 cm distance cap.
- Hardened race/lap finish records, career medals and rewards, required-
  vehicle replay rollback, active-race garage mutation blocking,
  recovery penalty precision, rival event lap targets and touch input
  cancellation after viewport/controller changes.
- Unreal 5.8 compilation, original map cook, installed Win64/Android
  packages, in-game QA and real FPS/performance are outstanding.
  **No playable EXE/APK/AAB is included.** See
  [complete release notes](docs/releases/v0.5.2.md).

## v0.5.1 — Reliability & Circuit Optimization (SOURCE-ONLY pre-release, 2026-10-22)

### Held-control cancellation on viewport resize (Android source only)

- Introduced a portable touch-viewport baseline guard to detect actual
  screen dimension changes and temporary unusable viewport sizes.
- Android pawn Tick and the touch dispatcher cancel both held steering
  and pedal controls on resize, without waiting for a touch-move event.
- Preserve physical-finger ownership across a resize so repeated Pressed
  notifications cannot reactivate acceleration or fire a second action.
- Added strict standalone C++20 regressions for rotation, smaller resize,
  viewport disappearance/recovery, retained touch-down state and re-press.
- Genuine Android devices, Unreal Engine 5.8 compilation and in-game
  focus/resize/touch verification are still outstanding.


### Batched scenery startup smoke policy

- Strengthened the **real installed-game log verifier** so a four-gate
  startup alone is not sufficient: it must also see one complete,
  correctly ordered scenery marker with seven ISM groups and 93 instances.
- Reject incomplete, wrong-count, missing, duplicated and out-of-order
  scenery assembly markers in CI's disposable negative parser fixtures.
- Passing a future genuine log check is not proof that Unreal renders,
  drives, saves or packages correctly. No new playable binary is present.

### Runtime scenery actor reduction (engine source only)

- Replaced the 93 cosmetic cube actors on the single starter circuit
  with one holder actor and seven `UInstancedStaticMeshComponent` groups.
- Reused the exact original centerline stripes, alternating curb palette,
  two grandstands and start/finish banner via a fixed portable batch plan.
- Kept nine physical road/ground/barrier actors separate and left all
  cosmetic meshes non-colliding, overlap-free, shadow-free and outside
  navigation influence.
- Added strict C++20 geometry/group-count tests and distinct startup
  diagnostics for complete vs incomplete scenery assembly.
- No engine compile, cook, real draw-call/FPS measurement, device test,
  playable binary or qualified Unreal runtime is claimed.


### Career launch and recovery accounting integrity

- Career event and required-owned-vehicle selections are now staged inside
  `ProfileWriteCheckpoint` and must be written to SaveGame **before** the
  running event, race clock, pawn tuning or rival is changed.
- If `SaveGameToSlot` reports failure, launch is rejected with actionable
  feedback, career and garage selection are rolled back, and the previous
  running race remains active instead of adopting an unsaved selection.
- Added portable `CareerLaunchSelection` model policy to prevent locked and
  vehicle-ineligible events from mutating either profile.
- Player recovery is now authorized only after successfully charging the
  complete five-second time fee **and** advancing its bounded count; if
  either fails, the car stays put rather than getting a free teleport.
- Added engine-independent C++20 `CareerLaunchSelection` and
  `RecoveryCharge` regression targets. Real Unreal 5.8 SaveGame and
  physics/Android playtesting are still required; no new playable build.


### Run-local sector timing (not Unreal qualified)

- Record the best valid split for each ordered checkpoint in the current
  event, without changing SaveGame schema or previous best-lap records.
- Display the signed delta to that same sector's **prior** best; first
  recorded splits have no invented comparison.
- Show clearly marked **THEORETICAL LAP (THIS RUN)** using the sum of sector
  bests, distinct from the fastest actually completed lap.
- Bound checkpoint storage to 256 without dynamic allocations; reject
  zero/nonfinite/overflowed records and reset on restart.
- Add standalone C++20 regression cases for splits, deltas, penalties,
  finish/reset, invalid gates and extreme clock values. Real Unreal HUD,
  frame timing and device tests remain outstanding.


### Single-fire touch action protection (post-v0.5.0)

- Added portable touch press ownership for all fingers, including touches
  starting outside driving pads and on action/menu buttons.
- Discrete touch actions now trigger at most once per physical press;
  duplicated Unreal touch-pressed events cannot repeat RECOVER penalties or
  RESTART while the same finger is held.
- Releases are tracked independently across fingers. Driving-input clears
  do not erase press ownership; true OS lifecycle resets do.
- Added C++20 regression coverage for duplicate-press protection and focus
  interruption. Android/Unreal runtime behavior is still unverified.


- Configured future cooked game/default map path for a real project-owned
  Unreal Editor generated starter level, without pretending that asset
  is already committed.
- Removed unused ChaosVehicles dependency; added editor-target-only
  Python map-authoring plugin registration.
- Added genuine UE 5.8 UBT Editor compilation, map-authoring and
  Win64/Android UAT BuildCookRun PowerShell scripts.
- Added independent per-platform package/evidence validation, deliberate
  negative-fixture packaging regression tests, and optional manual
  self-hosted Unreal candidate-build workflow.
- Corrected packaging-script PowerShell errors found by CI.
- No Unreal binaries, real Unreal map, Android testing, stable release,
  or playable v0.5.1 game build is claimed.


### Additional build and input hardening

- Corrected the Unreal 5.8 editor level-existence check, ensuring an
  existing binary level is loaded rather than recreated. Seven offline
  API-control-flow tests cover failures and idempotency.
- Added Unreal `.umap` package-tag preflight and nonempty output-directory
  rejection so stale/fake files cannot stand in for a new cooked game.
- Added UNVERIFIED package checksum/provenance manifest for any genuine
  Win64/Android build generated later from a licensed UE workstation.
- Fixed menu Pause/Escape routing to avoid resuming a race behind its menu.
- No actual Unreal Editor/game compilation or shipped executable is claimed.

### Further game runtime and build preflight

- Registered an Android-aware player controller for custom multi-touch
  controls without conflicting default virtual joysticks.
- Added a fresh Unreal Editor map-save result receipt, validated after
  every editor run to catch silent Python scripting failures.
- Added GameMode startup diagnostic markers checking all four gate actors
  and a narrowly scoped real-log verifier with negative regression tests.
- Portable source tests do not substitute for Unreal compilation or
  installed-device verification. No executable files are published.

### Save and progression integrity work

- Added engine-independent `ProfileWriteCheckpoint` with RAII rollback,
  protecting career medals, first-completion credits, championship bonus,
  quick-race records and completed race counts if saving fails.
- Integrated failed-write rollback into racing results and garage selection,
  purchases and engine/grip upgrades. Unsaved speed-unit setting changes
  now revert as well.
- Added strict multi-platform portable regression tests for failed writes,
  saved writes, duplicate rewards, garage rollback and championship replay.
- Does not guarantee power-loss durability; actual UE 5.8 SaveGame writes
  and device failure/restore tests remain necessary. No new binaries or
  playable build published.

### v0.5.1 touch input safety

- Introduced `TouchDriveState`, the portable two-finger driving capture
  state machine, and replaced the pawn's duplicated touch input fields.
- Prevented held steering/pedal drags from being interpreted as new
  restart/pause/recover presses or stealing another finger's controls.
- Made neutral pedal gaps, invalid coordinates, viewport interruptions
  and paused input fail closed; steering horizontal clamping remains.
- Added strict Windows/macOS/Linux C++20 tests for touch ownership,
  drag transitions and lifecycle resets. Real Android compilation,
  installation and touch QA are still outstanding.

### Candidate/archive provenance gates

- Added full candidate manifest validation for every packaged file's
  SHA256, size and safe relative path; rejects mismatches, duplicate,
  missing and unlisted files, bad platform metadata and dirty source.
- Executed integrity verification after genuine Unreal UAT packaging and
  before temporary candidate workflow-artifact uploads.
- Required per-platform package qualification to include the manifest,
  installed QA evidence, and matching `build_commit`.
- Replaced weak Win64+Android combined checker with two full
  per-platform checks and same-source/same-Unreal-version requirements.
- Extended PowerShell negative regression tests with disposable text-only
  archive fixtures. No Win64 game/Android APK has been built or attached to this source-only pre-release.

## v0.5.0 — Career & Championship Foundations (published source-only pre-release, 2026-10-20)

### Implemented in Unreal-independent C++20
- Immutable twelve-ID event catalog: two three-round championship series and six standalone challenges, all using the existing single course.
- Prerequisite unlocks, available/completed/mastered event states, bronze/silver/gold performance medals, points and series-completion bonuses.
- Deterministic time, clean recovery, ordered checkpoint precision, lap consistency, Kestrel-specific and six-lap endurance objectives.
- Once-only per-event first-medal credit rewards and once-only championship bonuses, with replay medal improvement and save/load resistance to duplicate awards.
- Defensive career snapshot migration and numeric validity; completed-lap slowest-time tracking.
- Portable ten-page menu state machine, expanded touch layout, career and UI regression tests.

### Unreal-facing **source code** integration (not Editor-verified)
- Configurable 1–6 lap career races, active-event selection, authoritative player-only result evaluation, garage economy integration and v2 SaveGame serialization.
- Asset-free paused menu with career/championship/challenge/garage/result/settings/profile pages, gamepad/keyboard/touch navigation and persisted KM/H or MPH choice.
- OS lifecycle input-clearing delegates on background/deactivate/foreground to reduce stuck mobile control risk.
- Documentation, test plans and source-only release workflow.

### Not verified / not playable
- Unreal Engine 5.8 UHT/game/editor compilation, real cooked project-owned map, installed Win64/Android binary, phone input/menu behavior, save/physics gameplay or performance.
- Full art, real competitive AI standings, production UMG UI, audio, multiple maps and signing/packaging are not claimed.


## v0.4.1 — Android touch UX and input reliability (published source-only pre-release)

### Added and changed
- Shared responsive geometry for six Android pads: steering, throttle, brake, pause, restart and recovery.
- Explicit touch-down capture rather than implicit capture while moving; ignored touches outside the pads cannot trigger driving.
- Two-finger steering and pedals; neutral gas/brake gap and invalid-coordinate fail-safe.
- Android HUD pad outlines/highlights; compact telemetry with enough room for driving pads.
- Touch PAUSE binding permitted during pause; restarting from pause unpauses.
- Portable C++20 regression checks for pad hit-testing, screen sizes, control separation, steering clamping and corrupt coordinates.

### Still to verify
- Unreal Engine 5.8 compilation and in-game input routing, pause behavior, touch focus-loss handling, HUD/device rendering, real Android builds and packaging. This is **not a playable game release**.


## v0.4.0 — starter course and environment expansion (source-only candidate)

### Added and changed
- Shared engine-independent course geometry for four gates, spawn, player/rival recovery and rival AI navigation targets.
- Primitive road stripes, alternating red-white curbs, trackside grandstand tiers and start/finish banner on the existing single starter circuit.
- Decorative actors have collision disabled; core drivable roads/barriers and race rules are unchanged.
- Compact desktop course map that shows the circuit path, next gate and live player position; hidden on smaller viewports.
- C++20 standalone regression targets validate road/gate/recovery consistency, ornament placement bounds and minimap coordinate normalization.

### Still to verify
- Unreal Engine 5.8 actor compilation and visual playtest; geometry material appearance, shadowing, physics, HUD overlap and draw-call performance on actual Windows/Android builds.
- No additional selectable tracks, real instanced-mesh optimization, shipped playable packages or original production environment art.

## v0.3.2 — published source-only pre-release

### Added and improved
- Portable C++20 driving-axis normalization, normalized touch steering, and bounded throttle smoothing, with numerical-edge-case tests.
- Sanitize keyboard, AI and mobile touch input; discard held controls on pause, resume and vehicle recovery.
- Reject invalid frame time or nonfinite vehicle velocities before physics integration.
- Cache garage tuning application based on selected car, engine/grip level and player pawn; do not reapply unchanged handling every tick.
- Expose local SaveGame write failures to the racing HUD rather than silently reporting progress as persisted.
- Portable tests validate tuning cache invalidation versus earned credits and vehicle unlocks.

### Not yet verified
- No UE 5.8 editor compilation, physics or touch device testing, Windows/Android packages, or in-game SaveGame write-failure playtest.

## v0.3.1 — published source-only pre-release

### Improved
- Reject negative/malformed garage ownership bitmasks without granting free cars.
- Centralize next-locked and next-owned vehicle lookups in the portable garage model.
- In-game feedback for unlock successes, shortfalls, maximum upgrade levels and car selection.
- Upgrade price previews exposed to the HUD; reduce repeated garage selection logic.
- Viewport-adaptive compact racing HUD with control footer and garage information.
- Record sanitation for implausible, negative or nonfinite saved best times and saturating completed race counter.
- Added pure C++20 regression targets for viewport layout and saved record validation, plus expanded garage tests.

### Still missing
- Unreal Engine-specific compilation/UHT test, real HUD/device layout check, real SaveGame migration validation, playable Win64/Android artifacts.

## v0.3.0 — published source-only pre-release

### Added
- Three fictional vehicle profiles and asset-free silhouette variants.
- Offline credit-based vehicle unlocking and engine/grip upgrades, with capped deterministic pricing and performance.
- Backward-compatible SaveGame garage ownership, selected profile and upgrade levels.
- Keyboard Q/E/U/I garage shortcuts and HUD profile, unlock and tuning readouts.
- Portable garage C++20 regression tests for purchases, profile sanitization and credit overflow.

### Still outstanding
- Unreal Engine editor/UHT compilation, original 3D vehicle models, real game packages, physical hardware and save serialization tests.

## v0.2.1 — published source-only pre-release

### Improved
- Pure C++20 rival route state tracks approximate waypoint laps and recovery requests after eight seconds at low speed.
- Experimental Unreal rival now uses route progress, recovers in place without resetting tracked lap progress, and applies pilot commands before the base car physics step.
- Basic corner-anticipation braking in the portable rival planner, with invalid negative speed handling.
- Shared deterministic steering slew limiter for both player and AI controls, including engine-independent tests and overflow guard.
- HUD shows approximate rival laps and number of AI recoveries, without claiming competitive race standings.
- Regression tests added for invalid route/sensor values, recovery/reset, corner braking and steering smoothing.

### Not yet verified
- Unreal Engine 5.8 reflection/build, actual driving/rival behavior, packaged Windows/Android binaries and physical device tests.

## v0.2.0 — published source-only pre-release

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
