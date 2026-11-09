# v0.5.98 source-only release preparation

**Prepared, not published.** Merging this preparation does not create a tag,
publish a release or produce any executable. The latest published source
preview remains v0.5.97 until a successful publication is read back.

| Item | Pinned value |
| --- | --- |
| Candidate source | `bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37` |
| Source PR | [Draft #295](https://github.com/sanskarIN/latebrake/pull/295) |
| Previous source tag | v0.5.97 |
| Previous source commit | `65ef6f2f192081b64f8256867403f40882cca462` |
| Candidate notes | [v0.5.98 source notes](releases/v0.5.98.md) |
| Workflow | `.github/workflows/prepare-v0598-source.yml` |
| Artifact policy | Zero executable binaries or attached game assets |

The candidate adds independent contact-phase wheel braking, delivered-frame
rotor/visual receipts and a conservative capacity-aware rival corner
forecast. Its road-point samples are collected during coasting as well as
braking. [Current-source CI](https://github.com/sanskarIN/latebrake/actions/runs/42630895071)
must complete successfully; older passing source revisions are historical.

## Manual publication

The repository owner can open [the prepared workflow](https://github.com/sanskarIN/latebrake/actions/workflows/prepare-v0598-source.yml),
choose **Run workflow**, and select **main**. The reviewed main workflow
independently qualifies the exact candidate before any write-scoped job.
Only the owner on protected main may publish. Ordinary merges, PR events,
feature-branch pushes and schedules cannot publish this preview.

The preflight and final readback authenticate protected main, the exact
same-repository draft head, source-note bytes, predecessor identity and
zero-assets policy. The candidate must remain at the source pin until
publication; a newer reviewed candidate requires a new pin and fresh gates.
Existing tags are never rewritten. A mismatched existing release, changed
main/head or asset drift fails the workflow instead of silently repairing it.

Read-only jobs build every strict portable CTest on Windows, Ubuntu and
macOS and require the named wheel/visual/capability/rival-approach targets.
They run all source/editor fixtures, production-source counting, PowerShell
syntax/synthetic packaging checks and rendered docs verification. The write
job executes only trusted main publisher code after all required jobs pass;
it never runs candidate scripts with repository write permission.

## Current limits

The candidate contains **20,219 nonblank noncomment production C++ lines**,
with tests, docs, workflows and generated/vendor material excluded. Both
the interim 100,000-line target and v0.6.0 200,000-line target remain unmet.

There is no verified UE5.8/UHT build, original cooked road, installed Win64
game, Android APK/AAB or hardware performance evidence. Modeled pressure,
rotor speed and estimated capacity are not measured hydraulics or certified
ABS. The physics PR stays draft and unmerged until the real-engine and
installed-game gates in its QA guides pass. A source-only release cannot
clear [playable gate #148](https://github.com/sanskarIN/latebrake/issues/148).

After publication, read back the exact tag, target commit, release-note
bytes and empty asset list before changing public latest-release labels.
Keep this guide as a historical publication record, with the original pin
and workflow, rather than retargeting an already published source tag.
