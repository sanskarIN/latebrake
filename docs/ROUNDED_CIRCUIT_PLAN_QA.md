# Rounded physical circuit plan — isolated source candidate

**Status: original Unreal 5.8 C++ source proposal, not a running circuit or Windows playable build.**

The currently active track is still four broad rectangular asphalt collision slabs, one ground slab and four barriers. The experimental [v0.5.95 source-only release](https://github.com/sanskarIN/latebrake/releases/tag/v0.5.95) did not change this.

## Candidate geometry

The new `RoundedCircuitRoadPlan.h` describes a rounded-rectangle **physical collision road**, not decorative curved lane paint. Its 68 independently specified WorldStatic cube placements are four straight sections and 16 rotated, overlapping slabs per 90-degree bend across four corners. The centerline corner radius is **800 cm**, physical full width **900 cm**, and every slab is flush to the old asphalt top at **z = -1 cm**. A separate continuous arc-length centerline enables future rival steering and minimap work.

The plan's exact on-road classifier uses the union of the **rotated physical cube footprints**, rather than inventing road pixels on bare grass. It is deliberately separate from the current rectangular `IsOnPavedSurface`, checkpoint authorization, physics pawn, start gates, collision startup audit and saves.

## Source verification

The strict portable C++20 test checks all 68 placements, finite transforms, width and physical height; missing or invalid indices and NaN/Inf positions; current player/rival grids, four physical gates and recoveries; **6,860** inside/outside corner-edge samples; and **4,096** continuous forward centerline pose samples. Python bindings confirm the source plan is registered in CMake and has **not** silently replaced the live Unreal road.

These checks do **not** prove actual collision shapes or professional visuals inside Unreal. The overlapping boxes may produce visible stepped edges or undesired wheel impulses at seams. They are a geometry foundation for a continuous cookable road mesh, not finished racing-circuit art.

## Before a real UE5.8 Windows integration

- Implement one selected track mode shared by road collision, off-pavement grip, checkpoint centerlines, player/rival recovery, pit service, AI steering, minimap, camera, material UVs and career/event selection. The original wide `IsOnPavedSurface` cannot authorize traction over the new empty infield.
- Update the mandatory startup audit for the selected collision plan. Partial road construction must block race timing and rewards, never quietly leave an incomplete drivable track.
- Author a genuine smooth 3D road and shoulders with matching physics collision, original PBR asphalt/curbs, road normals, vegetation, realistic independently authored cars, lighting and camera. Do not mistake thin rotated cubes for a photoreal racing game.
- Resolve conflicts and integrate deliberately with [draft physics #295](https://github.com/sanskarIN/latebrake/pull/295), [draft visuals #297](https://github.com/sanskarIN/latebrake/pull/297), and [draft asphalt #313](https://github.com/sanskarIN/latebrake/pull/313).
- Test licensed UE5.8/UHT Editor compilation, real cooked map, Chaos tire contact and seams, 30/60/120 FPS driving, checkpoints/AI and installed Win64 package before switching the runtime track or publishing a playable executable.

See [playable-release gate #148](https://github.com/sanskarIN/latebrake/issues/148). **No new pre-release or Windows EXE is implied by this source-only plan.**

## Isolated Editor physics-lab authoring stage

The candidate includes **a real Editor-only level-authoring workflow**, not
a handcrafted/fake map or a new playable track:

1. A new strict C++20 executable `latebrake_rounded_circuit_lab_exporter`
   exports the 68 collidable pieces from the **same** tested C++20 header as
   deterministic JSON in centimetres. Python is not allowed to invent a
   different road.
2. `scripts/New-RoundedCircuitLab.ps1` builds just that exporter through
   CMake, validates 68 records, then writes a local export in the ignored
   `Saved/LatebrakeAutomation` directory.
3. On a trusted Windows workstation with **an authorized Unreal Engine 5.8
   Editor installation**, it invokes the genuine Editor Python script
   `scripts/Unreal/CreateRoundedCircuitLab.py`. The script refuses an
   existing `.umap`, NaN/Inf, missing/duplicate/reordered road pieces,
   nonpositive/out-of-bounds geometry, and mismatched top elevation
   **before creating any level**.
4. The real Editor creates the separate
   `/Game/Maps/LatebrakeRoundedPhysicsLab` with **68 BlockAll physical
   asphalt cubes and one original-ground-sized BlockAll cube**.
   World Settings must override the LateBrake racing GameMode to
   `GameModeBase`. This prevents the old rectangular RaceGameMode
   from spawning over the lab during Editor Play and makes this a
   *track-physics inspection level*, not a scored race.
5. Only after an Editor save succeeds, a receipt records the real
   `.umap` and C++ JSON SHA256 digests, byte length, UE version,
   separate map identity, one ground and all 68 road instances.
   The PowerShell wrapper verifies a *fresh* receipt, rehashes both files,
   and validates a genuine serialized Unreal map package header.

Manual Windows operator command (run in a clean checkout with CMake and an
authorized genuine Unreal Editor installation):

```powershell
pwsh ./scripts/New-RoundedCircuitLab.ps1 -EngineRoot "C:/Program Files/Epic Games/UE_5.8"
```

**This command has NOT been executed in a licensed Unreal Editor.** Its
mock Python tests merely exercise the requested API contracts with
synthetic bytes in temporary folders. This workflow does not compile a
real Unreal C++ game target, cook a map, check collision seams in Chaos,
author road materials, drive cars, replace `LatebrakeCircuit`, or build
an installer. It **never overwrites an existing lab map**: review it
manually if the operation is repeated. A generated local `.umap`
must not be promoted to the game without independent visual, collision
and installed Win64 qualification.

The existing `main` circuit, controller preview, career save schema,
nine audited physical blocks and v0.5.95 release remain unchanged.

## November 7 Editor round-trip and orientation corrections

The first Editor-only lab script had an important unqualified Unreal Python
API risk: unlike C++ FRotator, **Python's Rotator constructor orders its
arguments (roll, pitch, yaw)**. It now passes all three with named
arguments so a curved slab is rotated horizontally, never pitched.
The mocked fixture now uses nonzero corner headings and checks both
pitch and roll remain zero; positional-argument regressions fail source CI.

Before the source script claims any save success, it now **reads back**
all 68 road actors and one ground actor from the actual Unreal Editor
world, requiring unique expected labels, original stock Cube mesh,
BlockAll query-and-physics collision, exact world positions/scales/yaw,
and the isolated GameModeBase override. It then asks the *real Editor*
to **reload the serialized level** and repeats that audit. A missing or
drifted actor, disabled collision, unexpected pitch/roll, lost game mode,
failed reload or failed save blocks its success receipt. The PowerShell
entrypoint requires `audited_actors=69` and
`verified_after_reload=true` alongside the fresh JSON/map SHA256 hashes.
These APIs and numeric tolerances still require genuine UE5.8 validation;
a mock unit test is **not** real physics or a rendered game.

## Canonical C++ geometry byte-identity handoff

Before Unreal Editor opens, the Windows wrapper now hashes the exact 68-piece
JSON output produced by its compiled C++ exporter. It passes that SHA-256
through the dedicated process-environment variable
`LATEBRAKE_ROUNDED_GEOMETRY_SHA256`, restoring the previous value after
the Editor process exits. The Unreal Python script **requires** a lowercase
64-digit hexadecimal digest and compares it to the incoming JSON bytes
*before creating any level*, and rechecks the file after schema validation.
Missing, malformed, stale or changed-but-plausible 68-piece geometry fails
closed. The post-reload receipt adds `source_identity_verified=true`;
the Windows wrapper requires it as well as the original geometry and map
digests. Mock-only tests cover a different-but-valid geometry payload and
invalid digests. **These checks establish source handoff consistency,
not smooth meshes, rendered quality or actual UE5.8 wheel collision.**

## End-to-end C++ exporter numeric contract

A passing C++ exporter process alone cannot guarantee it emitted the correct
map. The new **independent Python verifier** runs the **actually compiled**
`latebrake_rounded_circuit_lab_exporter` as an optional additional CTest
and checks every one of the 68 JSON positions, sizes, rotations, physical
asphalt top and exact arc/straight placement against a separately computed
rounded-rectangle plan (9-decimal export tolerance). The verifier rejects
nonstandard NaN/Infinity literals, wrong units or road counts, duplicate
indices, changed corner headings, altered widths, off-road displacement,
missing pieces or unknown fields. Synthetic source-only mutation tests
exercise those rejection paths separately.

This makes the C++ to JSON representation independently inspectable across
Windows, Linux and macOS. It still **does not** create a real Unreal map or
verify physics contact; those are gated by the genuine UE5.8 Editor workflow
and installed-game QA.

## First continuous original 3D road **visual** mesh

The original `RoundedCircuitSmoothMesh.h` now constructs a closed **1024-station
continuous rounded-rectangle ribbon** directly from the candidate's same
`RoundedRoadCenterlineAt` geometry. Its road appearance is **850 cm
wide**, inset 25 cm on each side of the 900-cm physical collision corridor,
and its top is at **z=-0.55 cm**, 0.45 cm above physical asphalt.
This is **not** a new collision surface. The original 68 WorldStatic slabs
remain the only candidate physical road authority and should not be
disabled or replaced by the visual overlay.

The C++ exporter creates an original portable Wavefront **OBJ + MTL** preview
(`latebrake_rounded_smooth_obj_exporter <path/to/road.obj>`).
It emits **4,096 vertices and 8,192 triangles**, with full upper/underside
and edge walls, a watertight closed manifold, anticlockwise asphalt top
normals, lap-wrapped texture coordinates, and two simple original material
placeholders. It does **not** output a genuine Unreal `.uasset` or
photorealistic road textures. Rendering or import from OBJ into UE5.8
is a further separate acceptance task.

Source CI checks every one of the 4,096 visual vertices against the
68 rotated candidate collision footprints, additionally checks all
upper triangle centroids and edge midpoints against that physical union,
requires every undirected mesh edge have exactly two opposite-direction
incidences, tests all face normals, and uses an *independent Python
centerline computation* to verify the actual compiled OBJ geometry and
UV lap seam. This is real custom 3D geometry, but still **not inspected
in genuine Unreal Editor, not tested against Chaos tires, and not a
playable Windows build**.

The next integration is a controlled, separate Editor import into the
`LatebrakeRoundedPhysicsLab` level with **collision disabled on the new
visual actor** and actual 68-slab physical collision readback, followed
by independent UE5.8 render/handling QA. It must not silently alter the
existing rectangular race/career map or qualify an executable pre-release.

## Portable real-geometry preview artifact (not an Unreal game)

The Ubuntu source CI matrix now builds the *actual* C++ exporter and
runs its independent OBJ geometry validator after the full source-test
suite. Only if those gates pass, Actions archives an artifact named
`UNVERIFIED-RoundedRoad-OBJ-Only-<source-SHA>` containing
`LateBrakeRoundedVisualRoad.obj`, `.mtl`, `READ_ME_FIRST.txt`
and `SHA256SUMS.txt` with seven-day retention. It contains **no EXE**,
no playable race, no Unreal Editor asset and no installer.

This is useful for looking at the continuous proposed original track
shape in a 3D model viewer. It is **not** UE5.8 rendering, realistic
textures, lighting or driving QA. Never substitute its dark placeholder
MTL colors for approved final road materials or claim its mesh is a
tested collision model.

## New full-scale profile — 1.70-km lap, preserved 9-m physical width

The 170-m original rounded lab is mathematically convenient for testing
but far too short for the intended full-length racing experience. The
new **isolated world-scale 10× profile** expands **only** the centreline
positions and the tangential lengths of all 68 physical candidate road
pieces. It does **not** inflate the road across its width or scale any
vehicle/camera/physics assets:

- Lap length: approximately **1,702.65 metres** instead of 170.27 m.
- Corner radius: **80 metres** instead of 8 m.
- Road physical width: **9.0 metres** in both profiles.
- Visible original asphalt top width: **8.5 metres**, still inset by
  25 cm on each side of the collision road.
- Original 1024 cross-sections, 4096 manifold vertices and 8192 triangles
  are retained, with longitudinal UV coordinates extended to the full lap.
- Both profiles still specify **68 overlapping physical collision-backed
  sections** and retain their `-1 cm` asphalt top. This does not yet
  eliminate the potential wheel bumps and stepping caused by separate
  collision boxes.

The full-scale mesh is generated from the same C++ source-of-truth
`WorldScaleRoundedCircuit.h` and exported by
`latebrake_world_scale_rounded_obj_exporter`. Existing standalone
C++ geometry tests and independent Python OBJ/UV validation remain in
place for the original 170-m candidate. Separate full-scale CTests
validate **8192 driving-line samples**, all 68 collision plans and all
4096 visible vertices plus triangle centroids/edge midpoints against
the scaled physical union.

**This is a separate, unactivated 3D artwork/physics proposal.**
It has NOT changed the active rectangular career circuit, scoring
gates, live player/rival spawn locations, AI racing line or driving
behaviour. No real UE5.8 UHT, Editor build, cooked material, real tyre
contact, Windows EXE or installer has been created or validated. The
full-scale course will need separate race checkpoints, start/recovery
positions, natural landscaping, surface mesh collider and real Chaos
seam tests before runtime activation.

## Full-scale original 1.70-km model preview artifact

After all three platform C++20 matrices pass, a separate source-only
GitHub Actions job compiles **the exact C++ 1.70-km model exporter** and
runs the independent `WorldScaleRoundedCircuit` and
`WorldScaleRoundedRoadObjContract` tests. It then publishes a seven-day
artifact `UNVERIFIED-Original-1p70km-Road-OBJ-<commit>`, containing
`LateBrake_1p70km_OriginalTrack.obj`, the original placeholder
material `.mtl`, `SHA256SUMS.txt`, and `READ_ME_FIRST.txt`.
It contains **zero playable Windows assets**, and this step intentionally
does not touch the older 170-m Editor-only physics lab, existing game
levels, GitHub releases, or the protected default branch.

Full-scale model previews can be inspected in a standard 3D OBJ viewer,
but *neither the model nor the source-plan C++ tests certify actual
Chaos tyre/grip behaviour or Unreal renderer fidelity*. The runtime
switch and installed 3D Windows preview remain separate later gates.


## Full-scale isolated Unreal Editor collision lab — source integration candidate

The **1.70-km** profile now has a *separate* portable C++ JSON exporter,
`latebrake_world_scale_rounded_circuit_lab_exporter`, with a compiled-output
contract test against an independent Python circle/straight model. Its geometry
is 10× in world X/Y and along-road piece length, **not** across the physical
road width: asphalt stays **9 m wide** and the planned 68 physical slabs
keep the original top elevation. The new authoring option deliberately does
**not** touch the original 170-m lab or the active playable/career map.

On a Windows machine with CMake, Python and an **authorized real Unreal
Engine 5.8 Editor** installation:

```powershell
pwsh ./scripts/New-RoundedCircuitLab.ps1 -EngineRoot "C:/Program Files/Epic Games/UE_5.8" -WorldScale
```

This builds the separate C++ exporter and verifies the shape of its output,
then passes the actual compiled-export **SHA256** and the explicit
`world_scale` mode to the Editor, restoring both process environment
variables afterward. It refuses an existing map and stores the candidate
map at `/Game/Maps/LatebrakeWorldScaleRoundedPhysicsLab`, with distinct
JSON and receipt paths under ignored `Saved/LatebrakeAutomation`.
The Editor must persist and reload **68 BlockAll collision road actors
plus one separate larger ground actor**, with the `GameModeBase` isolation
and exact readback of positions, road widths, heading and physical surface
height. Both source-data and map SHA256 digests must match the fresh receipt
before the wrapper reports success. No existing prototype map is overwritten.

**Current verification scope:** portable C++/Python/PowerShell source checks
and mocked Editor API contracts; a real UE5.8 Editor authoring run has **not
been performed**. A successful lab map would *still not* qualify driving:
the 68 box seams are not a continuous Chaos collision mesh, the separate
visual-only OBJ is not automatically imported, and the real race-mode
checkpoint/AI/vehicle/camera systems have not been wired to this profile.
Installed Win64 gameplay, material quality and frame-time remain blocked
under [#148](https://github.com/sanskarIN/latebrake/issues/148).


## Continuous full-width 9-m physics-surface source candidate

To eliminate the seam topology of 68 intersecting BlockAll boxes in a future
licensed-engine implementation, a **different, fully continuous and closed**
1.70265-km source geometry is now specified by
`WorldScaleSmoothCollisionRoad.h`. It reconstructs the physical surface
directly from the tested world-scale centreline at 1,024 evenly spaced
cross-sections, using 9.0 m of physical lateral width at every section,
not the 8.5-m decorative inset and not 90 m of accidentally scaled road.

The experimental volume has a top at `z=-1cm` and bottom at `z=-43cm`.
Its original 4,096 vertices and 8,192 triangles include road top,
underside and continuous sidewalls. Unlike 68 isolated boxes, it uses
the same **closed manifold** on every bend and around the lap seam.
The new `WorldScaleSmoothCollisionSurface` strict C++20 test requires
exact widths, thickness, centreline station positions, road-top/bottom
winding, nonzero triangle area, manifold edge incidence and a 25-cm
outboard offset from the independent 8.5-m cosmetic mesh. Every station
is checked, including the wrap seam, on all three desktop CI platforms.

**Important:** this is a source-level shape contract, **not** cooked
Unreal collision. It must **not** be enabled as a Chaos collider or
treated as wheel-contact-ready without an authorized UE5.8 Editor
mesh-cooking pipeline, correct `Collision Complexity` policy, joined
normal/edge handling, actual dynamic-vehicle tyre contact tests and
installed 30/60/120 FPS driving QA. The existing 68-cube lab remains
unchanged. The live career circuit still uses its original rectangular
road and score/AI policies. No 3D game EXE or installer is implied.
