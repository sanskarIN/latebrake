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

The intended cooked game default is `/Game/Maps/LatebrakeCircuit`, **not** `/Engine/Maps/Entry`. This repository does not yet contain the serialized project-owned `.umap`: a genuine licensed UE 5.8 Editor must author and save it before any UAT cooking or packaged gameplay qualification.

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

After real hardware gameplay tests, create **separate** truthful platform reports using `docs/releases/win64-qa.example.json` and `docs/releases/android-qa.example.json`. The built candidate's `Latebrake-candidate-manifest.json` is required for each platform: run `scripts/Verify-PlatformPackage.ps1` with `-PackagePath`, `-ManifestJson` and `-EvidenceJson`. For both platforms together, run `Verify-ReleaseArtifacts.ps1` with both manifests and both QA records. See [executable pipeline and honest QA](EXECUTABLE_BUILD_PIPELINE.md). An actual release still requires passing the acceptance checklist in `docs/releases/v0.1.0.md`.
## Packaging gate for prereleases

Create real Win64 and Android packages using your authorized UE 5.8 build environment and record installed gameplay verification in the **platform-specific** QA JSON templates. The combined `Verify-ReleaseArtifacts.ps1` now requires both platform-specific manifests and both QA files and calls the stronger `Verify-PlatformPackage.ps1` checks for each platform. All archived bytes must match the post-UAT SHA256 manifest, and the QA build commit must match it. The checks do not independently prove good gameplay or replace actual engine builds. Never publish a v0.1.0 pre-release from CI's standalone test executable.

## v0.5.0 executable qualification gate

This repository's portable C++20 CMake/CTest builds do **not** produce
a playable Windows EXE or Android APK/AAB. The current environment and
GitHub-hosted CI do not include an installed/licensed UE 5.8 Editor,
Unreal Header Tool, cookable project-owned `.umap`, Android toolchain
configured with UE, or physical game-device QA.

An authorized maintainer must use a local or appropriately licensed
runner with Unreal Engine 5.8 and required SDKs to:
1. Compile `LatebrakeEditor` and `Latebrake` game targets; fix UHT and
   C++ issues, including lifecycle delegate and menu input APIs.
2. Create/save a **real** Unreal level through the editor under project
   `Content`, set appropriate default map/package cooking entries, and
   verify the saved level is included.
3. Build signed/installed Win64 game and Android APK/AAB from Unreal, not
   renamed portable test binaries or source archives.
4. Test game startup, controls/inputs, career awards, save migration,
   menus, performance and application interruptions on real hardware.
5. Attach artifacts only with SHA256 hashes and tested device/build
   evidence. Keep issue #2/#13 open until those acceptance criteria pass.

Source-only pre-releases must never imply or attach fake playable binaries.

## Real project-owned level and executable generation

The previous built-in `/Engine/Maps/Entry` game default has been replaced
with `/Game/Maps/LatebrakeCircuit` and an explicit `MapsToCook` reference.
The real map is not yet present in Git, because it **must** be created and
serialized by a licensed Unreal 5.8 Editor. Use the scripts
`Build-Editor.ps1`, `New-StarterMap.ps1` and
`Build-PreRelease.ps1`, or call `Build-PlayableCandidate.ps1` to run
the three stages in order on a Windows Unreal workstation.

The map is a blank real Unreal level into which the C++ game mode builds
the prototype circuit at runtime; it is not a fabricated `.umap` or an
alternative game engine. No generated map is claimed in this repository
at this time. The scripts reject missing engines, version mismatches, Git
LFS pointer maps and incomplete packaged output.

See [executable pipeline and honest QA](EXECUTABLE_BUILD_PIPELINE.md).
An opt-in GitHub workflow can use a maintainer-operated Windows self-hosted
Unreal runner labeled `unreal-5-8`; it never publishes game candidates as
verified GitHub Release assets automatically.
