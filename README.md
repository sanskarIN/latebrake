# Latebrake

**Race Beyond Limits.** An original, community-developed racing game built with Unreal Engine 5.8 and C++20.

> **Development status: v0.4.0 source-only course/environment preview in progress; v0.1.0 through v0.3.2 are published as source-only pre-releases.** The repository contains a coded starter driving/racing experience, but a full UE 5.8 editor compile, in-engine playtest, Windows package, and Android package have **not** been verified. Do not describe it as a finished or downloadable game yet.

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
- Portable, engine-independent lap/checkpoint rules with C++20 CMake tests on Linux, Windows and macOS CI.

The starter vehicle uses **Chaos rigid-body physics**. It does **not yet** implement Chaos Vehicles' tire/suspension drivetrain; that upgrade requires an authored skeletal vehicle rig and physics assets. The procedural circuit is intentionally temporary. There are no shipping-quality vehicles, audio, traffic, multiplayer, career menus, or Android touch interfaces yet.

## Quick start (source)

1. Install Unreal Engine **5.8** (licensed and installed separately) and the applicable C++ build toolchain.
2. Clone this repository with Git LFS installed: `git lfs install && git clone https://github.com/sanskarIN/latebrake.git`.
3. Check out the development branch if the feature has not yet been merged.
4. Right-click `Latebrake.uproject` and generate project files (or open from the Unreal project browser).
5. Build the `LatebrakeEditor` target with your local toolchain; open `Latebrake.uproject`, then use Play in Editor.

The game mode is selected via `Config/DefaultEngine.ini`, which points at a built-in engine entry map and generates its starter track at runtime. Check **[Build instructions](docs/BUILDING.md)** for limitations and packaging prerequisites.

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

Experimental touch bindings: drag in the left half to steer, hold upper-right to accelerate, hold lower-right to brake. **No Android build or touch behavior has been verified**, and accessible visual touch controls still need work.

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
- `Tests/Standalone`: portable C++20 checks.
- `docs`: design, architecture, build instructions, status, and roadmap.

## Development and releases

**v0.1.0**, **v0.1.1**, **v0.2.0**, **v0.2.1**, and **v0.3.0** have been published as honest [source-only GitHub pre-releases](https://github.com/sanskarIN/latebrake/releases), without executable games. **v0.4.0** expands the starter circuit visuals and adds deterministic [course layout and mapping utilities](docs/TRACK.md); see [v0.4.0 release notes](docs/releases/v0.4.0.md) and [release history](CHANGELOG.md). A verified playable release still requires genuine Unreal builds and hardware tests. See [candidate release notes](docs/releases/v0.1.0.md), [local packaging helper](scripts/Build-PreRelease.ps1), and [release evidence example](docs/releases/test-evidence.example.json). See [current status](docs/STATUS.md), [roadmap](docs/ROADMAP.md), and [changelog](CHANGELOG.md). No playable game artifacts have been fabricated.

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
