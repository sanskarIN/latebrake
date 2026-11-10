# LateBrake release milestones — owner setup and PR routing

**Milestones proposed, NOT created by this file.** Live GitHub verification
on **2026-11-10** found **zero existing milestones** and 16 open PRs (including this proposed audit PR and live-grid safety PR #337) plus
four non-PR open issues. Do not mark anything assigned or complete until
GitHub's real milestone and artifact evidence confirms it.

The machine-readable source of this planning snapshot is
`scripts/releases/latebrake-milestones.json`, audited by
`scripts/AuditMilestonePlan.py`. The audit is **GET-only**; it cannot
create milestones, assign an issue, merge a PR, publish a release or
produce a game executable. Milestone membership is administrative
tracking, never a substitute for real engine, Windows/Android hardware,
track, physics and packaging tests.

## Create these three milestones on GitHub (no invented deadline)

Open **Issues → Milestones → New milestone** in the
[LateBrake repository](https://github.com/sanskarIN/latebrake).
Copy each title and description below. Leave its due date unset; the
release sequence is determined by evidence and genuinely working source,
not a made-up calendar deadline.

### 1. v0.5.98 — Source-Only Pre-release

**Description to paste:**

> Publish the pinned source-only LateBrake v0.5.98 Independent Wheel
> Braking and Rival Capacity preview with ZERO attached executable assets.
> Draft PR #295 remains at exact SHA
> `bc0d5606d4c415f7603cc97f5ba6c90f59fa9c37` and is NOT merged
> merely to publish the source archive. PR #334 provides a read-only
> milestone-independent publisher audit. The owner-only separate release
> workflow must recheck that source, notes, previous v0.5.97 immutable
> zero-asset tag, strict C++20 CTests on Windows/Ubuntu/macOS, Python
> contracts, Node guards, synthetic packaging and docs. Publish no Win64
> installer/APK. Real UE5.8/UHT/Game builds and installed gameplay remain
> outside this SOURCE-ONLY release; never mark it playable.

**Assign PRs:** [#295](https://github.com/sanskarIN/latebrake/pull/295),
[#334](https://github.com/sanskarIN/latebrake/pull/334).

**Exit:** v0.5.98 tag/release genuinely exists at its original frozen
source SHA, satisfies exactly the source-only plan and has **zero
attached executable**/game assets. The previous tag is unchanged.
Source ZIP/tarball downloads are not installers.

### 2. UE5.8 — Playable Evidence Gate

**Description to paste:**

> Prove a real licensed Unreal Engine 5.8 Editor/UHT + Game compilation,
> an original owned circuit .umap, genuine collision-checked map cook,
> manifest/provenance and an installed Win64 executable. Review the
> entire main-targeted integration PR #335, then run the existing trusted,
> owner-dispatched UE5.8 Editor → Win64 workflow with the exact PR head
> SHA on an isolated self-hosted Windows runner. Record actual engine,
> source, map and package hashes, logs, video and installed driving at
> 30/60/120 FPS, plus valid player/rival race, recovery, scoring, pause,
> reset, camera and six physical-car behavior. Issue #148 remains OPEN
> until its actual complete platform acceptance requirements (including
> separate Android obligations) are proven. Source-only CI is NOT
> qualifying gameplay evidence. Do not merge PR #335 solely to start a
> QA build; do not publish an installer because a build step passed.

**Assign:** [PR #335](https://github.com/sanskarIN/latebrake/pull/335)
and [issue #148](https://github.com/sanskarIN/latebrake/issues/148).

**Exit:** authenticated real UE5.8 engine/cooked level and installed
Win64 gameplay evidence. **Full issue #148 closure has additional
Android requirements and must not be inferred from Win64-only evidence.**

### 3. v0.6.0 — Racing Alpha & Source Quality

**Description to paste:**

> Build a genuinely high-quality original 3D racing alpha: physics and
> cornering stability, responsive multi-car physical AI, safe race start,
> checkpoint integrity, original track layout and visuals, track-limit
> enforcement, real camera transitions and effects, meaningful garage
> and racing experience. Review experimental feature PRs only after real
> UE5.8/Win64 validation; tests, docs and fake LOC never qualify gameplay.
> Owner's source target: at least 200,000 genuinely useful nonblank,
> noncomment production C++ lines within ONE validated source tree
> BEFORE v0.6.0, excluding tests/docs/vendor/generated/duplicates and
> avoiding sums of overlapping feature branches. Keep v1.0.0 stable
> scope separate; only publish a candid, evidence-gated alpha pre-release.

**Assign these component PRs:** [#297](https://github.com/sanskarIN/latebrake/pull/297),
[#313](https://github.com/sanskarIN/latebrake/pull/313),
[#316](https://github.com/sanskarIN/latebrake/pull/316),
[#318](https://github.com/sanskarIN/latebrake/pull/318),
[#327](https://github.com/sanskarIN/latebrake/pull/327),
[#328](https://github.com/sanskarIN/latebrake/pull/328),
[#329](https://github.com/sanskarIN/latebrake/pull/329),
[#330](https://github.com/sanskarIN/latebrake/pull/330),
[#331](https://github.com/sanskarIN/latebrake/pull/331),
[#332](https://github.com/sanskarIN/latebrake/pull/332),
[#333](https://github.com/sanskarIN/latebrake/pull/333),
[#337](https://github.com/sanskarIN/latebrake/pull/337).

**Exit:** real engine and installed driving qualification, meaningful
source features, demonstrably better game feel/graphics, and independently
measured source volume. The 200,000 line target is still FAR AWAY;
passing portable source tests does not meet it.

## Avoid misleading PR milestones and duplicate merges

GitHub assigns at most one milestone per PR/issue. The same source
commit may appear in stacked component PRs AND an aggregate integration
PR. **Do not add source line counts between these PRs**: their changes
overlap, and doing so manufactures progress toward 200,000 lines.

The ordered six-car branch chain is:

1. #329 proposed original start grid → #330 actor identity ledger
2. #330 → #331 four extra physical AI actors and observed fleet traffic
3. #331 → #332 paid player recovery and verified physical pre-GO grid
4. #332 → #333 faulted physical spawn remains blocked, career/solo independent
5. #335 **already aggregates #329–#333** onto a main-targeted QA
   candidate so the owner's strict Unreal workflow can review the entire
   source tree. Don't blindly merge the aggregate AND independently
   merge each component PR without a controlled integration strategy.

PR #337 is separately stacked on #333 and adds real pre-GO physical
slot pose/speed/overlap checks. It is NOT included in the frozen QA
aggregation #335; test an updated approved integration SHA before
claiming engine validation covers #337.

PR #327 is stacked on the still-open physics draft #295. It should
not be marked as an independently qualified retail effect or merged
without its actual physics dependency.

## Keep these issues outside the active three milestones

- [#13](https://github.com/sanskarIN/latebrake/issues/13):
  Android device/touch/hardware qualifications are still not completed.
  Create a separate **Android verification** milestone when real licensed
  engine cook/device work starts; Windows-only alpha results cannot
  certify Android.
- [#233](https://github.com/sanskarIN/latebrake/issues/233):
  multiplayer/authoritative sessions/private invites are future work.
  Do not promise matchmaking until the single-player installed game
  and network safety/security requirements are real.
- [#234](https://github.com/sanskarIN/latebrake/issues/234):
  Windows native **2D preview** QA is a separate optional experiment,
  not proof of a playable Unreal racing game.
- [PR #336](https://github.com/sanskarIN/latebrake/pull/336):
  this milestone-plan/audit tooling is administrative. It must not
  become an artificial blocker to the source-only v0.5.98 publisher,
  the UE5.8 engine build or game qualification. Review/merge it
  independently when its read-only workflow checks are green.

A future **v1.0.0 Stable** milestone should be created only when
release-quality gameplay, packaging, accessibility, assets, performance,
QA and supported-platform expectations can be scoped confidently.

## Verify coverage, offline or from live GitHub (NO writes)

```bash
python3 scripts/AuditMilestonePlan.py
python3 scripts/AuditMilestonePlan.py --json
# Optional live network audit; public read API can be rate limited:
python3 scripts/AuditMilestonePlan.py --live --json
```

The tool reports missing milestones, unassigned/misassigned PRs,
and new PRs/issues created after the snapshot. It never auto-assigns
GitHub milestones; the owner should confirm names and use each real
PR/issue sidebar **Milestone** selector once those milestones exist.

After this documentation/tooling PR is separately reviewed and merged
to protected `main`, the owner may alternatively use **GitHub Actions →
Audit LateBrake milestone assignments (READ-ONLY) → Run workflow**, with
`main` selected. It runs the same validation using only repository/issue/
pull-request **read** access and produces a human-readable Actions job
summary. It does not create or assign anything; it does not issue a tag,
compile Unreal, build packages or claim release qualification.

**Live milestone audit is advisory.** It does not run UHT, build/cook
Unreal, measure real vehicle dynamics, or certify a release.
For genuine gameplay acceptance, keep
[issue #148](https://github.com/sanskarIN/latebrake/issues/148)
open until the real evidence has been attached by the operator.
