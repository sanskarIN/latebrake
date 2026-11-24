#pragma once
#include "Vehicles/VehicleRearLampLayout.h"
#include "Vehicles/OriginalVehicleRenderSections.h"

#include <array>
#include <cmath>

namespace latebrake
{
// The existing three rear point lights attach to the Chaos chassis and
// have different correct mount positions for the 190cm legacy cube and
// the 222cm original 3D body. Light coordinates are intentionally
// render-only and must never affect brake/drive/control forces.
inline constexpr std::array<VehicleRearLampOffset,3>
PlanRearSignalVisualOffsets(bool originalSurfaceCommitted) noexcept
{
    if(!originalSurfaceCommitted)
        return VehicleRearLampOffsets;
    constexpr vehiclemesh::OriginalVehicleMeshSettings source{};
    const double bumper=-source.LengthCm*0.5;
    const double y=source.HalfWidthCm*0.66;
    constexpr double z=22.0;
    return {{
        {bumper-4.0,-y,z},
        {bumper-4.0,+y,z},
        {bumper-6.0,0.0,z}
    }};
}
inline constexpr bool RearSignalNearVisibleBumper(
    bool originalSurfaceCommitted,
    VehicleRearLampOffset offset) noexcept
{
    const auto source=vehiclemesh::OriginalVehicleMeshSettings{};
    const double bumper=originalSurfaceCommitted
        ? -source.LengthCm*0.5 : -VehicleChassisHalfLengthCm;
    const double halfWidth=originalSurfaceCommitted
        ? source.HalfWidthCm : VehicleChassisHalfWidthCm;
    const double beyond=bumper-offset.X;
    return beyond>=2.0 && beyond<=12.0 &&
        offset.Y>=-halfWidth+12.0 &&
        offset.Y<=+halfWidth-12.0 &&
        offset.Z>=15.0 && offset.Z<=VehicleChassisHalfHeightCm;
}
static_assert(PlanRearSignalVisualOffsets(false)[0].X==-194.0);
static_assert(PlanRearSignalVisualOffsets(true)[0].X==-226.0);
static_assert(PlanRearSignalVisualOffsets(true)[2].X==-228.0);
static_assert(RearSignalNearVisibleBumper(
    true,PlanRearSignalVisualOffsets(true)[0]));
static_assert(RearSignalNearVisibleBumper(
    false,PlanRearSignalVisualOffsets(false)[0]));
} // namespace latebrake
