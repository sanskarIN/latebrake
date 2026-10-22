# Changelog

## Unreleased — v0.5.1 executable readiness

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
  playable binary or v0.5.1 release is claimed.


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
  or new v0.5.1 GitHub release is claimed.


### Additional unreleased build and input hardening

- Corrected the Unreal 5.8 editor level-existence check, ensuring an
  existing binary level is loaded rather than recreated. Seven offline
  API-control-flow tests cover failures and idempotency.
- Added Unreal `.umap` package-tag preflight and nonempty output-directory
  rejection so stale/fake files cannot stand in for a new cooked game.
- Added UNVERIFIED package checksum/provenance manifest for any genuine
  Win64/Android build generated later from a licensed UE workstation.
- Fixed menu Pause/Escape routing to avoid resuming a race behind its menu.
- No actual Unreal Editor/game compilation or shipped executable is claimed.

### Further unreleased game runtime and build preflight

- Registered an Android-aware player controller for custom multi-touch
  controls without conflicting default virtual joysticks.
- Added a fresh Unreal Editor map-save result receipt, validated after
  every editor run to catch silent Python scripting failures.
- Added GameMode startup diagnostic markers checking all four gate actors
  and a narrowly scoped real-log verifier with negative regression tests.
- Portable source tests do not substitute for Unreal compilation or
  installed-device verification. No executable files are published.

### Unreleased save and progression integrity work

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
  GitHub release published.

### Unreleased v0.5.1 touch input safety

- Introduced `TouchDriveState`, the portable two-finger driving capture
  state machine, and replaced the pawn's duplicated touch input fields.
- Prevented held steering/pedal drags from being interpreted as new
  restart/pause/recover presses or stealing another finger's controls.
- Made neutral pedal gaps, invalid coordinates, viewport interruptions
  and paused input fail closed; steering horizontal clamping remains.
- Added strict Windows/macOS/Linux C++20 tests for touch ownership,
  drag transitions and lifecycle resets. Real Android compilation,
  installation and touch QA are still outstanding.

### Unreleased v0.5.1 candidate/archive provenance gates

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
  archive fixtures. No Win64 game/Android APK or v0.5.1 release exists yet.

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
