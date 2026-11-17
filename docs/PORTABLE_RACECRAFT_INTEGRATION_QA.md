# Portable racecraft integration — C++ source-only qualification

**Development status: integration candidate, not Unreal Engine 5.8 or packaged game QA.**

This branch consolidates previously authored and independently developed C++20
source-only work that was present in the historical experimental draft
[`#349`](https://github.com/sanskarIN/latebrake/pull/349) but absent
from current protected `main`. The original work remains attributable to its
existing Git blobs; this integration is not represented as newly written
physics or as a port of an already validated Unreal game. In particular, it
does NOT merge the older draft's `LatebrakeCarPawn.cpp`, `GameMode.cpp`,
Unreal actor/asset creation, or cooked Win64 preview code.

## Scope

The portable additions preserve existing engine-neutral code in four groups:

- **Telemetry:** bounded session history, timeline, physical lap/sector
  analysis, fuel, pace, reliability, driver comparisons, event review,
  data export, race strategy, coaching and training drill observations.
- **Track:** future world-scale original circuit centerline, geometry,
  smooth collision planning, road details, grid, line, track-limits and
  route-pace envelopes. These are candidate geometry policies, not UE maps.
- **Vehicle:** suspension/contact and wheel-brake prototypes, braking
  capacity, traction budgets, human/reverse governor decisions, observed
  speed, wheel visual trails and road response estimators.
- **AI:** isolated six-car identity, candidate grid/traffic and risk
  validation policies. No AI actor spawning or control binding is enabled.

A new `CircuitPaceAudit.h` connects the proposed world-scale route model
to trusted observed world-XY chassis motion **for advisory-only analysis**.
It resets on lost road support, missing identity, invalid frame timing,
discontinuous samples, actor/session change, lap-position inconsistency and
pause. Only consecutive qualified frames earn audited elapsed time.
It never changes speed, steering, checkpoint order, records or game state.

## Verifying this specific branch

From the repository root:

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --config Release --parallel 4
ctest --test-dir Tests/Standalone/build -C Release --output-on-failure
python3 -m unittest discover -s Tests/Editor -p '*Tests.py'
python3 scripts/MeasureSourceScale.py --json
```

The CMake additions register **47 new standalone behavioral targets**:
45 historical portable tests plus the new circuit pace and cross-header compatibility tests
test. Each uses strict C++20 and compiler warnings as errors on Windows,
Ubuntu and macOS. GitHub Actions results are separate and must be checked
against the final exact branch SHA, not an older passing revision.

## Interpreting line counts correctly

Report separately: (a) **new physical lines in `Source/**` C++ files**,
(b) **new physical lines in `Tests/**` C++**, (c) combined C/C++ changed
lines, and (d) the repository's stricter
`game_source.nonblank_noncomment_lines`. A 20,000-line combined
implementation-plus-test change is **not** automatically a 20,000-line
production milestone; generated, duplicate, comments and untested files
must not inflate the qualifying metric.

All code from historical branches has independent provenance. Do not
claim the imported algorithms were newly authored in this session.

## Native gating and release policy

Portable C++20 tests are NOT licensed UE5.8/UHT compilation, actual Chaos
vehicle physics, road support, correct 3D asphalt visualization, six-car
collision-free starts or measured Windows gameplay. Before a future runtime
connection, conduct independent architecture, provenance, static safety and
performance reviews. The owner must separately approve any actual Unreal
player/AI integration and run exact-head Editor and Win64 checks. Nothing
here authorizes a release, modifies existing gameplay or bypasses protected
main.
