# v0.5.98 source-only pre-release — read-only readiness QA

**Preparation only. NOT a game build. NOT a publication confirmation.**

The next sequence number is
[v0.5.98](releases/v0.5.98.md) — *Independent Wheel Braking and
Rival Capacity*. Its immutable candidate source is
`bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37` from
[draft PR #295](https://github.com/sanskarIN/latebrake/pull/295).
The pinned predecessor is the published [v0.5.97](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.97)
source commit `65ef6f2f192081b64f8256867403f40882cca462`.
Do not move either SHA or replace draft #295 with unmerged six-car
features from PRs #329–#333.

## Why a separate read-only workflow is useful

The existing `.github/workflows/prepare-v0598-source.yml`
uses a **manual owner dispatch** that requalifies the immutable physics
candidate, then may create the exact source tag/release. It is the
*publisher*, not a dry run. Do not dispatch it just to check whether
pre-release preparation is finished.

The new `.github/workflows/audit-ready-v0598-source.yml` is a
**read-only** manual audit. It runs from reviewed protected `main`
with a contents-read/pull-requests-read GitHub token and never checks
out or runs unmerged PR code. Its only GitHub interaction uses the
already-tested `guard.inspectState` path. It validates:

- owner dispatch from the current protected `main` HEAD;
- same-repo, owner-created, **open draft** PR #295 whose head is exactly
  `bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37`;
- exact match between pinned release notes on `main` and the source
  commit's corresponding note bytes;
- the immutable previous v0.5.97 tag and published **zero-asset**
  source-only pre-release;
- source-only warnings and current v0.5.98 tag/release state
  (none, exact tag-only interruption, or exact already-published);
- offline source-publisher state-machine tests.

**No release tag, GitHub Release, attached asset, artifact upload, engine
compile, source CTest matrix, UHT/UE5.8 build or executable** is produced
by the audit. It is a metadata gate only. The distinct publisher
independently reruns strict Windows/Ubuntu/macOS C++20, Python source
contracts, synthetic packaging/PowerShell, Node guard and rendered-docs
checks at the **frozen candidate source commit**.

## Owner verification procedure

This workflow becomes dispatchable only after its separate
**main-targeted PR** passes automated review and is intentionally merged
into protected `main`. The mere presence of this documentation on a
draft branch does not make that workflow available.

1. Review the main-targeted read-only readiness PR and confirm there are
   no publishing/write permissions, artifact uploads, automatic triggers
   or checkout of a pull-request head.
2. After a trusted merge, open **GitHub → Actions → Audit prepared
   v0.5.98 source prerelease (READ-ONLY) → Run workflow**, select
   **main**, and trigger it as the repository owner.
3. Check the job summary for exact candidate, open draft #295,
   unchanged predecessor, equal note bytes and tag/release state.
   A successful audit is **NOT** source CTest qualification.
4. Inspect the actual frozen PR #295 diff and ensure physical Engine,
   chassis brakes, drivetrain, real Win64 driving and owners' risk
   criteria are understood. The current source-only plan authorizes
   **no playable claim**.
5. Only if you intentionally want to publish a **source-only** next
   pre-release, separately run **Actions → Publish prepared v0.5.98
   SOURCE-ONLY prerelease (manual)** from **main**. That publisher
   reruns all exact-source checks and alone has a narrowly scoped
   write-permission step. A failed check must block publication.
6. After any publication, separately inspect the release/tag API and
   all asset counts. The source archive ZIP/tarball is **not an
   installer**. There should be **ZERO attached binaries** for this
   source-only workflow.

## Independent gameplay/installation gate

A genuine UE5.8/UHT Game and Editor build, original cooked circuit,
real Windows installed EXE, and physical driving at multiple frame
rates have **not** been verified by these source workflows.
[Issue #148](https://github.com/sanskarIN/latebrake/issues/148)
must remain open. Do not merge physics or the experimental six-car
drafts merely because source-only checks pass.

The 200,000 nonblank/noncomment game-source-line milestone toward
v0.6.0 is separate from this source release. Quality, actual engine
execution and installed usability remain more important than padding
the line count.
