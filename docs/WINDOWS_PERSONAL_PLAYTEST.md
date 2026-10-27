# Build LateBrake for personal Windows testing

**No ready-to-download LateBrake Windows game EXE has been produced or
verified by the source-only GitHub CI.** This guide builds the **actual
Unreal game** on a Windows PC; it does not download an Unreal installer,
rename a source ZIP, or turn the standalone CMake tests into a game.

## What you need on your Windows PC

- A legally authorized installed **Unreal Engine 5.8**, including
  `UnrealEditor.exe`, `Build.bat` and `RunUAT.bat`.
- Visual Studio's compatible **C++ game-development workload**, Windows
  SDK and other build components required by that UE 5.8 installation.
- **Git**, **Git LFS** and **PowerShell 7 (`pwsh`)**.
- Adequate free disk space for the Editor/UHT compile and cooked Win64
  Development game, and a working Windows graphics driver.

Do not use GitHub's “Download ZIP” as a substitute for a clean Git
checkout: the candidate pipeline deliberately requires traceable Git
source metadata. Run the following once:

```powershell
git clone https://github.com/sanskarIN/latebrake.git
cd latebrake
git lfs install
git lfs pull
```

## One-click: personal candidate

Double-click **`Build-Windows-Playtest.cmd`** in the repository root.
The wrapper defaults to
`C:\Program Files\Epic Games\UE_5.8` and uses PowerShell 7.

If the authorized engine lives elsewhere, run this from PowerShell 7
inside the repository:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8'
```

To open the game immediately **only after a successful real build**:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

The script calls the existing `Build-PlayableCandidate.ps1`, which
compiles the real `LatebrakeEditor` target through UE/UHT, asks licensed
Unreal Editor to create/save the genuine project circuit level (if it
does not exist), cooks a Win64 Development game through `RunUAT.bat`,
and verifies the package files and source/map/engine SHA256 manifest.

Each run gets a **new** directory below
`out/Win64Playtests/LateBrake-Win64-<timestamp>-<unique-id>/`.
This location is Git-ignored; previous packages are never replaced.
A successful run prints the **full exact path** of the packaged
`Latebrake.exe` and the SHA256 of that EXE and its candidate manifest.

**Keep all cooked files in their packaged directory.**
`Latebrake.exe` alone is not a portable game: moving it without its
supporting cooked assets can cause startup errors.

## Private diagnostic game run (new)

After a **successful** game package, run the genuine EXE through the
candidate verifier/diagnostic launcher, not an unrelated desktop EXE.
The original build command's `-Launch` option now does this
automatically, then waits for you to close the game:

```powershell
pwsh ./scripts/Build-WindowsPlaytest.ps1 -EngineRoot 'D:\UE_5.8' -Launch
```

To **relaunch an already-built** private candidate without compiling again,
drag the full built-candidate **folder onto** `Play-Windows-Candidate.cmd`
(which passes the folder as its first argument), or use:

```powershell
pwsh ./scripts/Run-WindowsPlaytest.ps1 -CandidateDirectory 'D:\LatebrakeCandidates\YourRealWin64BuildFolder'
```

The new launcher checks the complete closed-world SHA256 candidate
manifest, verifies Win64 EXE/cooked-content structure, then launches the
packaged `Latebrake.exe` with Unreal's documented `-log` and `-ABSLOG`
arguments. The latter directs the **private diagnostic runtime log**
outside the immutable candidate folder, to a sibling
`LateBrakePlaytestLogs` directory by default. You can customize
`-LogDirectory`, but it must not be inside the game folder.

After **you close the game**, the helper records an equally private
JSON receipt with source artifact checksums, UTC times, process exit
code and log presence. It also runs the existing `Verify-RuntimeStartupLog.ps1`
check for track geometry/gate/scenery markers when the log exists; the
receipt records whether this **text-only** audit passed and why it failed. No network upload occurs. A zero exit code
**does not** qualify rendering, user input, racing, saves or hardware
performance. Even with `-ABSLOG`, other engine runtime files may be
created under the package: re-run manifest validation before reusing
the exact archive. If the process crashes too early, an Unreal log
might not exist; retain the real UAT/Windows crash information.

Official Epic documentation:
[Unreal Engine command-line argument reference](
https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference)
describes `ABSLOG` as an absolute log filename argument.

## Record a private, self-reported Win64 QA review

After closing the real game session, the diagnostic runner prints the path
to a **private JSON receipt**. Keep it outside the candidate alongside the
runtime logs. On your own workstation, make an editable checklist tied to
the actual runtime receipt, candidate manifest and EXE hashes:

```powershell
python ./scripts/Review-WindowsPlaytest.py init `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --output 'D:\LateBrakeQA\manual-checklist.json'
```

