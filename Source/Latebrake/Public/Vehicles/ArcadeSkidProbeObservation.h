#pragma once

#include "Vehicles/SurfaceSkidTrail.h"

namespace latebrake
{
// A verified original asphalt component plus one actual ray hit and
// measured point velocity. The portable adapter intentionally cannot
// invent spring force, tire load, wheel slip, heat or physical friction.
struct ArcadeSkidProbeObservation final
{
    bool VerifiedAsphaltComponent=false;
    int RoadBlock=-1;
    std::uint64_t ComponentIdentity=0;
    ChassisGroundProbe Ground{};
    SkidVector HitPosition{};
    SkidVector HitNormal{};
    double ForwardCmPerSecond=0.0;
    double LateralCmPerSecond=0.0;
};

inline SurfaceSkidContact MakeArcadeSkidProbeContact(
    const ArcadeSkidProbeObservation& sample) noexcept
{
    SurfaceSkidContact result;
    if (!sample.VerifiedAsphaltComponent || sample.ComponentIdentity==0 ||
        !sample.Ground.Hit || !std::isfinite(sample.Ground.DistanceCm) ||
        sample.Ground.DistanceCm<0.0 ||
        sample.Ground.DistanceCm>ChassisContactMaximumSupportedHeightCm ||
        !std::isfinite(sample.Ground.GroundNormalZ) ||
        std::abs(sample.Ground.GroundNormalZ-sample.HitNormal.Z)>1.0e-7 ||
        !std::isfinite(sample.ForwardCmPerSecond) ||
        std::abs(sample.ForwardCmPerSecond)>15000.0 ||
        !std::isfinite(sample.LateralCmPerSecond) ||
        std::abs(sample.LateralCmPerSecond)>15000.0 ||
        !IsArcadeSkidRoadPoint(sample.RoadBlock,sample.HitPosition,sample.HitNormal))
        return result;

    result.Supported=true;
    result.PavedRoad=true;
    result.SurfaceKey=sample.ComponentIdentity;
    result.Position=sample.HitPosition;
    result.Normal=sample.HitNormal;
    result.ForwardCmPerSecond=sample.ForwardCmPerSecond;
    result.LateralCmPerSecond=sample.LateralCmPerSecond;
    result.Evidence=SkidContactEvidence::ArcadeGroundProbe;
    result.ArcadeRoadBlock=sample.RoadBlock;
    result.ProbeDistanceCm=sample.Ground.DistanceCm;
    // NormalLoadNewtons and ModeledBrakingSlip stay exactly zero.
    return result;
}
} // namespace latebrake
