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
