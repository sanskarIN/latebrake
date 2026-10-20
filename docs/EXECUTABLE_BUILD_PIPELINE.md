# Building genuine playable LateBrake candidates

**Status, 20 October 2026:** This project is source-only. The files below
make packaging more reproducible **on an authorized Windows UE 5.8
workstation**; no package, actual Unreal build or installed playtest has yet
been performed by this repository's ordinary three-OS CI.

## Requirements

- Genuine licensed Unreal Engine **5.8** installed on Windows, with
  Unreal Build Tool, Editor, Unreal Automation Tool and compatible MSVC.
- `git lfs install` and `git lfs pull` for the eventual binary map/asset
  files (Git LFS pointers are **not** game assets).
- Windows target: appropriate Windows SDK and Visual Studio C++ toolchain.
- Android target: Epic-supported Android SDK/NDK/JDK and any required signing
  or package settings configured **inside Unreal**.
- Enough storage and disk permissions for cooked/staged Unreal games. The
  source repository does not redistribute the engine or its licensed assets.

## One command per platform (run on the Windows host)

From the repository root in PowerShell 7:

```powershell
$Engine = 'C:\Program Files\Epic Games\UE_5.8'

pwsh ./scripts/Build-PlayableCandidate.ps1 `
  -EngineRoot $Engine -Platform Win64 `
  -OutputDirectory 'D:\LatebrakeCandidates\Win64'

# Android only after the Unreal Android toolchain is configured:
pwsh ./scripts/Build-PlayableCandidate.ps1 `
  -EngineRoot $Engine -Platform Android `
  -OutputDirectory 'D:\LatebrakeCandidates\Android'
```

The pipeline runs three **real Unreal** operations in sequence:

1. `scripts/Build-Editor.ps1` checks the installed engine's
   `Engine/Build/Build.version`, rejects non-5.8 installations, and calls
   Unreal `Build.bat LatebrakeEditor Win64 Development` (UHT/C++ compile).
2. `scripts/New-StarterMap.ps1` launches `UnrealEditor.exe` with
   `-ExecutePythonScript` to run
   `scripts/Unreal/CreateStarterMap.py`. The UE
   `LevelEditorSubsystem.new_level` and `save_current_level` API
   **genuinely create** `Content/Maps/LatebrakeCircuit.umap`. The script
   refuses to replace an existing authored level.
3. `scripts/Build-PreRelease.ps1` runs Unreal `RunUAT.bat BuildCookRun`
   against the actual project map, with `-build -cook -stage -pak -package
   -archive`, checks for cooked content and the platform package.
   Fake `.umap` text files and Git LFS pointer text are not acceptable.

The `.uproject` enables Python and Editor Scripting Utilities **for Editor
target only**. These are authoring dependencies, not latebrake runtime
dependencies. The game itself remains Unreal Engine + C++20.
A new blank level is sufficient for this prototype because
`ALatebrakeRaceGameMode::BuildCircuit()` constructs the actual test circuit
at runtime, but the engine/level must still be compiled and playtested.
The editor startup map remains a built-in level until a real authored map is
available; the packaged GameDefaultMap is the project-owned map.

## Once a real package exists

1. Install and run the exact candidate on real Windows hardware or an
   Android device. Test racing controls, ordered gates, restart, time attack,
   career/championship awards, garage, pause/resume and save/load.
2. For Android, test concurrent steering and gas/brake fingers, menu focus,
   background/foreground transitions, touch cancellation, orientation and
   lower-end device performance.
3. Copy the appropriate template and **fill it with true evidence**:
   `docs/releases/win64-qa.example.json` or
   `docs/releases/android-qa.example.json`. Set
   `installed_and_playtested` true only after actually testing. Keep evidence
   and full raw build/device logs private when they contain machine details.
4. Run the structural verifier and record every SHA256 hash:

```powershell
pwsh ./scripts/Verify-PlatformPackage.ps1 -Platform Win64 `
  -PackagePath 'D:\LatebrakeCandidates\Win64' `
  -EvidenceJson 'D:\LatebrakeCandidates\win64-qa.json'

pwsh ./scripts/Verify-PlatformPackage.ps1 -Platform Android `
  -PackagePath 'D:\LatebrakeCandidates\Android\YourActualBuild.apk' `
  -EvidenceJson 'D:\LatebrakeCandidates\android-qa.json'
```

The verifier checks real Win64 PE header/size and cooked asset payload
or Android APK ZIP/manifest/native application content. It refuses missing
or incomplete human-installed-playtest evidence, but **cannot independently
certify gameplay quality**. Only upload verified outputs to a GitHub Release.
Do not replace v0.5.0's immutable source release tag.

