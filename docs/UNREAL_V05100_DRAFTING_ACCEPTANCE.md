# v0.5.100 — physical drafting native acceptance (NOT YET VERIFIED)

**This is a test plan, not a test result, an authorization to publish,
or a claim that a playable game exists.** As prepared, no genuine UE5.8
Editor/UHT, cooked Win64 package or installed hands-on drafting evidence
has been provided for [draft gameplay PR #377](https://github.com/sanskarIN/latebrake/pull/377).
The latest published source-only pre-release is
[v0.5.99](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.99).

## 1. Freeze and authenticate a live PR head

- Review the exact current diff and full head SHA of PR #377.
- Confirm the head repository is `sanskarIN/latebrake`, its base is `main`
  and the owner-created PR remains open.
- In [Manual trusted Unreal 5.8 PR validation (NO release)](https://github.com/sanskarIN/latebrake/actions/workflows/validate-draft-unreal-pr.yml)
  select the **main branch** and supply `pr_number=377`,
  `expected_head_sha=<the live 40-character PR head>`, `validation=Editor`.
- Run this only using an owner-controlled isolated Windows runner with all
  labels `self-hosted`, `Windows`, `X64`, `unreal-5-8` and a real
  authorized UE5.8 installation. A queued job has not verified anything.
- On genuine Editor/UHT success, dispatch a **second** manual run with
  `validation=Win64` for the **same exact head SHA**. Do not substitute
  hosted portable C++20 for either native operation.
- Retain the Editor/Win64 run links, completed status, source HEAD,
  UE Build.version SHA-256, real cooked `.umap` SHA-256 and package manifest
  SHA-256. The Win64 artifact is `UNVERIFIED` until a real install/playtest.
- If the PR head changes, **invalidate the old native evidence** and rerun
  Editor then Win64. Never report old-source evidence for new source.

## 2. Verify on a real installed Windows candidate

Use [Windows personal playtesting](WINDOWS_PERSONAL_PLAYTEST.md),
[physical slipstream specification](SLIPSTREAM_DRAFT_QA.md) and
[native merge readiness](UNREAL_NATIVE_MERGE_READINESS.md).
Record actual running FPS, 3D car positions/velocities and the **applied**
expanded-HUD `SLIPSTREAM` value, not an inferred stand-alone model outcome.

| Real-world scenario | Required observation |
| --- | --- |
| Legitimate one-rival trailing | Draft only after **0.18 s uninterrupted** behind the actual moving rival at 7–38 m forward gap, less than 3.8 m lateral and within 2.3 m elevation |
| Straight + left/right bends | Forward chassis motion and observed heading/gap remain physically continuous without unexpected 0% interruption |
| Real road crest and descent | Genuine vertical Chaos velocity is honored; normal road elevation does not trigger phantom teleport detection |
| Actual collision occlusion | A solid Visibility-blocking barrier between cars immediately yields 0%; clearing it requires a fresh 0.18 s |
| Rival replaced or recovered | A different live actor identity cannot inherit the previous opponent's boost; no stale sightline/velocity |
| Player recovery / simultaneous rehome | Unexplained player/leader absolute-world relocation cannot borrow earlier drafting credit, even if relative gap is unchanged |
| Sideways slide, spin, airborne | No boost when either car lacks three real road-contact probes or exceeds lateral-slip policy |
| Reverse, braking, race end, pause, focus loss | Draft immediately disabled; no credit carried into a resumed session |
| Nitro at the same time | Extra drafting force at most **6%**, total additional motor force at most **40%**, nitro retains priority; verify actual applied HUD display |
| Unchanged race authority | No new lap, checkpoint, leaderboard, credit or SaveGame entries caused by the draft policy alone |

Repeat the eligible and interruption scenarios at measured **30, 60 and
120 FPS** where the test hardware supports them. Compare two consecutive
uninterrupted confirmation windows, including starting without draft and
reacquiring after a barrier/recovery. Include at least two real garage
configurations and true actor/physics debug evidence rather than screenshots
of a noninteractive menu. Treat a failure as a blocked release, not a waived
case.

## 3. Evidence record — fill only after witnessing the actual run

| Field | Required recorded evidence |
| --- | --- |
| PR head SHA and main base | *Not tested / to be recorded* |
| Editor workflow URL and conclusion | *Not tested / to be recorded* |
| Win64 workflow URL and conclusion | *Not tested / to be recorded* |
| Engine version and `Build.version` SHA-256 | *Not tested / to be recorded* |
| Original cooked circuit `.umap` SHA-256 | *Not tested / to be recorded* |
| Candidate manifest SHA-256 | *Not tested / to be recorded* |
| Installed PC and actual 30/60/120 FPS | *Not tested / to be recorded* |
| Physical obstacle, Z crest/descent and recovery traces | *Not tested / to be recorded* |
| Real HUD boost and 40% motor budget observations | *Not tested / to be recorded* |
| Owner acceptance decision and date | *Not tested / to be recorded* |

**Unfilled fields are explicitly NOT evidence.** Do not create a public
Win64/Android executable or describe source-only v0.5.100 as an installed
game on the basis of this checklist. A source-only pre-release also needs
the actual merged source SHA, tested release publisher and the owner's
**explicit approval of that SHA**. This document performs none of those
actions.
