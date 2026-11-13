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
enum class PitApproachPhase : std::uint8_t
{
    Outside, EntryAwareness, BrakeToLimit, UnderLimiter,
    SearchForBox, InBox, Service, AwaitRelease,
    MergeToTrack, Cleared, UnsafeHold
};
enum class PitApproachHazard : std::uint8_t
{
    None, UnqualifiedPit, Overspeed, MissingBrake,
    EntryClosed, OccupiedPitBox, UnsafeMerge,
    LossOfTraction, PitBarrier, WrongDirection,
    InvalidRacePhase, StalePhysicalFrame
};
inline constexpr int PitMergeObstacleCount=PhysicalFleetCapacity-1;
struct PitApproachParameters final
{
    double PitSpeedLimitCmS=2200.0;
    double MaximumPitSpeedCmS=6000.0;
    double MaximumEntrySpeedCmS=7500.0;
    double PitBrakeReactionSeconds=0.28;
    double PitExpectedDecelerationCmS2=680.0;
    double PitMinimumDecelerationCmS2=260.0;
    double PitMaximumDecelerationCmS2=1250.0;
    double PitSafetyMarginCm=250.0;
    double PitEntryAwarenessCm=3200.0;
    double PitLimiterToleranceCmS=100.0;
    double PitEntryBoundaryCm=0.0;
    double PitExitBoundaryCm=15000.0;
    double PitLaneLengthCm=15000.0;
    double PitLaneHalfWidthCm=450.0;
    double PitCarHalfWidthCm=95.0;
    double PitCarHalfLengthCm=220.0;
    double MinimumWallClearanceCm=85.0;
    double PitBoxStoppingToleranceCm=65.0;
    double PitBoxHalfLengthCm=450.0;
    double PitBoxHalfWidthCm=240.0;
    double MinimumBoxReservationSeconds=0.2;
    double MaximumPitStopSeconds=180.0;
    double MinimumPitStopSeconds=1.0;
    double MaximumPitMergeGapCm=4200.0;
    double MinimumPitMergeGapCm=950.0;
    double MinimumPitMergeTimeGapSeconds=1.2;
    double MaximumMergeSpeedCmS=6500.0;
    double MaximumRelativeClosingCmS=7500.0;
    double MergeForecastSeconds=2.2;
    double MinimumMergeVisibilityCm=1800.0;
    double MaximumYawErrorRadians=0.45;
    double MaximumCrossTrackCm=400.0;
    double MinimumGripG=0.20;
    double MaximumGripG=2.5;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.03;
    double LimiterBrakeFloor=0.12;
    double EmergencyBrakeFraction=1.0;
    double ThrottleCeilingInPit=0.35;
    double ReleaseThrottleRisePerSecond=0.8;
    double BrakeReleasePerSecond=1.6;
    double MinimumSensorConfidence=0.8;
    bool RequirePhysicalOriginalPit=true;
    bool RequireRealPitEntryTrigger=true;
    bool RequireAllRivalMergeEvidence=true;
    bool NeverAuthorizePitReleaseWithoutTrackGap=true;
    bool NeverCreditOfficialLap=true;
};
struct PhysicalPitMergeActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    double DistanceToMergeCm=0.0;
    double TrackLateralOffsetCm=0.0;
    double HalfLengthCm=220.0;
    double HalfWidthCm=95.0;
    double SensorConfidence=1.0;
    double PhysicsClockSeconds=0.0;
    bool EngineObserved=false;
    bool OriginalTrackLane=true;
    bool ActualChaosVelocityReady=false;
};
struct PhysicalPitApproachFrame final
{
    RaceIdentity Identity{};
    Pose2 CarPose{};
    Vec2 CarVelocityCmS{};
    std::array<PhysicalPitMergeActor,PitMergeObstacleCount> MergeTraffic{};
    int MergeTrafficCount=0;
    double DistanceUntilPitEntryCm=0.0;
    double DistanceAlongPitLaneCm=0.0;
    double DistanceUntilPitBoxCm=0.0;
    double DistanceUntilPitExitCm=0.0;
    double PhysicalPitHalfWidthCm=450.0;
    double PhysicalWallClearanceCm=400.0;
    double ChassisYawRateRadS=0.0;
    double ActualGripG=1.0;
    double ClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double DriverThrottle=0.0;
    double DriverBrake=0.0;
    double DriverSteering=0.0;
    double PitMergeVisibilityCm=4500.0;
    double SensorConfidence=1.0;
    bool ActualChaosBodyReady=false;
    bool OriginalPitMapReady=false;
    bool EnginePitEntryTrigger=false;
    bool PitLaneOpenedByRaceDirector=false;
    bool PitBoxPhysicallyReserved=false;
    bool PitBoxActuallyOccupied=false;
    bool PitServiceComplete=false;
    bool PitReleaseAuthorized=false;
    bool CurrentlyInPitLane=false;
    bool HasEnteredBox=false;
    bool EngineRaceRunning=false;
};
struct PitApproachMergeReview final
{
    int Slot=-1;
    std::uint64_t Actor=0;
    double RelativeAlongCm=0.0;
    double RelativeSpeedCmS=0.0;
    double PredictedMergeGapCm=0.0;
    double PredictedMergeTimeSeconds=0.0;
    double MinimumSafeGapCm=0.0;
    bool AheadOfMerge=false;
    bool ClosingFromBehind=false;
    bool OccupiesMergeCorridor=false;
    bool MergePermitted=false;
    bool PhysicalEvidenceQualified=false;
};
struct PitApproachDecision final
{
    RaceIdentity Identity{};
    PitApproachPhase Phase=PitApproachPhase::UnsafeHold;
    PitApproachHazard Hazard=PitApproachHazard::UnqualifiedPit;
    std::array<PitApproachMergeReview,PitMergeObstacleCount> Traffic{};
    int QualifiedMergeRivals=0;
    int BlockingMergeRivals=0;
    double SpeedCmS=0.0;
    double CurrentSpeedLimitCmS=0.0;
    double DistanceToSpeedBoundaryCm=0.0;
    double BrakingDistanceRequiredCm=0.0;
    double ExpectedBrakeDecelerationCmS2=0.0;
    double MinimumMergeGapCm=std::numeric_limits<double>::infinity();
    double MinimumTimeGapSeconds=std::numeric_limits<double>::infinity();
    double PitWallReserveCm=0.0;
    double PitBoxAlignmentErrorCm=0.0;
    double ExpectedPitStopRemainingSeconds=0.0;
    double ThrottleCeiling=0.0;
    double BrakeFloor=1.0;
    double CommandThrottle=0.0;
    double CommandBrake=1.0;
    double CommandSteering=0.0;
    bool PitLimiterRequired=false;
    bool PitBoxStopRequired=false;
    bool PhysicallyInBox=false;
    bool MergeLaneClear=false;
    bool EmergencyBrake=false;
    bool ActualUnrealCommandIssued=false;
    bool OfficialLapOrPenaltyChanged=false;
    bool Valid=false;
};
inline bool ValidPitApproachParameters(
    PitApproachParameters p) noexcept
{
    return Finite(p.PitSpeedLimitCmS) &&
        p.PitSpeedLimitCmS>0.0 &&
        Finite(p.MaximumPitSpeedCmS) &&
        p.MaximumPitSpeedCmS>p.PitSpeedLimitCmS &&
        Finite(p.MaximumEntrySpeedCmS) &&
        p.MaximumEntrySpeedCmS>=p.MaximumPitSpeedCmS &&
        Finite(p.PitBrakeReactionSeconds) &&
        p.PitBrakeReactionSeconds>=0.0 &&
        Finite(p.PitExpectedDecelerationCmS2) &&
        p.PitExpectedDecelerationCmS2>0.0 &&
        Finite(p.PitMinimumDecelerationCmS2) &&
        p.PitMinimumDecelerationCmS2>0.0 &&
        p.PitMinimumDecelerationCmS2<=
            p.PitExpectedDecelerationCmS2 &&
        Finite(p.PitMaximumDecelerationCmS2) &&
        p.PitMaximumDecelerationCmS2>=
            p.PitExpectedDecelerationCmS2 &&
        Finite(p.PitSafetyMarginCm) &&
        p.PitSafetyMarginCm>=0.0 &&
        Finite(p.PitEntryAwarenessCm) &&
        p.PitEntryAwarenessCm>0.0 &&
        Finite(p.PitLimiterToleranceCmS) &&
        p.PitLimiterToleranceCmS>=0.0 &&
        Finite(p.PitLaneLengthCm) &&
        p.PitLaneLengthCm>2000.0 &&
        Finite(p.PitLaneHalfWidthCm) &&
        p.PitLaneHalfWidthCm>0.0 &&
        Finite(p.PitCarHalfWidthCm) &&
        p.PitCarHalfWidthCm>0.0 &&
        Finite(p.PitCarHalfLengthCm) &&
        p.PitCarHalfLengthCm>0.0 &&
        Finite(p.MinimumWallClearanceCm) &&
        p.MinimumWallClearanceCm>=0.0 &&
        Finite(p.PitBoxStoppingToleranceCm) &&
        p.PitBoxStoppingToleranceCm>=0.0 &&
        Finite(p.PitBoxHalfLengthCm) &&
        p.PitBoxHalfLengthCm>0.0 &&
        Finite(p.PitBoxHalfWidthCm) &&
        p.PitBoxHalfWidthCm>0.0 &&
        Finite(p.MinimumBoxReservationSeconds) &&
        p.MinimumBoxReservationSeconds>=0.0 &&
        Finite(p.MaximumPitStopSeconds) &&
        p.MaximumPitStopSeconds>0.0 &&
        Finite(p.MinimumPitStopSeconds) &&
        p.MinimumPitStopSeconds>0.0 &&
        p.MinimumPitStopSeconds<p.MaximumPitStopSeconds &&
        Finite(p.MaximumPitMergeGapCm) &&
        p.MaximumPitMergeGapCm>0.0 &&
        Finite(p.MinimumPitMergeGapCm) &&
        p.MinimumPitMergeGapCm>0.0 &&
        p.MinimumPitMergeGapCm<p.MaximumPitMergeGapCm &&
        Finite(p.MinimumPitMergeTimeGapSeconds) &&
        p.MinimumPitMergeTimeGapSeconds>0.0 &&
        Finite(p.MaximumMergeSpeedCmS) &&
        p.MaximumMergeSpeedCmS>0.0 &&
        Finite(p.MaximumRelativeClosingCmS) &&
        p.MaximumRelativeClosingCmS>0.0 &&
        Finite(p.MergeForecastSeconds) &&
        p.MergeForecastSeconds>0.0 &&
        Finite(p.MinimumMergeVisibilityCm) &&
        p.MinimumMergeVisibilityCm>0.0 &&
        Finite(p.MaximumYawErrorRadians) &&
        p.MaximumYawErrorRadians>0.0 &&
        Finite(p.MaximumCrossTrackCm) &&
        p.MaximumCrossTrackCm>0.0 &&
        Finite(p.MinimumGripG) &&
        p.MinimumGripG>0.0 &&
        Finite(p.MaximumGripG) &&
        p.MaximumGripG>p.MinimumGripG &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.LimiterBrakeFloor) &&
        p.LimiterBrakeFloor>=0.0 &&
        p.LimiterBrakeFloor<=1.0 &&
        Finite(p.EmergencyBrakeFraction) &&
        p.EmergencyBrakeFraction>0.0 &&
        p.EmergencyBrakeFraction<=1.0 &&
        Finite(p.ThrottleCeilingInPit) &&
        p.ThrottleCeilingInPit>=0.0 &&
        p.ThrottleCeilingInPit<=1.0 &&
        Finite(p.ReleaseThrottleRisePerSecond) &&
        p.ReleaseThrottleRisePerSecond>0.0 &&
        Finite(p.BrakeReleasePerSecond) &&
        p.BrakeReleasePerSecond>0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0;
}
inline bool ValidPhysicalPitMergeActor(
    PhysicalPitMergeActor actor,
    PitApproachParameters p) noexcept
{
    return PhysicalActorIdentityValid(actor.Identity) &&
        actor.Identity.Frame>0 &&
        actor.EngineObserved &&
        actor.OriginalTrackLane &&
        actor.ActualChaosVelocityReady &&
        Finite(actor.Pose) &&
        Finite(actor.VelocityCmS) &&
        Length(actor.VelocityCmS)<=p.MaximumEntrySpeedCmS &&
        Finite(actor.DistanceToMergeCm) &&
        std::abs(actor.DistanceToMergeCm)<=
            p.MaximumPitMergeGapCm*3.0 &&
        Finite(actor.TrackLateralOffsetCm) &&
        std::abs(actor.TrackLateralOffsetCm)<=
            p.MaximumCrossTrackCm*3.0 &&
        Finite(actor.HalfLengthCm) &&
        actor.HalfLengthCm>0.0 &&
        actor.HalfLengthCm<500.0 &&
        Finite(actor.HalfWidthCm) &&
        actor.HalfWidthCm>0.0 &&
        actor.HalfWidthCm<250.0 &&
        Finite(actor.SensorConfidence) &&
        actor.SensorConfidence>=p.MinimumSensorConfidence &&
        actor.SensorConfidence<=1.0 &&
        Finite(actor.PhysicsClockSeconds) &&
        actor.PhysicsClockSeconds>=0.0;
}
inline bool ValidPhysicalPitFrame(
    const PhysicalPitApproachFrame& f,
    PitApproachParameters p) noexcept
{
    if (!ValidPitApproachParameters(p) ||
        !PhysicalActorIdentityValid(f.Identity) ||
        f.Identity.Frame==0 ||
        !f.ActualChaosBodyReady ||
        !f.OriginalPitMapReady ||
        !f.EngineRaceRunning ||
        (!f.EnginePitEntryTrigger &&
         !f.CurrentlyInPitLane) ||
        !Finite(f.CarPose) ||
        !Finite(f.CarVelocityCmS) ||
        Length(f.CarVelocityCmS)>p.MaximumEntrySpeedCmS ||
        !Finite(f.DistanceUntilPitEntryCm) ||
        f.DistanceUntilPitEntryCm<0.0 ||
        !Finite(f.DistanceAlongPitLaneCm) ||
        f.DistanceAlongPitLaneCm<0.0 ||
        f.DistanceAlongPitLaneCm>p.PitLaneLengthCm ||
        !Finite(f.DistanceUntilPitBoxCm) ||
        !Finite(f.DistanceUntilPitExitCm) ||
        f.DistanceUntilPitExitCm<0.0 ||
        !Finite(f.PhysicalPitHalfWidthCm) ||
        f.PhysicalPitHalfWidthCm<p.PitLaneHalfWidthCm*0.5 ||
        !Finite(f.PhysicalWallClearanceCm) ||
        f.PhysicalWallClearanceCm>=p.MinimumWallClearanceCm*10.0 ||
        f.PhysicalWallClearanceCm<0.0 ||
        !Finite(f.ChassisYawRateRadS) ||
        std::abs(f.ChassisYawRateRadS)>8.0 ||
        !Finite(f.ActualGripG) ||
        f.ActualGripG<p.MinimumGripG ||
        f.ActualGripG>p.MaximumGripG ||
        !Finite(f.ClockSeconds) ||
        f.ClockSeconds<0.0 ||
        !Finite(f.DeltaSeconds) ||
        f.DeltaSeconds<=0.0 ||
        f.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(f.DriverThrottle) ||
        f.DriverThrottle<0.0 ||
        f.DriverThrottle>1.0 ||
        !Finite(f.DriverBrake) ||
        f.DriverBrake<0.0 ||
        f.DriverBrake>1.0 ||
        !Finite(f.DriverSteering) ||
        std::abs(f.DriverSteering)>1.0 ||
        !Finite(f.PitMergeVisibilityCm) ||
        f.PitMergeVisibilityCm<0.0 ||
        !Finite(f.SensorConfidence) ||
        f.SensorConfidence<p.MinimumSensorConfidence ||
        f.SensorConfidence>1.0 ||
        f.MergeTrafficCount<0 ||
        f.MergeTrafficCount>PitMergeObstacleCount)
        return false;
    if(p.RequireAllRivalMergeEvidence &&
       f.CurrentlyInPitLane &&
       f.DistanceUntilPitExitCm<p.MaximumPitMergeGapCm &&
       f.MergeTrafficCount!=PitMergeObstacleCount)
        return false;
    std::array<std::uint64_t,PhysicalFleetCapacity> actors{};
    actors[static_cast<std::size_t>(
        f.Identity.Slot)]=f.Identity.Actor;
    for(int i=0;i<f.MergeTrafficCount;++i)
    {
        const auto& actor=f.MergeTraffic[
            static_cast<std::size_t>(i)];
        if (!ValidPhysicalPitMergeActor(actor,p) ||
            actor.Identity.Session!=f.Identity.Session ||
            actor.Identity.SessionRevision!=
                f.Identity.SessionRevision ||
            actor.Identity.Frame!=f.Identity.Frame ||
            std::abs(actor.PhysicsClockSeconds-
                     f.ClockSeconds)>p.MaximumClockSkewSeconds ||
            actors[static_cast<std::size_t>(
                actor.Identity.Slot)]!=0)
            return false;
        actors[static_cast<std::size_t>(
            actor.Identity.Slot)]=actor.Identity.Actor;
    }
    return true;
}
inline PitApproachMergeReview InspectPitMergeActor(
    const PhysicalPitApproachFrame& f,
    const PhysicalPitMergeActor& other,
    PitApproachParameters p) noexcept
{
    PitApproachMergeReview out;
    if (!ValidPhysicalPitMergeActor(other,p))
        return out;
    out.Slot=other.Identity.Slot;
    out.Actor=other.Identity.Actor;
    const Vec2 forward=HeadingVector(f.CarPose.HeadingRadians);
    const Vec2 offset=other.Pose.Position-
        f.CarPose.Position;
    const Vec2 relative=other.VelocityCmS-
        f.CarVelocityCmS;
    out.RelativeAlongCm=Dot(offset,forward);
    out.RelativeSpeedCmS=Dot(relative,forward);
    out.AheadOfMerge=out.RelativeAlongCm>0.0;
    out.ClosingFromBehind=
        out.RelativeAlongCm<0.0 &&
        out.RelativeSpeedCmS>0.0;
    const double projected=out.RelativeAlongCm+
        out.RelativeSpeedCmS*p.MergeForecastSeconds;
    out.PredictedMergeGapCm=std::abs(projected)-
        other.HalfLengthCm-p.PitCarHalfLengthCm;
    out.MinimumSafeGapCm=p.MinimumPitMergeGapCm+
        std::max(0.0,out.RelativeSpeedCmS)*
        p.MinimumPitMergeTimeGapSeconds;
    out.PredictedMergeTimeSeconds=
        std::abs(out.RelativeSpeedCmS)>PhysicsEpsilon ?
        SafeDivision(-out.RelativeAlongCm,
                     out.RelativeSpeedCmS,
                     p.MergeForecastSeconds):
        p.MergeForecastSeconds;
    out.OccupiesMergeCorridor=
        std::abs(other.TrackLateralOffsetCm)<
        p.MaximumCrossTrackCm;
    out.MergePermitted=
        !out.OccupiesMergeCorridor ||
        out.PredictedMergeGapCm>=out.MinimumSafeGapCm;
    out.PhysicalEvidenceQualified=
        Finite(out.PredictedMergeGapCm) &&
        Finite(out.PredictedMergeTimeSeconds) &&
        Finite(out.MinimumSafeGapCm);
    return out;
}
inline PitApproachDecision EvaluatePhysicalPitApproach(
    const PhysicalPitApproachFrame& f,
    PitApproachParameters p={}) noexcept
{
    PitApproachDecision out;
    if (!ValidPhysicalPitFrame(f,p))
        return out;
    out.Identity=f.Identity;
    out.SpeedCmS=Length(f.CarVelocityCmS);
    out.CurrentSpeedLimitCmS=p.PitSpeedLimitCmS;
    out.DistanceToSpeedBoundaryCm=
        f.DistanceUntilPitEntryCm;
    out.PitWallReserveCm=f.PhysicalWallClearanceCm-
        p.MinimumWallClearanceCm;
    out.PitBoxAlignmentErrorCm=
        std::abs(f.DistanceUntilPitBoxCm);
    out.ExpectedBrakeDecelerationCmS2=std::min(
        p.PitExpectedDecelerationCmS2,
        f.ActualGripG*GravityCmS2);
    out.BrakingDistanceRequiredCm=
        out.SpeedCmS*p.PitBrakeReactionSeconds+
        SafeDivision(std::max(0.0,
            out.SpeedCmS*out.SpeedCmS-
            p.PitSpeedLimitCmS*p.PitSpeedLimitCmS),
            2.0*std::max(p.PitMinimumDecelerationCmS2,
                         out.ExpectedBrakeDecelerationCmS2))+
        p.PitSafetyMarginCm;
    out.ThrottleCeiling=f.CurrentlyInPitLane ?
        p.ThrottleCeilingInPit:1.0;
    out.BrakeFloor=0.0;
    out.CommandThrottle=f.DriverThrottle;
    out.CommandBrake=f.DriverBrake;
    out.CommandSteering=f.DriverSteering;
    out.PitLimiterRequired=f.CurrentlyInPitLane;
    out.PhysicallyInBox=f.HasEnteredBox &&
        f.PitBoxActuallyOccupied &&
        out.PitBoxAlignmentErrorCm<=
            p.PitBoxStoppingToleranceCm;
    out.PitBoxStopRequired=
        f.CurrentlyInPitLane &&
        !f.PitServiceComplete;
    out.MergeLaneClear=true;
    if (!f.PitLaneOpenedByRaceDirector)
    {
        out.Phase=PitApproachPhase::UnsafeHold;
        out.Hazard=PitApproachHazard::EntryClosed;
        out.EmergencyBrake=true;
    }
    else if(!f.CurrentlyInPitLane &&
            f.DistanceUntilPitEntryCm<
                p.PitEntryAwarenessCm)
    {
        out.Phase=out.BrakingDistanceRequiredCm>=
                f.DistanceUntilPitEntryCm ?
            PitApproachPhase::BrakeToLimit:
            PitApproachPhase::EntryAwareness;
        if(out.Phase==PitApproachPhase::BrakeToLimit)
        {
            out.ThrottleCeiling=0.0;
            out.BrakeFloor=p.LimiterBrakeFloor;
        }
    }
    else if(f.CurrentlyInPitLane)
    {
        if(out.PhysicallyInBox)
            out.Phase=f.PitServiceComplete ?
                PitApproachPhase::AwaitRelease:
                PitApproachPhase::Service;
        else if(f.DistanceUntilPitExitCm<
                p.MaximumPitMergeGapCm)
            out.Phase=PitApproachPhase::MergeToTrack;
        else
            out.Phase=PitApproachPhase::SearchForBox;
        if(out.SpeedCmS>p.PitSpeedLimitCmS+
                p.PitLimiterToleranceCmS)
        {
            out.BrakeFloor=std::max(
                out.BrakeFloor,p.LimiterBrakeFloor);
            out.ThrottleCeiling=0.0;
            out.Hazard=PitApproachHazard::Overspeed;
        }
        if(out.PitBoxStopRequired &&
           out.PitBoxAlignmentErrorCm<
                out.BrakingDistanceRequiredCm+
                p.PitBoxStoppingToleranceCm)
        {
            out.ThrottleCeiling=0.0;
            out.BrakeFloor=std::max(
                out.BrakeFloor,p.LimiterBrakeFloor);
        }
        if(out.PhysicallyInBox &&
           !f.PitServiceComplete)
        {
            out.ThrottleCeiling=0.0;
            out.BrakeFloor=1.0;
        }
    }
    if(out.PitWallReserveCm<0.0 ||
       std::abs(f.ChassisYawRateRadS)>
           p.MaximumYawErrorRadians ||
       f.ActualGripG<p.MinimumGripG)
    {
        out.EmergencyBrake=true;
        out.Hazard=PitApproachHazard::PitBarrier;
    }
    if(out.Phase==PitApproachPhase::MergeToTrack ||
       out.Phase==PitApproachPhase::AwaitRelease)
    {
        if(f.PitMergeVisibilityCm<
           p.MinimumMergeVisibilityCm)
            out.MergeLaneClear=false;
        for(int i=0;i<f.MergeTrafficCount;++i)
        {
            const auto report=InspectPitMergeActor(
                f,f.MergeTraffic[static_cast<std::size_t>(i)],p);
            if (!report.PhysicalEvidenceQualified)
                return {};
            out.Traffic[static_cast<std::size_t>(i)]=report;
            ++out.QualifiedMergeRivals;
            out.MinimumMergeGapCm=std::min(
                out.MinimumMergeGapCm,
                report.PredictedMergeGapCm);
            out.MinimumTimeGapSeconds=std::min(
                out.MinimumTimeGapSeconds,
                report.PredictedMergeTimeSeconds);
            if(!report.MergePermitted)
            {
                ++out.BlockingMergeRivals;
                out.MergeLaneClear=false;
            }
        }
        if(!f.PitReleaseAuthorized ||
           !out.MergeLaneClear)
        {
            out.ThrottleCeiling=0.0;
            out.BrakeFloor=1.0;
            out.Hazard=PitApproachHazard::UnsafeMerge;
            out.EmergencyBrake=true;
        }
    }
    if(out.EmergencyBrake)
    {
        out.ThrottleCeiling=0.0;
        out.BrakeFloor=p.EmergencyBrakeFraction;
    }
    out.CommandThrottle=std::min(
        f.DriverThrottle,out.ThrottleCeiling);
    out.CommandBrake=std::max(
        f.DriverBrake,out.BrakeFloor);
    if(out.CommandBrake>0.05)
        out.CommandThrottle=0.0;
    out.ActualUnrealCommandIssued=false;
    out.OfficialLapOrPenaltyChanged=false;
    out.Valid=Finite(out.BrakingDistanceRequiredCm) &&
        Finite(out.CommandThrottle) &&
        Finite(out.CommandBrake) &&
        Finite(out.PitWallReserveCm);
    return out;
}
class PhysicalPitApproachSupervisor final
{
public:
    bool Configure(PitApproachParameters p) noexcept
    {
        if (!ValidPitApproachParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Source={};
        Phase=PitApproachPhase::Outside;
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        Qualified=0;
        Rejected=0;
        PitOverspeedFrames=0;
        UnsafeMergeFrames=0;
        SuccessfulPitExits=0;
        HasSource=false;
    }
    PitApproachDecision Step(
        const PhysicalPitApproachFrame& f) noexcept
    {
        PitApproachDecision result=
            EvaluatePhysicalPitApproach(f,Settings);
        const bool sameEpoch=HasSource &&
            Source.Session==f.Identity.Session &&
            Source.SessionRevision==f.Identity.SessionRevision &&
            Source.Actor==f.Identity.Actor;
        if (!result.Valid ||
            (sameEpoch && f.Identity.Frame<=Source.Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        if (sameEpoch && Phase==PitApproachPhase::MergeToTrack &&
            result.Phase==PitApproachPhase::Outside)
            ++SuccessfulPitExits;
        if (!sameEpoch)
        {
            PreviousThrottle=0.0;
            PreviousBrake=0.0;
        }
        result.CommandThrottle=std::min(
            result.CommandThrottle,
            PreviousThrottle+
            Settings.ReleaseThrottleRisePerSecond*
                f.DeltaSeconds);
        result.CommandBrake=std::max(
            result.CommandBrake,
            PreviousBrake-
            Settings.BrakeReleasePerSecond*f.DeltaSeconds);
        result.CommandThrottle=std::min(
            result.CommandThrottle,f.DriverThrottle);
        result.CommandBrake=std::max(
            result.CommandBrake,f.DriverBrake);
        if(result.CommandBrake>0.05)
            result.CommandThrottle=0.0;
        PreviousThrottle=result.CommandThrottle;
        PreviousBrake=result.CommandBrake;
        Source=f.Identity;
        HasSource=true;
        Phase=result.Phase;
        ++Qualified;
        PitOverspeedFrames+=
            result.Hazard==PitApproachHazard::Overspeed?1U:0U;
        UnsafeMergeFrames+=
            result.Hazard==PitApproachHazard::UnsafeMerge?1U:0U;
        Last=result;
        return Last;
    }
    PitApproachDecision Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t OverspeedFrames() const noexcept
    {return PitOverspeedFrames;}
    std::uint64_t UnsafeMergeStops() const noexcept
    {return UnsafeMergeFrames;}
    std::uint64_t CompletedPitExits() const noexcept
    {return SuccessfulPitExits;}
private:
    PitApproachParameters Settings{};
    PitApproachDecision Last{};
    RaceIdentity Source{};
    PitApproachPhase Phase=PitApproachPhase::Outside;
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t PitOverspeedFrames=0;
    std::uint64_t UnsafeMergeFrames=0;
    std::uint64_t SuccessfulPitExits=0;
    bool HasSource=false;
};
} // namespace latebrake::racecraft