## Optional manually triggered GitHub workflow

The opt-in `.github/workflows/build-unreal-candidate.yml` job requires a
**self-hosted Windows x64 runner** labeled `unreal-5-8`, with a licensed UE
5.8 installation and the `LATEBRAKE_UNREAL_ROOT` machine environment
variable configured for the runner service. Trigger it manually in Actions,
choose Win64 or Android, and use its temporary
`UNVERIFIED-candidate` workflow artifact for **subsequent real installed QA**.
It intentionally does not run on untrusted PRs, does not use ordinary
GitHub-hosted runners, and does **not** attach game files to GitHub Releases.
No runner of this type is known to be connected at this time.

## Validation boundaries

The ordinary C++20 CI and PowerShell negative packaging tests **do not**
compile UHT, Unreal game code or Editor Python APIs. The latter are syntax-
checked only. Error checking and documentation are steps toward the first
genuine build, not substitutes for it.

Unresolved: [Unreal game/packaging qualification issue #2](https://github.com/sanskarIN/latebrake/issues/2)
and [Android real-device issue #13](https://github.com/sanskarIN/latebrake/issues/13).

Official Epic documentation:
- https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python
- https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem

### Build-integrity hardening after v0.5.0

- Epic's Unreal Python API documents `EditorAssetLibrary.does_asset_exist`
  as excluding **levels**. The level automation now checks for an existing
  project-owned `.umap` on disk, then calls `LevelEditorSubsystem.load_level`
  if it exists; it never calls `new_level` over an existing level.
  Seven offline mock tests cover the creation/load/failure decisions. They
  do **not** run Unreal or create release-ready map assets.
- `Verify-UnrealMapHeader.ps1` rejects tiny/fake `.umap` and Git LFS
  pointers by checking the Unreal package tag. It is a basic sanity check,
  **not** a substitute for engine load/cook verification.
- `Build-PreRelease.ps1` requires a **fresh, empty output directory** for
  every build. It refuses to reuse an old archive: a stale EXE/APK must not
  pass a new build just because it was already present on disk.
- After a genuine successful Unreal package,
  `Write-CandidateManifest.ps1` writes
  `Latebrake-candidate-manifest.json` inside the candidate directory,
  fingerprinting the source commit, engine version, authored map SHA256 and
  all produced package files. It labels results
  `UNVERIFIED_CANDIDATE_NOT_PLAYTESTED`, not playable or stable.
- Existing keyboard/gamepad Escape and pause requests now close a
  still-open development menu before unpausing driving.

To retry a candidate build, choose a **different empty directory** instead
of overwriting any earlier output. The manifest provides traceability, not
evidence that packaged gameplay functions correctly.

## First real executable smoke evidence

A licensed UE 5.8 build is **still required**. The authoring helper now deletes
the previous `Saved/LatebrakeAutomation/starter-map-result.json` before
opening the Editor. The embedded Python script writes a new receipt only after
the actual level loads or is created and its save succeeds. The PowerShell
wrapper rejects Editor runs without a fresh matching receipt, even if
`UnrealEditor.exe` exits with code zero. This is important because a
Python exception may be logged without reliably propagating an OS exit code.

The C++ GameMode emits one of two markers once it has attempted to spawn all
four checkpoint actors:

- `LATEBRAKE_STARTUP_READY map=LatebrakeCircuit checkpoints=4/4`
- `LATEBRAKE_STARTUP_BLOCKED map=... checkpoints=.../4`

After installing and *actually starting* a cooked Win64 game, capture that
launch's real Unreal log (typically under the packaged game's `Saved/Logs`;
locations vary with packaging configuration) and run:

```powershell
pwsh ./scripts/Verify-RuntimeStartupLog.ps1 `
  -LogPath 'D:\ActualGameRun\Saved\Logs\Latebrake.log'
```

The verifier requires exactly one ready marker for `LatebrakeCircuit` and
rejects any blocked marker. Its passing result is a **limited runtime
startup check**, not a substitute for installed driving, frame-rate,
career/save, audio, input or Android QA. Synthetic fixtures are used only for
negative tests and are never attached to a release.

### Android touch ownership

`ALatebrakeRaceGameMode` now uses
`ALatebrakePlayerController`. On Android only, it enables touch-event
routing and activates game-only input with Unreal's stock virtual joystick
disabled; the car pawn's two-finger steering/pedal layout remains the source
of input and Canvas visualization. Verify both fingers, menus, pause,
focus loss and hardware orientation on a **physical Android device**. The
source-only CI cannot certify any of these behaviors.
