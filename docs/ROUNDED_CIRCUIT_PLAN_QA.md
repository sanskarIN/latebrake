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


## Dedicated 1.70-km continuous static collision — *licensed Unreal Editor lab only*

The 68 BlockAll overlap slabs were a useful source-backed prototype, **not a
professional physics surface**. A new, separately generated C++20 OBJ+MTL
handoff now captures the full 9.0-m physical road directly from the
closed manifold geometry: 1,024 stations, 4,096 source vertices,
8,192 consistently wound triangles, 42 cm thickness, and UV lap closure.
This is distinct from the **8.5-m visual-only ribbon**. Source-only
`WorldScaleCollisionRoadObjContract` independently rechecks every C++
vertex, all triangle connectivity and winding, UV continuity, material
bindings, and deliberate bad geometry/texture mutations across CTest.

**On your own licensed UE5.8 Windows workstation only**, check out the
experimental [draft PR #316](https://github.com/sanskarIN/latebrake/pull/316)
branch and run from the LateBrake repository root:

```powershell
pwsh ./scripts/New-ContinuousRoadLab.ps1 -EngineRoot "C:/Program Files/Epic Games/UE_5.8"
```

This wrapper fails before Unreal if the exact C++ numeric tests fail,
the engine version/project association is wrong, or an existing destination
asset/map would be overwritten. It passes the SHA256 of the **actual compiled
original OBJ and MTL bytes** into the UE5.8 Python process, which
independently validates all source vertices/faces again before any import.
The import targets only
`/Game/LatebrakeExperimental/SM_WorldScaleContinuousRoad`, with a distinct
non-racing GameModeBase map at
`/Game/Maps/LatebrakeWorldScaleContinuousCollisionLab`. It is never
`LatebrakeCircuit`, and does not use or rescale the physical car or AI.

The importer attempts a **static non-simulating** triangle mesh with
`BodySetup.collision_trace_flag = CTF_USE_COMPLEX_AS_SIMPLE`,
`BlockAll` and `QueryAndPhysics`. The script rejects importer-generated
alternate asset names, incorrect 1.7-km bounds, missing BodySetup, missing
physical triangles, lost collision configuration, map-save/reload failures
and tampered source/asset bytes. On actual success the **locally generated**
`Saved/LatebrakeAutomation/world-scale-continuous-road-import-result.json`
contains source OBJ/MTL SHA256, authentic `.uasset` and `.umap` hashes,
persisted collision settings, engine version and an explicit
`road_chaos_contact_tested: false` marker. The success receipt is **not**
created by portable source-only CI.

**What this does not prove:** The 5.8 Editor import may fail depending on
the installed OBJ/Interchange plugins and engine API. Portable mock tests
can verify fail-closed logic but cannot prove actual engine import, Chaos
cooking, dynamic tire collisions, suspension comfort, triangulation
performance, material appearance or gameplay. Inspect imported faces in
the Editor, confirm the map reloads, then test tyre/vehicle contact and
frame-time behavior with real physics. If an import fails after making a
partial `.uasset`, do not simply rerun over it; review the generated
asset/map and remove only the intentionally generated lab artifacts yourself
before retrying. **Do not merge into live GameMode or ship installers without
all of [playable gate #148](https://github.com/sanskarIN/latebrake/issues/148).**
The published v0.5.96 pre-release is an older immutable **SOURCE-ONLY**
snapshot and does not include this later experimental import work.


## Full-lap *real Editor collision query* QA — 2,322 source-derived probes

The earlier continuous-road import checks that Unreal can serialize a
static `.uasset` and dedicated lab map; that is **not evidence** the actual
unloaded/loaded scene will answer useful road-surface collision queries.
A new, separately gated **static query audit** checks an **existing
imported** continuous physical road without generating or overwriting any
asset. It uses actual `unreal.SystemLibrary.line_trace_single` calls in an
authorized Unreal **5.8 Editor world** (the Python mock suite is separate).

After **successfully completing** `New-ContinuousRoadLab.ps1` on your own
licensed UE5.8 workstation, from the repository root run:

```powershell
pwsh ./scripts/Audit-ContinuousRoadQueries.ps1 -EngineRoot "C:/Program Files/Epic Games/UE_5.8"
```

What the new workflow **requires**, rather than merely claiming:

- Generate a deterministic JSON probe plan using the *independent Python
  centreline model*, covering **256 stations across the complete 1.70-km
  lap** and **two additional stations just 0.25 cm either side of the
  closed loop seam**.
- At each of the 258 positions, trace vertically with **complex collision
  enabled** at nine fixed lateral offsets: centre, ±400 cm (inside the
  450-cm physical half-width), ±449 cm (**1 cm inside both asphalt edges**),
  ±451 cm (**1 cm outside both edges**), and ±550 cm (further off-road).
  Verify **1,290 hits on the authored static physical road** and **1,032 hits
  on the separately authored ground**, never an unrelated actor. This
  paired 2-cm rim check detects collision overshoot and missing edge
  contact that distant probes could not detect.
- Require surface heights at z=−1 cm (physical asphalt) and z=−37.5 cm
  (off-road ground) to be within **1.5 cm**, XY impact points within
  **2 cm** of the probe, and **upward normal Z ≥ 0.98**. Missed/incorrect
  actors, wrong normals, 90-m pavement, missing seam support and
  ground-as-road false positives must fail.
- Before loading any Editor map, authenticate the original imported asset
  and map using exact SHA256 from the real import receipt, the compiled
  C++ OBJ+MTL bytes, and a SHA256 of the independently validated probe
  JSON. After **all 2,322 actual queries**, verify hashes still match.
- If and only if every query passes, create a separate
  `Saved/LatebrakeAutomation/world-scale-continuous-ray-audit-result.json`
  receipt recording actual hit counts, maximum error, minimum normal-Z,
  source/assets/probe hashes and explicitly
  `dynamic_chaos_wheel_contact_tested: false`,
  `playable_game_tested: false`. Existing result or signed source
  probe files cannot silently be overwritten; review/move prior
  inspection evidence before repeating the audit.

The 2,322-point plan is also checked as a strict CTest
`WorldScaleCollisionRayProbePlan`; disposable fake-Editor tests verify
the audit **refuses** missing rays, changed assets, wrong normals/actor,
altered probes and already existing receipts. **Green CI only validates
the source code and mocks.** No actual licensed UE Editor run or
vehicle-contact simulation has happened in the hosted CI.

**Scope limit:** a static Editor line trace is *not* a drivable physics
test. It cannot verify dynamic Chaos rigid-body wheel/suspension
contact, vehicle slip, seam ride quality, loading under motion,
render quality, frame rate, cook/packaging, career gameplay or a
Win64 installer. The full playable gate remains
[issue #148](https://github.com/sanskarIN/latebrake/issues/148);
do not close it using the static-query receipt. The already-published
v0.5.96 source-only snapshot is immutable and does not contain this
newer unmerged PR #316 experiment.


## Portable C++ triangle-contact coverage (source only)

The earlier `WorldScaleSmoothCollisionSurface` test establishes mesh
manifold edges, face normals and 42-cm volume, while the Python OBJ
contract independently validates every imported-source triangle. A
separate `WorldScaleCollisionRayTriangleCoverage` CTest now intersects
actual downward vertical mathematical rays against the **original C++
physical candidate mesh triangles**, not just sampled centerline positions:

- At 256 full-lap stations plus 2 extra near the welded lap seam:
  **1,290 rays** at centre, ±400 cm and **±449 cm (1 cm within each
  edge)** must hit *both* z=−1 cm road top and z=−43 cm road underside.
  **1,032 rays** at **±451 cm (1 cm outside each edge)** and ±550 cm
  must hit **neither** physical-road face. The near-edge pairs
  strengthen the source-only geometric coverage of the 9-m corridor.
- An additional **3,072 source rays** at all 1,024 triangulation
  midpoints (0 and ±400 cm) require both top and underside hits.
  This catches gaps **between** sampled section vertices.
- The checks use an explicit independent barycentric 2-D
  ray/triangle intersection over the source mesh triangles. A
  zero-area horizontal triangle, missing top/bottom, or unintended
  off-road cover is an error, even if a mesh-exporter process itself
  returned success.

[Historical source CI run #42386650331](https://github.com/sanskarIN/latebrake/actions/runs/42386650331)
passed the **earlier 4,362-probe version** of this CTest on Windows,
Ubuntu and macOS, with **183/183 tests passed** on each runner. The
expanded **5,394-probe version** (2,322 station and 3,072 midpoint
rays) must pass its **own exact-head CI**; the historical run does not
qualify these new near-edge checks. Source triangle intersections are
a stronger geometric qualification than
simple manifold closure, but are **not** real
`unreal.SystemLibrary.line_trace_single` calls, physical Chaos
collision cooking, runtime rigid-body/suspension contacts, or a
playable Win64 racing game. Perform the separate 2,322-query
**genuine Editor** audit on a licensed Unreal 5.8 workstation before
treating the map as a viable static collision level; even an
Editor PASS leaves real vehicle-driving and tire-contact gates open.


## November 8: experimental full-lap braking and route progression reference

This UNMERGED source-only candidate uses 512 nodes sampled from the real
1.70-km rounded centreline, rather than the old 170m road. Curvature is
derived from adjacent actual route headings. A first-pass 850 cm/s²
lateral budget, 850 cm/s² braking limit and 380 cm/s² acceleration limit
generate a closed-loop target speed that anticipates all four bends.
No friction coefficient, vehicle test or real lap time has been measured.
These numbers are strictly candidate AI coaching parameters.

A separate O(512) polyline projection converts a hypothetical nearby
vehicle position into wrapped lap distance, signed lateral offset,
distance from centreline, geometric pace and physical-candidate-road
membership. The physical road Boolean is sourced from the existing
full-scale 9m union; projections and speeds NEVER award real gates,
car grip, saved records, motor forces or track eligibility.

Strict C++20 builds check 512 nodes, 8192 repeated-lap speed samples,
3072 straight/curve/lateral positions, false infield/malformed locations,
start/finish seam and per-segment braking/acceleration ceilings. The
existing GameMode, old rectangular track, original AI and all releases
remain unchanged. No licensed UE5.8 build or installed Win64 racer has
been executed. A real smooth road and cars, AI/controller integration,
Chaos tire tests and hardware handling are still required by issue #148.


### November 8 — full-lap target-speed continuity correction

The first portable C++ test exposed a discrete target lookup that took
the minimum of adjacent speed nodes. Exactly at a 512-node boundary,
floating-point repeated-lap inputs could select a different neighbor and
cause an abrupt speed discontinuity, despite the brake envelopes passing
at every stored node. The source candidate now interpolates **squared
speed** between successive bounded nodes, yielding continuous targets and
piecewise constant-distance acceleration. An explicit C++ regression
checks both sides of ALL 512 knots including start/finish, and the original
8192 repeated-lap samples. This is a mathematical source correction, not
proof of real vehicle handling or Chaos tire adhesion.


## November 8 — candidate physics-aware rival driver (isolated source)

The new Source/Latebrake/Public/AI/WorldScaleCandidateRivalPilot.h
consumes measured physical XY, signed forward chassis velocity, flat speed,
body yaw, source physics-readiness and post-GO eligibility. It projects
the car onto the genuine candidate 1.70km road, anticipates a lookahead
centreline point, and issues bounded steering, throttle and braking
advice using the independent 512-station continuous pace model.

The pilot refuses throttle on a road departure, reverse motion, severe
misalignment, invalid/paused/finished sample, overlong hitch, malformed
physics or discontinuous car/route movement. It detects implausible
teleports between valid frames and reacquires separately, and handles
the real start/finish seam without inventing progress or rewards.
30/60/120 FPS deterministic forward-travel samples, left/right
cross-track steering, overspeed, wrong-way, reversal, invalid sample,
10.5-km/h movement discontinuity and 1.7km seam checks are in
the strict portable CTest executable.

This is a candidate mathematical driver, not a real UE AI pawn, tire
contact, optimized production per-tick search, wheel/road simulation
or genuinely driven lap. Neither the original Unreal AI nor GameMode
instantiates it: actual activation requires the approved map, physical
car, full checkpoint/recovery integration and installed Win64 QA.

A zero-speed rival facing more than 2.2 radians away from its required
route is now denied throttle as well as an already-moving wrong-way car.
The controller cannot automatically right a stationary wrong-way car;
that remains an Unreal AI recovery task, not a guessed steering success.
