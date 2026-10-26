# Licensed Unreal workstation candidate workflow

LateBrake has a **manual** GitHub Actions workflow named
**Unreal 5.8 game packaging (manual authorized runner ONLY)** in
`.github/workflows/build-unreal-candidate.yml`. This is the path
toward real engine builds, not evidence that one has run or passed.

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
  Before cleanup it rejects output-directory junctions/reparse-point
  links rather than recursively traversing a possibly redirected path.
- Source contracts cannot prove that this workflow has executed in
  Unreal or that the machine meets the required platform settings.

Track actual evidence in [playable release qualification #148](
https://github.com/sanskarIN/latebrake/issues/148).

**As of this document, actual UE 5.8 compilation, owned cooked map,
Win64 EXE, Android APK/AAB and installed hardware QA are unverified.**
