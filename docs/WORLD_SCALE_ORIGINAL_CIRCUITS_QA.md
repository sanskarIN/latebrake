# Six original physical-road circuit recipes (source-only)

LateBrake now has **six engine-neutral, distinct rounded-circuit plans**
as candidates for future real gameplay. They share consistent road
width and collision-block semantics but have different straight
lengths and bend radii:

| Circuit | Half-X | Half-Y | Corner radius | Approx lap |
|---|---:|---:|---:|---:|
| Original Loop | 280m | 180m | 80m | 1.70km |
| Endurance Ring | 450m | 180m | 80m | 2.38km |
| Technical Crest | 260m | 220m | 120m | 1.71km |
| Harbor Club | 230m | 150m | 70m | 1.40km |
| Canyon Run | 340m | 230m | 90m | 2.13km |
| Test Oval | 190m | 190m | 140m | 1.28km |

Source files:
- `Track/WorldScaleCircuitVariants.h`
- `Track/WorldScaleVariantRacePace.h`

Each track has a complete closed, arc-length-parameterized 2D
centerline with headings; four straight and 64 rotated curve
**candidate physical-road slabs**, each with the same 900cm asphalt
width and original -1cm top; a closed non-colliding 850cm-wide
visual road ribbon; its own independent 512-node curvature, braking
and acceleration reference; an exact route station sampler and a
world-XY nearest-segment projector with support checks.

The first variant intentionally reproduces the original future
1.70km circuit geometry for backwards source-geometry compatibility.
The other five are independently authored dimensional recipes,
not online/downloaded levels or licensed real-world racetracks.

## Strict source verification

```sh
cmake -S Tests/Standalone -B build/variant-circuits -DCMAKE_BUILD_TYPE=Release
cmake --build build/variant-circuits --config Release --target latebrake_world_scale_original_circuits
ctest --test-dir build/variant-circuits -C Release -R '^WorldScaleOriginalCircuits$' --output-on-failure
```

The C++20 regression checks six unique source lap lengths,
68 separate correctly dimensioned physical collision proposals
per circuit, actual road-union support across 1,024 sample stations
and five lateral offsets each, every source visual mesh vertex,
512-station closed braking and propulsion constraints, source
seam wrapping and nearest XY route projection. It rejects invalid
track selections, mismatched lap lengths, expanded asphalt widths,
nonfinite geometry and unsupported samples.

## Important separation from playable UE

These are **source-only candidate levels**, not six actually
installed/chosen Unreal Engine tracks. A native Unreal 5.8
integration must construct the appropriate 68 genuine WorldStatic
collision components in individual cooked maps, check 3D driveable
shoulders/armco/kerbs, build static/streamed source art, implement
real respawn and ordered gates for EACH selected variant, attach
AI cars, qualify first-person and chase cameras, perform manual
desktop playtests and verify original materials. This code does
not make a packaged executable, simulate six physically driven
cars, implement multiplayer, or grant a race reward/lap.

The source-only `v0.5.100-test.2` protected-main snapshot remains
pinned at `a92a4799bfd2e43de30089f1cb9c069012529d5c` for owner
manual publication before any new feature branch is merged.
