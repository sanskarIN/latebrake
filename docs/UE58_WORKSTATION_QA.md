# Licensed Unreal workstation candidate workflow

LateBrake has a **manual** GitHub Actions workflow named
**Unreal 5.8 game packaging (manual authorized runner ONLY)** in
`.github/workflows/build-unreal-candidate.yml`. This is the path
toward real engine builds, not evidence that one has run or passed.

## Read-only workstation prerequisite preflight

Before spending time on a real Engine Editor/UHT build, inspect the installed
workstation with PowerShell 7:

```powershell
pwsh ./scripts/Test-UnrealWorkstationPrerequisites.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Platform Win64
```

For an Android candidate substitute `-Platform Android`. This check performs
no compilation, cooking, map creation, downloads or filesystem writes. It
checks that the project targets UE 5.8, the specified engine has its
`Build.version`, `Build.bat`, `RunUAT.bat` and `UnrealEditor.exe`,
and the checkout has a Git commit. Missing essentials fail early.
It **does not authenticate engine binaries**, validate Visual Studio/MSVC,
the Windows SDK, Android NDK/JDK or an Unreal license; actual Build.bat,
Editor, UAT and installed hardware QA remain mandatory. Android environment
hints are informational only because SDK settings can live inside Unreal.
Synthetic source-only negative fixtures are run by CI and never masquerade as an
actual licensed Unreal build.

Both `Build-PlayableCandidate.ps1` and the private Windows playtest
launcher invoke this preflight before build or candidate output creation.

## Requirements

1. A privately controlled Windows machine with a legally authorized
   Unreal Engine **5.8** installation, C++/Windows SDK toolchain and
   whichever Android SDK/NDK/JDK is needed for Android.
2. A secured self-hosted GitHub Actions runner with labels
   `self-hosted`, `Windows`, `X64`, `unreal-5-8`. Do not put this
   runner on an internet-exposed or shared machine, and restrict who
   can administer repository workflows and main branch.
3. The runner service environment variable `LATEBRAKE_UNREAL_ROOT`
   points to the authorized UE 5.8 folder; the engine's `Build.bat`
   and `Build.version` must both exist. The workflow refuses an
   unrecognized major/minor version.
4. Trigger the workflow **manually on main**, selecting `Win64` or
   `Android`. No push, pull_request, fork or user-supplied source
   reference is allowed to execute in the licensed runner. The job
   checks out `github.sha` and disables persisted checkout credentials.
5. The platform and generated candidate output path are carried through
   GitHub Actions **environment variables**, never interpolated into
   PowerShell code. Each run uses a new candidate path within the runner temporary
   directory. Build-PlayableCandidate invokes the project's genuine
   Editor/UHT compilation, real map authoring, UAT cook, provenance
   hash/manifest generation and post-build candidate consistency
   scripts. Missing tools/map or stale output fail the job closed.
6. **If** real packaging succeeds, the artifact is uploaded to the
   workflow run for up to three days as `UNVERIFIED` candidate data
   for subsequent human-device QA; it is never attached to a GitHub
   Release, classified as stable, or called a tested game.
7. Perform **actual** Windows/Android installs, on-device driving,
   ordered gates, AI/recovery, save/reload, controls, gameplay and
   performance testing. Complete the separate platform QA evidence
   templates and run `Verify-PlatformPackage.ps1` and
   `Verify-ReleaseArtifacts.ps1` against *exact* candidate hashes.

## Trust boundaries

- The self-hosted runner is under the maintainer's control and must
  be isolated from untrusted PR execution; GitHub-hosted runners
  do **not** contain licensed Unreal Engine and are not used here.
- The workflow requires manual dispatch from `main`, verifies an
  exact source SHA, and runs with read-only GitHub contents access.
- License and SDK installation remain the runner owner's responsibility.
- The private Actions artifact is a build *candidate*, not a published
  executable pre-release. It is explicitly not uploaded as a game
  release asset or declared playable without installed QA.
- The job cleans its unique temporary candidate directory after the
  Actions artifact handoff, without deleting other runner directories.
  Before cleanup it refuses both a reparse-point/junction **candidate
  root** and **any nested junction/symlink**, walking direct directory
  entries without following links. This matters even when an editor/cook
  step fails before the manifest writer gets to scan package contents.
  Unsafe cleanup aborts and leaves the candidate for authorized manual
  inspection; never force-delete redirected data.
- This is defense-in-depth for a trusted, access-controlled workstation,
  not a guarantee against filesystem races or a substitute for restricting
  who can write to the licensed runner's workspace/temp directories.
- Source contracts cannot prove that this workflow has executed in
  Unreal or that the machine meets the required platform settings.

Track actual evidence in [playable release qualification #148](
https://github.com/sanskarIN/latebrake/issues/148).

**As of this document, actual UE 5.8 compilation, owned cooked map,
Win64 EXE, Android APK/AAB and installed hardware QA are unverified.**
