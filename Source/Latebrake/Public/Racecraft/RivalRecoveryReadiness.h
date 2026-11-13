#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class RecoveryCause : std::uint8_t
{
    Unknown, StuckOffroad, VehicleSpun,
    BarrierWedge, PhysicsFall, WheelSupportLost,
    RacerManualRequest, Timeout
};
enum class RecoveryVeto : std::uint8_t
{
    None, MissingPhysicalCar, WrongSession,
    InsufficientStuckTime, RoadUnavailable,
    OccupiedRecoveryLocation, UnsafeTraffic,
    WrongGamePhase, BadHeading, UnsafeBody,
    MissingWorldStatic, RecoveryCooldown,
    NoAuthenticatedCheckpoint
};
struct RecoveryParameters final
{
    double MinimumStuckSeconds=2.5;
    double MinimumManualRequestSeconds=0.25;
    double MaximumStuckSpeedCmS=80.0;
    double MaximumSpinYawRateRadS=3.0;
    double MinimumGroundContacts=2.0;
    double RecoveryLaneClearanceCm=680.0;
    double MaximumTrafficRangeCm=1700.0;
    double MaximumTrafficSpeedCmS=10000.0;
    double MaximumTrafficApproachCmS=6000.0;
    double MinimumSafeTimeToContactSeconds=1.5;
    double RestartThrottleHoldSeconds=0.50;
    double RecoveryCooldownSeconds=10.0;
    double MaximumChassisHalfLengthCm=260.0;
    double MaximumChassisHalfWidthCm=160.0;
    double MaximumHeadingErrorRadians=0.32;
    double MaximumValidFrameSeconds=0.1;
    bool RequireOriginalCheckpoint=true;
    bool RequireActualWorldStatic=true;
    bool RequireWholeCarOnAsphalt=true;
    bool NeverTeleportWithoutGameMode=true;
    bool RequireAllOtherActors=true;
};
struct RecoveryActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    Vec3 WorldPositionCm{};
    double HalfLengthCm=210.0;
    double HalfWidthCm=94.0;
    int NextCheckpointIndex=0;
    double TimeStuckSeconds=0.0;
    double TimeSinceLastRecoverySeconds=1000.0;
    int PhysicalGroundProbes=4;
    bool OriginalCollisionBody=true;
    bool OriginalWorldStatic=true;
    bool RealRecoveryRequested=false;
    bool TrueGameModeAuthorized=false;
};
struct RecoverySituation final
{
    RecoveryActor Self{};
    std::array<RecoveryActor,PhysicalFleetCapacity-1> Nearby{};
    int NearbyCount=0;
    RecoveryCause Cause=RecoveryCause::Unknown;
    double RaceClockSeconds=0.0;
    double DeltaSeconds=0.0;
    bool OriginalRaceRunning=true;
    bool OriginalMapReady=true;
    bool ActualRecoveryGateVerified=false;
    bool TrafficRosterComplete=false;
};
struct RecoveryCandidate final
{
    Pose2 OriginalCheckpointPose{};
    Rect2 RequiredFootprint{};
    RecoveryVeto Reason=RecoveryVeto::MissingPhysicalCar;
    int RecoveryGate=-1;
    int NearestTrafficSlot=-1;
    double DistanceToClosestCarCm=0.0;
    double MinimumPredictedTrafficClearanceCm=0.0;
    double TimeToNearestContactSeconds=0.0;
    double RequiredThrottleHoldSeconds=0.0;
    double RoadMarginCm=0.0;
    int OriginalCarsReviewed=0;
    bool CheckpointPositionAuthenticated=false;
    bool CandidateBodyFullyInsideOneSlab=false;
    bool OriginalWorldStaticConfirmed=false;
    bool ActuallyTeleportedCar=false;
    bool SafeForOwnerReview=false;
    bool Valid=false;
};
inline bool ValidRecoveryParameters(RecoveryParameters p) noexcept
{
    return Finite(p.MinimumStuckSeconds) &&
        p.MinimumStuckSeconds>0.0 &&
        Finite(p.MinimumManualRequestSeconds) &&
        p.MinimumManualRequestSeconds>=0.0 &&
        Finite(p.MaximumStuckSpeedCmS) &&
        p.MaximumStuckSpeedCmS>0.0 &&
        Finite(p.MaximumSpinYawRateRadS) &&
        p.MaximumSpinYawRateRadS>0.0 &&
        Finite(p.MinimumGroundContacts) &&
        p.MinimumGroundContacts>=0.0 &&
        p.MinimumGroundContacts<=4.0 &&
        Finite(p.RecoveryLaneClearanceCm) &&
        p.RecoveryLaneClearanceCm>0.0 &&
        Finite(p.MaximumTrafficRangeCm) &&
        p.MaximumTrafficRangeCm>
            p.RecoveryLaneClearanceCm &&
        Finite(p.MaximumTrafficSpeedCmS) &&
        p.MaximumTrafficSpeedCmS>0.0 &&
        Finite(p.MinimumSafeTimeToContactSeconds) &&
        p.MinimumSafeTimeToContactSeconds>0.0 &&
        Finite(p.RecoveryCooldownSeconds) &&
        p.RecoveryCooldownSeconds>=0.0 &&
        Finite(p.MaximumChassisHalfLengthCm) &&
        p.MaximumChassisHalfLengthCm>0.0 &&
        Finite(p.MaximumChassisHalfWidthCm) &&
        p.MaximumChassisHalfWidthCm>0.0 &&
        Finite(p.MaximumValidFrameSeconds) &&
        p.MaximumValidFrameSeconds>0.0 &&
        p.MaximumValidFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidRecoveryActor(
    RecoveryActor actor,RecoveryParameters p) noexcept
{
    return PhysicalActorIdentityValid(actor.Identity) &&
        actor.OriginalCollisionBody &&
        actor.OriginalWorldStatic &&
        Finite(actor.Pose) &&
        Finite(actor.VelocityCmS) &&
        Finite(actor.WorldPositionCm) &&
        Finite(actor.HalfLengthCm) &&
        Finite(actor.HalfWidthCm) &&
        Finite(actor.TimeStuckSeconds) &&
        Finite(actor.TimeSinceLastRecoverySeconds) &&
        actor.HalfLengthCm>0.0 &&
        actor.HalfLengthCm<=p.MaximumChassisHalfLengthCm &&
        actor.HalfWidthCm>0.0 &&
        actor.HalfWidthCm<=p.MaximumChassisHalfWidthCm &&
        actor.TimeStuckSeconds>=0.0 &&
        actor.TimeSinceLastRecoverySeconds>=0.0 &&
        actor.NextCheckpointIndex>=0 &&
        actor.NextCheckpointIndex<OriginalSegmentCount &&
        actor.PhysicalGroundProbes>=0 &&
        actor.PhysicalGroundProbes<=4 &&
        Length(actor.VelocityCmS)<=
            p.MaximumTrafficSpeedCmS;
}
inline bool ValidRecoverySituation(
    const RecoverySituation& sample,
    RecoveryParameters p) noexcept
{
    if (!sample.OriginalRaceRunning ||
        !sample.OriginalMapReady ||
        !sample.ActualRecoveryGateVerified ||
        !sample.TrafficRosterComplete ||
        !ValidFrameDelta(sample.DeltaSeconds) ||
        sample.DeltaSeconds>p.MaximumValidFrameSeconds ||
        !Finite(sample.RaceClockSeconds) ||
        sample.RaceClockSeconds<0.0 ||
        !ValidRecoveryActor(sample.Self,p) ||
        sample.NearbyCount!=PhysicalFleetCapacity-1)
        return false;
    for (int i=0;i<sample.NearbyCount;++i)
    {
        const auto& other=sample.Nearby[
            static_cast<std::size_t>(i)];
        if (!ValidRecoveryActor(other,p) ||
            other.Identity.Actor==sample.Self.Identity.Actor ||
            other.Identity.Session!=sample.Self.Identity.Session ||
            other.Identity.Slot==sample.Self.Identity.Slot)
            return false;
        for (int j=0;j<i;++j)
        {
            if (sample.Nearby[static_cast<std::size_t>(j)].
                Identity.Actor==other.Identity.Actor)
                return false;
        }
    }
    return true;
}
inline RecoveryCandidate EvaluateRecoveryReadiness(
    const RecoverySituation& s,
    RecoveryParameters p={}) noexcept
{
    RecoveryCandidate out;
    if (!ValidRecoveryParameters(p) ||
        !ValidRecoverySituation(s,p))
        return out;
    out.Valid=true;
    out.RecoveryGate=s.Self.NextCheckpointIndex;
    const auto recovery=latebrake::PlayerRecovery(
        s.Self.NextCheckpointIndex);
    out.OriginalCheckpointPose={
        {recovery.Position.X,recovery.Position.Y},
        recovery.HeadingDegrees*Pi/180.0
    };
    out.RequiredFootprint={
        out.OriginalCheckpointPose.Position,
        {s.Self.HalfLengthCm,
         s.Self.HalfWidthCm},
        out.OriginalCheckpointPose.HeadingRadians
    };
    out.CheckpointPositionAuthenticated=true;
    out.CandidateBodyFullyInsideOneSlab=
        AnyOriginalSlabContains(out.RequiredFootprint);
    out.RoadMarginCm=DistanceToNearestSlabEdge(
        out.OriginalCheckpointPose.Position);
    if (!out.CandidateBodyFullyInsideOneSlab)
    {
        out.Reason=RecoveryVeto::RoadUnavailable;
        return out;
    }
    if (s.Self.TimeSinceLastRecoverySeconds<
        p.RecoveryCooldownSeconds)
    {
        out.Reason=RecoveryVeto::RecoveryCooldown;
        return out;
    }
    const bool stuck=s.Self.TimeStuckSeconds>=
        p.MinimumStuckSeconds &&
        Length(s.Self.VelocityCmS)<
            p.MaximumStuckSpeedCmS;
    if (!stuck && !s.Self.RealRecoveryRequested)
    {
        out.Reason=RecoveryVeto::InsufficientStuckTime;
        return out;
    }
    out.DistanceToClosestCarCm=
        std::numeric_limits<double>::infinity();
    out.MinimumPredictedTrafficClearanceCm=
        std::numeric_limits<double>::infinity();
    out.TimeToNearestContactSeconds=
        std::numeric_limits<double>::infinity();
    for (int i=0;i<s.NearbyCount;++i)
    {
        const auto& other=s.Nearby[
            static_cast<std::size_t>(i)];
        const Vec2 offset=other.Pose.Position-
            out.OriginalCheckpointPose.Position;
        const double distance=Length(offset);
        out.DistanceToClosestCarCm=std::min(
            out.DistanceToClosestCarCm,distance);
        const double clearance=distance-
            (s.Self.HalfLengthCm+other.HalfLengthCm+
             p.RecoveryLaneClearanceCm);
        out.MinimumPredictedTrafficClearanceCm=std::min(
            out.MinimumPredictedTrafficClearanceCm,
            clearance);
        ++out.OriginalCarsReviewed;
        if (RectsOverlap(out.RequiredFootprint,{
            other.Pose.Position,
            {other.HalfLengthCm,other.HalfWidthCm},
            other.Pose.HeadingRadians},p.RecoveryLaneClearanceCm))
        {
            out.Reason=RecoveryVeto::OccupiedRecoveryLocation;
            out.NearestTrafficSlot=other.Identity.Slot;
            return out;
        }
        const Vec2 approach=Normalized(
            out.OriginalCheckpointPose.Position-
            other.Pose.Position);
        const double approaching=std::max(
            0.0,Dot(other.VelocityCmS,approach));
        const double time=approaching>1.0 ?
            distance/approaching :
            std::numeric_limits<double>::infinity();
        out.TimeToNearestContactSeconds=std::min(
            out.TimeToNearestContactSeconds,time);
        if (time<p.MinimumSafeTimeToContactSeconds)
        {
            out.Reason=RecoveryVeto::UnsafeTraffic;
            out.NearestTrafficSlot=other.Identity.Slot;
            return out;
        }
    }
    if (!s.Self.TrueGameModeAuthorized)
    {
        out.Reason=RecoveryVeto::MissingWorldStatic;
        return out;
    }
    out.Reason=RecoveryVeto::None;
    out.RequiredThrottleHoldSeconds=
        p.RestartThrottleHoldSeconds;
    out.OriginalWorldStaticConfirmed=true;
    out.ActuallyTeleportedCar=false;
    out.SafeForOwnerReview=true;
    return out;
}
struct RecoveryHistory final
{
    std::uint64_t Assessed=0;
    std::uint64_t Rejected=0;
    std::uint64_t SafeCandidates=0;
    std::uint64_t TrafficVetoes=0;
    std::uint64_t RoadVetoes=0;
    double LastAssessedRaceClock=0.0;
    double MinimumClearanceCm=0.0;
    bool ActualRecoveryIssued=false;
};
class RivalRecoveryReadinessMonitor final
{
public:
    bool Configure(RecoveryParameters p) noexcept
    {
        if (!ValidRecoveryParameters(p))return false;
        Params=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        History={};
        Session=0;
        Actor=0;
    }
    RecoveryCandidate Observe(const RecoverySituation& s) noexcept
    {
        const auto candidate=EvaluateRecoveryReadiness(s,Params);
        if (!candidate.Valid ||
            (Session!=0 &&
             (s.Self.Identity.Session!=Session ||
              s.Self.Identity.Actor!=Actor)))
        {
            ++History.Rejected;
            return {};
        }
        Session=s.Self.Identity.Session;
        Actor=s.Self.Identity.Actor;
        ++History.Assessed;
        if (candidate.SafeForOwnerReview)++History.SafeCandidates;
        if (candidate.Reason==RecoveryVeto::UnsafeTraffic ||
            candidate.Reason==RecoveryVeto::OccupiedRecoveryLocation)
            ++History.TrafficVetoes;
        if (candidate.Reason==RecoveryVeto::RoadUnavailable)
            ++History.RoadVetoes;
        History.LastAssessedRaceClock=s.RaceClockSeconds;
        History.MinimumClearanceCm=
            candidate.MinimumPredictedTrafficClearanceCm;
        History.ActualRecoveryIssued=false;
        Last=candidate;
        return Last;
    }
    RecoveryCandidate Snapshot() const noexcept{return Last;}
    RecoveryHistory Statistics() const noexcept{return History;}
private:
    RecoveryParameters Params{};
    RecoveryCandidate Last{};
    RecoveryHistory History{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
};
} // namespace latebrake::racecraft
