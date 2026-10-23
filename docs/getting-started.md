# Getting started

Latebrake is currently a **source project**. The steps below are for developers who have their own authorized Unreal Engine setup, not for installing a finished racing game.

## 1. Clone the repository

```bash
git lfs install
git clone https://github.com/sanskarIN/latebrake.git
cd latebrake
```

## 2. Test portable C++20 logic without Unreal

Install Git, CMake and a C++20-capable compiler (MSVC, Clang or GCC).

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
```

This runs race rules, input, vehicle safety and other portable tests. It **does not build or launch the Unreal game**.

## 3. Prepare the actual Unreal project

1. Install **Unreal Engine 5.8** through an authorized source and the compatible platform C++ toolchain.
2. Open `Latebrake.uproject` in the Unreal project browser and generate native project files as needed.
3. Build the `LatebrakeEditor` target and inspect the complete compiler/UHT results.
4. Create or load the project-owned `/Game/Maps/LatebrakeCircuit` via the supplied Editor automation; the source repo does **not** contain a verified authentic cooked `.umap`.
5. Run the game's map in Editor, inspect the runtime startup audit, drive the car, cross each gate in order, restart and recover, and record real QA evidence.

**None of these actual Unreal steps has been completed by hosted CI.** Read the full [build instructions](BUILDING.md).

## 4. Packaging and QA

The repository includes helper scripts for editor map creation, BuildCookRun and candidate manifest verification. They require real Unreal output. Do not treat disposable policy fixtures or GitHub-generated source archives as runnable game binaries.

- [Build and packaging guide](EXECUTABLE_BUILD_PIPELINE.md)
- [Testing and manual acceptance](TESTING.md)
- [Current qualification blockers](development-status.md)

## 5. Preview these docs locally

The GitHub Pages documentation uses MkDocs with a pinned Material theme version, independently of Unreal.

```bash
python -m pip install -r docs-requirements.txt
python -m mkdocs serve
```

Then open the local URL printed by MkDocs. To produce static site files, run `python -m mkdocs build`.

## New contributor?

Read [contribution guidelines](contributing.md) and start with a small bug fix, test, or clear documentation correction.
