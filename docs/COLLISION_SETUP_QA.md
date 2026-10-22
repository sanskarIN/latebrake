# LateBrake v0.5.7 — Collision Actor Configuration and Spawn Safety

**DEVELOPMENT ONLY: no v0.5.7 pre-release or verified playable executable.**

## C++ source files

`Source/Latebrake/Public/Track/CollisionBlockSetup.h` is a tiny,
allocation-free engine-neutral decision classifier. A spawn succeeds
only when the source collision plan is valid and the real Unreal Actor
has the expected StaticMeshComponent, built-in Cube, explicitly
configured `QueryAndPhysics` collision and requested transform.

`Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp` feeds those
facts into the classifier after actually configuring each Actor. A
failed component/setup is logged and its Actor destroyed, so the
v0.5.6 `CircuitStartupAudit` cannot count a merely returned
`AStaticMeshActor*` as a complete collidable track piece.

`Source/Latebrake/Public/Track/CircuitSpawnSafety.h` runs a
deterministic pavement and barrier clearance policy over the actual
shared `PlayerStart`, `RivalStart`, `PlayerRecoveries` and
`RivalRecoveries`. It assumes the stock UE cube has a
100-centimetre side at scale 1, uses 150 cm conservative XY barrier
clearance, and requires finite position/heading values.
Its result gates startup as `LATEBRAKE_SPAWN_LAYOUT_BLOCKED`
if future source geometry makes planned starts unsafe.

`Tests/Standalone/CollisionBlockSetupTests.cpp` and
`Tests/Standalone/CircuitSpawnSafetyTests.cpp` are strict C++20
regressions; `scripts/Test-PackagingPolicy.ps1` additionally rejects
synthetic logs containing either new failure marker.

## Limits and real acceptance

This is **source-level policy**. Valid Unreal component property values
still do not guarantee actual physics, cooked map availability,
correct tire/chassis interactions, real game progression or performance.

- Compile UE 5.8 UHT/editor/game targets on an authorized machine.
- Create a genuine `/Game/Maps/LatebrakeCircuit.umap`, then cook it.
- Inspect all nine source-created collision Actor components in-engine.
- Verify the real player car can drive each road, collide with
  boundary barriers and restart/recover without overlapping an obstacle.
- Check the same against player/rival changes, 30/60 FPS, artificial
  hitches and actual Android input interruptions.
- Capture authentic process logs and package manifests with SHA256
  hashes and device-specific playtest observations.
- Keep existing Unreal and Android acceptance issues open until those
  tasks are complete. A passed CTest or synthetic PowerShell fixture
  must not be attached as proof of a playable game.
