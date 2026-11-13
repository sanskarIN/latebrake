#pragma once
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class TrackLimitStatus : std::uint8_t
{
    Unknown, InsideAsphalt, NearEdge, PartiallyOutside,
    FullyOffRoad, Rejoining, JunctionAmbiguous
};
struct TrackLimitsParameters final
{
    double PhysicalHalfLengthCm=210.0;
    double PhysicalHalfWidthCm=94.0;
    double WheelTrackHalfCm=82.0;
    double SafetyPaddingCm=18.0;
    double NearEdgeDistanceCm=90.0;
    double OffRoadThresholdSeconds=0.15;
    double RejoinClearanceSeconds=0.40;
    double MaximumValidatedFrameSeconds=0.10;
    double MaximumValidPlanarSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumJunctionConfidenceFraction=0.9;
    int MinimumVerifiedWheels=3;
    int MinimumOffroadWheels=2;
    bool RequireChassisOnWorldStatic=true;
    bool CountAmbiguousJunctionAsOffroad=false;
    bool RequireEngineWheelContacts=true;
    bool RequireOriginalAsphaltGeometry=true;
};
struct TrackLimitObservation final
{
    RaceIdentity Identity{};
    Pose2 ChassisPose{};
    Vec2 PlanarVelocityCmS{};
    std::array<Vec2,4> WheelContactPositionCm{};
    std::array<bool,4> WheelContactVerified{};
    double DeltaSeconds=0.0;
    double ActiveRaceClockSeconds=0.0;
    double ChassisHalfLengthCm=210.0;
    double ChassisHalfWidthCm=94.0;
    bool OriginalPhysicalVehicle=true;
    bool EngineObserved=true;
    bool ChassisOnWorldStatic=true;
    bool RaceClockRunning=true;
    bool RecoveryInProgress=false;
};
struct TrackLimitDecision final
{
    TrackLimitStatus Status=TrackLimitStatus::Unknown;
    double VehicleCenterRoadMarginCm=0.0;
    double RoadCoverageFraction=0.0;
    double DistanceToNearestCenterlineCm=0.0;
    double LateralRoadMarginCm=0.0;
    double EstimatedOffroadDepthCm=0.0;
    double MaximumCornerDistanceBeyondRoadCm=0.0;
    double MeasuredPlanarSpeedCmS=0.0;
    double ObservedRaceClockSeconds=0.0;
    int WheelsOnPavement=0;
    int WheelsOutsidePavement=0;
    int QualifiedWheelContacts=0;
    int BodyCornersOnRoad=0;
    bool CenterOnRoad=false;
    bool FullBodySingleSlab=false;
    bool JunctionAmbiguity=false;
    bool SourceGeometryConservative=true;
    bool OfficialTrackLimitWarningIssued=false;
    bool Valid=false;
};
struct TrackLimitHistory final
{
    std::uint64_t AcceptedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t OffroadEpisodes=0;
    std::uint64_t RejoinEpisodes=0;
    std::uint64_t JunctionAmbiguousFrames=0;
    std::uint64_t VerifiedPhysicalOffroadFrames=0;
    double CurrentOffroadSeconds=0.0;
    double LongestOffroadSeconds=0.0;
    double TotalOffroadSeconds=0.0;
    double MinimumRoadMarginCm=0.0;
    double MaximumOffroadDepthCm=0.0;
    double RejoinTimerSeconds=0.0;
    TrackLimitStatus LastStatus=TrackLimitStatus::Unknown;
    bool IsOutside=false;
    bool OfficialWarningsChanged=false;
};
inline bool ValidTrackLimitsParameters(
    TrackLimitsParameters s) noexcept
{
    return Finite(s.PhysicalHalfLengthCm) &&
        s.PhysicalHalfLengthCm>0.0 &&
        Finite(s.PhysicalHalfWidthCm) &&
        s.PhysicalHalfWidthCm>0.0 &&
        Finite(s.WheelTrackHalfCm) &&
        s.WheelTrackHalfCm>0.0 &&
        Finite(s.SafetyPaddingCm) &&
        s.SafetyPaddingCm>=0.0 &&
        Finite(s.NearEdgeDistanceCm) &&
        s.NearEdgeDistanceCm>=0.0 &&
        Finite(s.OffRoadThresholdSeconds) &&
        s.OffRoadThresholdSeconds>=0.0 &&
        Finite(s.RejoinClearanceSeconds) &&
        s.RejoinClearanceSeconds>=0.0 &&
        Finite(s.MaximumValidatedFrameSeconds) &&
        s.MaximumValidatedFrameSeconds>0.0 &&
        s.MaximumValidatedFrameSeconds<=MaxSimulationFrameSeconds &&
        Finite(s.MinimumJunctionConfidenceFraction) &&
        s.MinimumJunctionConfidenceFraction>=0.0 &&
        s.MinimumJunctionConfidenceFraction<=1.0 &&
        s.MinimumVerifiedWheels>=1 &&
        s.MinimumVerifiedWheels<=4 &&
        s.MinimumOffroadWheels>=1 &&
        s.MinimumOffroadWheels<=4;
}
inline bool ValidTrackLimitObservation(
    TrackLimitObservation o,
    TrackLimitsParameters p) noexcept
{
    if (!PhysicalActorIdentityValid(o.Identity) ||
        !o.OriginalPhysicalVehicle ||
        !o.EngineObserved ||
        !o.RaceClockRunning ||
        (p.RequireChassisOnWorldStatic &&
         !o.ChassisOnWorldStatic) ||
        !Finite(o.ChassisPose) ||
        !Finite(o.PlanarVelocityCmS) ||
        !Finite(o.DeltaSeconds) ||
        !Finite(o.ActiveRaceClockSeconds) ||
        !Finite(o.ChassisHalfLengthCm) ||
        !Finite(o.ChassisHalfWidthCm) ||
        o.DeltaSeconds<=0.0 ||
        o.DeltaSeconds>p.MaximumValidatedFrameSeconds ||
        o.ActiveRaceClockSeconds<0.0 ||
        o.ChassisHalfLengthCm<=0.0 ||
        o.ChassisHalfLengthCm>350.0 ||
        o.ChassisHalfWidthCm<=0.0 ||
        o.ChassisHalfWidthCm>240.0 ||
        Length(o.PlanarVelocityCmS)>
            p.MaximumValidPlanarSpeedCmS)
        return false;
    for (std::size_t i=0;i<4;++i)
    {
        if (o.WheelContactVerified[i] &&
            !Finite(o.WheelContactPositionCm[i]))
            return false;
    }
    return true;
}
inline TrackLimitDecision EvaluateTrackLimits(
    TrackLimitObservation observation,
    TrackLimitsParameters p={}) noexcept
{
    TrackLimitDecision out;
    if (!ValidTrackLimitsParameters(p) ||
        !ValidTrackLimitObservation(observation,p))
        return out;
    const Rect2 body{
        observation.ChassisPose.Position,
        {observation.ChassisHalfLengthCm+p.SafetyPaddingCm,
         observation.ChassisHalfWidthCm+p.SafetyPaddingCm},
        observation.ChassisPose.HeadingRadians
    };
    const auto corners=RectCorners(body);
    const auto projection=ProjectOriginalCircuit(
        observation.ChassisPose.Position,
        observation.ChassisPose.HeadingRadians);
    if (!projection.Valid)return out;
    out.CenterOnRoad=CenterOnOriginalRoad(
        observation.ChassisPose.Position);
    out.FullBodySingleSlab=AnyOriginalSlabContains(body);
    out.DistanceToNearestCenterlineCm=
        projection.CenterlineDistanceCm;
    out.VehicleCenterRoadMarginCm=
        DistanceToNearestSlabEdge(
            observation.ChassisPose.Position);
    out.LateralRoadMarginCm=projection.RoadClearanceCm;
    out.MeasuredPlanarSpeedCmS=
        Length(observation.PlanarVelocityCmS);
    out.ObservedRaceClockSeconds=
        observation.ActiveRaceClockSeconds;
    for (Vec2 corner:corners)
    {
        if (RoadCornerSamplePasses(corner))
            ++out.BodyCornersOnRoad;
        else
            out.MaximumCornerDistanceBeyondRoadCm=
                std::max(out.MaximumCornerDistanceBeyondRoadCm,
                    LineCenterDistance(corner)-
                        OriginalRoadHalfWidthCm);
    }
    out.RoadCoverageFraction=
        out.BodyCornersOnRoad/4.0;
    for (std::size_t i=0;i<4;++i)
    {
        if (!observation.WheelContactVerified[i])
            continue;
        ++out.QualifiedWheelContacts;
        if (CenterOnOriginalRoad(
                observation.WheelContactPositionCm[i]))
            ++out.WheelsOnPavement;
        else
            ++out.WheelsOutsidePavement;
    }
    if (p.RequireEngineWheelContacts &&
        out.QualifiedWheelContacts<p.MinimumVerifiedWheels)
        return {};
    out.JunctionAmbiguity=
        !out.FullBodySingleSlab &&
        out.BodyCornersOnRoad==4 &&
        out.CenterOnRoad;
    if (observation.RecoveryInProgress)
        out.Status=TrackLimitStatus::Rejoining;
    else if (out.JunctionAmbiguity)
        out.Status=TrackLimitStatus::JunctionAmbiguous;
    else if (!out.CenterOnRoad &&
             out.BodyCornersOnRoad==0 &&
             out.WheelsOutsidePavement>=p.MinimumOffroadWheels)
        out.Status=TrackLimitStatus::FullyOffRoad;
    else if (out.BodyCornersOnRoad<4 &&
             out.WheelsOutsidePavement>=p.MinimumOffroadWheels)
        out.Status=TrackLimitStatus::PartiallyOutside;
    else if (out.VehicleCenterRoadMarginCm<
             p.NearEdgeDistanceCm ||
             out.BodyCornersOnRoad<4)
        out.Status=TrackLimitStatus::NearEdge;
    else
        out.Status=TrackLimitStatus::InsideAsphalt;
    out.EstimatedOffroadDepthCm=std::max(
        0.0,out.MaximumCornerDistanceBeyondRoadCm);
    out.SourceGeometryConservative=true;
    out.OfficialTrackLimitWarningIssued=false;
    out.Valid=true;
    return out;
}
class TrackLimitsMonitor final
{
public:
    bool Configure(TrackLimitsParameters settings) noexcept
    {
        if (!ValidTrackLimitsParameters(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        History={};
        Last={};
        Actor=0;
        Session=0;
        HasEpoch=false;
    }
    TrackLimitDecision Observe(TrackLimitObservation observation) noexcept
    {
        const auto reviewed=EvaluateTrackLimits(
            observation,Settings);
        if (!reviewed.Valid)
        {
            ++History.RejectedFrames;
            History.CurrentOffroadSeconds=0.0;
            return {};
        }
        if (HasEpoch &&
            (observation.Identity.Actor!=Actor ||
             observation.Identity.Session!=Session))
        {
            const auto settings=Settings;
            Reset();
            Settings=settings;
        }
        Actor=observation.Identity.Actor;
        Session=observation.Identity.Session;
        HasEpoch=true;
        const bool offRoad=
            reviewed.Status==TrackLimitStatus::PartiallyOutside ||
            reviewed.Status==TrackLimitStatus::FullyOffRoad;
        if (offRoad)
        {
            History.CurrentOffroadSeconds+=
                observation.DeltaSeconds;
            History.TotalOffroadSeconds+=
                observation.DeltaSeconds;
            History.LongestOffroadSeconds=std::max(
                History.LongestOffroadSeconds,
                History.CurrentOffroadSeconds);
            ++History.VerifiedPhysicalOffroadFrames;
            if (!History.IsOutside &&
                History.CurrentOffroadSeconds>=
                    Settings.OffRoadThresholdSeconds)
            {
                History.IsOutside=true;
                ++History.OffroadEpisodes;
            }
            History.RejoinTimerSeconds=0.0;
        }
        else
        {
            if (History.IsOutside)
            {
                History.RejoinTimerSeconds+=
                    observation.DeltaSeconds;
                if (History.RejoinTimerSeconds>=
                    Settings.RejoinClearanceSeconds)
                {
                    ++History.RejoinEpisodes;
                    History.IsOutside=false;
                    History.RejoinTimerSeconds=0.0;
                }
            }
            else History.CurrentOffroadSeconds=0.0;
        }
        if (reviewed.JunctionAmbiguity)
            ++History.JunctionAmbiguousFrames;
        History.MaximumOffroadDepthCm=std::max(
            History.MaximumOffroadDepthCm,
            reviewed.EstimatedOffroadDepthCm);
        if (History.AcceptedFrames==0)
            History.MinimumRoadMarginCm=
                reviewed.VehicleCenterRoadMarginCm;
        else
            History.MinimumRoadMarginCm=std::min(
                History.MinimumRoadMarginCm,
                reviewed.VehicleCenterRoadMarginCm);
        ++History.AcceptedFrames;
        History.LastStatus=reviewed.Status;
        History.OfficialWarningsChanged=false;
        Last=reviewed;
        return Last;
    }
    TrackLimitHistory Snapshot() const noexcept{return History;}
    TrackLimitDecision LastDecision() const noexcept{return Last;}
    std::uint64_t OffroadEpisodes() const noexcept
    {return History.OffroadEpisodes;}
private:
    TrackLimitsParameters Settings{};
    TrackLimitHistory History{};
    TrackLimitDecision Last{};
    std::uint64_t Actor=0;
    std::uint64_t Session=0;
    bool HasEpoch=false;
};
} // namespace latebrake::racecraft
