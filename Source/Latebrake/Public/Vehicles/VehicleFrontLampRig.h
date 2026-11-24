#pragma once
#include "Vehicles/VehicleRearLampLayout.h"
#include "Vehicles/OriginalVehicleRenderSections.h"
#include <array>

namespace latebrake
{
// World-relative centimetre positions are applied AFTER division by the
// nonuniform physical-root scale; lamps do not own collision or controls.
// The source-generated six-car body is longer than the fallback Cube and
// has a different headlight Z origin. Both share the exact chassis actor.
struct FrontLampPose final
{
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
};

struct FrontLampRig final
{
    std::array<FrontLampPose,2> Mounts{};
    double PitchDegrees=-3.0;
    double BeamInnerDegrees=12.0;
    double BeamOuterDegrees=24.0;
    double AttenuationRadiusCm=7500.0;
    double ActiveIntensity=1100.0;
    double OutputIntensity=0.0;
    bool OriginalSurface=false;
};

// Always-on driving lamps are visible only during an accepted race-drive
// state. Paused/menu/focus-interrupted sessions immediately set output zero
// using the *same* guard as the existing brake and reverse signals.
// No headlight intensity feeds physics, score, race timing or rewards.
inline constexpr FrontLampRig PlanFrontLampRig(
    bool originalSurfaceReady, bool drivingAllowed) noexcept
{
    constexpr vehiclemesh::OriginalVehicleMeshSettings Source{};
    const double x=originalSurfaceReady
        ? 0.5*Source.LengthCm+4.0
        : VehicleChassisHalfLengthCm+4.0;
    const double y=originalSurfaceReady
        ? 0.66*Source.HalfWidthCm : 64.0;
    const double z=originalSurfaceReady
        ? 85.0+vehiclemesh::OriginalVehicleVisualOffsetZCm
        : 23.0;
    FrontLampRig rig{};
    // Explicit element assignments are portable in constexpr C++20 and
    // avoid aggregate-brace differences between Clang, MSVC and GCC.
    rig.Mounts[0]={x,-y,z};
    rig.Mounts[1]={x,y,z};
    rig.OutputIntensity=drivingAllowed?rig.ActiveIntensity:0.0;
    rig.OriginalSurface=originalSurfaceReady;
    return rig;
}

constexpr bool FrontLampPoseIsSafe(FrontLampPose p) noexcept
{
    return p.Xcm>=VehicleChassisHalfLengthCm+2.0 &&
           p.Xcm<=300.0 &&
           p.Ycm>=-VehicleChassisHalfWidthCm &&
           p.Ycm<=VehicleChassisHalfWidthCm &&
           p.Zcm>=12.0 && p.Zcm<=50.0;
}
static_assert(FrontLampPoseIsSafe(PlanFrontLampRig(false,true).Mounts[0]));
static_assert(FrontLampPoseIsSafe(PlanFrontLampRig(true,true).Mounts[0]));
static_assert(PlanFrontLampRig(false,false).OutputIntensity==0.0);
static_assert(PlanFrontLampRig(true,true).OutputIntensity>0.0);
} // namespace latebrake
