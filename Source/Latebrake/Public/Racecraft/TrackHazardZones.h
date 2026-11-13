#pragma once
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/RaceMarshalFlags.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class PhysicalTrackHazard : std::uint8_t
{
    Unknown, ChassisDebris, TireRubber, OilSpill,
    FuelSpill, StandingWater, LooseGravel,
    DisabledVehicle, FallenBarrier, EmergencyPersonnel
};
struct TrackHazardSettings final
{
    double MaximumHazardLengthCm=1600.0;
    double MaximumHazardHalfWidthCm=450.0;
    double MinimumHazardLengthCm=20.0;
    double MaximumSensorAgeSeconds=25.0;
    double MaximumHazardLifetimeSeconds=900.0;
    double MinimumHazardConfidence=0.75;
    double MinimumPhysicalDebrisMassKg=0.01;
    double MaximumPhysicalDebrisMassKg=5000.0;
    double MaximumRoadLateralCm=OriginalRoadHalfWidthCm;
    double MinimumApproachDistanceCm=250.0;
    double MaximumApproachDistanceCm=3500.0;
    double MaximumSlowdownFraction=0.80;
    double MaximumBrakeFraction=0.75;
    double DryRubberSpeedFraction=0.94;
    double WetOilSpeedFraction=0.42;
    double DisabledVehicleSpeedFraction=0.36;
    double PersonnelSpeedFraction=0.20;
    double MaximumFrameSeconds=0.10;
    int MaximumActiveHazards=24;
    bool RequireOriginalPhysicalObstacle=true;
    bool RequireTrackCollisionSensor=true;
    bool RejectUnverifiedDebrisIdentity=true;
    bool NeverCreatePhysicalObstacles=true;
};
struct TrackHazardObservation final
{
    PhysicalTrackHazard Kind=PhysicalTrackHazard::Unknown;
    Vec2 PositionCm{};
    Vec2 MeasuredVelocityCmS{};
    double TrackDistanceCm=0.0;
    double LateralOffsetCm=0.0;
    double HalfLengthCm=100.0;
    double HalfWidthCm=70.0;
    double MassKg=1.0;
    double Confidence=0.0;
    double ObservedSeconds=0.0;
    double ExpirationSeconds=0.0;
    std::uint64_t Session=0;
    std::uint64_t SourceActor=0;
    std::uint64_t ObstacleToken=0;
    std::uint64_t FrameOrdinal=0;
    bool EngineCollisionObject=true;
    bool OriginalMapVerified=true;
    bool ActualPhysicsObservation=false;
    bool VerifiedContactRisk=false;
};
struct TrackHazardDirective final
{
    PhysicalTrackHazard Kind=PhysicalTrackHazard::Unknown;
    double DistanceAheadCm=0.0;
    double LateralSeparationCm=0.0;
    double RecommendedSpeedCeilingFraction=1.0;
    double RecommendedThrottleCeiling=1.0;
    double RecommendedBrakeFloor=0.0;
    double TimeToHazardSeconds=0.0;
    double HazardRoadCoverageFraction=0.0;
    double HazardConfidence=0.0;
    std::uint64_t ObstacleToken=0;
    int ObservedHazards=0;
    int OverlappingLaneHazards=0;
    bool RequiresPhysicalBrake=false;
    bool ClearEscapeLaneIdentified=false;
    bool HasActualEngineAuthority=false;
    bool Valid=false;
};
struct TrackHazardInventory final
{
    std::array<TrackHazardObservation,24> Hazards{};
    int Count=0;
    std::uint64_t Session=0;
    std::uint64_t LastFrameOrdinal=0;
    int Observed=0;
    int Rejected=0;
    int Expired=0;
    int Updated=0;
    double LastClockSeconds=0.0;
    bool AnyEngineHazardsCreated=false;
};
inline bool ValidHazardSettings(TrackHazardSettings p) noexcept
{
    return Finite(p.MaximumHazardLengthCm) &&
        p.MaximumHazardLengthCm>0.0 &&
        Finite(p.MaximumHazardHalfWidthCm) &&
        p.MaximumHazardHalfWidthCm>0.0 &&
        Finite(p.MinimumHazardLengthCm) &&
        p.MinimumHazardLengthCm>0.0 &&
        Finite(p.MaximumSensorAgeSeconds) &&
        p.MaximumSensorAgeSeconds>0.0 &&
        Finite(p.MaximumHazardLifetimeSeconds) &&
        p.MaximumHazardLifetimeSeconds>0.0 &&
        Finite(p.MinimumHazardConfidence) &&
        p.MinimumHazardConfidence>0.0 &&
        p.MinimumHazardConfidence<=1.0 &&
        Finite(p.MinimumPhysicalDebrisMassKg) &&
        p.MinimumPhysicalDebrisMassKg>=0.0 &&
        Finite(p.MaximumPhysicalDebrisMassKg) &&
        p.MaximumPhysicalDebrisMassKg>
            p.MinimumPhysicalDebrisMassKg &&
        Finite(p.MaximumApproachDistanceCm) &&
        p.MaximumApproachDistanceCm>
            p.MinimumApproachDistanceCm &&
        Finite(p.MaximumSlowdownFraction) &&
        p.MaximumSlowdownFraction>0.0 &&
        p.MaximumSlowdownFraction<=1.0 &&
        p.MaximumActiveHazards>=1 &&
        p.MaximumActiveHazards<=24;
}
inline bool ValidTrackHazardObservation(
    TrackHazardObservation h,TrackHazardSettings p) noexcept
{
    return h.Kind!=PhysicalTrackHazard::Unknown &&
        h.Session!=0 &&h.SourceActor!=0 &&
        h.ObstacleToken!=0 &&
        h.EngineCollisionObject &&
        h.OriginalMapVerified &&
        h.ActualPhysicsObservation &&
        h.VerifiedContactRisk &&
        Finite(h.PositionCm) &&
        Finite(h.MeasuredVelocityCmS) &&
        Finite(h.TrackDistanceCm) &&
        Finite(h.LateralOffsetCm) &&
        Finite(h.HalfLengthCm) &&
        Finite(h.HalfWidthCm) &&
        Finite(h.MassKg) &&
        Finite(h.Confidence) &&
        Finite(h.ObservedSeconds) &&
        Finite(h.ExpirationSeconds) &&
        h.TrackDistanceCm>=0.0 &&
        h.TrackDistanceCm<OriginalCircuitLapCm &&
        std::abs(h.LateralOffsetCm)<=
            p.MaximumRoadLateralCm &&
        h.HalfLengthCm>=p.MinimumHazardLengthCm &&
        h.HalfLengthCm<=p.MaximumHazardLengthCm &&
        h.HalfWidthCm>0.0 &&
        h.HalfWidthCm<=p.MaximumHazardHalfWidthCm &&
        h.MassKg>=p.MinimumPhysicalDebrisMassKg &&
        h.MassKg<=p.MaximumPhysicalDebrisMassKg &&
        h.Confidence>=p.MinimumHazardConfidence &&
        h.Confidence<=1.0 &&
        h.ObservedSeconds>=0.0 &&
        h.ExpirationSeconds>h.ObservedSeconds &&
        h.ExpirationSeconds-h.ObservedSeconds<=
            p.MaximumHazardLifetimeSeconds &&
        CenterOnOriginalRoad(h.PositionCm);
}
inline double HazardSpeedFraction(
    PhysicalTrackHazard kind,
    TrackHazardSettings p) noexcept
{
    switch(kind)
    {
    case PhysicalTrackHazard::TireRubber:
        return p.DryRubberSpeedFraction;
    case PhysicalTrackHazard::OilSpill:
    case PhysicalTrackHazard::FuelSpill:
        return p.WetOilSpeedFraction;
    case PhysicalTrackHazard::DisabledVehicle:
    case PhysicalTrackHazard::FallenBarrier:
        return p.DisabledVehicleSpeedFraction;
    case PhysicalTrackHazard::EmergencyPersonnel:
        return p.PersonnelSpeedFraction;
    case PhysicalTrackHazard::StandingWater:
        return 0.52;
    case PhysicalTrackHazard::LooseGravel:
        return 0.64;
    default:return 0.78;
    }
}
inline TrackHazardDirective ReviewTrackHazards(
    const TrackHazardInventory& source,
    double carProgressCm,double lateralOffsetCm,
    double speedCmS,double clockSeconds,
    TrackHazardSettings p={}) noexcept
{
    TrackHazardDirective out;
    if (!ValidHazardSettings(p) ||
        !Finite(carProgressCm) ||
        !Finite(lateralOffsetCm) ||
        !Finite(speedCmS) ||
        !Finite(clockSeconds) ||
        speedCmS<0.0 ||
        speedCmS>MaximumPhysicalSpeedCmS ||
        clockSeconds<0.0)
        return out;
    out.Valid=true;
    out.DistanceAheadCm=
        std::numeric_limits<double>::infinity();
    for (int i=0;i<source.Count;++i)
    {
        const auto& hazard=source.Hazards[
            static_cast<std::size_t>(i)];
        if (!ValidTrackHazardObservation(hazard,p) ||
            clockSeconds<hazard.ObservedSeconds ||
            clockSeconds>hazard.ExpirationSeconds ||
            clockSeconds-hazard.ObservedSeconds>
                p.MaximumSensorAgeSeconds)
            continue;
        ++out.ObservedHazards;
        const double along=WrapDistance(
            hazard.TrackDistanceCm-carProgressCm,
            OriginalCircuitLapCm);
        if (along>p.MaximumApproachDistanceCm ||
            along<p.MinimumApproachDistanceCm)
            continue;
        const double side=std::abs(
            hazard.LateralOffsetCm-lateralOffsetCm);
        if (side>hazard.HalfWidthCm+180.0)
            continue;
        ++out.OverlappingLaneHazards;
        const double velocityFraction=std::clamp(
            1.0-SafeDivision(
                along,p.MaximumApproachDistanceCm),
            0.0,1.0);
        const double fraction=HazardSpeedFraction(
            hazard.Kind,p);
        const double reduction=Clamp01(
            (1.0-fraction)*(0.4+0.6*velocityFraction));
        const double allowed=1.0-reduction;
        if (along<out.DistanceAheadCm)
        {
            out.DistanceAheadCm=along;
            out.LateralSeparationCm=side;
            out.Kind=hazard.Kind;
            out.ObstacleToken=hazard.ObstacleToken;
            out.HazardConfidence=hazard.Confidence;
            out.TimeToHazardSeconds=SafeDivision(
                along,std::max(1.0,speedCmS));
            out.HazardRoadCoverageFraction=Clamp01(
                SafeDivision(
                    hazard.HalfWidthCm*2.0,
                    OriginalRoadHalfWidthCm*2.0));
        }
        out.RecommendedSpeedCeilingFraction=std::min(
            out.RecommendedSpeedCeilingFraction,allowed);
        out.RecommendedThrottleCeiling=std::min(
            out.RecommendedThrottleCeiling,allowed);
        out.RecommendedBrakeFloor=std::max(
            out.RecommendedBrakeFloor,
            std::clamp(reduction,0.0,p.MaximumBrakeFraction));
        out.RequiresPhysicalBrake=out.RequiresPhysicalBrake ||
            reduction>0.55;
    }
    out.HasActualEngineAuthority=false;
    out.ClearEscapeLaneIdentified=false;
    return out;
}
class PhysicalHazardRegistry final
{
public:
    bool Configure(TrackHazardSettings settings) noexcept
    {
        if (!ValidHazardSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept{Inventory={};}
    bool Begin(std::uint64_t session) noexcept
    {
        if (session==0)return false;
        Reset();
        Inventory.Session=session;
        return true;
    }
    bool Observe(TrackHazardObservation hazard) noexcept
    {
        if (!ValidTrackHazardObservation(hazard,Settings) ||
            Inventory.Session!=hazard.Session ||
            hazard.FrameOrdinal<=Inventory.LastFrameOrdinal)
        {
            ++Inventory.Rejected;
            return false;
        }
        for (int i=0;i<Inventory.Count;++i)
        {
            auto& previous=Inventory.Hazards[
                static_cast<std::size_t>(i)];
            if (previous.ObstacleToken==hazard.ObstacleToken)
            {
                if (previous.FrameOrdinal>=hazard.FrameOrdinal)
                {
                    ++Inventory.Rejected;
                    return false;
                }
                previous=hazard;
                Inventory.LastFrameOrdinal=hazard.FrameOrdinal;
                ++Inventory.Updated;
                return true;
            }
        }
        if (Inventory.Count>=Settings.MaximumActiveHazards)
        {
            ++Inventory.Rejected;
            return false;
        }
        Inventory.Hazards[
            static_cast<std::size_t>(Inventory.Count++)]=hazard;
        Inventory.LastFrameOrdinal=hazard.FrameOrdinal;
        ++Inventory.Observed;
        Inventory.AnyEngineHazardsCreated=false;
        return true;
    }
    bool Expire(double clockSeconds) noexcept
    {
        if (!Finite(clockSeconds) ||
            clockSeconds<Inventory.LastClockSeconds)
            return false;
        Inventory.LastClockSeconds=clockSeconds;
        int target=0;
        for (int i=0;i<Inventory.Count;++i)
        {
            const auto& hazard=Inventory.Hazards[
                static_cast<std::size_t>(i)];
            if (clockSeconds>=hazard.ExpirationSeconds)
            {
                ++Inventory.Expired;
                continue;
            }
            Inventory.Hazards[
                static_cast<std::size_t>(target++)]=hazard;
        }
        Inventory.Count=target;
        return true;
    }
    TrackHazardDirective Review(
        double progressCm,double laneOffsetCm,
        double speedCmS,double clockSeconds) const noexcept
    {
        return ReviewTrackHazards(Inventory,progressCm,
            laneOffsetCm,speedCmS,clockSeconds,Settings);
    }
    TrackHazardInventory Snapshot() const noexcept{return Inventory;}
private:
    TrackHazardSettings Settings{};
    TrackHazardInventory Inventory{};
};
} // namespace latebrake::racecraft
