# World-scale source roadway scene plan

This is a source-only, portable C++20 preparation for a future LateBrake
**1.7 km** original road preview. The actual Unreal game is unchanged.

## What is modeled

`WorldScaleSurfaceScenePlan.h` creates ordered, visually oriented top
quads for each of **1,024** original source-road sections:

- independent left/right solid white edge markings;
- a discontinuous centre dash for orientation, not a passing law;
- red/white alternating bend-only decorative kerb stripes.

It derives every quad from the actual future-circuit centreline, heading,
and existing 68-piece physical WorldStatic road plan. Each **corner** of
each decorative quad must lie on the union of the authored physical
road rectangles. All decorative Z values lie just above the physical
road top and are visual-only; triangles are not cooked or collision
enabled. No cosmetic stripe may authorize a new drivable shoulder.

`WorldScaleApproachBoardPlan.h` proposes **150/100/50m** braking boards
outside the roadway on both sides before each of the **four** original
rounded corners (24 total). It detects actual curvature transitions on
the 512-node source pace plan, wraps the lap seam, checks each board is
outside physical asphalt, rejects overlaps and gives each a unique
placement token. The signs are placeholders, not UE actors or
WorldStatic barriers, and have zero braking or race-control authority.

## How to verify

```sh
cmake -S Tests/Standalone -B build/scene-source -DCMAKE_BUILD_TYPE=Release
cmake --build build/scene-source --config Release --target latebrake_world_scale_source_scene
ctest --test-dir build/scene-source -C Release -R '^WorldScaleSourceScene$' --output-on-failure
```

The regression checks source support for every quad, CCW top winding,
red and white kerb alternation, deterministic repeated scene builds,
four corner inventories, 24 off-road board positions, approach
distances and unique physical proposals. It also rejects
out-of-bounds paint, bad source geometry, nonfinite heights, illegal
roadway sign placement, and any setting that would give visual
paint, markers or kerbs independent collision or checkpoint authority.

## Remaining real-game tasks

Create an original UE5.8 editor scene component that translates the
portable quads to non-colliding mesh sections with authentic original
materials, physically-grounded guardrails, usable streaming/LOD,
manual visual inspection, and a single complete original circuit
level. Native Unreal/UHT validation, world coordinates/Z contact,
camera visibility, installed Win64 gameplay, desktop graphics
quality and racing feel are **not** established by this source-only
planner. No online multiplayer is implemented here.

The reviewed main release snapshot `a92a4799bfd2e43de30089f1cb9c069012529d5c`
must not be altered before the owner manually publishes and audits
`v0.5.100-test.2`; this work remains a draft PR.
