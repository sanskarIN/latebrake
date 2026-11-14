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
`main` only. A native gameplay candidate such as **PR #377** (the v0.5.100
physical drafting change) must not merge before real UE compilation and
driving tests. The separate suspension experiment PR #295 has its own
native qualification obligation. A main-only build
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
2. Review the exact diff and **live current head** of the PR being tested.
   For the proposed v0.5.100 drafting change, that is
   [PR #377](https://github.com/sanskarIN/latebrake/pull/377);
   for the unrelated suspension experiment, [PR #295](https://github.com/sanskarIN/latebrake/pull/295).
   Copy the complete head SHA directly from the PR at dispatch time.
   Review all scripts called by the engine workflow: approved PR source
   executes native code on the owner-managed runner.
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
   [Windows personal playtesting](WINDOWS_PERSONAL_PLAYTEST.md).
   For PR #377 specifically, follow the
   [v0.5.100 physical drafting acceptance matrix](UNREAL_V05100_DRAFTING_ACCEPTANCE.md)
   with authentic observed gaps, vehicle Z motion, obstacle traces and
   the actual expanded-HUD applied boost, across 30/60/120 FPS.
   For PR #295 use its separate sprung-chassis qualification instead.
   Capture real runtime logs, hardware and manifest hashes; never
   substitute portable C++20 tests for installed gameplay.

### Optional local diagnostics when the trusted runner is unavailable

The [Windows personal playtest guide](WINDOWS_PERSONAL_PLAYTEST.md) now
documents fetching the **live head of draft PR #377** in a fresh Windows
clone, verifying its exact reviewed SHA and invoking the existing real
UE5.8 Editor/Win64 build commands. This can identify compiler/cook issues
early. It **does not replace** the trusted manual Editor-then-Win64
workflow, installed-game acceptance or exact-main source release approval.
No GitHub pre-release is created by local diagnostics.

**Do not merge the PR just because Editor/UHT passes.** Win64 cooking,
installed driving, real collision/suspension stability and issue
[native merge-readiness checklist](UNREAL_NATIVE_MERGE_READINESS.md) remain
mandatory. The separate ~200 KB native 2D preview is not the Unreal
game package.

## Current v0.5.100 manual dispatch (no release)

The owner-controlled main-branch workflow has three inputs. Its values
must be filled from **the live open PR**, not from an old release note:

| Workflow input | Value for the current drafting candidate |
| --- | --- |
| `pr_number` | `377` |
| `expected_head_sha` | Full **current** 40-character [PR #377](https://github.com/sanskarIN/latebrake/pull/377) head SHA, rechecked immediately before dispatch |
| `validation` | `Editor` first; after success dispatch `Win64` at that same SHA |

Select the **main branch** in the workflow UI, not the PR branch.
An Editor-only success does not certify the cooked map, installed game,
nor physical 3D slipstream. An uploaded Win64 artifact remains
**UNVERIFIED**, never a public pre-release or installer.
A subsequent commit to PR #377 invalidates earlier exact-head native
results and requires the native validation sequence again.

See [PR #377 real-driving acceptance](UNREAL_V05100_DRAFTING_ACCEPTANCE.md)
and the [native merge checklist](UNREAL_NATIVE_MERGE_READINESS.md).
The explicit release source SHA authorization and source-only publication
policy are separate from this manual game-candidate workflow.

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
any specific draft PR has passed real hardware QA.
