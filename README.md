# Latebrake

**Race Beyond Limits.** An original, community-developed racing game built with Unreal Engine 5.8 and C++20.

> **Latest published: [v0.5.42](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.42) — SOURCE-ONLY PRE-RELEASE, NOT A PLAYABLE GAME.** This milestone adds player driving-readiness gating across manual/menu pause failures, interrupted focus, unavailable track and completed race, backed by 64-state C++20 checks. **There is no verified Windows EXE, Android APK/AAB or Linux game.** GitHub source archives are not installers. Genuine UE 5.8 Editor/UHT compilation, an authentic cooked circuit map, installed packages and physical driving QA remain outstanding.

**Documentation website:** [Latebrake project guide](https://sanskarin.github.io/latebrake/) — project overview, gameplay systems, build setup, verified-vs-unverified status, roadmap and contribution documentation. The website is documentation only, not a browser-playable game.

## What's in source today?

- Unreal Engine 5.8 game/editor targets and a C++20 runtime module.
- A physics-simulated, keyboard/gamepad controlled starter car assembled from engine-built-in shapes.
- Arcade and simulation-inspired force/grip presets, braking, camera switching, recovery, pause and experimental two-finger touch regions.
- A generated four-checkpoint circuit with consistent player/rival/AI course geometry, lane stripes, curbs, simple grandstands, course overview map (desktop), three-lap ordered race rules, HUD, race restart and finish state.
- Locally saved race best times (separate circuit and time attack), fastest lap, earned offline credits and completion counter.
- Two racing event definitions, experimental one-car circuit AI and reward rules.
- v0.2.1 improvements: AI approximate route-lap tracking, recovery after sustained stalls, anticipatory waypoint braking, live AI diagnostic telemetry and shared input steering smoothing.
- v0.3.0: three fictional garage tuning profiles, offline unlocks and upgrades with SaveGame persistence, shortcuts, and prototype cabin silhouettes.
- v0.3.1: safer corrupt-save handling, centralized purchase lookup, upgrade-price previews, visible transaction feedback, compact HUD layout and bounded racing record counters.
- v0.3.2: invalid input and physics-value guards, smoother throttle, pause/recovery input clearing, cached garage tuning updates, and a HUD warning for failed saves.
- v0.4.1: source-only Android touch HUD with visible steering, gas, brake, pause, restart and recovery pads.
- v0.5.0: offline career/championship and challenge rules, versioned career progression saves, prototype menu/navigation, and extended portable tests.
- Portable, engine-independent lap/checkpoint rules with C++20 CMake tests on Linux, Windows and macOS CI.
- v0.5.32 (published source-only pre-release): expanded HUD shows live current-lap **best-sector potential** against the achieved best lap, backed by strict C++20 tests; this estimate cannot award records.
- v0.5.42 (published source-only pre-release): block player thrust/steering and clear chassis momentum whenever the authoritative race is paused, in a menu, finished or unsafe to drive; run 64-state portable readiness tests. Real Unreal engine/device behavior is still unverified.
- v0.5.41 (published source-only pre-release): strengthen authentic Unreal Editor map receipt with UE 5.8 version preflight and SHA256 byte identity, backed by synthetic same-size tampering regressions. Source QA is not actual editor/game qualification.
- v0.5.40 (published source-only pre-release): award integrity now requires a valid active-clock advance on every completed lap. Zero-duration or penalty-only laps are not eligible for credits, medals, records or a final results card. Real engine/device qualification remains outstanding.
- v0.5.39 (published source-only pre-release): on OS focus loss, neutralize vehicle linear/angular momentum and held input if Unreal cannot pause; while suspended, only a new PAUSE touch may explicitly resume racing. This is source-verified, not installed-device qualified.
- v0.5.38 (published source-only pre-release): any new app focus-loss suspension advances the checkpoint epoch even if the OS already paused Unreal, preventing a pre-loss position sample from counting as a post-resume crossing. Portable and source binding tests do not replace real device QA.
- v0.5.35 (published source-only pre-release): app focus-loss suspension blocks ghost race time, checkpoint credit, paid recovery and rival route advancement until explicit player resume even if engine pausing fails. Real Unreal/device QA remains required.
- v0.5.33 (published source-only pre-release): expanded HUD adds the live time remaining until a measured best sector or the already elapsed over-best time, with strict portable and source-binding tests.
- v0.5.1 source preview: matched-sector split deltas and theoretical lap, transactional career-start selection, charged recovery safeguards, single-fire and viewport-safe Android touch, 93 decorations grouped into seven instanced-mesh batches, and stricter future startup-log checks. **All remain unqualified in Unreal**; see [v0.5.1 source release notes](docs/releases/v0.5.1.md).

The starter vehicle uses **Chaos rigid-body physics**. It does **not yet** implement Chaos Vehicles' tire/suspension drivetrain; that upgrade requires an authored skeletal vehicle rig and physics assets. The procedural circuit is intentionally temporary. There are no shipping-quality vehicles, audio, traffic, multiplayer, production-quality career menus or verified Android touch interfaces yet.

## Quick start (source)

1. Install Unreal Engine **5.8** (licensed and installed separately) and the applicable C++ build toolchain.
2. Clone this repository with Git LFS installed: `git lfs install && git clone https://github.com/sanskarIN/latebrake.git`.
3. Check out the development branch if the feature has not yet been merged.
4. Right-click `Latebrake.uproject` and generate project files (or open from the Unreal project browser).
5. Build the `LatebrakeEditor` target with your local toolchain; open `Latebrake.uproject`, then use Play in Editor.

The game mode is selected via `Config/DefaultEngine.ini`, which now targets `/Game/Maps/LatebrakeCircuit`; the project-owned binary `.umap` still must be generated and saved with an authorized Unreal Editor 5.8 installation. The starter circuit actors are spawned at runtime. Check **[Build instructions](docs/BUILDING.md)** for the actual map, tooling and packaging prerequisites.

### Current controls

| Control | Action |
| --- | --- |
| W / S | Acceleration / reverse |
| A / D | Steering |
| Space | Brake |
| R | Vehicle recovery |
| T | Restart race |
| C | Camera distance |
| 1 / 2 | Arcade / simulation-inspired handling |
| Esc | Pause or resume |
| 3 / 4 | Circuit Race / Time Attack (restarts current race) |
| Q / E | Unlock next vehicle / select next owned vehicle |
| U / I | Upgrade engine / grip using in-game earned credits |
| Gamepad | Triggers, left stick, face buttons and shoulder button (see input config) |

Experimental Android controls (v0.4.1 source candidate): drag on the lower-left **STEER** pad; hold the lower-right **GAS**/**BRAKE** pads; tap upper-right **PAUSE**, **RESTART**, or **RECOVER**. See the [touch-control guide and required Android QA](docs/TOUCH_CONTROLS.md). **No Android game build, physical device behavior or accessibility compliance has been verified.**

## Test the portable race, rival, rewards, garage and course logic

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

These tests do not compile the Unreal gameplay classes or certify packaged game builds.

## Source layout

- `Source/Latebrake/Public/Racing` and `Private/Racing`: deterministic rules/checkpoints.
- `Source/Latebrake/.../Vehicles`: starter car.
- `Source/Latebrake/.../Game`: procedural course and race orchestration.
- `Source/Latebrake/.../UI`: racing HUD.
- `Source/Latebrake/.../Progression`: save data.
- `Config`: maps, rendering, and input.
- `Tests/Standalone`: portable C++20 checks, including responsive touch hit-testing (not Unreal touch routing).
- `docs`: design, architecture, build instructions, status, and roadmap.

## v0.5.2 published source changes

[v0.5.2](docs/releases/v0.5.2.md) is a **source-only** pre-release
covering directional 3D checkpoint plane crossing, bounded low-FPS sweeps,
short-teleport rejection, safer finished-race records/rewards, career replay
rollback and garage fairness, event-specific rival laps and Android input
cancellation. Strict portable CI passed; the Unreal Editor/UHT/game build,
cookable map, installed driving QA and EXE/APK remain unverified.

## v0.5.3 published source changes

An allocation-free checkpoint diagnostic evaluator reports a named reason
when a gate sample is rejected, and a trace diagnostic distinguishes the
first position from a recovery/restart teleport rebase. Gameplay still
uses the existing bool API and the v0.5.2 crossing/teleport rules.
A default-off, non-shipping Unreal console setting can log bounded local
checkpoint events (`latebrake.CheckpointDiagnostics 1`). Portable C++20
tests exercise all four gates. This is **not** evidence of
Unreal gameplay, checkpoint instrumentation on device or a playable build.
See [gate diagnostic design](docs/GATE_DIAGNOSTICS.md).

## v0.5.4 source-only pre-release

A fixed-memory `GateDiagnosticCounters` helper aggregates accepted
crossings, teleport rebases, malformed samples and rejected gate reasons
without collecting positions, names or remote telemetry. A non-shipping
Unreal console setting (`latebrake.CheckpointDiagnostics 2`) is proposed
to emit one bounded per-gate summary roughly every 10 seconds; mode 1
retains v0.5.3's event-only logs, and mode 0 remains silent by default.
Strict portable C++20 tests check classification, bounded time
windows, resets and saturation. An [offline checkpoint QA log summarizer](scripts/AnalyzeCheckpointQA.py)
can review a tester-supplied log without contacting any service; its output
never certifies game builds or playability.
**Published v0.5.4 is source-only, not Unreal-installed QA or a playable game.** See
[QA counter design](docs/GATE_QA_SUMMARIES.md).

## v0.5.5 published source changes

A new `FinishedRaceInsights` policy computes a reliable finished-race
scorecard from the **actual** `RaceProgress` clock and sector splits:
best and last recorded lap, a bounded allocation-free history of the most
recent eight positive timed laps, a final-vs-previous-lap signed delta,
a within-run theoretical best-sectors lap,
the gap between achieved fastest lap and that theoretical lap, and
fastest-to-slowest spread when **at least two positively timed laps**
exist. Instant zero-duration laps are not counted as timed evidence;
unrecordable clocks cannot generate a scorecard. The results menu and
non-compact finished HUD show these summaries, without changing career
credits, medals or checkpoint scoring. Strict C++20 regression tests
are in `Tests/Standalone/FinishedRaceInsightsTests.cpp`.

**Published source-only pre-release; not a playable game.** Real Unreal 5.8 gameplay,
HUD readability on screen and packaged Win64/Android behavior remain
unverified. [Finish insights design](docs/FINISH_INSIGHTS.md).

## v0.5.6 published source preview — circuit startup integrity

The original nine collision-bearing blocks (ground, four roads and four
barriers) are now defined in a shared **C++20 collision plan** instead of
nine Unreal-only hard-coded spawns. A new allocation-free
`CircuitStartupAudit` requires all nine successful block spawns, four
**unique** correctly indexed checkpoint actors, and seven groups / 93
scenery instances before the GameMode can emit
`LATEBRAKE_STARTUP_READY` or accept a race checkpoint. The runtime
checker also requires an exact `LATEBRAKE_GEOMETRY_READY collision=9/9`
marker in the correct order; synthetic log fixtures cannot qualify a
real playable build.

The portable collision-plan/startup-audit C++ tests and PowerShell
packaging-policy fixtures run on CI. **The Unreal 5.8 C++ compilation,
actual actor collisions and installed Win64/Android gameplay have not
been verified.** [Detailed audit and acceptance guide](docs/CIRCUIT_STARTUP_AUDIT.md).

## v0.5.7 source-only preview — collision configuration and safe spawns

The next C++ iteration strengthens the v0.5.6 course acceptance contract:
a spawned track Actor only counts if its actual Unreal StaticMesh
component is present, points to the expected built-in Cube, retains
`QueryAndPhysics` collision and has its requested scale applied.
A new engine-independent `CollisionBlockSetup.h` classifies failures;
`SpawnBlock` destroys rejected actors instead of falsely counting them
as collision geometry. A second C++20 preflight,
`CircuitSpawnSafety.h`, checks the original player/rival start and
recovery positions for finite coordinates, paved-surface placement and
clearance from perimeter barriers before allowing course readiness.

These changes have **not** been compiled or driven inside a licensed
Unreal Engine installation. Portable CI verifies numeric policies and
PowerShell synthetic log rejection only. The [development QA guide](docs/COLLISION_SETUP_QA.md)
lists actual on-device requirements.

## v0.5.8 published source-only — collision response integrity

A new `CollisionResponsePolicy.h` defines which Unreal collision
channels the prototype circuit must actually **block**: player pawns,
physics bodies and other dynamic world objects. The GameMode now reads
the real `UStaticMeshComponent` responses to `ECC_Pawn`,
`ECC_PhysicsBody` and `ECC_WorldDynamic` after applying `BlockAll`.
If any response is Ignore/Overlap rather than Block, it reports
`LATEBRAKE_COLLISION_SETUP_FAILED`, destroys the incomplete Actor,
and refuses to count the track as ready. This prevents a misleading
startup success if the mode is `QueryAndPhysics` but one of its
critical channels does not block.

The standalone `CollisionResponsePolicyTests.cpp` checks every
failure class with strict C++20. These are **source-only tests**:
real physical collision, licensed Unreal 5.8 Editor/UHT compilation,
the cookable map, and installed Windows/Android gameplay remain
unverified. See [collision response QA](docs/COLLISION_RESPONSE_QA.md).

## v0.5.9 source development — race clock and startup readiness (NOT released)

A new `RaceClockReadiness.h` runs the **real** portable
`RaceProgress` race clock only when the whole circuit has been
certified as source-created and the world is unpaused. Invalid frame
durations and races already finished cannot advance the timer. Unlike
the previous unconditional tick, a failed ground, barrier, collision
response, checkpoint Actor or safe spawn check can no longer produce
a misleading race clock. The prototype Results/normal HUD and
menu explicitly show `TRACK NOT READY` if circuit startup failed.

`Tests/Standalone/RaceClockReadinessTests.cpp` links the original
`RaceProgress.cpp` and checks unready/paused/invalid/finished
timing, pause-resume, a real gate finish and restart. This remains
**source-only development**: Unreal Engine 5.8 UHT/game compilation,
actual map cooking and installed Win64/Android gameplay are unverified.
[QA acceptance guide](docs/RACE_CLOCK_READINESS_QA.md).

## v0.5.10 source development — player spawn/clock guard (NOT released)

A new portable `PlayerRaceClockReadiness.h` prevents ghost lap time
from accumulating on an otherwise-ready circuit when there is no
valid local player car. The Unreal GameMode and prototype HUD now share
the same player-readiness check and show `PLAYER CAR NOT READY`
when the course exists but the driver pawn is missing.
Strict tests link the genuine portable `RaceProgress.cpp` source and
cover pawn loss/return, blocked startup, pause, finish and restart.
**No Unreal 5.8 UHT/editor compilation, gameplay or executable is
claimed.**

## v0.5.11 source development — checkpoint pause boundaries (NOT released)

A new engine-neutral `CheckpointSessionTrace.h` discards motion
samples collected before a menu pause. Both menu and Escape/gamepad pausing and resuming
advance a GameMode session revision, so the next checkpoint observation
establishes a baseline rather than crediting a crossing that spans
the interruption. The existing teleport generation protection remains.
GameMode also rejects gate awards if there is no local player or if
a menu is still open. See [session QA](docs/CHECKPOINT_SESSION_QA.md)
and `Tests/Standalone/CheckpointSessionTraceTests.cpp`.

This is source-only development, **not** a playable game or a verified
UE 5.8 build.

## v0.5.12 source development — safer candidate archives (NOT released)

The real-engine candidate pipeline now refuses symlinks and Windows
junctions at archive-root and nested-directory levels before signing
off on SHA256 file-list integrity. This closes a gap where a manifest
path through a linked folder could point outside the intended package.
The writer also rejects dirty tracked source and empty packaged files.
Disposable negative fixtures run in CI; passing them is **not**
evidence that Unreal has built a working game. See
[build and archive QA](docs/EXECUTABLE_BUILD_PIPELINE.md).

## v0.5.13 source development — exact candidate QA fingerprint (NOT released)

The platform release verifier now rejects installed-game QA whose
`candidate_manifest_sha256` does not match the SHA256 of the exact
manifest produced for that candidate. This matters because the same
source commit can produce different cooked packages; the old source
SHA check alone could not identify the specific game humans tested.
Both Win64/Android evidence templates are updated. A successful
source-policy fixture is **not** proof of compiled Unreal gameplay.
See [real-game build/QA instructions](docs/EXECUTABLE_BUILD_PIPELINE.md).

## v0.5.14 source development — reject malformed UAT candidates

After a genuine Unreal packaging command, the pipeline now runs
`scripts/Test-UATPackageStructure.ps1` before writing a candidate
manifest. It checks the Win64 executable header and cooked files,
or the Android APK ZIP/manifest/compiled-code structure, rejecting
obviously incomplete archives. Tests use invalid disposable files:
**no Unreal build, Windows executable or Android game is certified**.
See [executable build pipeline](docs/EXECUTABLE_BUILD_PIPELINE.md).

## Development and releases

Source-only pre-releases have been published from **v0.1.0** up to **v0.5.22** (not every intermediate development milestone received a tag); browse the [GitHub release history](https://github.com/sanskarIN/latebrake/releases). These are not verified executable game releases. **v0.4.0** expands the starter circuit visuals and adds deterministic [course layout and mapping utilities](docs/TRACK.md); see [v0.4.0 release notes](docs/releases/v0.4.0.md) and [release history](CHANGELOG.md). A verified playable release still requires genuine Unreal builds and hardware tests. See [candidate release notes](docs/releases/v0.1.0.md), [local packaging helper](scripts/Build-PreRelease.ps1), and [release evidence example](docs/releases/test-evidence.example.json). See [the v0.4.1 source release notes](docs/releases/v0.4.1.md), [current status](docs/STATUS.md), [roadmap](docs/ROADMAP.md), and [changelog](CHANGELOG.md). No playable game artifacts have been fabricated.

## License and contributions

The repository's **original game code and documentation** are BSD-3-Clause (see [LICENSE](LICENSE)). Unreal Engine itself remains subject to Epic's separate terms. Engine binaries/source and restricted marketplace assets are **not** bundled. Read [third-party notices](THIRD_PARTY_NOTICES.md) and [asset licenses](ASSET_LICENSES.md).

See [CONTRIBUTING.md](CONTRIBUTING.md), [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [SECURITY.md](SECURITY.md).

## Developer / contact

- GitHub: https://github.com/sanskarIN
- Business Gmail: sanskarprogrammer@gmail.com
- Business Outlook: sanskarcodes@outlook.com

## Voluntary project support

Latebrake is free to develop and contribute to. Support is **optional** and confers no gameplay or racing advantage.

- Buy Me a Coffee: https://buymeacoffee.com/SanskarCodes
- Razorpay: https://razorpay.me/@SanskarCodes
- PayPal.me: https://paypal.me/SanskarCodes

Platform distribution policies must be reviewed before showing external payment links inside any released app.

## v0.5.0 career and menu development

The source now defines **two three-round championship series**, **six standalone
challenges**, permanent best medals (bronze/silver/gold), prerequisite unlocks,
once-only completion credits, and a versioned career SaveGame extension. All
objectives reuse the **existing single prototype circuit**. These additions
are covered by engine-independent C++20 regression tests, **not** verified
Unreal gameplay tests.

- **N / M**: browse and start unlocked career events.
- **Tab / PageDown / PageUp / Enter**: toggle, navigate and activate
  the ten-page asset-free development menu.
- **Gamepad**: D-pad and face/menu buttons are mapped in
  `Config/DefaultInput.ini`.
- **Android experimental source**: visible touch-driven menu controls plus
  app background/foreground input-clearing hooks; no device qualification yet.
- See [career](docs/CAREER.md), [championships](docs/CHAMPIONSHIPS.md),
  [challenges](docs/CHALLENGES.md), [save migration](docs/SAVE_MIGRATION.md)
  and the [v0.5.0 release candidate notes](docs/releases/v0.5.0.md).

## v0.5.1 source — Android viewport touch safety (not device-verified)

Mobile held steering/pedal inputs are now cleared when the viewport
resizes or temporarily becomes unavailable. Tracking the physical
fingers independently prevents held touches from being replayed as
new race actions; the player must lift and press again. Portable tests
cover viewport changes, but actual Android device and UE 5.8 integration
remain unverified. See [touch control qualification](docs/TOUCH_CONTROLS.md).

## v0.5.1 source — batched scenery (not engine-verified)

The source now groups 93 non-colliding decorative cubes into seven
instanced-mesh batches under one scenery actor, preserving the original
one-circuit layout while reducing its cosmetic actor count. Portable tests
verify group membership and transforms; no actual Unreal 5.8 compile,
graphics output, draw-call measurement or mobile FPS claim has been verified.
See [track and scenery QA](docs/TRACK.md).

## v0.5.1 source — racing/progression integrity (not engine-verified)

- Career event selection and its required owned vehicle are now saved
  **before** the new race starts. If the profile write fails, the previous
  active race is preserved and the staged selection rolls back.
- Recovering the car requires successfully charging the full five-second
  race-clock penalty and updating the recovery count; failed charges
  cannot authorize a free teleport.
- New C++20 model regression coverage tests both behaviors, but actual
  Unreal Engine 5.8 integration, save-slot failure handling and on-device
  driving remain unverified.

## Real executable build path — still pending engine qualification

A reproducible **Windows-hosted Unreal Engine 5.8 candidate pipeline** is
being prepared. It compiles the Unreal Editor target, creates an actual
project-owned level via Unreal Editor, packages Win64 or Android through UAT,
then requires independently recorded installed-playtest evidence before
uploading real packages. The default game/cook map is now
`/Game/Maps/LatebrakeCircuit`; it is **not included until an authorized
Unreal Editor generates and saves the binary map**. Editor scripting
dependencies are editor-target-only, not part of the racing runtime.

**No verified executable files have been produced or uploaded from this source
revision.** A licensed Unreal installation and target hardware are still
mandatory. See the [step-by-step executable build pipeline](docs/EXECUTABLE_BUILD_PIPELINE.md).
The v0.5.0 source-only pre-release remains immutable. The v0.5.1 source-only pre-release adds touch hardening and the development features above without attaching a playable game; Android hardware verification is still pending.
