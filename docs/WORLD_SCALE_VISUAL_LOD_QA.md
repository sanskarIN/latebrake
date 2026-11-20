# Three-level source-only visual road mesh LOD

For the future original **1.7 km** circuit, the new portable
`Track/WorldScaleVisualLodPlan.h` prepares **three noncolliding**
closed-ribbon visual geometry levels:

| LOD | Source sections | Vertices | Triangles |
|---|---:|---:|---:|
| High | 1,024 | 4,096 | 8,192 |
| Medium | 512 | 2,048 | 4,096 |
| Far | 256 | 1,024 | 2,048 |

All source sections come from the same arc-length-parameterized
original circuit; only tessellation density changes. Every visible
road edge retains the original **8.5m visual width**, with every
vertex checked against the original 9.0m physical road union.
The code independently audits curvature-chord midpoint error and
retains the original 4-vertex/8-triangle per-section closed manifold
with outward top/bottom winding and first/last seamless wrap.

The 68 original physical road collision blocks remain untouched.
No new Chaos road mesh is cooked or used by any active actor.

A pure camera-distance LOD policy selects high below roughly 50m,
medium through 150m, and far beyond that, with a 10m hysteresis band
to reduce render-resource thrashing. These are default future
rendering **suggestions**, not measured FPS improvements or gameplay
camera behavior.

### Validation

```sh
cmake -S Tests/Standalone -B build/scene-source -DCMAKE_BUILD_TYPE=Release
cmake --build build/scene-source --config Release --target latebrake_world_scale_visual_lod
ctest --test-dir build/scene-source -C Release -R '^WorldScaleVisualLod$' --output-on-failure
```

The portable strict C++20 test checks independent LOD counts,
true physical-road coverage, top/bottom CCW winding, visual ribbon
width, four corner quadrants, exact high-detail point compatibility
with `BuildWorldScaleVisualRoadMesh`, camera hysteresis, and fail-closed
invalid settings.

**Still required:** actual UE 5.8 cooked procedural/static mesh
creation, visual materials, texture UVs, render-thread lifetime,
LOD switching under a native camera, occlusion, collision separation,
asset ownership, screenshot review and installed Win64 hardware QA.

This source-only research remains draft/unmerged while the approved
`v0.5.100-test.2` snapshot is unpublished.
