#pragma once
#include "Racecraft/RivalLaneOccupancyForecast.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class OvertakeState : std::uint8_t
{
    Disarmed, Following, Evaluating, Committed,
    Alongside, Clearing, Recovering, Aborted
};
enum class OvertakeVeto : std::uint8_t
{
    None, MissingPhysics, RaceNotGreen, UnsafeRoad,
    NoClearLane, OpponentTooFast, NoPassingAdvantage,
    InsufficientSpace, YellowFlag, PitEntry,
    MultipleThreats, ExcessYaw, TireGripUncertain,
    RosterChanged, InconsistentFrame, DriverAbort,
    StaleDecision, TimedOut
};
struct OvertakeCommitmentSettings final
{
    double MinimumClosingAdvantageCmS=180.0;
    double MinimumSelfSpeedCmS=800.0;
    double MaximumSelfSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumLeadGapCm=500.0;
    double MaximumLeadGapCm=2800.0;
    double MinimumAdjacentGapCm=250.0;
    double MinimumRearGapCm=600.0;
    double MinimumRoadEdgeCm=220.0;
    double MinimumGripG=0.45;
    double MaximumYawRateRadS=2.2;
    double MaximumHeadingErrorRadians=0.30;
    double MaximumRoadCurvaturePerCm=0.00035;
    double MaximumLateralOffsetCm=480.0;
    double MaximumPassingSteering=0.65;
    double MaximumSteeringRatePerSecond=2.0;
    double MaximumThrottleFraction=0.9;
    double MaximumBrakeFraction=1.0;
    double MaximumTimeToContactSeconds=6.0;
    double MinimumPassingVisibilityCm=2500.0;
    double MinimumRoadWidthCm=750.0;
    double MinimumSafetyWindowSeconds=0.75;
    double MinimumPrepareSeconds=0.30;
    double MinimumCommitSeconds=0.50;
    double MaximumCommitSeconds=5.0;
    double MaximumAlongsideSeconds=4.0;
    double MaximumRecoverySeconds=2.0;
    double MaximumCleanFramesBeforeCommit=3.0;
    double MaximumFrameSeconds=0.10;
    double MaximumBrakeHoldSeconds=0.30;
    double AbortedThrottleCeiling=0.12;
    double RiskScoreThreshold=0.25;
    double OccupancyMarginCm=110.0;
    double MinimumPassTargetAdvanceCm=350.0;
    double LateAbortDistanceCm=300.0;
    double DriverSkillBias=0.5;
    bool RequireOriginalCircuit=true;
    bool RespectOfficialYellow=true;
    bool RequireVerifiedTireGrip=true;
    bool RequirePhysicalSixCarRoster=true;
    bool AbortOnRosterChange=true;
    bool NeverAuthorizeLapCredit=true;
};
struct OvertakeObservation final
{
    RivalLaneTrafficFrame Traffic{};
    RivalLaneForecast Occupancy{};
    double SelfGripG=1.0;
    double SelfYawRateRadiansSecond=0.0;
    double SelfHeadingErrorRadians=0.0;
    double RoadCurvaturePerCm=0.0;
    double PhysicalVisibilityDistanceCm=0.0;
    double CurrentTrackDistanceCm=0.0;
    double DistanceUntilCornerCm=0.0;
    double ActualRoadHalfWidthCm=800.0;
    double DriverSkill=0.5;
    bool OfficialGreenFlag=false;
    bool LocalYellowActive=false;
    bool OfficialPitEntryActive=false;
    bool ActualEnginePhysicsQualified=false;
    bool WheelGripMeasured=false;
    bool OriginalRaceTrackVerified=false;
    bool DriverWantsToPass=true;
};
struct OvertakeDecision final
{
    RaceIdentity Identity{};
    OvertakeState State=OvertakeState::Disarmed;
    OvertakeVeto Veto=OvertakeVeto::MissingPhysics;
    RivalLaneClass Lane=RivalLaneClass::Center;
    int TargetActorSlot=-1;
    std::uint64_t TargetActor=0;
    double PhysicalLeadingGapCm=0.0;
    double ClosingAdvantageCmS=0.0;
    double AvailablePassingWindowSeconds=0.0;
    double RequestedLateralOffsetCm=0.0;
    double OriginalRouteSteering=0.0;
    double ProposedSteering=0.0;
    double ProposedThrottle=0.0;
    double ProposedBrake=1.0;
    double SteeringDelta=0.0;
    double PlannedSafetyMarginCm=0.0;
    double MinimumCollisionTimeSeconds=0.0;
    double RiskScore=0.0;
    double CommitmentElapsedSeconds=0.0;
    double PreparatoryElapsedSeconds=0.0;
    double AlongsideElapsedSeconds=0.0;
    double RecoveryElapsedSeconds=0.0;
    int CleanDecisionFrames=0;
    int AvailableLaneCount=0;
    bool EarlyAbortRequested=false;
    bool TrafficBrakePreserved=true;
    bool DriverCommandsAltered=false;
    bool UnrealSteeringApplied=false;
    bool OfficialRaceStateChanged=false;
    bool Valid=false;
};
inline bool ValidOvertakeSettings(
    OvertakeCommitmentSettings p) noexcept
{
    return Finite(p.MinimumClosingAdvantageCmS) &&
        p.MinimumClosingAdvantageCmS>0.0 &&
        Finite(p.MinimumSelfSpeedCmS) &&
        p.MinimumSelfSpeedCmS>=0.0 &&
        Finite(p.MaximumSelfSpeedCmS) &&
        p.MaximumSelfSpeedCmS>p.MinimumSelfSpeedCmS &&
        p.MaximumSelfSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumLeadGapCm) && p.MinimumLeadGapCm>0.0 &&
        Finite(p.MaximumLeadGapCm) &&
        p.MaximumLeadGapCm>p.MinimumLeadGapCm &&
        Finite(p.MinimumAdjacentGapCm) &&
        p.MinimumAdjacentGapCm>0.0 &&
        Finite(p.MinimumRearGapCm) && p.MinimumRearGapCm>0.0 &&
        Finite(p.MinimumRoadEdgeCm) &&
        p.MinimumRoadEdgeCm>=0.0 &&
        Finite(p.MinimumGripG) && p.MinimumGripG>0.0 &&
        Finite(p.MaximumYawRateRadS) &&
        p.MaximumYawRateRadS>0.0 &&
        Finite(p.MaximumHeadingErrorRadians) &&
        p.MaximumHeadingErrorRadians>0.0 &&
        Finite(p.MaximumRoadCurvaturePerCm) &&
        p.MaximumRoadCurvaturePerCm>0.0 &&
        Finite(p.MaximumLateralOffsetCm) &&
        p.MaximumLateralOffsetCm>0.0 &&
        Finite(p.MaximumPassingSteering) &&
        p.MaximumPassingSteering>0.0 &&
        p.MaximumPassingSteering<=1.0 &&
        Finite(p.MaximumSteeringRatePerSecond) &&
        p.MaximumSteeringRatePerSecond>0.0 &&
        Finite(p.MaximumThrottleFraction) &&
        p.MaximumThrottleFraction>0.0 &&
        p.MaximumThrottleFraction<=1.0 &&
        Finite(p.MaximumBrakeFraction) &&
        p.MaximumBrakeFraction>0.0 &&
        p.MaximumBrakeFraction<=1.0 &&
        Finite(p.MaximumTimeToContactSeconds) &&
        p.MaximumTimeToContactSeconds>0.0 &&
        Finite(p.MinimumPassingVisibilityCm) &&
        p.MinimumPassingVisibilityCm>0.0 &&
        Finite(p.MinimumRoadWidthCm) &&
        p.MinimumRoadWidthCm>0.0 &&
        Finite(p.MinimumSafetyWindowSeconds) &&
        p.MinimumSafetyWindowSeconds>0.0 &&
        Finite(p.MinimumPrepareSeconds) &&
        p.MinimumPrepareSeconds>0.0 &&
        Finite(p.MinimumCommitSeconds) &&
        p.MinimumCommitSeconds>0.0 &&
        Finite(p.MaximumCommitSeconds) &&
        p.MaximumCommitSeconds>p.MinimumCommitSeconds &&
        Finite(p.MaximumAlongsideSeconds) &&
        p.MaximumAlongsideSeconds>0.0 &&
        Finite(p.MaximumRecoverySeconds) &&
        p.MaximumRecoverySeconds>0.0 &&
        Finite(p.MaximumCleanFramesBeforeCommit) &&
        p.MaximumCleanFramesBeforeCommit>=1.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumBrakeHoldSeconds) &&
        p.MaximumBrakeHoldSeconds>=0.0 &&
        Finite(p.AbortedThrottleCeiling) &&
        p.AbortedThrottleCeiling>=0.0 &&
        p.AbortedThrottleCeiling<=1.0 &&
        Finite(p.RiskScoreThreshold) &&
        p.RiskScoreThreshold>=0.0 &&
        Finite(p.OccupancyMarginCm) &&
        p.OccupancyMarginCm>=0.0 &&
        Finite(p.MinimumPassTargetAdvanceCm) &&
        p.MinimumPassTargetAdvanceCm>0.0 &&
        Finite(p.LateAbortDistanceCm) &&
        p.LateAbortDistanceCm>0.0 &&
        Finite(p.DriverSkillBias) &&
        p.DriverSkillBias>=0.0 &&
        p.DriverSkillBias<=1.0;
}
inline bool ValidOvertakeObservation(
    const OvertakeObservation& s,
    OvertakeCommitmentSettings p) noexcept
{
    return ValidOvertakeSettings(p) &&
        ValidRivalLaneFrame(s.Traffic,{}) &&
        s.Occupancy.Valid &&
        s.Occupancy.VerifiedOtherCars==RivalTrafficCount &&
        s.Occupancy.Identity.Session==
            s.Traffic.Self.Identity.Session &&
        s.Occupancy.Identity.SessionRevision==
            s.Traffic.Self.Identity.SessionRevision &&
        s.Occupancy.Identity.Actor==
            s.Traffic.Self.Identity.Actor &&
        s.Occupancy.Identity.Frame==
            s.Traffic.Self.Identity.Frame &&
        s.ActualEnginePhysicsQualified &&
        (!p.RequireOriginalCircuit ||
         s.OriginalRaceTrackVerified) &&
        (!p.RequireVerifiedTireGrip ||
         s.WheelGripMeasured) &&
        Finite(s.SelfGripG) &&
        s.SelfGripG>=0.15 &&
        s.SelfGripG<=2.5 &&
        Finite(s.SelfYawRateRadiansSecond) &&
        std::abs(s.SelfYawRateRadiansSecond)<=8.0 &&
        Finite(s.SelfHeadingErrorRadians) &&
        std::abs(s.SelfHeadingErrorRadians)<=Pi &&
        Finite(s.RoadCurvaturePerCm) &&
        std::abs(s.RoadCurvaturePerCm)<0.01 &&
        Finite(s.PhysicalVisibilityDistanceCm) &&
        s.PhysicalVisibilityDistanceCm>=0.0 &&
        Finite(s.CurrentTrackDistanceCm) &&
        s.CurrentTrackDistanceCm>=0.0 &&
        Finite(s.DistanceUntilCornerCm) &&
        s.DistanceUntilCornerCm>=0.0 &&
        Finite(s.ActualRoadHalfWidthCm) &&
        s.ActualRoadHalfWidthCm>=p.MinimumRoadWidthCm &&
        Finite(s.DriverSkill) &&
        s.DriverSkill>=0.0 &&
        s.DriverSkill<=1.0;
}
inline OvertakeVeto EvaluateOvertakeVeto(
    const OvertakeObservation& s,
    OvertakeCommitmentSettings p) noexcept
{
    if (!ValidOvertakeObservation(s,p))
        return OvertakeVeto::MissingPhysics;
    if (!s.OfficialGreenFlag || !s.Traffic.RaceGreen)
        return OvertakeVeto::RaceNotGreen;
    if (p.RespectOfficialYellow && s.LocalYellowActive)
        return OvertakeVeto::YellowFlag;
    if (s.OfficialPitEntryActive)
        return OvertakeVeto::PitEntry;
    if (!s.DriverWantsToPass)
        return OvertakeVeto::DriverAbort;
    if (s.SelfGripG<p.MinimumGripG)
        return OvertakeVeto::TireGripUncertain;
    if (std::abs(s.SelfYawRateRadiansSecond)>
            p.MaximumYawRateRadS ||
        std::abs(s.SelfHeadingErrorRadians)>
            p.MaximumHeadingErrorRadians)
        return OvertakeVeto::ExcessYaw;
    if (std::abs(s.RoadCurvaturePerCm)>
            p.MaximumRoadCurvaturePerCm ||
        s.DistanceUntilCornerCm<
            p.MinimumPassingVisibilityCm ||
        s.PhysicalVisibilityDistanceCm<
            p.MinimumPassingVisibilityCm)
        return OvertakeVeto::UnsafeRoad;
    if (s.Occupancy.SafelyAvailableLanes<1)
        return OvertakeVeto::NoClearLane;
    if (s.Occupancy.TotalPotentialContacts>0)
        return OvertakeVeto::MultipleThreats;
    return OvertakeVeto::None;
}
inline int FindOvertakeTarget(
    const OvertakeObservation& s,
    OvertakeCommitmentSettings p) noexcept
{
    if (!ValidOvertakeObservation(s,p))
        return -1;
    const double selfSpeed=ForwardSpeed(
        s.Traffic.Self.VelocityCmS,
        s.Traffic.Self.Pose.HeadingRadians);
    if (selfSpeed<p.MinimumSelfSpeedCmS ||
        selfSpeed>p.MaximumSelfSpeedCmS)
        return -1;
    int found=-1;
    double closest=p.MaximumLeadGapCm;
    for(int i=0;i<s.Traffic.OtherCount;++i)
    {
        const auto& rival=s.Traffic.Others[
            static_cast<std::size_t>(i)];
        const auto& contact=s.Occupancy.Contacts[
            static_cast<std::size_t>(i)];
        const double rivalSpeed=ForwardSpeed(
            rival.VelocityCmS,
            s.Traffic.Self.Pose.HeadingRadians);
        const double gap=contact.ForwardDistanceCm-
            s.Traffic.Self.HalfLengthCm-rival.HalfLengthCm;
        const bool sameLane=std::abs(
            contact.LateralDistanceCm)<
            s.Traffic.Self.HalfWidthCm+
            rival.HalfWidthCm+p.OccupancyMarginCm;
        if(contact.Ahead && sameLane &&
           gap>=p.MinimumLeadGapCm &&
           gap<closest &&
           selfSpeed-rivalSpeed>=
               p.MinimumClosingAdvantageCmS)
        {
            found=i;
            closest=gap;
        }
    }
    return found;
}
inline OvertakeDecision ProposeOvertake(
    const OvertakeObservation& s,
    OvertakeCommitmentSettings p={}) noexcept
{
    OvertakeDecision out;
    if (!ValidOvertakeObservation(s,p))
        return out;
    out.Identity=s.Traffic.Self.Identity;
    out.OriginalRouteSteering=s.Traffic.CurrentSteering;
    out.ProposedSteering=s.Traffic.CurrentSteering;
    out.ProposedThrottle=s.Traffic.CurrentThrottle;
    out.ProposedBrake=s.Traffic.CurrentBrake;
    out.Veto=EvaluateOvertakeVeto(s,p);
    out.AvailableLaneCount=s.Occupancy.SafelyAvailableLanes;
    out.RiskScore=Clamp01(
        SafeDivision(s.Occupancy.TotalLateralRiskScore,
            static_cast<double>(RivalTrafficCount)));
    out.MinimumCollisionTimeSeconds=
        s.Occupancy.MinimumTrafficTimeToContactSeconds;
    if (out.Veto!=OvertakeVeto::None)
    {
        out.State=OvertakeState::Following;
        out.Valid=true;
        return out;
    }
    const int target=FindOvertakeTarget(s,p);
    if (target<0)
    {
        out.Veto=OvertakeVeto::NoPassingAdvantage;
        out.State=OvertakeState::Following;
        out.Valid=true;
        return out;
    }
    const auto& rival=s.Traffic.Others[
        static_cast<std::size_t>(target)];
    const auto& contact=s.Occupancy.Contacts[
        static_cast<std::size_t>(target)];
    const double speed=ForwardSpeed(
        s.Traffic.Self.VelocityCmS,
        s.Traffic.Self.Pose.HeadingRadians);
    out.TargetActor=rival.Identity.Actor;
    out.TargetActorSlot=rival.Identity.Slot;
    out.PhysicalLeadingGapCm=
        contact.ForwardDistanceCm-
        s.Traffic.Self.HalfLengthCm-rival.HalfLengthCm;
    out.ClosingAdvantageCmS=std::max(
        0.0,-contact.RelativeForwardSpeedCmS);
    out.AvailablePassingWindowSeconds=
        SafeDivision(out.PhysicalLeadingGapCm,
                     out.ClosingAdvantageCmS,
                     p.MaximumTimeToContactSeconds);
    if (out.AvailablePassingWindowSeconds<
        p.MinimumSafetyWindowSeconds)
    {
        out.Veto=OvertakeVeto::InsufficientSpace;
        out.Valid=true;
        return out;
    }
    int best=-1;
    double bestClearance=-std::numeric_limits<double>::infinity();
    for(int i=0;i<RivalLaneCount;++i)
    {
        if (i==1)continue;
        const auto& lane=s.Occupancy.Lanes[
            static_cast<std::size_t>(i)];
        if (!lane.Valid ||
            !lane.RoadEdgeVerified ||
            !lane.SweptMergeClear ||
            lane.Hazard!=RivalLaneHazard::Clear)
            continue;
        const double clearance=std::min(
            lane.AvailableLeftClearanceCm,
            lane.AvailableRightClearanceCm);
        if(clearance>bestClearance)
        {
            best=i;
            bestClearance=clearance;
        }
    }
    if (best<0)
    {
        out.Veto=OvertakeVeto::NoClearLane;
        out.Valid=true;
        return out;
    }
    const auto& lane=s.Occupancy.Lanes[
        static_cast<std::size_t>(best)];
    out.Lane=lane.Lane;
    out.PlannedSafetyMarginCm=bestClearance;
    out.RequestedLateralOffsetCm=lane.LaneOffsetCm;
    out.State=OvertakeState::Evaluating;
    out.ProposedThrottle=std::min(
        out.ProposedThrottle,
        p.MaximumThrottleFraction);
    out.ProposedBrake=std::max(
        out.ProposedBrake,
        lane.RecommendedBrakeFloor);
    if(out.ProposedBrake>0.05)
        out.ProposedThrottle=0.0;
    out.TrafficBrakePreserved=true;
    out.UnrealSteeringApplied=false;
    out.OfficialRaceStateChanged=false;
    out.Valid=Finite(out.RequestedLateralOffsetCm) &&
        Finite(out.ProposedThrottle) &&
        Finite(out.ProposedBrake);
    return out;
}
class RivalOvertakeSupervisor final
{
public:
    bool Configure(OvertakeCommitmentSettings settings) noexcept
    {
        if (!ValidOvertakeSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        State=OvertakeState::Disarmed;
        Source={};
        TargetActor=0;
        TargetSlot=-1;
        CommittedLane=RivalLaneClass::Center;
        PreparationSeconds=0.0;
        CommitmentSeconds=0.0;
        AlongsideSeconds=0.0;
        RecoverySeconds=0.0;
        PreviousSteering=0.0;
        CleanFrames=0;
        Qualified=0;
        Rejected=0;
        Attempts=0;
        Aborts=0;
        Completed=0;
    }
    OvertakeDecision Step(
        const OvertakeObservation& sample) noexcept
    {
        OvertakeDecision decision=ProposeOvertake(
            sample,Settings);
        const bool sameEpoch=Source.Session!=0 &&
            Source.Session==sample.Traffic.Self.Identity.Session &&
            Source.SessionRevision==
                sample.Traffic.Self.Identity.SessionRevision &&
            Source.Actor==sample.Traffic.Self.Identity.Actor;
        if (!decision.Valid ||
            (sameEpoch &&
             sample.Traffic.Self.Identity.Frame<=Source.Frame))
        {
            ++Rejected;
            State=OvertakeState::Aborted;
            Last={};
            return {};
        }
        if (!sameEpoch)
        {
            State=OvertakeState::Following;
            TargetActor=0;
            CleanFrames=0;
            PreparationSeconds=0.0;
            CommitmentSeconds=0.0;
            AlongsideSeconds=0.0;
            RecoverySeconds=0.0;
            PreviousSteering=sample.Traffic.CurrentSteering;
        }
        const double dt=sample.Traffic.DeltaSeconds;
        const bool viable=decision.Veto==OvertakeVeto::None &&
            decision.TargetActor!=0 &&
            decision.PlannedSafetyMarginCm>=
                Settings.MinimumRoadEdgeCm &&
            decision.RiskScore<=Settings.RiskScoreThreshold;
        if (!viable)
        {
            if(State==OvertakeState::Committed ||
               State==OvertakeState::Alongside)
            {
                ++Aborts;
                State=OvertakeState::Aborted;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
            }
            else if(State!=OvertakeState::Aborted)
                State=OvertakeState::Following;
            CleanFrames=0;
            PreparationSeconds=0.0;
        }
        else if(State==OvertakeState::Following ||
                State==OvertakeState::Disarmed)
        {
            State=OvertakeState::Evaluating;
            TargetActor=decision.TargetActor;
            TargetSlot=decision.TargetActorSlot;
            CommittedLane=decision.Lane;
            PreparationSeconds=0.0;
            CleanFrames=1;
        }
        else if (State==OvertakeState::Evaluating)
        {
            if(TargetActor!=decision.TargetActor ||
               CommittedLane!=decision.Lane)
            {
                TargetActor=decision.TargetActor;
                CommittedLane=decision.Lane;
                PreparationSeconds=0.0;
                CleanFrames=0;
            }
            ++CleanFrames;
            PreparationSeconds+=dt;
            if(CleanFrames>=static_cast<int>(
                Settings.MaximumCleanFramesBeforeCommit) &&
               PreparationSeconds>=Settings.MinimumPrepareSeconds)
            {
                State=OvertakeState::Committed;
                ++Attempts;
                CommitmentSeconds=0.0;
            }
        }
        else if(State==OvertakeState::Committed)
        {
            CommitmentSeconds+=dt;
            if(CommitmentSeconds>Settings.MaximumCommitSeconds)
            {
                State=OvertakeState::Aborted;
                ++Aborts;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
            }
            else if(decision.PhysicalLeadingGapCm<
                Settings.LateAbortDistanceCm)
            {
                State=OvertakeState::Alongside;
                AlongsideSeconds=0.0;
            }
        }
        else if(State==OvertakeState::Alongside)
        {
            AlongsideSeconds+=dt;
            if(AlongsideSeconds>Settings.MaximumAlongsideSeconds)
            {
                State=OvertakeState::Aborted;
                ++Aborts;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
            }
            else if(decision.TargetActorSlot!=TargetSlot)
            {
                State=OvertakeState::Clearing;
                ++Completed;
            }
        }
        else if(State==OvertakeState::Aborted)
        {
            RecoverySeconds=std::max(
                0.0,RecoverySeconds-dt);
            if(RecoverySeconds<=0.0)
                State=OvertakeState::Following;
        }
        else if(State==OvertakeState::Clearing)
        {
            RecoverySeconds=std::max(
                0.0,RecoverySeconds-dt);
            if(RecoverySeconds<=0.0)
                State=OvertakeState::Following;
        }
        const bool laneAuthorized=
            State==OvertakeState::Committed ||
            State==OvertakeState::Alongside;
        if(laneAuthorized)
        {
            const double steeringTarget=std::clamp(
                SafeDivision(decision.RequestedLateralOffsetCm,
                    Settings.MaximumLateralOffsetCm)*
                Settings.MaximumPassingSteering,
                -Settings.MaximumPassingSteering,
                Settings.MaximumPassingSteering);
            decision.ProposedSteering=RateLimited(
                PreviousSteering,steeringTarget,
                Settings.MaximumSteeringRatePerSecond,
                Settings.MaximumSteeringRatePerSecond,dt);
            decision.DriverCommandsAltered=
                decision.ProposedSteering!=
                sample.Traffic.CurrentSteering;
        }
        else
        {
            decision.ProposedSteering=
                sample.Traffic.CurrentSteering;
            decision.DriverCommandsAltered=false;
        }
        if(State==OvertakeState::Aborted)
        {
            decision.ProposedThrottle=std::min(
                decision.ProposedThrottle,
                Settings.AbortedThrottleCeiling);
            decision.ProposedBrake=std::max(
                decision.ProposedBrake,
                Settings.MaximumBrakeFraction*0.20);
        }
        decision.ProposedThrottle=std::min(
            decision.ProposedThrottle,
            sample.Traffic.CurrentThrottle);
        decision.ProposedBrake=std::max(
            decision.ProposedBrake,
            sample.Traffic.CurrentBrake);
        if(decision.ProposedBrake>0.05)
            decision.ProposedThrottle=0.0;
        decision.State=State;
        decision.CleanDecisionFrames=CleanFrames;
        decision.PreparatoryElapsedSeconds=PreparationSeconds;
        decision.CommitmentElapsedSeconds=CommitmentSeconds;
        decision.AlongsideElapsedSeconds=AlongsideSeconds;
        decision.RecoveryElapsedSeconds=RecoverySeconds;
        decision.TrafficBrakePreserved=true;
        decision.UnrealSteeringApplied=false;
        decision.OfficialRaceStateChanged=false;
        PreviousSteering=decision.ProposedSteering;
        Source=sample.Traffic.Self.Identity;
        ++Qualified;
        Last=decision;
        return Last;
    }
    OvertakeDecision Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t Commitments() const noexcept{return Attempts;}
    std::uint64_t AbortedPasses() const noexcept{return Aborts;}
    std::uint64_t ClearedPasses() const noexcept{return Completed;}
    OvertakeState CurrentState() const noexcept{return State;}
private:
    OvertakeCommitmentSettings Settings{};
    OvertakeDecision Last{};
    OvertakeState State=OvertakeState::Disarmed;
    RaceIdentity Source{};
    RivalLaneClass CommittedLane=RivalLaneClass::Center;
    std::uint64_t TargetActor=0;
    int TargetSlot=-1;
    double PreparationSeconds=0.0;
    double CommitmentSeconds=0.0;
    double AlongsideSeconds=0.0;
    double RecoverySeconds=0.0;
    double PreviousSteering=0.0;
    int CleanFrames=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t Attempts=0;
    std::uint64_t Aborts=0;
    std::uint64_t Completed=0;
};
} // namespace latebrake::racecraft
