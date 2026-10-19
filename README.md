# Latebrake

**Race Beyond Limits.** An original, community-developed racing game built with Unreal Engine 5.8 and C++20.

> **Development status: v0.1.0 in progress, not released.** The repository contains a coded starter driving/racing experience, but a full UE 5.8 editor compile, in-engine playtest, Windows package, and Android package have **not** been verified. Do not describe it as a finished or downloadable game yet.

## What's in source today?

- Unreal Engine 5.8 game/editor targets and a C++20 runtime module.
- A physics-simulated, keyboard/gamepad controlled starter car assembled from engine-built-in shapes.
- Arcade and simulation-inspired force/grip presets, braking, camera switching, recovery, pause and experimental two-finger touch regions.
- A generated four-checkpoint circuit, three-lap ordered race rules, HUD, race restart and finish state.
- A locally saved race best-time record and completed-races counter.
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
| Gamepad | Triggers, left stick, face buttons and shoulder button (see input config) |

Experimental touch bindings: drag in the left half to steer, hold upper-right to accelerate, hold lower-right to brake. **No Android build or touch behavior has been verified**, and accessible visual touch controls still need work.

## Test the portable race logic

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

First milestone: **v0.1.0 pre-release**, only after verified compilation, playtesting, and real packaging. A separate **v0.1.1 stabilization branch** adds lap/sector telemetry, gate direction rules and a five-second recovery penalty; it is not released. See [candidate v0.1.0 notes](docs/releases/v0.1.0.md) and [v0.1.1 plan](docs/releases/v0.1.1-plan.md). See [current status](docs/STATUS.md), [roadmap](docs/ROADMAP.md), and [changelog](CHANGELOG.md). No release files have been fabricated.

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
