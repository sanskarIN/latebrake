// Original authored asphalt/kerb/barrier and six-car body geometry are
// engine-neutral CPU models, not a cooked or rendered UE scene. Keep all
// four independent namespaces composable in one strict C++20 translation unit.
#include "Track/OriginalArmcoBarrierMesh.h"
#include "Track/OriginalPhysicalAsphaltSkin.h"
#include "Track/OriginalCircuitKerbMesh.h"
#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <iostream>
int main()
{
    std::cout << "Four original portable road/vehicle mesh models compiled.\n";
    return 0;
}
