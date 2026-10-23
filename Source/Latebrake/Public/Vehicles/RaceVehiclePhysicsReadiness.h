#pragma once

#include <cmath>

namespace latebrake
{
// Engine-independent snapshot of the local car required for meaningful race
// time and checkpoint scoring. The Unreal wrapper gathers these flags from
// the live chassis; this function does not assert actual road contact or
// independently certify physics gameplay on hardware.
struct RaceVehiclePhysicsSnapshot final
{
    bool ChassisPresent = false;
    bool MeshPresent = false;
    bool SimulatingPhysics = false;
    bool QueryAndPhysicsCollision = false;
    bool BlocksWorldStatic = false;

    double PositionX = 0.0;
    double PositionY = 0.0;
    double PositionZ = 0.0;
    double VelocityX = 0.0;
    double VelocityY = 0.0;
    double VelocityZ = 0.0;
};

inline bool IsRaceVehiclePhysicsReady(
    const RaceVehiclePhysicsSnapshot& vehicle) noexcept
{
    return vehicle.ChassisPresent &&
           vehicle.MeshPresent &&
           vehicle.SimulatingPhysics &&
           vehicle.QueryAndPhysicsCollision &&
           vehicle.BlocksWorldStatic &&
           std::isfinite(vehicle.PositionX) &&
           std::isfinite(vehicle.PositionY) &&
           std::isfinite(vehicle.PositionZ) &&
           std::isfinite(vehicle.VelocityX) &&
           std::isfinite(vehicle.VelocityY) &&
           std::isfinite(vehicle.VelocityZ);
}
} // namespace latebrake
