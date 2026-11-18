# World-scale route-projection profiling (source-only)

**Status:** development-only research. No licensed UE5.8 execution,
cooked circuit, installed Windows game, real six-car roster or
playable release is certified by this test.

## Why the measurement is needed

The current `ProjectWorldScaleRoute` scans all
`WorldScalePaceStations == 512` candidate centerline segments for
**every** XY projection. Six observations per frame imply exactly
3,072 segment candidates per frame, or 368,640 per second at 120 Hz,
before collision, Chaos, suspension, networking or rendering work.
These are loop counts, **not measured CPU timings**.

The audit intentionally does **not** replace the existing global
closest-segment implementation with a local-neighborhood shortcut:
hairpins, overlaps, pit access and lap seams require correctness
evidence before a search-space optimization can be accepted.

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

- Default CTest mode is deterministic and lightweight: checks all
  512 source centerline nodes, repeated projection identity,
  candidate road support and fail-closed invalid reference.
- Manual `--profile` mode precomputes six independent XY samples for
  60 simulated seconds at **30, 60 and 120 Hz**. It builds the pace
  plan and creates input coordinates **outside** the timed section.
- Each measured result is checked for validity, road support and zero
  centerline residual; the output reports projection count, average
  nanoseconds per query and a finite checksum to keep the work live.
- No wall-clock timing threshold is enforced in CI: machine speed,
  compiler optimization, thermal state and virtualization vary. Record
  compiler version, Release flags, architecture, run count, median
  and warm/cold behavior when doing a real engineering comparison.

These figures are **synthetic portable geometry costs only**. Actual
native optimization decisions require profiling the fully integrated
Unreal Tick on the original level with six distinct real actors, true
velocity/contact provenance, track crossings, roadside surfaces and
native Windows hardware at 30/60/120 FPS.

## Release isolation

This work is a separate draft QA change, **not part of**
the already-reviewed `v0.5.100-test.2` immutable main source
candidate (`a92a4799bfd2e43de30089f1cb9c069012529d5c`).
Do not merge it or any new code into `main` before the owner has
manually published and verified that source-only pre-release.
No tag, release, executable, GitHub issue/PR comment or runtime
gameplay change is created by this profiling program.
