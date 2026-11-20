#pragma once

class UWorld;
class UStaticMesh;

namespace latebrake::trackskin
{
// Non-shipping, opt-in original asphalt visual overlay. This cannot replace
// the existing collision-bearing track or certify the native UE cook.
bool SpawnOriginalAsphaltSkinPreview(UWorld* world,UStaticMesh* sourceMesh);
}