Edit `manual-checklist.json` *after actual hands-on driving*. For each
check, set `result` to `pass`, `fail`, or `not_tested`; give an actual
observation in `notes` (at least 12 characters for pass/fail). Record
Windows version and CPU/GPU/controller details if you wish. The nine
required checks include scene/physics, brake/reverse, steering/camera,
GO/gates, pause/focus/restart, recovery penalty, save/reload and
hardware performance. **Never substitute source tests for manual
observations.** For example, if no real game started, leave everything
`not_tested` or describe the failure honestly.

```powershell
python ./scripts/Review-WindowsPlaytest.py evaluate `
  --receipt 'D:\LateBrakePlaytestLogs\LateBrake-your-session.json' `
  --checklist 'D:\LateBrakeQA\manual-checklist.json' `
  --output 'D:\LateBrakeQA\manual-review.json'
```

The offline evaluator refuses a changed EXE/manifest, another session's
receipt, missing checks, undocumented pass/fail choices and output paths
inside the sealed package. It does not overwrite existing evidence,
upload files or run a game. The report always says
`SELF_REPORTED_WIN64_QA_UNVERIFIED` and
`ready_for_release: false`, **even when every test was marked pass**.
That is intentional: user-authored JSON can be edited and does not prove
independent QA.

Before treating a candidate as a genuine playable build, also rerun
`scripts/Test-CandidateManifest.ps1` against the whole archive,
preserve real UHT/UAT build receipts, inspect actual installed driving
and satisfy [playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148).
The Python reviewer checks the two main file hashes and runtime
receipt link; **it does not revalidate every cooked asset**. Protect
your personal hardware details and do not upload private QA files
without reviewing them.

## What to test personally

1. Launch the actual `Latebrake.exe` printed by the script. If it fails,
   keep the full UHT/UAT console output and the real Unreal game log.
2. Confirm the circuit, physical car, engine-provided meshes and HUD
   appear; try basic steering, accelerate, reverse, brake and camera switch.
3. Test the 3/2/1/GO gate, each of four sequential checkpoints, laps,
   Time Attack, restart, paid recovery, pause/unpause and focus loss.
4. Drive several sessions, inspect save/reload, offline garage/career,
   rival car, race Results and advisory launch/drift readings.
5. Record Windows version, CPU/GPU, frame rates, issues and the printed
   EXE/candidate-manifest SHA256. Do not report an untested option as passed.

An output directory and an EXE signature alone do not prove that a game
really starts or handles input. A Windows **personal test candidate**
should remain **UNVERIFIED** until you run and inspect it yourself.
The formal QA/release gate is stricter and remains open in
[issue #148](https://github.com/sanskarIN/latebrake/issues/148);
Android is independently tracked in
[issue #13](https://github.com/sanskarIN/latebrake/issues/13).

## Troubleshooting

- **Missing Unreal Engine 5.8 component**: install/configure the
  authorized engine or pass the correct `-EngineRoot`; no EXE can be
  built from portable CMake checks alone.
- **PowerShell 7 not found**: install `pwsh` before using the double-click
  entry point.
- **Dirty/untracked source**: commit intentional source changes or use
  a fresh `git clone`; only the Editor-authored starter map exception
  is accepted and separately hashed.
- **UHT/C++ or cooker failure**: inspect the actual Unreal error output.
  This helper does **not** suppress compiler or packaging errors.
- **Missing packaged Latebrake.exe**: do not distribute or rename a
  different EXE. Preserve failed build logs, inspect the real UAT output
  and correct the packaging problem.

This helper does not upload private game files or change GitHub release
status. Source tests can check script structure, but only an installed
UE 5.8 Windows workstation can establish actual compilation/gameplay.
