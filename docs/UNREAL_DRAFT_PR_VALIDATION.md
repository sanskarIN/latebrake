# Real UE 5.8 validation of an unmerged draft pull request

**This workflow cannot run Unreal by itself.** It requires an owner-managed,
isolated Windows self-hosted GitHub Actions runner with a genuine licensed
UE **5.8** installation, MSVC/Windows SDK, PowerShell 7, sufficient disk
space and the runner label `unreal-5-8`. It does not run on public hosted
CI and does not certify a playable game.

**If the manual job is queued before any runner step begins**, use the
[read-only self-hosted runner queue diagnostics](UNREAL_RUNNER_QUEUE_QA.md)
first. An unstarted run is not a failed Unreal compile; it has not yet
validated the PR number or source commit.

## Why this is needed

The original `compile-unreal-editor.yml` and
`build-unreal-candidate.yml` workflows intentionally run against
`main` only. A major physics candidate such as **PR #295** must not
merge before real UE compilation and driving tests. A main-only build
gate otherwise cannot compile the draft candidate on an engine machine.

`.github/workflows/validate-draft-unreal-pr.yml` closes that gap
without weakening the existing protected main workflows. It is always
**manually dispatched from the main branch**, then separately queries
GitHub's live API for the exact reviewed PR head before checking out
or compiling any feature-branch code.

## Manual operator procedure

Only the GitHub owner may dispatch this as a trusted local-engine
operation. Do not enable automatic `pull_request` execution on a
self-hosted runner, or accept source from outside this repository.

1. Set up an isolated, owner-controlled Windows self-hosted runner with
   labels `self-hosted`, `Windows`, `X64`, `unreal-5-8`. Restrict
   its OS privileges, network access, sensitive files and available
   tokens. Supply `LATEBRAKE_UNREAL_ROOT` as a runner environment
   variable containing the absolute UE 5.8 installation directory.
   Do not configure publishing credentials or unrelated user data.
2. Review the exact diff and current head commit SHA of
   [PR #295](https://github.com/sanskarIN/latebrake/pull/295).
   In particular review scripts called by the engine workflow; an
   approved PR can execute arbitrary code on the local engine runner.
3. Open GitHub **Actions → Manual trusted Unreal 5.8 PR validation
   (NO release) → Run workflow**, select **main** (not the feature
   branch). Enter the open PR number, exact reviewed **40-character
   head SHA**, and either `Editor` or `Win64`.
4. Start with `Editor`. The gate fetches live PR metadata before
   checkout and rejects a closed PR, a fork, a non-main base,
   a non-owner author, or an out-of-date SHA. It rechecks the PR
   after checkout and invokes the existing honest
   `scripts/Compile-UnrealEditor.ps1` pipeline.
5. If Editor/UHT succeeds on the exact revision, repeat the trusted
   dispatch with `Win64`. The genuine UE5.8 Editor/map/UAT pipeline
   must compile, cook, and generate a source- and engine-hashed
   **UNVERIFIED Win64 candidate artifact**. It is retained for only
   **3 days**, never published as a GitHub Release.
6. Download the complete cooked archive onto a Windows test machine,
   verify its manifest, then follow
   [Windows personal playtesting](WINDOWS_PERSONAL_PLAYTEST.md)
   and [sprung-chassis QA](https://github.com/sanskarIN/latebrake/blob/feat/chaos-four-corner-suspension/docs/SPRUNG_CHASSIS_QA.md). Capture true hardware
   FPS, car stability, steering, braking, AI, body/wheel attitude,
   starts, focus/pause and recovery. Inspect
   `LATEBRAKE_SUSPENSION v=1` debug logs from an actual Development
   build. This is a separate human acceptance gate.

**Do not merge the PR just because Editor/UHT passes.** Win64 cooking,
installed driving, real collision/suspension stability and issue
[native merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md) remain
mandatory. The separate ~200 KB native 2D preview is not the Unreal
game package.

## Security boundaries

- GitHub Actions job runs only from a **manually dispatched main** workflow.
  No `push`, `pull_request`, or `pull_request_target` trigger exists.
- The API authorization step runs before checking out PR source.
  Only an **open owner-created PR** with head repository
  `sanskarIN/latebrake`, base `main`, and exact reviewed current SHA
  is eligible. Fork heads and stale SHAs are rejected twice.
- The workflow token has only `contents: read` and
  `pull-requests: read`, is used only during preflight, and
  `actions/checkout` uses `persist-credentials: false`.
- A genuinely trusted owner PR can still contain malicious build
  scripts or C++ source. The automatic checks cannot replace
  **manual source review and an isolated runner**.
- Candidate output is created under `RUNNER_TEMP` with a unique
  run/attempt name, never reused, and temporary cleanup refuses
  junctions/symlinks. The artifact has a short retention period.
- No installer, public pre-release, GitHub Release, store upload,
  or stable release is created by this workflow.

This documentation describes an **available manual verification
mechanism**, not a claim that an Unreal 5.8 machine has run it or that
PR #295 has passed real hardware QA.
