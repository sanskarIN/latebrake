# World-scale non-colliding roadside guardrail scene source

The future **1.7 km** route can now calculate three visual Armco-style
rail levels and a source-aligned post pair at each of approximately
425 track stations. These are **render-only proposals**, not a
complete player-protecting safety barrier in Unreal Engine.

The source algorithm uses the same original candidate centerline,
heading and **68 rotated physical asphalt slabs**, with rail
placements approximately 9.9m to either side of the road centerline.
Every rail endpoint, every section midpoint and every post must be
OUTSIDE the original physical asphalt union. Rail faces have three
distinct elevations, contiguous section start/end positions, bounded
lengths and unique deterministic tokens for eventual UE mesh
instancing.

The code never extends the drivable roadway, creates independent
WorldStatic/Chaos collisions, changes checkpoint authority or
records a physical vehicle contact. Collision-bearing safety walls,
original materials, contact normals, breakable barriers, run-off
geometry, real graphics, memory management and visual checks must
be constructed by a separate native Unreal5.8 implementation.

The exact settings reject unsafe placement in the traffic lane,
nonfinite inputs, impossible density, invalid height/width, or any
request to give these props physics/official race authority.

## Portable verification

```sh
cmake -S Tests/Standalone -B build/scene-source -DCMAKE_BUILD_TYPE=Release
cmake --build build/scene-source --config Release --target latebrake_world_scale_guardrail_scene
ctest --test-dir build/scene-source -C Release -R '^WorldScaleGuardrailScene$' --output-on-failure
```

The C++20 tests validate the full three-level left/right inventory,
all off-pavement placements, source rebuild determinism, paired
post identity and height, alternate spacing, lap seam, unsafe
in-road requests and collision-authority gating.

The approved main source pre-release remains pinned to
`a92a4799bfd2e43de30089f1cb9c069012529d5c` until owner
publication of `v0.5.100-test.2`. Keep this source-only candidate
draft/unmerged until all checks and genuine native integration pass.
