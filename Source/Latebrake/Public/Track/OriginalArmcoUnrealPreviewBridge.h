#pragma once
class UWorld;
class UStaticMesh;

namespace latebrake::armco
{
// Only an explicitly requested non-shipping render preview. No new Chaos
// collision and no implication of native build or installed-game validation.
bool SpawnOriginalArmcoPreview(
    UWorld* world,UStaticMesh* sourceMesh);
}
