#pragma once

class UProceduralMeshComponent;
class UStaticMeshComponent;

namespace latebrake::vehiclemesh
{
// Editor-preview opt-in only. No collision/physics/lap/race authority.
// A caller must explicitly show the preview and hide the legacy visuals
// only after this routine returns true.
bool CreateOriginalVehicleVisualSections(
    UProceduralMeshComponent* preview,
    UStaticMeshComponent* originalChassis,
    int vehicleId);
}
