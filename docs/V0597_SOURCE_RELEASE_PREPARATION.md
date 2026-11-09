# v0.5.97 source-only release preparation

**Prepared, not published by merging this workflow. No playable Windows EXE.**
The latest published source snapshot remains v0.5.96 until a real GitHub
release object for v0.5.97 has been created and independently read back.

## Pinned candidate

- Source: `65ef6f2f192081b64f8256867403f40882cca462`.
- Unmerged physics draft: [PR #295](https://github.com/sanskarIN/latebrake/pull/295).
- Scope: load-distributed braking at real road contacts, measured road
  tangents on slopes/banks, passive contact allocation, rear-only handbrake
  and full 3D brake-magnitude accounting in the shared grip budget.
- Prepared [release notes](releases/v0.5.97.md) are byte-identical in the
  publisher and candidate. They do not qualify Unreal or installed gameplay.
- Production quantity: 190 C++ files, 23,506 physical / 21,880 nonblank /
  **19,124 nonblank noncomment lines**. The 200,000-line v0.6.0 target remains
  unmet. Source-only v0.5.97 is independent of that later quantity gate.

## Manual publication

After this preparation is merged and reviewed, open GitHub Actions →
**Publish prepared v0.5.97 SOURCE-ONLY prerelease (manual)**, select **main**,
and run `.github/workflows/prepare-v0597-source.yml` as the repository owner.
This action creates a public source-only pre-release after all its gates
pass. It does not build or publish game executables or merge the physics PR.
No token or credential should be pasted into chat or the workflow inputs.

The workflow authenticates protected main, the exact open owner-created
same-repository draft, matching source-note bytes and immutable v0.5.96
predecessor identity. It rebuilds all portable C++20 tests on three operating
systems, requires actual road-brake/suspension targets, runs Python contracts,
prints the production count, checks PowerShell policies and verifies docs.
Only the final protected-main job receives contents-write scope; candidate
code never executes in that job. The publisher's own 44 offline API fixtures
are run in CI and again before qualification starts.

Existing tags/releases must match the exact commit, title, notes,
prerelease flag and zero attached assets. Incorrect existing releases are
rejected before any new tag mutation. Source/main drift is rechecked after
tag creation and after publication. No retagging, release replacement,
asset upload or main merge is performed. An interrupted attempt can leave
the correct source tag without a release; a retry is permitted only while
all original source/main/notes conditions still hold.

If the draft head moves, stop and prepare a newly reviewed candidate pin,
matching notes and fresh checks. Older green results do not qualify it.
Do not silently update the tag or bypass a guard. After actual publication,
read back the tag/target/notes/zero-assets identity before changing public
latest-release labels to v0.5.97.

## Separate playable-game qualification

Run [real Unreal draft validation](UNREAL_DRAFT_PR_VALIDATION.md), Editor/UHT
first and Win64 second, then installed 30/60/120 FPS driving with genuine
engine/map/package hashes and hardware evidence. Slopes, banking, crest
landing, reverse braking, spin contact velocities, rear-only support,
pause/restart and full races remain mandatory. [Gate #148](https://github.com/sanskarIN/latebrake/issues/148)
stays open. The portable regressions do not establish calibrated tire
physics, ABS, production art, a cooked circuit or a playable installer.
