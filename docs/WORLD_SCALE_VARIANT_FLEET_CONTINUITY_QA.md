# Six physical-observation continuity for six circuit source plans

This is **source-only racing-integrity infrastructure**, not six real
UE5.8 Chaos actors or an authoritative multiplayer/standings module.

`AI/WorldScaleVariantFleetContinuityAudit.h` accepts a **single,
unchanged, independently built** `WorldScaleVariantPacePlan` for one
of the six future source-only circuit recipes. The exact per-circuit
lap length, geometry and projected XY positions are used for all
six independently supplied actor observations. There is no shared
1.7km lap assumption across tracks.

## Mandatory physical evidence contract

Each observed frame requires a new frame ordinal and real physics
timestamp, matching nonzero race session, verified green race phase,
six physically observed bodies with all six unique actor tokens and
grid slots, finite XY pose and velocity, real road contact per actor,
valid signed spatial-revolution index and a wrapped independent
reported route station. Velocity is projected onto the **actual
authored geometric tangent**, never nose direction, gear selection,
simulated AI intention or arbitrary player HUD lap distance.

The full 512-segment source centerline projector independently checks
the measured XY chassis and source-reported route station. Every
consecutive frame must match the trapezoidal observed world motion
and signed route travel to a **frame-time-scaled 15% error budget
(with 5cm floor)**. This prevents reused frozen poses, forged speed,
impossible gate jumps, swapped actor identities, duplicate roster
slots, and a missing sixth real car from becoming accepted
continuity evidence.

A game session cannot silently switch to another circuit's source
plan and keep its continuity history. Skipped frame ordinals,
different session epoch, paused race or lost physics evidence
restarts/invalidates the source-only baseline; no interpolation over
unknown Chaos states, external producer clock or fake positions.
Invalid observations clear continuity and count rejection.

**Advisory order** is sorted using actual projected and signed
unwrapped source positions. It NEVER awards a lap, an overtake,
a podium, official standing, result, penalty, save, reward or
steering/throttle control.

## Portable regression

```sh
cmake -S Tests/Standalone -B build/six-variant-fleet -DCMAKE_BUILD_TYPE=Release
cmake --build build/six-variant-fleet --config Release --target latebrake_world_scale_variant_fleet_continuity
ctest --test-dir build/six-variant-fleet -C Release -R '^WorldScaleVariantFleetContinuity$' --output-on-failure
```

The strict C++20 regression walks six source-authored actor streams on
each of the **six distinct circuit length/radius variants** at
30/60/120Hz across the start/finish seam. It checks all-slot ranking,
monotone frames, actual XY projected station and signed revolution,
authentic reverse motion, rejected stale timestamp, forged progress,
frozen chassis, swapped track, duplicate actors, unqualified contact,
misreported lane station, and session/frame discontinuity.

These positions are deterministic source-derived fixtures; they
do **not** establish actual native Unreal actor spawning, Chaos
suspension, steering, collision forecast, UE5.8 Editor/UHT build,
cooked source maps, installed Win64 driving or online multiplayer.

This is a **draft dependent on PR #405**, and must not be merged
before owner-manual source-only `v0.5.100-test.2` publication and
source/engine qualification.
