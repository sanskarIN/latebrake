# Source-only opt-in indexed world-scale rival pilot

The experimental `WorldScaleCandidateRivalPilot` now has two distinct
methods. Its original `Step(plan, measuredSample)` continues using the
unchanged full `ProjectWorldScaleRoute` scan of 512 source segments.
Its new **explicitly opt-in** `StepIndexed(plan, index, measuredSample)`
uses the conservative block-indexed route lookup from draft PR #399.
Neither method is called by the Unreal race GameMode.

Both variants execute the **same** validation, observation continuity,
route lookahead, signed steering, pace reference, overspeed brake,
backward travel and zero-throttle rejection logic. Only the geometry
query differs. A missing, invalid or foreign index resets continuity
and fails closed with the pre-existing stopped pilot command; there
is no implicit fallback or synthetic actor movement.

The index must be built once from the **exact same, stable, immutable**
pace-plan object and cannot outlive or move its owner. Track rebuilding
must invalidate indexes before usage. Indexing does not establish
authentic Chaos tire contact, real actor identity, 3D collision authority,
checkpoint validation, actual timing or safe physical six-car racing.

The existing portable CTest target
`WorldScaleCandidateRivalPilot` now compares entire decisions from
the full-scan and indexed pilot across 720 observations each at
30/60/120 Hz, continuous start/finish-seam crossings, smooth lane
changes, invalid contact, wrong-way and reverse input, and index
mismatch/reacquisition. **Exact equality** is required for every
valid/invalid state, brake/throttle/steering and all advisory fields.

No Unreal Editor/UHT/Chaos execution, cooked original level, installed
Win64 game or measured frame time was certified by this code. Native
profiling and actor-backed QA remain mandatory before using the indexed
path in a real racing tick. The work is stacked on PR #399 and PR #398,
and all remain **draft and unmerged** while `v0.5.100-test.2` is
awaiting owner-manual publication from the reviewed main SHA.
