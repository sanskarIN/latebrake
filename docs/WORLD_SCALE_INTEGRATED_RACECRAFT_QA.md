# Integrated original-road and six-car racecraft source QA

Status: **DRAFT / SOURCE-ONLY / UNMERGED**. The protected
`main` release snapshot `a92a4799bfd2e43de30089f1cb9c069012529d5c`
is reserved for the owner's manual pre-release
`v0.5.100-test.2` and must not be moved by this candidate.

## Purpose

This branch integrates source logic from the reviewed future-circuit
PRs #401–#404 and #405–#408 into one **common portable C++20
compilation/test topology**. Each dependency branch remains separately
inspectable; no source lines are counted twice merely because an
intermediate PR is stacked.

## Authored modules included

- 6 separate original geometric circuit recipes (each 68 physical-road
  **proposals**, 9m paved width, independent route distance).
- 512-node independent curvature/pace for each source layout.
- Six-car on-road grid footprint safety, four ordered gate planes,
  and false/reverse-gate crossing rejection per layout.
- Authenticated signed-revolution fleet source continuity (all six
  actor identities, accurate physics timestamps, 30/60/120Hz).
- Measured actual-XY rival pilot steering and conservative speed.
- Six-car same-frame on-road following/braking arbitration, with
  **all-or-nothing** admission of the complete physical roster.
- 1.7km original road scene paint and kerb striping, 150/100/50m
  corner approach boards, visual ribbon high/medium/far LOD and
  roadside guardrail/post **render-only proposals**.
- 512-station candidate racing line on the original physical road
  union, bound by vehicle footprint, lateral offset, source speed
  and acceleration/braking constraints.
- Exact-source block-pruned 512-station nearest-route projection with
  source/profile benchmark and independent A/B reference checks.
- Optional indexed original 1.7km rival pilot with exact plain-pilot
  output-parity checks; existing simple-pilot behavior is preserved.
- Full-ring **measured-grip** world-scale lap speed planning that
  rejects unqualified or stale traction observations and never
  assumes dry tire grip as a real physical measurement.
- Conservative measured-grip original rival pilot adapter with
  no additional Chaos control authority.

## Reproduce the portable tests

```sh
cmake -S Tests/Standalone -B build/integrated-source -DCMAKE_BUILD_TYPE=Release
cmake --build build/integrated-source --config Release
ctest --test-dir build/integrated-source -C Release --output-on-failure
python3 scripts/MeasureSourceScale.py --json
```

The build/CTest registration explicitly includes:

- `WorldScaleOriginalCircuits`
- `WorldScaleVariantGridAndGates`
- `WorldScaleVariantFleetContinuity`
- `WorldScaleVariantRivalPilot`
- `WorldScaleVariantTrafficSupervisor`
- `WorldScaleSourceScene`
- `WorldScaleVisualLod`
- `WorldScaleGuardrailScene`
- `WorldScaleCandidateRacingLine`
- `WorldScaleProjectionProfile`, `WorldScaleProjectionBlockIndex`,
  and `WorldScaleProjectionABProfile`
- `WorldScaleVariableGripPace` and `WorldScaleConservativeGripPilot`

Tests must pass on Windows/MSVC, Ubuntu/GCC and macOS/Clang
at the **exact final integration SHA**. Docs/release-policy checks
also remain mandatory. A passing job on an ancestor does NOT qualify
new commits.

## IMPORTANT: not yet real native gameplay

This branch does not instantiate even one of these proposed future
tracks as an Unreal 5.8 cooked 3D level. The 68 road blocks per track
are placement recipes, not existing WorldStatic components. The
render-only kerbs, visuals, guardrails and signs do not carry Chaos
collision. Signed actor/frame tokens in portable tests use source
fixtures, not real UE physics observations. No six true Chaos actor
roster, collision or tire contact is established by the portable
test runs.

Before a playable claim: integrate authentic actors, real racing
camera/controls, render and collision meshes, per-track ordered
checkpoint authority, physical braking, 3D surface elevations,
materials, dynamic loading/performance, realistic audio, installed
Win64 QA and independent player driving review. Do not attach
an untested 2D/Win32 or source-only executable as a genuine UE game.

## Counting

Use the repository's `MeasureSourceScale.py --json` output
`game_source.nonblank_noncomment_lines`, excluding tests,
documentation, generated files, external assets and duplicate
staged branches. A line target is an engineering milestone,
not evidence of gameplay completeness or native readiness.
