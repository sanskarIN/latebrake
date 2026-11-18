# Original world-scale candidate racing line

This subsystem computes a **source-only path advisory** for the future
original 1.7-km continuous circuit. It does not steer a live car,
replace any authored WorldStatic road object, widen drivable asphalt,
change tire grip/Chaos collisions, credit checkpoints or create
a playable Windows build.

## Qualified input and corridor

`Track/WorldScaleCandidateRacingLine.h` accepts 512 proposed signed
lateral offsets keyed to the 512 source pace stations and an explicit
nonzero proposal ID. Offset requests are bounded inside the 9.0m
physical asphalt corridor, preserving a **98cm** vehicle half-width
and a **55cm** conservative shoulder margin (configurable only inside
a checked safe envelope).

The candidate positions are computed from the actual authored
centreline and tangent and must be physically supported on the
existing planned union of 68 rotated asphalt slabs. Quarter-span
checks also reject shortcuts through unpaved corners even if both
endpoints appear paved.

## Plan construction

A bounded, circular 512-node smoothing pass mitigates sudden
single-node steering demands, including across the 511→0 seam.
A second circular neighbor-slope pass caps lateral displacement per
station. This is geometric reference design, not a closed-loop
physical steering controller.

A discrete three-point curvature estimate is computed from each
candidate route. Its speed ceiling may only be **less than or equal to**
the established dry source target and original local curvature
ceiling. Closed-lap speed-squared relaxation enforces existing maximum
braking and propulsion acceleration budgets, including at the seam.
The returned geometric lap-time estimate cannot be faster than the
reviewed dry centerline; it is not a real lap time, record or tire
performance claim.

The resulting advisory can be sampled at negative or repeated lap
distances with physically supported position, signed offset,
direction and finite speed reference. The curvature/offset planner
does not implement passing, opponent avoidance, engine possession or
control authority.

## Reproducible source QA

```sh
cmake -S Tests/Standalone -B build/candidate-line -DCMAKE_BUILD_TYPE=Release
cmake --build build/candidate-line --config Release --target latebrake_world_scale_candidate_racing_line
ctest --test-dir build/candidate-line -C Release -R '^WorldScaleCandidateRacingLine$' --output-on-failure
```

The strict C++20 regression verifies all 512 spans against original
road union support, original dry target bounds, maximal lane offset,
body clearance, lateral slope, brake/drive acceleration, a requested
line crossing the start/finish seam, a single-station abrupt input,
untrusted/NaN/beyond-road proposals, and 30/60/120Hz sampling.

Before any real steering AI integrates these candidates, independently
test authentic tire/road contacts, real actor identity, 3D vertical
camber, physical collision bodies, route blockers, other cars, Chaos
steering/braking, UE5.8 cooking, and installed Win64 gameplay.

This remains a **draft unmerged source contribution**, because main
is frozen at `a92a4799bfd2e43de30089f1cb9c069012529d5c` for
the owner's manual source-only `v0.5.100-test.2` pre-release.
