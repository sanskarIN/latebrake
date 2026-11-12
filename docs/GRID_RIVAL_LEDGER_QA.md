# Grid rival actor identity and recovery observations

**DRAFT SOURCE-ONLY — NOT a six-car playable game.**

The original six-position grid source from PR #329 defines one actual human,
one actual primary opponent, and four possible future opponents. This stacked
feature supplies independent, fixed-capacity **observation records** for
rival grid slots 1 through 5. Only the existing physical opponent in slot 1
is connected to the current Unreal GameMode. Slots 2–5 remain unbound.

## Behavior and invariants

- Each slot binds to one **real actor token**; tokens cannot simultaneously
  appear in two slots. Only a matching token can release a slot, advance a
  gate, or report a recovery. Rebinding a freed slot starts at zero.
- An observed ordinal must be exactly last + 1. Duplicates, missing gates,
  rewinds, stale actors and equal/backdated/nonfinite timestamps are refused.
- A completed event cannot earn additional observed gates. Session reset
  resets **all five** records before new actors are bound.
- Recovery observations require sequential attempt receipts and record real
  engine-verified success or failure. They never authorize, initiate or
  repair a teleport and cannot clear prior accepted gates.
- The actual primary rival still drives with the original autonomous
  controller and independently verified route crossing. Existing
  `RivalMatchedGateSplits`, race scoring, SaveGame and HUD are unchanged.
- The call from the real rival to the existing primary-rival gate observer
  now checks actor identity before submitting an observation. A future
  additional AI actor cannot impersonate the primary rivalry in the HUD.
- The ledger contains no actor pointers, dynamic memory, new Unreal traces,
  multiplayer networking, rank/placement or vehicle-force commands.

## Portable regression commands

    cmake -S Tests/Standalone -B Tests/Standalone/build
    cmake --build Tests/Standalone/build --parallel
    ctest --test-dir Tests/Standalone/build -R '^GridRivalLedger$' --output-on-failure
    python3 -m unittest discover -s Tests/Editor -p 'GridRivalLedgerBindingTests.py' -v

The portable contract validates six-grid capacity (human slot excluded),
five independently bound tokens, duplicate and skipped gate rejection,
monotonic elapsed times, recovery receipts, finished routes, actor replacement,
and 160 event geometries with 1–5 independently completed opponents.

## Actual UE5.8 and gameplay gates

1. Compile real UE5.8/UHT and both Editor/Game targets at the exact PR head.
2. In an owned cooked original map, observe primary-rival actual forward
   gate crossing. Verify its read-only grid snapshot agrees with the actor's
   independent route while preserving existing split/finish/HUD behavior.
3. Restart and event-switch with live AI, pause/focus interruptions, actor
   destruction/recreation and physical recovery success/failure. Confirm no
   old token can write into a new session and no duplicated observations.
4. Future *separate* multi-car implementation must add real per-rival
   occupancy-aware spawning and recoveries, rival-vs-rival collision avoidance,
   route/clock attribution and 30/60/120 FPS worst-frame testing before
   slots 2–5 can be activated. The new ledger does not simulate these.
5. Keep [installed playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148)
   open until actual engine, owned cooked map and installed Win64 evidence
   are independently reviewed.

This branch is stacked on [draft PR #329](https://github.com/sanskarIN/latebrake/pull/329)
and must stay draft/unmerged until prerequisites and real-engine QA pass.
The published source-only version remains v0.5.97; v0.5.98 is a separate
frozen physics release candidate in draft PR #295.
