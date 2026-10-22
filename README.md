# Latebrake

**Race Beyond Limits.** An original, community-developed racing game built with Unreal Engine 5.8 and C++20.

> **Source preview: [v0.5.4](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.4) — SOURCE-ONLY PRE-RELEASE (22 October 2026), NOT A PLAYABLE GAME.** GitHub source ZIP/tar.gz files are for developers; there is **no Windows EXE, Android APK/AAB or Linux game**. Portable C++20 CI does not compile Unreal 5.8. The editor/UHT build, real cookable project map, Win64/Android packages and installed playtests remain unverified.

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

## v0.5.5 development (NOT released)

A new `FinishedRaceInsights` policy computes a reliable finished-race
scorecard from the **actual** `RaceProgress` clock and sector splits:
best and last recorded lap, a within-run theoretical best-sectors lap,
the gap between achieved fastest lap and that theoretical lap, and
fastest-to-slowest spread when **at least two positively timed laps**
exist. Instant zero-duration laps are not counted as timed evidence;
unrecordable clocks cannot generate a scorecard. The results menu and
non-compact finished HUD show these summaries, without changing career
credits, medals or checkpoint scoring. Strict C++20 regression tests
are in `Tests/Standalone/FinishedRaceInsightsTests.cpp`.

**Unreleased source development only.** Real Unreal 5.8 gameplay,
HUD readability on screen and packaged Win64/Android behavior remain
unverified. [Finish insights design](docs/FINISH_INSIGHTS.md).

## Development and releases

**v0.1.0** through **v0.5.4** have been published as honest [source-only GitHub pre-releases](https://github.com/sanskarIN/latebrake/releases), without executable games. **v0.4.0** expands the starter circuit visuals and adds deterministic [course layout and mapping utilities](docs/TRACK.md); see [v0.4.0 release notes](docs/releases/v0.4.0.md) and [release history](CHANGELOG.md). A verified playable release still requires genuine Unreal builds and hardware tests. See [candidate release notes](docs/releases/v0.1.0.md), [local packaging helper](scripts/Build-PreRelease.ps1), and [release evidence example](docs/releases/test-evidence.example.json). See [the v0.4.1 source release notes](docs/releases/v0.4.1.md), [current status](docs/STATUS.md), [roadmap](docs/ROADMAP.md), and [changelog](CHANGELOG.md). No playable game artifacts have been fabricated.

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
