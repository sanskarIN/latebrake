# Race engineer telemetry — development QA

**Status: ENGINE-NEUTRAL, SOURCE-ONLY.** The current implementation is
an offline/deterministic diagnostic subsystem, NOT a claim that Unreal
Engine 5.8 or an installed Windows racing game now records this data.
Do not enable it for official lap timing, recovery fees, rewards,
checkpoint verification, player control, network play or SaveGame.

## Feature scope

The ten new C++20 headers under `Source/Latebrake/Public/Telemetry`
form one cohesive read-only race-engineering core:

- `TelemetryMath.h`: world-unit-safe planar/vector geometry, finite
  checks, heading wrap, curvature, projected speeds, stopping envelopes.
- `TelemetryTypes.h`: stable, provenance-tagged chassis observations,
  four wheel measurements, controls, race status and typed diagnostics.
- `TelemetryValidation.h`: configurable fail-closed frame gate;
  session/actor revisions, monotonic time, physics readiness, speed and
  pose limits, finite input, duplicate ordinals and discontinuities.
- `TelemetryFilters.h`: bounded rolling quantiles, unbiased statistics,
  angular and vector filters, rate limiters, one-euro smoothing, stable
  duration windows and saturating integrators.
- `TelemetryKinematics.h`: observed velocity/acceleration/yaw/slip,
  positional consistency, wheel loads and motion anomaly summaries.
- `TelemetryTrack.h`: geometric centerline projection anchored to
  original LateBrake circuit gate positions, closest segment, cross-track
  deviation, sector and backtracking review; it is NOT a substitute for
  collision meshes or physically verified checkpoint progression.
- `TelemetryBraking.h`: complete pedal attack/trail/release events,
  wheel locking, thermal fade, stopping-distance and brake-zone reviews.
- `TelemetryCornering.h`: corner turn-in/apex/exit observations, line
  precision, under/oversteer proxies, exit speed and event history.
- `TelemetryGrip.h`: four-wheel measured contact/load, tire wear,
  temperature, longitudinal/lateral saturation, traction-demand review.
  The math is an ADVISORY estimate; it does not implement Chaos tires
  or authorize an arcade grip/torque change.
- `TelemetryOpponents.h`: six-actor identity-aware relative positions,
  short constant-velocity closest-approach prediction, side-by-side and
  provisional overtaking history. It NEVER commands braking, steering,
  rank or lap credit. Predictions are NOT proof of contact-free motion.

No file imports Unreal headers. No file creates assets, modifies
positions, requests physics impulses, runs background threads or
writes player state. The existing experimental six-car game logic
remains unchanged. A later separately reviewed UE5.8 source binding
would have to collect genuine `ActorToken`/physics samples before these
advisory metrics could honestly appear in-game.

## What the code count means

The milestone audit command is:

```bash
python3 scripts/MeasureSourceScale.py --json
```

The reference integrated six-car candidate PR #341 has **19,801**
qualifying nonblank/noncomment production game C++ lines. This
telemetry development branch was measured at **25,165**, or **+5,364**
qualifying production lines. The new headers contain real validation,
geometry, filters and stateful analysis, rather than inflated tables
or generated test scaffolding. Test files, docs and CI are excluded.
The full 200,000-line v0.6.0 quantity objective still requires
**174,835** more qualifying lines. This is a source inventory
only, never a shipping quality certification.

## Regression commands

```bash
cmake -S Tests/Standalone -B Tests/Standalone/build -DCMAKE_BUILD_TYPE=Release
cmake --build Tests/Standalone/build --parallel 4
ctest --test-dir Tests/Standalone/build -R '^RaceEngineerTelemetry$' --output-on-failure
python3 -m unittest discover -s Tests/Editor -p 'RaceEngineerTelemetryBindingTests.py' -v
```

The C++ target includes every new production module at C++20 with
`/W4 /WX` (Windows MSVC) and `-Wall -Wextra -Wpedantic -Werror`
(macOS/Linux). Regression scenarios cover finite/no-NaN invariants,
initial and stale/duplicate session frames, sensor discontinuities,
rolling sensor filters, mapped real original-circuit straights,
measured brake events, effective grip/wheel load changes, valid corner
entry/apex/exit, exact six-car opponent identity, permutation
independence and an additional deterministic 30,000-frame motion sweep.
They do NOT run UHT, cook a map, or actually control a car.

## Unreal runtime integration gate

1. Preserve owner-pinned PR #341 for real Editor/UHT/Win64
   validation without mutating its head. Merge no diagnostic
   changes into that source candidate during the trusted run.
2. If future native instrumentation is selected, explicitly pass
   actual current game-frame position, yaw, velocity, brake pedal,
   four-wheel physical contact and genuine actor identity.
   Label any unavailable wheel, temperature or load sensor as
   missing rather than inventing data. The existing starter
   vehicle may not expose all four genuine wheel channels.
3. Run genuine gameplay at 30/60/120 FPS, trigger pause/focus
   resets, session restart, recovery and actor replacement;
   verify that every stale frame is rejected and no snapshot can
   change official progress, AI motor demand or player controls.
4. Benchmark fixed-capacity memory and worst-case repeated
   six-car analysis. Keep telemetry completely optional for
   shipping builds until source/engine QA is complete.
5. Distinguish observed native positions/speeds from
   conceptual risk and tire-grip estimates in every HUD label.
6. Publish no Win64 installer or pre-release based on source-only
   metrics. Keep gameplay readiness issue #148 unresolved until
   the real engine and installed race QA requirements are met.

The new telemetry headers are suitable for independent developer
review and experimental replay use. They are not a verified feature
of a playable Unreal racing game yet.
