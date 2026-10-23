# Building genuine playable LateBrake candidates

**Status, 23 October 2026:** This project is source-only. The files below
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

## Source provenance check (v0.5.24 development, not released)

Before running the expensive UE 5.8 build, `Test-SourceProvenance.ps1` now
requires a committed Git `HEAD` with **no staged, modified, or unexpected
untracked source files**. It is executed before `Build-Editor.ps1`, after
Editor map authoring, before direct `Build-PreRelease.ps1`, and again when
writing the candidate manifest. This closes a gap in the previous
`git status --porcelain --untracked-files=no` check: that command ignored
new untracked `.cpp`, `.h`, `.ini`, or plugin files even though they could
change a cooked game without changing its recorded source commit.

The **only** nonignored untracked exception is
`Content/Maps/LatebrakeCircuit.umap` itself, because the licensed
Unreal Editor may create it in the same build attempt. The pipeline
separately validates its serialized package header and records its
exact SHA256 in the candidate manifest. That exception does **not**
authenticate the map or make an untracked/locally generated level
reproducible across machines; commit real authored assets via Git LFS
for distributable releases. Ignored Unreal intermediates remain ignored.

You can inspect source provenance without Unreal:

```powershell
pwsh ./scripts/Test-SourceProvenance.ps1
pwsh ./scripts/Test-SourceProvenancePolicy.ps1
```

The one-command `Build-PlayableCandidate.ps1` wrapper additionally pins
the **starting 40-character Git commit** and checks that same exact revision
after map authoring, immediately before and after UAT cooking, and when
writing the manifest. A different *clean* commit is not sufficient:
the candidate must be discarded rather than misattributed to a revision
that was not the one initially compiled. Direct standalone invocations
of `Build-PreRelease.ps1` and `Write-CandidateManifest.ps1` accept an
optional `-ExpectedCommit <40-hex-SHA>` to enforce the same policy.

The second command creates and deletes only disposable Git test histories.
These checks **do not** prove Unreal compilation, map cook, package origin,
hardware installation or gameplay.

## Circuit map continuity (v0.5.25 source development)

An unchanged Git commit **does not imply identical circuit content**:
the licensed Editor can create an untracked `LatebrakeCircuit.umap`,
and it was previously hashed only when the packaging manifest was written.
The candidate wrapper now pins the Editor-saved map's SHA256 before UAT.
`Build-PreRelease.ps1` compares its pre-cook and post-cook bytes, and
`Write-CandidateManifest.ps1` rechecks the original digest. A changed or
missing map fails the candidate build rather than producing misleading
metadata. Direct calls can pass `-ExpectedMapSha256` to bind them to a
previously approved snapshot. Byte matching alone **cannot prove** that
Unreal cooked or ran the expected map.

A complete Win64 + Android manual release review now checks that both
fully verified platform manifests refer to the **same source commit,
Unreal Engine version, and circuit map SHA256**. Two builds from the same
commit but different freshly generated level files cannot be treated as
one common release candidate. The map should be committed as an actual
Git LFS asset before a distributable release.

The following tests require no Unreal SDK and generate only temporary
synthetic data (not game assets):

```powershell
pwsh ./scripts/Test-CandidateMapSnapshotPolicy.ps1
pwsh ./scripts/Test-CandidatePairIdentityPolicy.ps1
```

These tests enforce identity rules, not binary provenance, gameplay
quality, physics, Android input, or real hardware qualification.

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
   Every report must match both the `source_commit` and the **exact
   candidate manifest SHA256**. After a successful genuine UAT candidate
   build and before any installed test, record the manifest hash:
   ```powershell
   (Get-FileHash -LiteralPath 'D:\LatebrakeCandidates\Win64\Latebrake-candidate-manifest.json' -Algorithm SHA256).Hash.ToLowerInvariant()
   ```
   Enter that 64-character value as `candidate_manifest_sha256` in the
   Win64 report; calculate the Android candidate's manifest hash separately.
   `Build-PlayableCandidate.ps1` also prints this value for the QA handoff.
   This is an identifier, not proof that the build was installed or played.
   Do **not** reuse a prior run's QA evidence even if the source commit matches.
4. Run the structural verifier and record every SHA256 hash:

```powershell
pwsh ./scripts/Verify-PlatformPackage.ps1 -Platform Win64 `
  -PackagePath 'D:\LatebrakeCandidates\Win64' `
  -ManifestJson 'D:\LatebrakeCandidates\Win64\Latebrake-candidate-manifest.json' `
  -EvidenceJson 'D:\LatebrakeCandidates\win64-qa.json'

