# Original six-car Unreal visual-surface runtime — DRAFT QA

**Status: source candidate / unverified 3D rendering; no cooked Win64 build.** LateBrake previously
had six fully deterministic original C++ surface meshes
(`OriginalVehicleSurfaceMesh.h`), with body shape, windows, four wheels,
brake rotors, lamps, intake, carbon aero, normal and UV data, but the live
Unreal Pawn still rendered only basic engine cubes/cylinders. This work
attempts to connect those **existing six tested mesh generators** to
the actual Pawn via `UProceduralMeshComponent`.

## Rendering contract

- The `ProceduralMeshComponent` engine plugin is enabled in
  `Latebrake.uproject` and its module is declared in Build.cs.
- The component is a visual-only child of the SAME Physics-Chaos
  `Chassis` used for gameplay. Its scale compensates the chassis root
  shape; it does not replace or resize that collision body.
- A portable validated `OriginalVehicleRenderSections` plan partitions
  all original triangles into the ten existing source material categories.
  Unreal builds one mesh section per category with **bCreateCollision=false**.
  No generated render mesh may simulate physics or trigger overlaps.
- The current source candidate uses the engine's
  `/Engine/BasicShapes/BasicShapeMaterial` and one dynamic
  material per section to apply body, accent, glass, tire, rims, lamps
  and trim colors. It is an early render material, **not authored PBR
  automotive paint**. The real material's `Color` parameter and normal
  lighting response must be inspected in the exact UE5.8 installation.
- The old cube/cylinder geometry is hidden **only if** the source
  mesh validates, the engine base Material loads, all nonempty sections
  create their tintable instances, and the renderer is ready. Any such
  failure restores the old visible legacy fallback. It must never
  leave the player invisible because an optional material is missing.
- Only BeginPlay or a change to the *equipped* immutable vehicle ID
  rebuilds 3D geometry; paint/stripe/rim updates reuse existing
  dynamic materials and the still-available attached stripe/rim controls.
  All saved GarageAppearance data and original ownership IDs are intact.

## Real engine acceptance (not yet performed)

1. On a licensed **UE5.8 real Windows** workstation, run UnrealHeaderTool (UHT)
   and build the full Editor for the protected source candidate. Confirm
   `ProceduralMeshComponent` exists, loads and cooks on target Windows.
2. Launch the real Editor circuit. Compare the actual six generated
   meshes across 720p/1080p/1440p, including roof, windows, tires,
   lamps, wing geometry and correct orientation/height. Capture
   real screenshots; source topology tests cannot prove material quality.
3. Change all six equipped owned cars and all eight paint options,
   three stripe styles and three rim styles. Confirm each material slot
   is visibly tinted and no false duplicate cube body is visible.
4. Disable or remove the expected base material on a disposable
   workstation copy, restart and verify the old cube and cylinders
   remain visible as the **fallback**, with unchanged physical chassis
   collision. Restore the material and inspect both modes.
5. Verify no collision, braking, ground contact, driver body mass,
   camera view, race clock, scoring, saved credits, old save migration
   or competitor AI behavior changed. Inspect low-end GPU performance,
   number of material sections and CPU spikes when switching cars.
6. Cook and package Win64 on the actual machine, drive the car in a
   real Windows playtest, inspect material and lighting under day/night
   circuits, check all controls and capture true frame-time evidence.
   Do not publish an Unreal pre-release without these observations.

The old no-`Content/` source-only repository still lacks shipped
professional material assets, 3D map authoring artifacts, sound and
art-direction QA. A procedurally generated mesh is a substantial
geometry step, **not proof of a polished or release-ready game**.

## Roof stripe preservation on the generated bodies

The source-generated opaque body includes normal and glazing surfaces,
but its triangular section generator does not yet author per-car racing
stripes. Consequently the original cosmetic roof stripes remain distinct
small collision-free attached meshes, while the coarse cube body, old
wheels and older aerodynamic cubes are hidden. Their height is raised
to the original generator's 174cm roof minus the fixed -62cm visual
offset, with a 2cm surface clearance. On native UE5.8 inspect paint
and twin-stripe placement for all six roof shapes; physically textured
PBR paint and correct roof curvature remain separate art tasks.
