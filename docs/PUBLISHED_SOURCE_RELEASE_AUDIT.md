# Independently audit a published source-only release

GitHub Actions has an **Audit published source-only GitHub release
(READ-ONLY)** workflow. It runs when GitHub publishes a release and
also supports an explicit manual audit of an already published
v0.5.N source-only tag.

The audit requires **contents: read** only. It cannot retag, create
a new release, change existing releases, attach assets or rewrite history.

## Why this is necessary

The v0.5.85 publisher's full Windows/Ubuntu/macOS C++20 suite, Python
source contracts, provenance fixtures and documentation all passed.
The GitHub release API then returned HTTP 500 while creating the
release, **after GitHub had already published it successfully**.
The publishing job was marked failed. Independent GitHub API checks
subsequently confirmed the immutable tag, expected source SHA,
matching published notes, draft=false, prerelease=true and zero
attached binaries. A network error alone cannot establish whether
publication succeeded or failed.

## What the audit checks

- A real, published, non-draft GitHub v0.5.N pre-release exists.
- The source tag is a direct 40-hex commit ref, identical to the
  release target commit.
- Release notes load from the tagged commit, NOT the moving main
  branch; the public release description matches those bytes exactly.
- The notes explicitly state SOURCE-ONLY, NOT playable, no Win64 EXE,
  no Android APK/AAB, no installers and zero executable binaries.
- GitHub reports zero uploaded release assets.
- On a release.published event, the event release ID matches the
  independently fetched release identity.

## Manually audit an existing publication

1. Open GitHub Actions in the LateBrake repository.
2. Select **Audit published source-only GitHub release (READ-ONLY)**.
3. Select **Run workflow** on the default branch. Enter the published
   tag, for example v0.5.85.
4. Inspect the API audit job outcome. On failure, review the immutable
   tag, notes, publication state and assets; do NOT retag or destroy
   previously published history to make a check green.

This is **metadata verification only**, not evidence of publisher
test-job success, Unreal Engine 5.8/UHT compilation, a cooked circuit,
a genuine Win64 EXE/Android APK, installed gameplay or hardware QA.
