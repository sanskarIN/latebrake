# Building Latebrake

**Status:** The Unreal targets and module have not yet been compiled in a licensed Unreal Engine installation. Portable racing logic is CI-tested separately. These instructions describe the intended reproducible workflow, not completed verification.

## Prerequisites

- Unreal Engine 5.8, acquired from Epic under applicable terms.
- Windows: Visual Studio with the C++ game development workload and the SDK/toolchain compatible with your installed Unreal Engine version.
- Linux: an Unreal-supported toolchain and the engine's required native dependencies.
- Git LFS for future authorized binary game assets.
- CMake 3.16+ and a C++20 compiler for independent tests.

Unreal Engine is **not** redistributed by this repository.

## Windows editor workflow

1. `git clone https://github.com/sanskarIN/latebrake.git`.
2. `cd latebrake` and `git lfs install` (then `git lfs pull` when LFS assets are introduced).
3. Generate Visual Studio project files from `Latebrake.uproject`.
4. Build the **Development Editor** `LatebrakeEditor` target for **Win64**.
5. Open `Latebrake.uproject` using UE 5.8.
6. Inspect Output Log for missing built-in engine mesh/map references.
7. Use Play in Editor; verify possession, movement, checkpoint overlap, race completion, restart, save/reload and pause.

The configured map is `/Engine/Maps/Entry`. Confirm it is present and suitable in your local engine installation. Create and cook an **original project-owned .umap** before treating packaging as release ready; this repository does not yet contain one.

## Linux development

Use the exact engine-supplied scripts and compiler supported by your UE 5.8 installation to generate project files and build `LatebrakeEditor`. Do not announce Linux packages until engine compilation, runtime and controller checks pass on real Linux hardware.

## Android

Android is a **target**, not a currently verified build. Configure UE 5.8's approved Android SDK/NDK/JDK through Unreal Turnkey, add mobile-specific touch controls/UI, and test input, save location, renderer/memory consumption and suspend/resume behavior before building test APKs or store AABs.

## Portable tests (no Unreal installation)

```sh
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

These compile the **same RaceProgress.cpp** used by the game. They cannot validate Unreal-specific C++ symbols, Unreal Header Tool generation, blueprints, maps, graphics, packaging or device performance.

## Known source-level validation gaps

- No Unreal Engine binary/editor exists in the present automated coding environment.
- Unreal classes need a real compile and gameplay inspection; treat engine integration as **unverified** until the above steps pass.
- Starter camera, physics values, and checkpoint dimensions need iteration inside the engine.
- Development runtime creates road and props using engine built-in primitive assets; no project-owned cooked map/art package is checked in.
- No packaged executable, APK, AAB, or platform benchmarks exist yet.

## Candidate packaging helpers (unverified until executed with UE 5.8)

On a licensed Windows Unreal Engine build workstation, the optional helper `scripts/Build-PreRelease.ps1` calls Epic's own `RunUAT.bat BuildCookRun` and checks for a real platform package. Example:

```powershell
pwsh ./scripts/Build-PreRelease.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Win64 -OutputDirectory 'D:/LatebrakeBuilds/Win64'
pwsh ./scripts/Build-PreRelease.ps1 -EngineRoot 'C:/Program Files/Epic Games/UE_5.8' -Platform Android -OutputDirectory 'D:/LatebrakeBuilds/Android'
```

Android requires Unreal's supported Android toolchain plus real touch/lifecycle testing. Confirm the built-in entry map is cookable or add a project-owned map if needed. These commands have **not** yet succeeded in this environment. The script does not assert a build succeeded unless Unreal returns a success exit and candidate platform files exist.

After real hardware gameplay tests, record honest evidence using `docs/releases/test-evidence.example.json` and validate package structure plus SHA-256 hashes using `scripts/Verify-ReleaseArtifacts.ps1`. An actual release still requires passing the acceptance checklist in `docs/releases/v0.1.0.md`.
## Packaging gate for prereleases

Create real Win64 and Android packages using your authorized UE 5.8 build environment, record actual device and gameplay verification in a JSON file based on `docs/releases/test-evidence.example.json`, and check package structure and hashes with `scripts/Verify-ReleaseArtifacts.ps1`. The verification script performs only **basic structural checks**, not a UE compile, malware audit, or gameplay qualification. Never publish a v0.1.0 pre-release from CI's standalone test executable.
