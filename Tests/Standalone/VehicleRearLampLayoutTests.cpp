#include "Vehicles/VehicleRearLampLayout.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool result, const char* message)
{
    if (!result)
    {
        std::cerr << "VehicleRearLampLayout FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a-b) < 1e-9;
}
}
int main()
{
    using namespace latebrake;
    Require(Near(VehicleChassisHalfLengthCm, 190.0) &&
            Near(VehicleChassisHalfWidthCm, 90.0) &&
            Near(VehicleChassisHalfHeightCm, 32.5),
            "real scaled stock-Cube half extents");
    Require(-VehicleChassisHalfLengthCm - (-385.0) >
                VehicleRearLampRadiusCm &&
            -VehicleChassisHalfLengthCm - (-390.0) >
                VehicleRearLampRadiusCm,
            "original point lights cannot illuminate the real bumper");
    Require(VehicleRearLampOffsets.size() == 3,
            "two red rear and one reverse lamp");
    for (auto lamp : VehicleRearLampOffsets)
    {
        const double rearClearance =
            -VehicleChassisHalfLengthCm - lamp.X;
        Require(IsRearLampNearPhysicalBumper(lamp) &&
                rearClearance >= 2.0 &&
                rearClearance <= 12.0 &&
                rearClearance < VehicleRearLampRadiusCm,
                "all new positions 4-6cm from actual rear shell");
        Require(Near(lamp.X / VehicleRootScaleX * VehicleRootScaleX, lamp.X) &&
                Near(lamp.Y / VehicleRootScaleY * VehicleRootScaleY, lamp.Y) &&
                Near(lamp.Z / VehicleRootScaleZ * VehicleRootScaleZ, lamp.Z),
                "parent-scale compensation preserves world centimetres");
    }
    const auto l=VehicleRearLampOffsets[0];
    const auto r=VehicleRearLampOffsets[1];
    const auto w=VehicleRearLampOffsets[2];
    Require(l.Y < 0.0 && r.Y > 0.0 &&
            Near(l.X, r.X) && Near(l.Y, -r.Y) &&
            Near(l.Z, r.Z) && Near(w.Y, 0.0) &&
            w.X < l.X,
            "symmetric red pair and centred reverse lamp");
    for (auto invalid : {
        VehicleRearLampOffset{-385.0,-115.0,22.0},
        VehicleRearLampOffset{-390.0,0.0,22.0},
        VehicleRearLampOffset{-180.0,0.0,22.0},
        VehicleRearLampOffset{-194.0,85.0,22.0},
        VehicleRearLampOffset{-194.0,64.0,200.0}
    })
        Require(!IsRearLampNearPhysicalBumper(invalid),
                "detached, buried, off-width or elevated lamp refused");
    std::cout << "Real scaled chassis rear-lamp layout PASS.\n";
    return EXIT_SUCCESS;
}
