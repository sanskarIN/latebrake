# LateBrake: native Unreal PR merge-readiness evidence

This checklist exists so **protected main receives only reviewed changes that
meet their actual verification requirements**. It is not a gate waiver, source
release, CI replacement, new engine runner, or a substitute for the user's
installed Windows playtest.

As of 12 November 2026, the old qualification issue #148 has been **deleted**.
It cannot receive evidence or authorize a merge. Use the current review PR,
the actual workflow run URL, and the source-pinned artifact manifest instead.
The source-only tests and the protected branch ruleset remain relevant, but
neither proves that a licensed Unreal Editor or cooked binary worked.

## First classify the change

- **Documentation-only:** check the diff contains no production code, native
  asset, workflow trust-boundary or release-claim changes. Verify Markdown
  links, factual statements, exact PR head, and ordinary docs/source CI. The
  owner may merge through a review-ready PR without requiring a new game cook.
- **Source-only test / portable library:** verify the tested C++20 target,
  portable Windows/Linux/macOS CI and the lack of new Unreal runtime callers.
  Never advertise simulated integration as installed-game proof.
- **Unreal runtime, collision, camera, vehicle, AI, rendering, race scoring,
  SaveGame, startup, generated .umap or shipping material:** require the
  complete real UE5.8 and installed-candidate evidence below. Keep the draft
  closed to merging until this evidence exists. A passing hosted C++ matrix
  cannot waive this requirement.
- **Release publisher / automation trust boundary:** independently review
  permissions, input validation, exact-ref binding, secrets exposure, runner
  trust, source provenance and artifact integrity before any merge.

## Required steps for native gameplay PRs

1. **Pin the source:** record the candidate PR number, target branch
   (\`main\` for owner-trusted validation), full 40-character head SHA and
   checkout tree. Do not accept tests from a different commit or a recycled
   receipt.
2. **Hosted source gate:** obtain the completed passing exact-head CI matrix;
   include portable C++20 unit tests on Ubuntu/macOS/Windows, Python/editor
   source-binding tests, workflow guards and PowerShell syntax checks. Note
   failed or cancelled jobs explicitly.
3. **Real engine runner:** on the owner-controlled Windows x64 runner, verify
   labels \`self-hosted\`, \`Windows\`, \`X64\`, \`unreal-5-8\`, and an authorized
   UE 5.8 installation. Check \`LATEBRAKE_UNREAL_ROOT\`, version metadata,
   UBT/UHT/Editor tools, MSVC, SDK and disk capacity. A queued job with zero
   steps is **not** a pass. See [runner queue QA](UNREAL_RUNNER_QUEUE_QA.md).
4. **Real Editor/UHT:** dispatch the main-targeted trusted
   [manual PR validation workflow](https://github.com/sanskarIN/latebrake/actions/workflows/validate-draft-unreal-pr.yml)
   for the exact PR/head and \`Editor\` mode. Require real native compilation,
   authentic UHT output, Editor startup, material and map load, and a
   successful completed job. Log the workflow and inspected head identities.
5. **Cook and package:** only after Editor passes, perform the owner-trusted
   \`Win64\` validation with the exact approved source. Require a genuinely
   cooked \`.umap\`, no placeholder .uasset or synthetic compiler receipts,
   real packaged executables and SHA-256-bound provenance. Merely generating
   a zip/manifest is insufficient.
6. **Install and play:** on actual Windows hardware, unpack/install the
   candidate, launch the EXE, drive a timed lap, observe four authoritative
   checkpoints, AI overlap/avoidance, real collision, wheel-ground contact,
   suspension/braking, camera, material lighting, restarts, saves, and exit.
   Test with and without each explicitly non-shipping preview flag. Use
   actual logs, screenshots, hardware notes and genuine runtime hashes.
7. **Six-car experiment:** verify six independently spawned physical Chaos
   actors, five AI identities, valid physical slots, all collision-bearing
   barriers, actual event clock and unmodified reward authority. A six-car
   geometry *plan* or six synthetic sound voices is not enough to claim
   tested six-car gameplay.
8. **Performance and regression:** inspect frame stability at 30/60/120 FPS
   where available, steering feel, stutters, physics race state,
   one-rival normal default, pause/recovery, restart and menu transitions.
   Reproduce any failure and keep the PR draft while the defect is open.
9. **Promote deliberately:** review changes against current \`main\`, ensure
   no overlap from stacked PRs is double-applied, respect protected main
   rules, move the PR to ready-for-review only after qualification, and
   merge by an allowed linear-history method. Do not auto-merge an unfinished
   experiment or bypass repository policy.

## Currently active source candidates

- [Six-car source QA PR #341](https://github.com/sanskarIN/latebrake/pull/341)
  is a native validation *candidate*, not a certified release.
- [Stacked development PR #349](https://github.com/sanskarIN/latebrake/pull/349)
  contains additional opt-in visual, audio and physics experiments. It
  requires an independent native compile/cook and installed-game check
  of its own head after the older validation candidate qualifies.
- A previously dispatched
  [trusted native workflow](https://github.com/sanskarIN/latebrake/actions/runs/42971473466)
  was queued with no assigned runner at the latest recorded check. Look up
  its **current** job status before using it as evidence; no future success
  is implied by this document.

**Invariant:** no open draft or source-only pass authorizes a native gameplay
merge, real Windows installer claim, pre-release tag, or protected-main
override. If any required evidence is unavailable, keep the PR draft and
record the precise missing item instead of silently declaring success.