pwsh ./scripts/Verify-PlatformPackage.ps1 -Platform Android `
  -PackagePath 'D:\LatebrakeCandidates\Android\YourActualBuild.apk' `
  -ManifestJson 'D:\LatebrakeCandidates\Android\Latebrake-candidate-manifest.json' `
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

## v0.5.14 UAT package preflight (source only)

`Build-PreRelease.ps1` calls `Test-UATPackageStructure.ps1` after
Unreal BuildCookRun exits zero and before `Write-CandidateManifest.ps1`.
This detects candidate archives missing a recognizable Win64 MZ
executable, sufficiently sized cooked .pak/.ucas payloads, or a valid
Android APK ZIP containing its nonempty manifest and native library or
DEX. It also rejects junctions and symlinks within the package.
Hosted CI runs only disposable **negative** fixtures: small or padded
non-executable files and malformed APKs. It does not actually run UAT
or create an installable candidate. These checks complement but never
replace map cook, a real launch or installed hardware QA.

## v0.5.12 archive path safety checks (source only)

Before trusting an archive's file hashes,
`Test-CandidateManifest.ps1` now inspects the manifest, package root
and every directory component of each listed relative path for
symbolic links, junctions and other filesystem reparse points.
It also rejects otherwise-unlisted linked directories, including
empty junctions that do not contribute files. This prevents a valid
checksum of a file reached through a link from silently authenticating
bytes outside the candidate archive. The companion
`Write-CandidateManifest.ps1` rejects all linked package entries,
zero-length payload files and dirty tracked source before writing.
The disposable Windows junction QA fixtures use ordinary text,
**not** manufactured executables or Unreal output.

Archive integrity and provenance are still only prerequisites:
even after a future authentic Editor/UHT + UAT success, physical
installed-game QA remains necessary before a playable release.

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

The verifier now requires exactly one ready marker for
`LatebrakeCircuit` **and** one prior
`LATEBRAKE_SCENERY_READY groups=7 instances=93 collision=off` marker from
the *same genuine runtime log*. Any `LATEBRAKE_STARTUP_BLOCKED` or
`LATEBRAKE_SCENERY_INCOMPLETE` line rejects the smoke gate. The batching
marker proves only that the Unreal GameMode constructed its seven instances
groups and 93 instance transforms: it is **not** visual output or GPU
performance evidence.

The successful parser result is a **limited runtime startup check**, not a
substitute for installed driving, frame-rate, career/save, visual review,
input or Android QA. CI uses synthetic log text only in **negative** parser
tests, and never attaches it to a release.

### Android touch ownership

`ALatebrakeRaceGameMode` now uses
`ALatebrakePlayerController`. On Android only, it enables touch-event
routing and activates game-only input with Unreal's stock virtual joystick
disabled; the car pawn's two-finger steering/pedal layout remains the source
of input and Canvas visualization. Verify both fingers, menus, pause,
focus loss and hardware orientation on a **physical Android device**. The
source-only CI cannot certify any of these behaviors.

## Release archive and human-QA integrity gate (unreleased)

`Build-PlayableCandidate.ps1` now runs
`Test-CandidateManifest.ps1` **after** generating the source/engine/map
provenance manifest. The independent validator checks **every** archived
file's relative path, byte count and SHA256; rejects archive omissions,
additional unlisted files, duplicate names, escaping paths, dirty tracked
source trees, unknown platforms, and mismatched engine/source metadata.
Ordinary hosted CI uses disposable **text-only fixtures**, never real game
binaries, to test this behavior.

After the **actual UAT package** has been installed/playtested, the
`Verify-PlatformPackage.ps1` QA gate requires all three inputs:
`-PackagePath`, `-ManifestJson`, and `-EvidenceJson`.
The exact EXE/APK must be inside the manifest archive. The QA evidence's
`build_commit` must match the manifest's `source_commit`, AND its
`candidate_manifest_sha256` must match the SHA256 of the verified
`Latebrake-candidate-manifest.json` file. Two candidate archives from
the same source may cook into different binaries; a source commit alone
does not identify the package humans installed. Complete
the separate win64/android QA templates with **true** installed gameplay
observations, the full 40-character source commit, and exact candidate SHA256. Every entry
under the template's `checks` object must also be explicitly `true`
after a real successful test. A missing, false, or not-yet-tested
driving, save/load, mobile focus, performance or other required check
blocks release; `installed_and_playtested` alone is insufficient.

For a combined, manually reviewed Win64 + Android release, run:

```powershell
pwsh ./scripts/Verify-ReleaseArtifacts.ps1 `
  -Win64Directory 'D:\LatebrakeCandidates\Win64' `
  -AndroidApk 'D:\LatebrakeCandidates\Android\YourActualBuild.apk' `
  -Win64ManifestJson 'D:\LatebrakeCandidates\Win64\Latebrake-candidate-manifest.json' `
  -AndroidManifestJson 'D:\LatebrakeCandidates\Android\Latebrake-candidate-manifest.json' `
  -Win64EvidenceJson 'D:\LatebrakeCandidates\win64-qa.json' `
  -AndroidEvidenceJson 'D:\LatebrakeCandidates\android-qa.json'
```

It checks each platform's real game package **and** QA record, and also
requires both archives to originate at the exact same Git commit and Unreal
Engine version. It does not automatically publish a release, prove that QA
claims are truthful or replace installed gameplay observations. The older
single `test-evidence.example.json` is not a release-qualification input.
