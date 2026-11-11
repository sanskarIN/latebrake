#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int RivalLaneCount=3;
inline constexpr int RivalTrafficCount=PhysicalFleetCapacity-1;
enum class RivalLaneClass : std::uint8_t
{
    Left, Center, Right
};
enum class RivalLaneHazard : std::uint8_t
{
    Unqualified, Clear, OccupiedNow, ClosingAhead,
    ApproachingBehind, SideBySide, PredictedMerge,
    WallOrKerb, UnknownPhysicalRoad
};
struct RivalLaneSettings final
{
    double LaneCenterOffsetCm=220.0;
    double LaneHalfWidthCm=155.0;
    double MaximumLaneOffsetCm=480.0;
    double MinimumRoadHalfWidthCm=450.0;
    double MaximumRoadHalfWidthCm=2400.0;
    double RoadEdgeSafetyCm=140.0;
    double VehicleHalfLengthCm=220.0;
    double VehicleHalfWidthCm=95.0;
    double CollisionPaddingCm=55.0;
    double MinimumPassingClearanceCm=120.0;
    double MaximumLongitudinalInfluenceCm=5000.0;
    double MaximumLateralInfluenceCm=1100.0;
    double MaximumObservedSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumRelativeSpeedCmS=9000.0;
    double MinimumClosingSpeedCmS=70.0;
    double ForecastSeconds=1.75;
    double ReactionSeconds=0.25;
    double SampleSpacingSeconds=0.05;
    double MinimumMergeTimeSeconds=0.70;
    double MaximumMergeTimeSeconds=2.0;
    double MinimumFollowingTimeGapSeconds=1.0;
    double MinimumRearwardTimeGapSeconds=0.8;
    double MaximumHeadingErrorRadians=0.9;
    double MaximumLateralVelocityCmS=2500.0;
    double MaximumSpeedOnKerbCmS=6000.0;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.02;
    double MinimumSensorConfidence=0.8;
    double ConfidencePenalty=0.1;
    double MaximumBrakeFraction=1.0;
    double BrakeReleasePerSecond=2.0;
    double ThrottleRecoveryPerSecond=1.0;
    double EmergencyTimeToContactSeconds=0.6;
    double OvertakeClosingAdvantageCmS=120.0;
    double MaximumSteeringForCommittedLane=0.75;
    bool RequireSixPhysicalCars=true;
    bool RequireOriginalRoadGeometry=true;
    bool RequireSameEngineFrame=true;
    bool NeverSteerIntoUnknownLane=true;
    bool NeverOverrideTrafficEmergencyBrake=true;
};
struct RivalLaneActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    Vec2 AccelerationCmS2{};
    double HalfLengthCm=220.0;
    double HalfWidthCm=95.0;
    double MeasuredGripG=1.0;
    double TrackLateralOffsetCm=0.0;
    double TrackDistanceCm=0.0;
    double RoadHalfWidthCm=800.0;
    double SensorConfidence=1.0;
    double PhysicsClockSeconds=0.0;
    bool OriginalChassisVerified=false;
    bool OriginalPavementVerified=false;
    bool LiveChaosVelocityQualified=false;
};
struct RivalLaneTrafficFrame final
{
    RivalLaneActor Self{};
    std::array<RivalLaneActor,RivalTrafficCount> Others{};
    int OtherCount=0;
    double DeltaSeconds=0.0;
    double CurrentThrottle=0.0;
    double CurrentBrake=0.0;
    double CurrentSteering=0.0;
    bool OriginalCircuitConfirmed=false;
    bool SixCarRosterAuthenticated=false;
    bool RaceGreen=false;
    bool OriginalTrackCollisionReady=false;
};
struct RivalLaneContact final
{
    int Slot=-1;
    std::uint64_t Actor=0;
    double ForwardDistanceCm=0.0;
    double LateralDistanceCm=0.0;
    double RelativeForwardSpeedCmS=0.0;
    double RelativeLateralSpeedCmS=0.0;
    double PredictedContactSeconds=std::numeric_limits<double>::infinity();
    double MinimumPredictedLongitudinalSeparationCm=0.0;
    double MinimumPredictedLateralSeparationCm=0.0;
    double ClosestTimeSeconds=0.0;
    double SafeMergeTimeSeconds=0.0;
    bool Ahead=false;
    bool Behind=false;
    bool CurrentlyOverlapping=false;
    bool PredictedOverlap=false;
    bool ApproachingFromRear=false;
    bool PhysicallyQualified=false;
};
struct RivalLaneCandidate final
{
    RivalLaneClass Lane=RivalLaneClass::Center;
    RivalLaneHazard Hazard=RivalLaneHazard::Unqualified;
    double LaneOffsetCm=0.0;
    double AvailableLeftClearanceCm=0.0;
    double AvailableRightClearanceCm=0.0;
    double MinimumForwardGapCm=std::numeric_limits<double>::infinity();
    double MinimumRearGapCm=std::numeric_limits<double>::infinity();
    double MinimumTimeToContactSeconds=std::numeric_limits<double>::infinity();
    double MinimumMergeClearanceCm=std::numeric_limits<double>::infinity();
    double MaximumClosingSpeedCmS=0.0;
    double RequiredStoppingDistanceCm=0.0;
    double RecommendedThrottleCeiling=0.0;
    double RecommendedBrakeFloor=1.0;
    double SteeringOffsetFraction=0.0;
    double Confidence=0.0;
    int LimitingActorSlot=-1;
    int PotentialContacts=0;
    int ApproachingRearCars=0;
    int OverlappingCars=0;
    bool RoadEdgeVerified=false;
    bool CollisionFreeAtCurrentTime=false;
    bool SweptMergeClear=false;
    bool Valid=false;
};
struct RivalLaneForecast final
{
    RaceIdentity Identity{};
    std::array<RivalLaneContact,RivalTrafficCount> Contacts{};
    std::array<RivalLaneCandidate,RivalLaneCount> Lanes{};
    int ObservedActors=0;
    int VerifiedOtherCars=0;
    int TotalPotentialContacts=0;
    int ValidLaneCount=0;
    int SafelyAvailableLanes=0;
    int PreferredSafeLane=-1;
    double ProposedThrottle=0.0;
    double ProposedBrake=1.0;
    double ProposedSteering=0.0;
    double MinimumTrafficTimeToContactSeconds=
        std::numeric_limits<double>::infinity();
    double NearestPhysicalCarCm=
        std::numeric_limits<double>::infinity();
    double MinimumRoadClearanceCm=0.0;
    double TotalLateralRiskScore=0.0;
    double MaximumActualClosingSpeedCmS=0.0;
    double MinimumRivalConfidence=0.0;
    bool EmergencyBrake=false;
    bool RoverMustHoldLane=true;
    bool NoUnrealActuation=true;
    bool Valid=false;
};
inline double RivalLaneOffset(
    RivalLaneClass lane,RivalLaneSettings p) noexcept
{
    switch(lane)
    {
    case RivalLaneClass::Left:return -p.LaneCenterOffsetCm;
    case RivalLaneClass::Center:return 0.0;
    case RivalLaneClass::Right:return p.LaneCenterOffsetCm;
    }
    return 0.0;
}
inline bool ValidRivalLaneSettings(
    RivalLaneSettings p) noexcept
{
    return Finite(p.LaneCenterOffsetCm) &&
        p.LaneCenterOffsetCm>0.0 &&
        Finite(p.LaneHalfWidthCm) &&
        p.LaneHalfWidthCm>0.0 &&
        Finite(p.MaximumLaneOffsetCm) &&
        p.MaximumLaneOffsetCm>p.LaneCenterOffsetCm &&
        Finite(p.MinimumRoadHalfWidthCm) &&
        p.MinimumRoadHalfWidthCm>p.LaneHalfWidthCm &&
        Finite(p.MaximumRoadHalfWidthCm) &&
        p.MaximumRoadHalfWidthCm>p.MinimumRoadHalfWidthCm &&
        Finite(p.RoadEdgeSafetyCm) &&
        p.RoadEdgeSafetyCm>=0.0 &&
        Finite(p.VehicleHalfLengthCm) &&
        p.VehicleHalfLengthCm>0.0 &&
        Finite(p.VehicleHalfWidthCm) &&
        p.VehicleHalfWidthCm>0.0 &&
        Finite(p.CollisionPaddingCm) &&
        p.CollisionPaddingCm>=0.0 &&
        Finite(p.MinimumPassingClearanceCm) &&
        p.MinimumPassingClearanceCm>=0.0 &&
        Finite(p.MaximumLongitudinalInfluenceCm) &&
        p.MaximumLongitudinalInfluenceCm>0.0 &&
        Finite(p.MaximumLateralInfluenceCm) &&
        p.MaximumLateralInfluenceCm>0.0 &&
        Finite(p.MaximumObservedSpeedCmS) &&
        p.MaximumObservedSpeedCmS>0.0 &&
        p.MaximumObservedSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MaximumRelativeSpeedCmS) &&
        p.MaximumRelativeSpeedCmS>0.0 &&
        Finite(p.MinimumClosingSpeedCmS) &&
        p.MinimumClosingSpeedCmS>=0.0 &&
        Finite(p.ForecastSeconds) &&
        p.ForecastSeconds>0.0 &&
        Finite(p.ReactionSeconds) &&
        p.ReactionSeconds>=0.0 &&
        Finite(p.SampleSpacingSeconds) &&
        p.SampleSpacingSeconds>0.0 &&
        p.SampleSpacingSeconds<=p.ForecastSeconds &&
        Finite(p.MinimumMergeTimeSeconds) &&
        p.MinimumMergeTimeSeconds>0.0 &&
        Finite(p.MaximumMergeTimeSeconds) &&
        p.MaximumMergeTimeSeconds>=p.MinimumMergeTimeSeconds &&
        Finite(p.MinimumFollowingTimeGapSeconds) &&
        p.MinimumFollowingTimeGapSeconds>0.0 &&
        Finite(p.MinimumRearwardTimeGapSeconds) &&
        p.MinimumRearwardTimeGapSeconds>0.0 &&
        Finite(p.MaximumHeadingErrorRadians) &&
        p.MaximumHeadingErrorRadians>0.0 &&
        Finite(p.MaximumLateralVelocityCmS) &&
        p.MaximumLateralVelocityCmS>0.0 &&
        Finite(p.MaximumSpeedOnKerbCmS) &&
        p.MaximumSpeedOnKerbCmS>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.ConfidencePenalty) &&
        p.ConfidencePenalty>=0.0 &&
        p.ConfidencePenalty<1.0 &&
        Finite(p.MaximumBrakeFraction) &&
        p.MaximumBrakeFraction>0.0 &&
        p.MaximumBrakeFraction<=1.0 &&
        Finite(p.BrakeReleasePerSecond) &&
        p.BrakeReleasePerSecond>0.0 &&
        Finite(p.ThrottleRecoveryPerSecond) &&
        p.ThrottleRecoveryPerSecond>0.0 &&
        Finite(p.EmergencyTimeToContactSeconds) &&
        p.EmergencyTimeToContactSeconds>0.0 &&
        Finite(p.OvertakeClosingAdvantageCmS) &&
        p.OvertakeClosingAdvantageCmS>0.0 &&
        Finite(p.MaximumSteeringForCommittedLane) &&
        p.MaximumSteeringForCommittedLane>0.0 &&
        p.MaximumSteeringForCommittedLane<=1.0;
}
inline bool ValidRivalLaneActor(
    RivalLaneActor a,RivalLaneSettings p) noexcept
{
    return PhysicalActorIdentityValid(a.Identity) &&
        a.Identity.Frame>0 &&
        a.OriginalChassisVerified &&
        a.OriginalPavementVerified &&
        a.LiveChaosVelocityQualified &&
        Finite(a.Pose) &&
        Finite(a.VelocityCmS) &&
        Finite(a.AccelerationCmS2) &&
        Length(a.VelocityCmS)<=p.MaximumObservedSpeedCmS &&
        Finite(a.HalfLengthCm) &&
        a.HalfLengthCm>0.0 &&
        a.HalfLengthCm<500.0 &&
        Finite(a.HalfWidthCm) &&
        a.HalfWidthCm>0.0 &&
        a.HalfWidthCm<250.0 &&
        Finite(a.MeasuredGripG) &&
        a.MeasuredGripG>=0.15 &&
        a.MeasuredGripG<=2.5 &&
        Finite(a.TrackLateralOffsetCm) &&
        Finite(a.TrackDistanceCm) &&
        Finite(a.RoadHalfWidthCm) &&
        a.RoadHalfWidthCm>=p.MinimumRoadHalfWidthCm &&
        a.RoadHalfWidthCm<=p.MaximumRoadHalfWidthCm &&
        std::abs(a.TrackLateralOffsetCm)+
            a.HalfWidthCm<a.RoadHalfWidthCm &&
        Finite(a.SensorConfidence) &&
        a.SensorConfidence>=p.MinimumSensorConfidence &&
        a.SensorConfidence<=1.0 &&
        Finite(a.PhysicsClockSeconds) &&
        a.PhysicsClockSeconds>=0.0;
}
inline bool ValidRivalLaneFrame(
    const RivalLaneTrafficFrame& f,
    RivalLaneSettings p) noexcept
{
    if (!ValidRivalLaneSettings(p) ||
        !f.RaceGreen ||
        !f.OriginalCircuitConfirmed ||
        !f.OriginalTrackCollisionReady ||
        !f.SixCarRosterAuthenticated ||
        !ValidRivalLaneActor(f.Self,p) ||
        f.Self.Identity.Slot==0 ||
        f.OtherCount!=RivalTrafficCount ||
        !Finite(f.DeltaSeconds) ||
        f.DeltaSeconds<=0.0 ||
        f.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(f.CurrentThrottle) ||
        f.CurrentThrottle<0.0 || f.CurrentThrottle>1.0 ||
        !Finite(f.CurrentBrake) ||
        f.CurrentBrake<0.0 || f.CurrentBrake>1.0 ||
        !Finite(f.CurrentSteering) ||
        std::abs(f.CurrentSteering)>1.0)
        return false;
    std::array<std::uint64_t,PhysicalFleetCapacity> actors{};
    actors[static_cast<std::size_t>(f.Self.Identity.Slot)]=
        f.Self.Identity.Actor;
    for(int i=0;i<f.OtherCount;++i)
    {
        const auto& other=f.Others[static_cast<std::size_t>(i)];
        if (!ValidRivalLaneActor(other,p) ||
            other.Identity.Session!=f.Self.Identity.Session ||
            other.Identity.SessionRevision!=
                f.Self.Identity.SessionRevision ||
            (p.RequireSameEngineFrame &&
             other.Identity.Frame!=f.Self.Identity.Frame) ||
            std::abs(other.PhysicsClockSeconds-
                     f.Self.PhysicsClockSeconds)>
                p.MaximumClockSkewSeconds ||
            actors[static_cast<std::size_t>(
                other.Identity.Slot)]!=0)
            return false;
        for(int prior=0;prior<i;++prior)
        {
            if (other.Identity.Actor==f.Others[
                static_cast<std::size_t>(prior)].Identity.Actor)
                return false;
        }
        if (other.Identity.Actor==f.Self.Identity.Actor)
            return false;
        actors[static_cast<std::size_t>(other.Identity.Slot)]=
            other.Identity.Actor;
    }
    return std::all_of(actors.begin(),actors.end(),
        [](std::uint64_t id){return id!=0;});
}
inline RivalLaneContact AnalyzeRivalLaneContact(
    RivalLaneActor self,RivalLaneActor other,
    RivalLaneSettings p) noexcept
{
    RivalLaneContact out;
    if (!ValidRivalLaneActor(self,p) ||
        !ValidRivalLaneActor(other,p) ||
        self.Identity.Actor==other.Identity.Actor ||
        self.Identity.Slot==other.Identity.Slot)
        return out;
    const Vec2 forward=HeadingVector(
        self.Pose.HeadingRadians);
    const Vec2 toOther=other.Pose.Position-
        self.Pose.Position;
    const Vec2 deltaVelocity=other.VelocityCmS-
        self.VelocityCmS;
    const double dx=Dot(toOther,forward);
    const double dy=Cross(forward,toOther);
    const double vx=Dot(deltaVelocity,forward);
    const double vy=Cross(forward,deltaVelocity);
    const double boundsX=self.HalfLengthCm+
        other.HalfLengthCm+p.CollisionPaddingCm;
    const double boundsY=self.HalfWidthCm+
        other.HalfWidthCm+p.CollisionPaddingCm;
    out.Slot=other.Identity.Slot;
    out.Actor=other.Identity.Actor;
    out.ForwardDistanceCm=dx;
    out.LateralDistanceCm=dy;
    out.RelativeForwardSpeedCmS=vx;
    out.RelativeLateralSpeedCmS=vy;
    out.Ahead=dx>boundsX;
    out.Behind=dx< -boundsX;
    out.CurrentlyOverlapping=std::abs(dx)<=boundsX &&
        std::abs(dy)<=boundsY;
    out.ApproachingFromRear=dx<0.0 && vx>0.0;
    double enter=0.0;
    double leave=p.ForecastSeconds;
    const std::array<double,2> displacement{dx,dy};
    const std::array<double,2> velocity{vx,vy};
    const std::array<double,2> bounds{boundsX,boundsY};
    for(std::size_t axis=0;axis<2;++axis)
    {
        if (std::abs(velocity[axis])<PhysicsEpsilon)
        {
            if (std::abs(displacement[axis])>bounds[axis])
            {
                leave=-1.0;
                break;
            }
            continue;
        }
        const double left=(-bounds[axis]-
                           displacement[axis])/velocity[axis];
        const double right=(bounds[axis]-
                            displacement[axis])/velocity[axis];
        enter=std::max(enter,std::min(left,right));
        leave=std::min(leave,std::max(left,right));
        if (enter>leave)break;
    }
    out.PredictedOverlap=enter<=leave &&
        enter<=p.ForecastSeconds && leave>=0.0;
    out.PredictedContactSeconds=out.PredictedOverlap ?
        std::max(0.0,enter):
        std::numeric_limits<double>::infinity();
    const double relativeSpeedSquared=
        vx*vx+vy*vy;
    out.ClosestTimeSeconds=relativeSpeedSquared>
        PhysicsEpsilon ?
        std::clamp(
            -(dx*vx+dy*vy)/relativeSpeedSquared,
            0.0,p.ForecastSeconds):0.0;
    out.MinimumPredictedLongitudinalSeparationCm=
        std::abs(dx+vx*out.ClosestTimeSeconds)-boundsX;
    out.MinimumPredictedLateralSeparationCm=
        std::abs(dy+vy*out.ClosestTimeSeconds)-boundsY;
    out.SafeMergeTimeSeconds=std::clamp(
        p.MinimumMergeTimeSeconds+
            SafeDivision(
                std::max(0.0,p.MinimumPassingClearanceCm-
                    out.MinimumPredictedLateralSeparationCm),
                p.MaximumRelativeSpeedCmS),
        p.MinimumMergeTimeSeconds,p.MaximumMergeTimeSeconds);
    out.PhysicallyQualified=Finite(dx) && Finite(dy) &&
        Finite(vx) && Finite(vy) &&
        Finite(out.ClosestTimeSeconds) &&
        std::abs(vx)<=p.MaximumRelativeSpeedCmS;
    return out;
}
inline RivalLaneCandidate EvaluateRivalLaneCandidate(
    const RivalLaneTrafficFrame& f,
    const std::array<RivalLaneContact,RivalTrafficCount>& contacts,
    RivalLaneClass lane,RivalLaneSettings p) noexcept
{
    RivalLaneCandidate out;
    if (!ValidRivalLaneFrame(f,p))
        return out;
    out.Lane=lane;
    out.LaneOffsetCm=RivalLaneOffset(lane,p);
    const double vehicleEdge=f.Self.HalfWidthCm+
        p.RoadEdgeSafetyCm;
    out.AvailableLeftClearanceCm=
        f.Self.RoadHalfWidthCm-
        std::abs(f.Self.TrackLateralOffsetCm+
                 out.LaneOffsetCm)-vehicleEdge;
    out.AvailableRightClearanceCm=
        out.AvailableLeftClearanceCm;
    out.RoadEdgeVerified=
        out.AvailableLeftClearanceCm>0.0 &&
        out.AvailableRightClearanceCm>0.0;
    out.Confidence=f.Self.SensorConfidence;
    if (!out.RoadEdgeVerified)
    {
        out.Hazard=RivalLaneHazard::WallOrKerb;
        out.Valid=true;
        return out;
    }
    const double forwardSpeed=std::max(
        0.0,ForwardSpeed(f.Self.VelocityCmS,
                         f.Self.Pose.HeadingRadians));
    for(int i=0;i<f.OtherCount;++i)
    {
        const auto& opponent=f.Others[
            static_cast<std::size_t>(i)];
        const auto& contact=contacts[
            static_cast<std::size_t>(i)];
        if (!contact.PhysicallyQualified)return {};
        out.Confidence=std::min(
            out.Confidence,opponent.SensorConfidence);
        const double lateral=std::abs(
            contact.LateralDistanceCm-out.LaneOffsetCm);
        const double overlapWidth=f.Self.HalfWidthCm+
            opponent.HalfWidthCm+
            p.CollisionPaddingCm;
        const double gap=std::abs(
            contact.ForwardDistanceCm)-
            f.Self.HalfLengthCm-
            opponent.HalfLengthCm;
        const double expectedLateral=
            contact.LateralDistanceCm+
            contact.RelativeLateralSpeedCmS*
                p.MinimumMergeTimeSeconds;
        const bool laneOccupied=lateral<=
            overlapWidth+p.MinimumPassingClearanceCm;
        const bool sweptMergeRisk=std::abs(
            expectedLateral-out.LaneOffsetCm)<=
            overlapWidth+p.MinimumPassingClearanceCm;
        if (!laneOccupied && !sweptMergeRisk)
            continue;
        if(contact.Ahead)
            out.MinimumForwardGapCm=std::min(
                out.MinimumForwardGapCm,gap);
        if(contact.Behind)
        {
            out.MinimumRearGapCm=std::min(
                out.MinimumRearGapCm,gap);
            if (contact.ApproachingFromRear)
                ++out.ApproachingRearCars;
        }
        const double closing=std::max(
            0.0,-contact.RelativeForwardSpeedCmS);
        out.MaximumClosingSpeedCmS=std::max(
            out.MaximumClosingSpeedCmS,closing);
        const double futureTime=closing>PhysicsEpsilon ?
            std::max(0.0,gap/closing):
            std::numeric_limits<double>::infinity();
        if (contact.PredictedOverlap)
        {
            ++out.PotentialContacts;
            out.MinimumTimeToContactSeconds=std::min(
                out.MinimumTimeToContactSeconds,
                contact.PredictedContactSeconds);
        }
        if (laneOccupied &&
            std::abs(contact.ForwardDistanceCm)<=
                f.Self.HalfLengthCm+
                opponent.HalfLengthCm+
                p.CollisionPaddingCm)
            ++out.OverlappingCars;
        if (laneOccupied && contact.Ahead &&
            closing>p.MinimumClosingSpeedCmS)
        {
            out.MinimumTimeToContactSeconds=std::min(
                out.MinimumTimeToContactSeconds,futureTime);
        }
        out.MinimumMergeClearanceCm=std::min(
            out.MinimumMergeClearanceCm,
            std::abs(expectedLateral-out.LaneOffsetCm)-
                overlapWidth);
        out.LimitingActorSlot=out.LimitingActorSlot<0 ?
            opponent.Identity.Slot :
            out.LimitingActorSlot;
    }
    out.RequiredStoppingDistanceCm=
        forwardSpeed*p.ReactionSeconds+
        SafeDivision(forwardSpeed*forwardSpeed,
                     2.0*std::max(
                         100.0,f.Self.MeasuredGripG*GravityCmS2));
    out.CollisionFreeAtCurrentTime=
        out.OverlappingCars==0;
    out.SweptMergeClear=
        out.PotentialContacts==0 &&
        out.ApproachingRearCars==0 &&
        out.OverlappingCars==0 &&
        out.MinimumMergeClearanceCm>=
            p.MinimumPassingClearanceCm;
    if (out.PotentialContacts>0)
        out.Hazard=RivalLaneHazard::PredictedMerge;
    else if (out.OverlappingCars>0)
        out.Hazard=RivalLaneHazard::SideBySide;
    else if(out.ApproachingRearCars>0)
        out.Hazard=RivalLaneHazard::ApproachingBehind;
    else if (out.MinimumForwardGapCm<
             out.RequiredStoppingDistanceCm)
        out.Hazard=RivalLaneHazard::ClosingAhead;
    else
        out.Hazard=RivalLaneHazard::Clear;
    out.RecommendedThrottleCeiling=
        out.Hazard==RivalLaneHazard::Clear?1.0:
        out.Hazard==RivalLaneHazard::ClosingAhead?0.30:0.0;
    out.RecommendedBrakeFloor=
        out.PotentialContacts>0 &&
        out.MinimumTimeToContactSeconds<
            p.EmergencyTimeToContactSeconds ?
            p.MaximumBrakeFraction:
        out.Hazard==RivalLaneHazard::ClosingAhead?0.40:0.0;
    out.SteeringOffsetFraction=std::clamp(
        SafeDivision(out.LaneOffsetCm,
                     p.MaximumLaneOffsetCm),
        -p.MaximumSteeringForCommittedLane,
        p.MaximumSteeringForCommittedLane);
    out.Valid=Finite(out.RecommendedThrottleCeiling) &&
        Finite(out.RecommendedBrakeFloor) &&
        Finite(out.SteeringOffsetFraction);
    return out;
}
inline RivalLaneForecast ForecastPhysicalRivalLanes(
    const RivalLaneTrafficFrame& f,
    RivalLaneSettings p={}) noexcept
{
    RivalLaneForecast out;
    if (!ValidRivalLaneFrame(f,p))return out;
    out.Identity=f.Self.Identity;
    out.ObservedActors=PhysicalFleetCapacity;
    out.MinimumRivalConfidence=f.Self.SensorConfidence;
    out.ProposedThrottle=f.CurrentThrottle;
    out.ProposedBrake=f.CurrentBrake;
    out.ProposedSteering=f.CurrentSteering;
    for(int i=0;i<f.OtherCount;++i)
    {
        const auto& actor=f.Others[
            static_cast<std::size_t>(i)];
        const RivalLaneContact contact=
            AnalyzeRivalLaneContact(f.Self,actor,p);
        if (!contact.PhysicallyQualified)return {};
        out.Contacts[static_cast<std::size_t>(i)]=contact;
        ++out.VerifiedOtherCars;
        out.MinimumRivalConfidence=std::min(
            out.MinimumRivalConfidence,
            actor.SensorConfidence);
        if(contact.PredictedOverlap)
        {
            ++out.TotalPotentialContacts;
            out.MinimumTrafficTimeToContactSeconds=
                std::min(
                    out.MinimumTrafficTimeToContactSeconds,
                    contact.PredictedContactSeconds);
        }
        out.NearestPhysicalCarCm=std::min(
            out.NearestPhysicalCarCm,
            std::hypot(contact.ForwardDistanceCm,
                       contact.LateralDistanceCm));
        out.MaximumActualClosingSpeedCmS=std::max(
            out.MaximumActualClosingSpeedCmS,
            std::max(0.0,-contact.RelativeForwardSpeedCmS));
    }
    double bestScore=std::numeric_limits<double>::infinity();
    for(int lane=0;lane<RivalLaneCount;++lane)
    {
        const RivalLaneCandidate candidate=
            EvaluateRivalLaneCandidate(
                f,out.Contacts,static_cast<RivalLaneClass>(lane),p);
        if (!candidate.Valid)return {};
        out.Lanes[static_cast<std::size_t>(lane)]=candidate;
        ++out.ValidLaneCount;
        const double clearance=std::min(
            candidate.AvailableLeftClearanceCm,
            candidate.AvailableRightClearanceCm);
        out.MinimumRoadClearanceCm=lane==0 ?
            clearance:std::min(out.MinimumRoadClearanceCm,clearance);
        if(candidate.RoadEdgeVerified &&
           candidate.SweptMergeClear &&
           candidate.Hazard==RivalLaneHazard::Clear)
        {
            ++out.SafelyAvailableLanes;
            const double deviation=std::abs(
                candidate.LaneOffsetCm-
                f.Self.TrackLateralOffsetCm);
            const double score=deviation+
                0.1*std::abs(candidate.SteeringOffsetFraction);
            if(score<bestScore)
            {
                bestScore=score;
                out.PreferredSafeLane=lane;
            }
        }
        out.TotalLateralRiskScore+=
            candidate.PotentialContacts*1.0+
            candidate.OverlappingCars*0.6+
            candidate.ApproachingRearCars*0.4;
    }
    const auto& currentLane=out.Lanes[1];
    out.ProposedThrottle=std::min(
        out.ProposedThrottle,
        currentLane.RecommendedThrottleCeiling);
    out.ProposedBrake=std::max(
        out.ProposedBrake,
        currentLane.RecommendedBrakeFloor);
    out.EmergencyBrake=
        currentLane.MinimumTimeToContactSeconds<
            p.EmergencyTimeToContactSeconds &&
        currentLane.PotentialContacts>0;
    if (out.EmergencyBrake)
    {
        out.ProposedThrottle=0.0;
        out.ProposedBrake=1.0;
    }
    if (out.ProposedBrake>0.05)
        out.ProposedThrottle=0.0;
    out.RoverMustHoldLane=true;
    out.NoUnrealActuation=true;
    out.Valid=out.VerifiedOtherCars==RivalTrafficCount &&
        out.ValidLaneCount==RivalLaneCount &&
        Finite(out.TotalLateralRiskScore) &&
        Finite(out.ProposedThrottle) &&
        Finite(out.ProposedBrake);
    return out;
}
class RivalLaneOccupancySupervisor final
{
public:
    bool Configure(RivalLaneSettings p) noexcept
    {
        if (!ValidRivalLaneSettings(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Identity={};
        Roster.fill(0);
        HasRoster=false;
        Qualified=0;
        Rejected=0;
        UnsafeMergeFrames=0;
        EmergencyFrames=0;
        HighestRisk=0.0;
    }
    RivalLaneForecast Step(
        const RivalLaneTrafficFrame& input) noexcept
    {
        auto forecast=ForecastPhysicalRivalLanes(input,Settings);
        const bool sameEpoch=HasRoster &&
            Identity.Session==input.Self.Identity.Session &&
            Identity.SessionRevision==
                input.Self.Identity.SessionRevision &&
            Identity.Actor==input.Self.Identity.Actor;
        if (!forecast.Valid ||
            (sameEpoch &&
             input.Self.Identity.Frame<=Identity.Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        std::array<std::uint64_t,PhysicalFleetCapacity> now{};
        now[static_cast<std::size_t>(
            input.Self.Identity.Slot)]=input.Self.Identity.Actor;
        for(const auto& rival:input.Others)
            now[static_cast<std::size_t>(
                rival.Identity.Slot)]=rival.Identity.Actor;
        if (HasRoster && now!=Roster)
        {
            // A new physical car invalidates merge commitment. Hold the
            // current lane and require fresh traffic clearance next frame.
            forecast.ProposedThrottle=0.0;
            forecast.ProposedBrake=1.0;
            forecast.EmergencyBrake=true;
        }
        Identity=input.Self.Identity;
        Roster=now;
        HasRoster=true;
        Last=forecast;
        ++Qualified;
        UnsafeMergeFrames+=
            forecast.SafelyAvailableLanes==0?1U:0U;
        EmergencyFrames+=forecast.EmergencyBrake?1U:0U;
        HighestRisk=std::max(
            HighestRisk,forecast.TotalLateralRiskScore);
        return Last;
    }
    RivalLaneForecast Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t NoSafeLaneFrames() const noexcept
    {return UnsafeMergeFrames;}
    std::uint64_t EmergencyBrakeFrames() const noexcept
    {return EmergencyFrames;}
    double MaximumAggregateRisk() const noexcept
    {return HighestRisk;}
private:
    RivalLaneSettings Settings{};
    RivalLaneForecast Last{};
    RaceIdentity Identity{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t UnsafeMergeFrames=0;
    std::uint64_t EmergencyFrames=0;
    double HighestRisk=0.0;
    bool HasRoster=false;
};
} // namespace latebrake::racecraft
