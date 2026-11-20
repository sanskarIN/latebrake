# Experimental block-indexed world-scale route projection

**Source-only research.** Nothing in this branch changes the reference
`ProjectWorldScaleRoute` algorithm, Unreal Engine adapter, game physics,
checkpoint/lap authority, packaging, release tags or live six-car racing.

## Question being tested

The previously audited global projector examines all **512** original
centerline segments per position. Does a build-once, conservative
bounding-box broad phase avoid unnecessary segment calculations while
returning **exactly** the same route station, signed road offset,
centerline residual, candidate road-support flag and speed reference?

`Source/Latebrake/Public/Track/WorldScaleProjectionBlockIndex.h`
builds **32** bounds, each enclosing the full endpoints of **16**
consecutive segments. Each query checks all 32 bounds in source order,
only skipping a block when the proven axis-aligned box's squared
distance is STRICTLY greater than the already measured best squared
distance plus a floating-point safety margin. It evaluates surviving
segments with the existing reference formula in original order, so
near ties favor the same lowest-index segment. The closing segment
(511 to 0) is enclosed; the start-line wrap is preserved.

This is *not* a blind fixed-radius local window. The original
global nearest segment remains the source of truth.

## Important limitations

- Build the candidate index once from a fully qualified **immutable**
  `WorldScalePacePlan`, then use it only with that **same living**
  plan object. The search checks plan-object identity and rejects
  uninitialized or foreign indexes. Do not move/free/mutate the plan
  after building the index. Real integration must own source lifetime
  explicitly and rebuild on track changes.
- The builder prevalidates **all 512** segment lengths, including
  nonfinite/zero-length geometry, so pruning cannot conceal an invalid
  distant source segment. This is not a general spatial database.
- The block search is advisory-only, and not yet connected to
  `SixCarRouteContinuityAudit` or Unreal physics.
- C++ floating-point math and corner ambiguity must be independently
  tested with actual game geometry and compiler configurations before
  claiming guaranteed runtime speedup or identical behavior at all
  external inputs.

## Reproducible verification

```sh
cmake -S Tests/Standalone -B build/route-block-index -DCMAKE_BUILD_TYPE=Release
cmake --build build/route-block-index --config Release --target latebrake_world_scale_projection_blocks
ctest --test-dir build/route-block-index -C Release -R '^WorldScaleProjectionBlocks$' --output-on-failure
```

The standalone regression compares **complete returned structures**
against the unchanged full scan, including valid/invalid flags and
**exact double results**, for 512 source stations × four subsegment
positions × seven physical or off-road offsets, separate off-road
points, and six independent source-coordinate streams sampled at
30/60/120 Hz. Invalid plans, NaN/Inf, out-of-world input and
cross-plan index reuse fail closed. Test receipts report eliminated
candidate segment evaluations; these are operation counts, **not
wall-clock speedups**.

The existing PR #398 profiling harness is the baseline for later
manual A/B CPU measurements. A separate measured comparison on
release-optimized Windows and in Unreal Engine 5.8 is mandatory before
adopting a different search in any runtime Tick.

## Merge/release boundary

This PR is stacked on the draft profiling PR #398. Keep both
**DRAFT and UNMERGED** until the repository owner publishes and
independently audits the previously approved
`v0.5.100-test.2` source-only snapshot from protected-main SHA
`a92a4799bfd2e43de30089f1cb9c069012529d5c`.
No compiled game, installer, native engine test or playable binary
has been produced by this research.
