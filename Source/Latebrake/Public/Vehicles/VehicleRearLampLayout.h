#pragma once

#include <array>

namespace latebrake
{
// Actual stock Unreal Cube is 100cm on each axis. All six garage
// silhouettes have the same physical chassis root scale.
inline constexpr double VehicleRootScaleX = 3.8;
inline constexpr double VehicleRootScaleY = 1.8;
inline constexpr double VehicleRootScaleZ = 0.65;
inline constexpr double VehicleChassisHalfLengthCm = 50.0 * VehicleRootScaleX;
inline constexpr double VehicleChassisHalfWidthCm = 50.0 * VehicleRootScaleY;
inline constexpr double VehicleChassisHalfHeightCm = 50.0 * VehicleRootScaleZ;

struct VehicleRearLampOffset final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

// Car-relative centimetres BEFORE dividing by the nonuniform parent
// scale. Order: left red, right red, centre white reverse. They are
// 4-6cm behind the true -190cm bumper, not 195-200cm detached.
inline constexpr std::array<VehicleRearLampOffset, 3>
    VehicleRearLampOffsets{{
        {-194.0, -64.0, 22.0},
        {-194.0,  64.0, 22.0},
        {-196.0,   0.0, 22.0}
    }};
inline constexpr double VehicleRearLampRadiusCm = 170.0;

constexpr bool IsRearLampNearPhysicalBumper(VehicleRearLampOffset lamp) noexcept
{
    const double beyondBumperCm = -VehicleChassisHalfLengthCm - lamp.X;
    return beyondBumperCm >= 2.0 && beyondBumperCm <= 12.0 &&
           beyondBumperCm < VehicleRearLampRadiusCm &&
           lamp.Y >= -VehicleChassisHalfWidthCm + 12.0 &&
           lamp.Y <=  VehicleChassisHalfWidthCm - 12.0 &&
           lamp.Z >= 15.0 && lamp.Z <= VehicleChassisHalfHeightCm;
}
static_assert(VehicleChassisHalfLengthCm == 190.0);
static_assert(VehicleChassisHalfWidthCm == 90.0);
static_assert(VehicleChassisHalfHeightCm == 32.5);
static_assert(IsRearLampNearPhysicalBumper(VehicleRearLampOffsets[0]));
static_assert(IsRearLampNearPhysicalBumper(VehicleRearLampOffsets[1]));
static_assert(IsRearLampNearPhysicalBumper(VehicleRearLampOffsets[2]));
static_assert(VehicleRearLampOffsets[0].X == VehicleRearLampOffsets[1].X);
static_assert(VehicleRearLampOffsets[0].Y == -VehicleRearLampOffsets[1].Y);
static_assert(VehicleRearLampOffsets[0].Z == VehicleRearLampOffsets[2].Z);
} // namespace latebrake
