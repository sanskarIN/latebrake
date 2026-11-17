# LateBrake v0.5.100-test.2 — source-only owner publication checklist

**PREPARATION ONLY. This file is not an authorization to publish.**
The next experimental source preview is `v0.5.100-test.2`, NOT the
separately gated official `v0.5.100` and NOT a playable Windows game.

## Audited development boundary

- Last published experimental tag: `v0.5.100-test.1`, immutable direct
  commit `a04db1ccc15ee2e3be6771d7e4a5159dcadb2384`, zero assets.
- Previously verified protected-main post-merge commit:
  `7ec8079f4f79851fbf51a39c892eb5e7df1fa972`. This is a **development
  reference, not automatically the final test.2 tag SHA**, because this
  release preparation must itself be merged and CI-verified first.
- PR #395 was merged, bringing **87,792 unique nonblank, noncomment
  production C++ lines**, up from 71,567. Its source-only research
  modules are not new Unreal player/AI runtime integrations.
- Gameplay PRs #377, #391 and #394 remain excluded. A change to their
  merge state invalidates this exact release-note scope; re-review before
  publishing. A source-only pre-release does not satisfy UE5.8 Editor,
  UHT, original circuit cook, Win64 installer or hands-on driving QA.

## Owner-only source preflight

1. Merge this preparation PR only after its 7 mandatory source checks pass.
   Neither CI success nor this guide is permission to publish.
2. Retrieve **the new current protected-main SHA** after the merge;
   do not reuse the reference above and do not select a PR-head SHA.
3. Check the seven named exact-main jobs for this same SHA: Windows,
   Ubuntu and macOS Portable C++20; Unreal editor-only Python syntax
   (not Unreal runtime); Release PowerShell syntax; Offline
   source-release publication guards; Build and verify docs.
4. In GitHub Actions, choose **Audit v0.5.100-test.2 SOURCE-ONLY
   readiness (READ ONLY)**. Use the `main` branch and provide the
   exact 40-character lowercase commit SHA in `candidate_sha`.
   Only the repository owner may dispatch this job.
5. Require the report to say
   `readyForOwnerManualPublication: true`,
   `sourceChecks: 7`, `writesPerformed: 0`,
   `nativeUnrealQualified: false` and
   `playableWin64Qualified: false`. **Any failure means stop.**
   Its provenance checks also require the old test.1 tag/release to be
   unmodified, PR #395 ancestry in main, new tag absent, and the
   exact candidate's release-note bytes present at
   `docs/releases/v0.5.100-test.2.md`.

## Manual owner publication — only after explicit approval

**Do not run these commands before a separately reviewed source SHA
passes the owner-only read-only gate and the owner deliberately chooses
to publish this source preview.** The code below is a reference for
the human operator, not a workflow to run automatically:

```bash
# In an up-to-date, authenticated owner checkout:
git fetch origin main
git switch main
git pull --ff-only origin main
SHA="$(git rev-parse HEAD)"
test "${#SHA}" -eq 40
# The SHA must equal the reviewed exact-main preflight receipt.
git show "$SHA:docs/releases/v0.5.100-test.2.md" > v0.5.100-test.2.notes.md

# Only after the owner has inspected and confirmed all values:
gh api -X POST repos/sanskarIN/latebrake/git/refs \
  -f ref=refs/tags/v0.5.100-test.2 -f sha="$SHA"
gh api -X POST repos/sanskarIN/latebrake/releases \
  -f tag_name=v0.5.100-test.2 \
  -f target_commitish="$SHA" \
  -f name="LateBrake v0.5.100-test.2 — Portable Racecraft Integration (SOURCE-ONLY Pre-release)" \
  -F body=@v0.5.100-test.2.notes.md \
  -F prerelease=true -F draft=false -f make_latest=false \
  -F generate_release_notes=false
```

Use a **direct lightweight commit tag**, a matching exact-SHA release
`target_commitish`, `prerelease=true`, `draft=false`, the exact
UTF-8 notes already committed at the tagged SHA, and **ZERO attachments**.
Do not upload a Windows executable, installer, APK, AAB, or repackaged
source ZIP as an allegedly playable game. GitHub's automatic archives
are **not installers**.

After the manual action, run the existing
**Audit published source-only GitHub release (READ-ONLY)** with
`tag=v0.5.100-test.2` and verify the immutable tag, release metadata,
tagged note bytes and empty assets list. If a partial tag/release exists,
**do not retry blindly or force-move its tag**; inspect its exact
state and resolve with owner review.

## Separate official v0.5.100 gate

The official v0.5.100 publisher deliberately requires merged PR #377
and additional source gates. This test.2 source snapshot is not a
shortcut around that requirement; no real UE5.8 Editor/Win64 driving
evidence is available for the still-draft native gameplay changes.
Do not rerun or edit the old v0.5.100-test.1 publication.
