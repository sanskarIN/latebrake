# v0.5.8 — Track collision response policy

**Published source-only GitHub pre-release. No verified playable Win64 EXE or Android APK.**

## The gap

The v0.5.7 `SpawnBlock` validates the engine Cube mesh and
`ECollisionEnabled::QueryAndPhysics` state. A collision component
might nevertheless report `ECR_Ignore` or `ECR_Overlap` for the
vehicle's relevant channels. A returned `AStaticMeshActor*` alone
does not prove physics or even the intended response configuration.

## C++ implementation

- `Source/Latebrake/Public/Track/CollisionResponsePolicy.h` is an
  allocation-free engine-neutral policy. It requires `BlocksPawn`,
  `BlocksPhysicsBody`, and `BlocksWorldDynamic` to all be true;
  rejects the first missing channel with a stable diagnostic name.
- `Source/Latebrake/Private/Game/LatebrakeRaceGameMode.cpp`
  explicitly includes owning engine headers and reads
  `Mesh->GetCollisionResponseToChannel(ECC_Pawn)`,
  `ECC_PhysicsBody` and `ECC_WorldDynamic` on the configured
  `UStaticMeshComponent` after setting `BlockAll`.
- On a nonblocking response the failed Actor is destroyed, a
  `LATEBRAKE_COLLISION_SETUP_FAILED` log line is emitted, and
  the v0.5.6 nine-block startup audit refuses ready/race progress.
- `Tests/Standalone/CollisionResponsePolicyTests.cpp` verifies
  the policy and diagnostic classification in portable C++20.

## Still required before actual gameplay acceptance

Compile the real licensed Unreal Engine 5.8 UHT/editor/game C++
targets on the engine-supported Windows environment, author and cook
a genuine `/Game/Maps/LatebrakeCircuit` map, then build and install
real Win64/Android game packages. Inspect components *in the engine*,
confirm all three channels actually Block, and drive into every
barrier/along each road to observe real car contact and input behavior.
Capture genuine source SHA, package hashes, device, Unreal logs and
framerate/physics outcomes.

A passing `CollisionResponsePolicy` CTest only proves that the
portable boolean policy correctly rejects supplied false values.
It does not establish that physical collisions or real gameplay work.
