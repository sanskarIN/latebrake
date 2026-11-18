# World-scale route-projection profiling (source-only)

**Status:** development-only research. No licensed UE5.8 execution,
cooked circuit, installed Windows game, real six-car roster or
playable release is certified by this test.

## Why the measurement is needed

The current `ProjectWorldScaleRoute` scans
`WorldScalePaceStations == 512` candidate centerline segments for
**every** XY projection. Six observations per frame imply 3,072
segment comparisons per frame, or 368,640 per second at 120 Hz,
before collision, Chaos, suspension, networking or rendering work.
These are static loop counts, **not measured CPU timings**.

The audit intentionally does **not** replace the existing global
closest-segment implementation with a local-neighborhood shortcut:
hairpins, overlaps and lap seams require correctness evidence
before a search-space optimization can be accepted.

## Reproducible portable measurement

```sh
cmake -S Tests/Standalone -B build/route-projection-profile -DCMAKE_BUILD_TYPE=Release
cmake --build build/route-projection-profile --config Release --target latebrake_world_scale_projection_profile
ctest --test-dir build/route-projection-profile -C Release -R '^WorldScaleProjectionProfile$' --output-on-failure
```

Run the resulting `latebrake_world_scale_projection_profile`
executable with `--profile` from the build's platform-specific
executable directory (for example, `build/route-projection-profile/`
on a single-config Linux generator or `.../Release/` with Visual
Studio).

- Default CTest mode is deterministic and lightweight: check all
  512 exact source-centerline knots, **1,536** halfway-between-station
  left/center/right road offsets, repeated centerline projection,
  failure on invalid source reference and untrusted nonfinite or
  out-of-range XY inputs.
- Opt-in `--profile` precomputes six staggered station/lane-offset
  XY observations per frame for **60 simulated seconds** at
  **30, 60 and 120 Hz**. It builds the pace plan, verifies source
  road support and creates input coordinates **outside** the timer.
- Each of three full trials writes projection results to a buffer
  that is independently validated and consumed **after** the timer.
  This isolates the projector plus unavoidable result writes from
  input generation, assertions and checksumming. The reported
  nanoseconds/query are the **median of three trials**, and the
  six-query millisecond figure is only a synthetic source estimate.
- No time limit is enforced in CI: machine speed, Release flags,
  compiler optimization, virtualization and thermal state vary.
  Record compiler version, architecture, run count and repeat
  measurements on actual target hardware before design decisions.

Source positions and lateral offsets are **fabricated geometric test
inputs**, not a simulation of six chassis and not evidence of physical
actors, braking, racing interaction or installed performance.

These figures are **portable geometry costs only**. Actual native
optimization decisions require profiling an integrated Unreal Tick
on the original level with six distinct real actors, true
velocity/contact provenance, real track crossings, road shoulders
and native Windows hardware at 30/60/120 FPS.

## Release isolation

This work stays in draft QA, **not in** the already-reviewed
`v0.5.100-test.2` immutable protected-main candidate
(`a92a4799bfd2e43de30089f1cb9c069012529d5c`).
Do not merge into `main` before owner manual source-only
pre-release publication and independent published-source audit.
No release/tag, executable, GitHub issue/PR comment or native
gameplay change is created by the profiling program.
