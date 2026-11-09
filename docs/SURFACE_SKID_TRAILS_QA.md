# Contact-driven skid trails — source candidate

This feature is cosmetic source on a separate draft branch after the frozen
v0.5.98 wheel-brake candidate. It has not been compiled by real UE5.8/UHT,
rendered in the licensed Editor, cooked or verified on installed Win64.
It does not change friction, collision, tire wear, rewards or race timing.

## Implemented behavior

The four existing WorldStatic wheel rays supply actual impact points,
surface normals, component identities and road-point velocities. Only the
four original asphalt collision actors receive `LatebrakePavedRoad`; grass,
barriers and cosmetic geometry do not. A positive accepted spring load and
tagged paved hit are both required. No extra trace or invented road point
is introduced by this visual path.

Lateral chassis-point motion and, when available, committed modeled brake
slip determine a visual intensity. This is an appearance estimate, not a
measured tire sensor. Quiet rolling, unsupported/low-load corners and low
speed do not stamp. The independent rotor model is not required for a
lateral skid; the legacy A/B path can still show real sideways motion.

Strips fit a road-tangent orthonormal basis to measured endpoints and
normals. Spacing is by distance, with at most four pieces per wheel and
sixteen per frame. Large jumps, height discontinuities, changed surfaces,
teleport revisions, missing support and interrupted frames break the
continuity. Dividing a long movement cannot disguise an unsupported road
height jump. Grade/bank fit is still an approximation between samples.

One non-replicated actor per car owns a movable, non-colliding instanced
mesh pool. It is not attached to the moving car. World transforms stay at
the road contact; strips do not follow the chassis. A fixed 2,048-slot ring
reuses indices without removal/compaction or continued instance growth.
Generation identities prevent old retirement work from hiding a reused
slot. Expiry examines at most 64 slots per accepted game frame.

Marks have a twelve-second accepted-game-time lifetime plus at most one
scan cycle: up to 3.2 further seconds at 100ms frames. Pause freezes that
clock. Restart clears the pool; recovery breaks continuity while preserving
existing world marks. This is hard retirement, not opacity fading or a
persistent physical rubber/grip simulation.

## Original material and package path

`scripts/Unreal/CreateTireRubberMaterial.py` authors an original dark,
rough material through genuine UE5.8 material graph APIs at
`/Game/Materials/M_LatebrakeTireRubber`. It enables instanced-mesh usage,
checks graph connections and material compiler errors, then asks Unreal to
save the asset. It preserves an existing user material rather than silently
overwriting its graph. No synthetic `.uasset` bytes or downloaded textures
are supplied.

Run this optional authoring script in the actual licensed Editor, inspect
its saved material and commit the genuine project-owned `.uasset` before
using the trusted candidate-build pipeline. The existing source-provenance
gate covers that tracked asset; its sole untracked-map exception stays
unchanged. The starter-map receipt continues to prove only map saving,
not material success. The native skid renderer checks the real asset and
disables itself when the material or mesh is absent; there is no white-strip
fallback presented as finished rubber. No material binary is supplied or
claimed verified by this source-only change.

The project packaging configuration includes `/Game/Materials`. In the
licensed Editor, inspect the material, shader usage, actual saved package
and cooked reference. A successful Python process exit or portable stub
test cannot establish that those assets were authored or cooked.

The native instance operations follow Epic's UE5.8 [AddInstance](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent/AddInstance)
and [instance-transform](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Components/InstancedStaticMesh/UpdateInstanceTransform)
APIs. Material authoring follows the [UE5.8 MaterialEditingLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary).
Documentation review is not a successful engine compilation.

## Required real-engine acceptance

1. Compile the exact branch with UE5.8 Editor/UHT/Game. Author the material
   with the real Editor; inspect saved graph, shader compilation and ISM
   material usage. Confirm the missing-material path disables only visuals.
2. At 30/60/120 FPS, compare normal rolling, loaded lateral skids, service
   braking and rear mechanical locking on every car and both modes. Confirm
   individual marks follow the accepted road contact and correct wheel side.
3. Drive over slopes/banks and original road seams. Check strip basis,
   surface lift, clipping, shadow/reflection cost and absence of floating
   bridges. Test crest landing, one-corner support, grass and barriers.
4. Test pause/focus loss, recovery, restart, car changes and actor teardown.
   Verify no cross-teleport streaks, moving world marks, stale pooled indices,
   unbounded memory or lifetime work after many laps.
5. Profile multi-car CPU/render/GPU cost at a full pool, high speed and
   100ms frames. Tune from actual hardware evidence. Cook/install Win64 and
   confirm the real material package is included and renders correctly.

Portable checks cover independent plane geometry, 100,000 plane fixtures,
100,000 pool reference frames, lifetime/generation/teleport rules, FPS length
coverage and synthetic Editor failure decisions. They cannot certify visual
quality, native API compatibility, cooked assets or installed gameplay.
