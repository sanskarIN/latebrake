#pragma once
#include "Racecraft/RivalLaneOccupancyForecast.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

// Keep this independently authored overtake-commitment experiment separate
// from AIOvertakePlanner's distinct OvertakeObservation type. Both headers
// must be composable in one TU; there is no implicit conversion/authority.
namespace latebrake::racecraft::commitment
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
    // Additional reserve AFTER the lane planner's 140-cm edge margin.
    // The default 800-cm road accepts a 400-cm passing lane only when
    // its combined 290-cm-plus clearance is physically measured.
    double MinimumRoadEdgeCm=150.0;
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
        s.DriverSkill<=1.0 &&
        [&]()
        {
            for(int i=0;i<s.Traffic.OtherCount;++i)
            {
                const auto& actor=s.Traffic.Others[
                    static_cast<std::size_t>(i)];
                const auto& contact=s.Occupancy.Contacts[
                    static_cast<std::size_t>(i)];
                // The topology of a six-car sensor frame is positional,
                // not an unordered set: swapping two contact records
                // must never redirect the overtake target silently.
                if(contact.Actor!=actor.Identity.Actor ||
                   contact.Slot!=actor.Identity.Slot ||
                   !contact.PhysicallyQualified)
                    return false;
            }
            return true;
        }();
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
// A physical passing commitment must retain the SAME source actor and
// target strip across frames. The 'new pass' selector intentionally has
// a minimum leading gap, so it cannot be reused after closing in on a
// rival or it will abort every otherwise successful pass.
struct TrackedOvertakeEvidence final
{
    OvertakeVeto Veto=OvertakeVeto::MissingPhysics;
    std::uint64_t ActualTargetActor=0;
    int ActualTargetSlot=-1;
    double ForwardDistanceCm=0.0;
    double LeadingGapCm=0.0;
    double ClosingSpeedCmS=0.0;
    double RemainingLateralOffsetCm=0.0;
    double RoadEdgeReserveCm=0.0;
    int ConflictingObservedActors=0;
    bool InCommittedLane=false;
    bool SideBySide=false;
    bool PassedWithFullClearance=false;
    bool PhysicalTargetObserved=false;
    bool Valid=false;
};
inline TrackedOvertakeEvidence InspectTrackedOvertake(
    const OvertakeObservation& sample,
    std::uint64_t expectedTargetActor,
    int expectedTargetSlot,
    double committedTrackOffsetCm,
    OvertakeCommitmentSettings settings={}) noexcept
{
    TrackedOvertakeEvidence out;
    if (!ValidOvertakeObservation(sample,settings) ||
        expectedTargetActor==0 ||
        expectedTargetSlot<0 ||
        expectedTargetSlot>=PhysicalFleetCapacity ||
        expectedTargetSlot==sample.Traffic.Self.Identity.Slot ||
        !Finite(committedTrackOffsetCm))
        return out;
    out.Veto=EvaluateOvertakeVeto(sample,settings);
    const auto& self=sample.Traffic.Self;
    const double leftRightReserve=
        self.RoadHalfWidthCm-
        std::abs(committedTrackOffsetCm)-
        self.HalfWidthCm;
    out.RoadEdgeReserveCm=leftRightReserve;
    out.RemainingLateralOffsetCm=
        committedTrackOffsetCm-self.TrackLateralOffsetCm;
    out.InCommittedLane=
        std::abs(out.RemainingLateralOffsetCm)<=
            settings.MinimumAdjacentGapCm;
    if(leftRightReserve<settings.MinimumRoadEdgeCm ||
       std::abs(committedTrackOffsetCm)>
           settings.MaximumLateralOffsetCm+
           std::abs(self.TrackLateralOffsetCm))
        out.Veto=OvertakeVeto::UnsafeRoad;
    const double desiredRelativeCm=
        committedTrackOffsetCm-self.TrackLateralOffsetCm;
    // This checks the absolute committed strip, not a NEW left/right
    // offset relative to a chassis already moving sideways.
    for(int i=0;i<sample.Traffic.OtherCount;++i)
    {
        const auto& rival=sample.Traffic.Others[
            static_cast<std::size_t>(i)];
        const auto& contact=sample.Occupancy.Contacts[
            static_cast<std::size_t>(i)];
        if(contact.Actor!=rival.Identity.Actor ||
           contact.Slot!=rival.Identity.Slot ||
           !contact.PhysicallyQualified)
        {
            out.Veto=OvertakeVeto::InconsistentFrame;
            return out;
        }
        if(rival.Identity.Actor==expectedTargetActor)
        {
            if(rival.Identity.Slot!=expectedTargetSlot)
            {
                out.Veto=OvertakeVeto::RosterChanged;
                return out;
            }
            out.PhysicalTargetObserved=true;
            out.ActualTargetActor=rival.Identity.Actor;
            out.ActualTargetSlot=rival.Identity.Slot;
            out.ForwardDistanceCm=contact.ForwardDistanceCm;
            out.LeadingGapCm=contact.ForwardDistanceCm-
                self.HalfLengthCm-rival.HalfLengthCm;
            out.ClosingSpeedCmS=std::max(
                0.0,-contact.RelativeForwardSpeedCmS);
            out.SideBySide=std::abs(contact.ForwardDistanceCm)<=
                self.HalfLengthCm+rival.HalfLengthCm+
                    settings.LateAbortDistanceCm;
            out.PassedWithFullClearance=
                contact.ForwardDistanceCm<=
                    -(self.HalfLengthCm+rival.HalfLengthCm+
                      settings.MinimumPassTargetAdvanceCm);
        }
        const double lateralNow=std::abs(
            contact.LateralDistanceCm-desiredRelativeCm);
        const double lateralFuture=std::abs(
            contact.LateralDistanceCm+
            contact.RelativeLateralSpeedCmS*
                settings.MinimumSafetyWindowSeconds-
            desiredRelativeCm);
        const double occupiedWidth=self.HalfWidthCm+
            rival.HalfWidthCm+settings.OccupancyMarginCm;
        const double longitudinalSafetyCm=
            self.HalfLengthCm+rival.HalfLengthCm+
            settings.MinimumAdjacentGapCm+
            std::abs(contact.RelativeForwardSpeedCmS)*
                settings.MinimumSafetyWindowSeconds;
        // A real opponent overlapping the committed strip at present
        // or within the safety horizon vetoes the pass, even if another
        // projected lane is nominally empty.
        if(std::min(lateralNow,lateralFuture)<
                occupiedWidth &&
           std::abs(contact.ForwardDistanceCm)<
                longitudinalSafetyCm)
            ++out.ConflictingObservedActors;
    }
    if(!out.PhysicalTargetObserved)
        out.Veto=OvertakeVeto::RosterChanged;
    else if(out.ConflictingObservedActors>0)
        out.Veto=OvertakeVeto::MultipleThreats;
    else if(out.Veto==OvertakeVeto::NoClearLane &&
            out.RoadEdgeReserveCm>=settings.MinimumRoadEdgeCm &&
            sample.Occupancy.TotalPotentialContacts==0)
    {
        // During a verified lane change the discrete "left/center/right
        // relative to the CURRENT chassis" choices can all be poor.
        // The locked ABSOLUTE track strip is still demonstrably clear
        // using every live rival's predicted contact envelope. Do not
        // discard that physical evidence and accidentally abort mid-pass.
        out.Veto=OvertakeVeto::None;
    }
    out.Valid=out.PhysicalTargetObserved &&
        Finite(out.ForwardDistanceCm) &&
        Finite(out.LeadingGapCm) &&
        Finite(out.ClosingSpeedCmS) &&
        Finite(out.RemainingLateralOffsetCm) &&
        Finite(out.RoadEdgeReserveCm);
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
        Roster.fill(0);
        TargetActor=0;
        TargetSlot=-1;
        CommittedLane=RivalLaneClass::Center;
        CommittedTrackOffsetCm=0.0;
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
        HasRoster=false;
    }
    OvertakeDecision Step(
        const OvertakeObservation& sample) noexcept
    {
        OvertakeDecision decision=ProposeOvertake(
            sample,Settings);
        const auto& own=sample.Traffic.Self;
        const bool sameEpoch=HasRoster &&
            Source.Session==own.Identity.Session &&
            Source.SessionRevision==
                own.Identity.SessionRevision &&
            Source.Actor==own.Identity.Actor;
        if (!decision.Valid ||
            (sameEpoch &&
             own.Identity.Frame<=Source.Frame))
        {
            ++Rejected;
            // Never continue a committed manoeuvre from invalid/replayed
            // physics. No guessed steering command may be returned.
            State=OvertakeState::Aborted;
            Last={};
            CleanFrames=0;
            TargetActor=0;
            return {};
        }
        std::array<std::uint64_t,PhysicalFleetCapacity> roster{};
        roster[static_cast<std::size_t>(
            own.Identity.Slot)]=own.Identity.Actor;
        for(const auto& rival:sample.Traffic.Others)
            roster[static_cast<std::size_t>(
                rival.Identity.Slot)]=rival.Identity.Actor;
        const bool rosterChanged=sameEpoch &&
            Settings.AbortOnRosterChange &&
            roster!=Roster;
        if (!sameEpoch)
        {
            State=OvertakeState::Following;
            TargetActor=0;
            TargetSlot=-1;
            CleanFrames=0;
            PreparationSeconds=0.0;
            CommitmentSeconds=0.0;
            AlongsideSeconds=0.0;
            RecoverySeconds=0.0;
            PreviousSteering=sample.Traffic.CurrentSteering;
        }
        const double dt=sample.Traffic.DeltaSeconds;
        const bool activePass=State==OvertakeState::Committed ||
            State==OvertakeState::Alongside;
        TrackedOvertakeEvidence tracked;
        if(activePass)
        {
            tracked=InspectTrackedOvertake(
                sample,TargetActor,TargetSlot,
                CommittedTrackOffsetCm,Settings);
            // New-pass eligibility may disappear as the cars converge.
            // A committed manoeuvre is reviewed against the already
            // authenticated target, not replaced with another rival.
            decision.Veto=tracked.Veto;
            decision.TargetActor=TargetActor;
            decision.TargetActorSlot=TargetSlot;
            decision.Lane=CommittedLane;
            decision.RequestedLateralOffsetCm=
                tracked.RemainingLateralOffsetCm;
            decision.PlannedSafetyMarginCm=
                tracked.RoadEdgeReserveCm;
            decision.PhysicalLeadingGapCm=
                tracked.LeadingGapCm;
            decision.ClosingAdvantageCmS=
                tracked.ClosingSpeedCmS;
        }
        if(rosterChanged)
            decision.Veto=OvertakeVeto::RosterChanged;
        const bool viable=decision.Veto==OvertakeVeto::None &&
            decision.TargetActor!=0 &&
            decision.PlannedSafetyMarginCm>=
                Settings.MinimumRoadEdgeCm &&
            decision.RiskScore<=Settings.RiskScoreThreshold;
        // Recovery is independent of whether a NEW passing target is
        // available. Otherwise the acquisition-only lead-gap test can
        // strand a completed or aborted pass in its terminal state forever.
        if(State==OvertakeState::Aborted ||
           State==OvertakeState::Clearing)
        {
            RecoverySeconds=std::max(
                0.0,RecoverySeconds-dt);
            if(RecoverySeconds<=0.0)
            {
                State=OvertakeState::Following;
                TargetActor=0;
                TargetSlot=-1;
                CommittedTrackOffsetCm=0.0;
            }
        }
        else if (!viable)
        {
            if(activePass)
            {
                ++Aborts;
                State=OvertakeState::Aborted;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
                decision.EarlyAbortRequested=true;
            }
            else if(State!=OvertakeState::Aborted &&
                    State!=OvertakeState::Clearing)
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
            CommittedTrackOffsetCm=
                own.TrackLateralOffsetCm+
                decision.RequestedLateralOffsetCm;
            PreparationSeconds=0.0;
            CleanFrames=1;
        }
        else if(State==OvertakeState::Evaluating)
        {
            if(TargetActor!=decision.TargetActor ||
               CommittedLane!=decision.Lane)
            {
                TargetActor=decision.TargetActor;
                TargetSlot=decision.TargetActorSlot;
                CommittedLane=decision.Lane;
                CommittedTrackOffsetCm=
                    own.TrackLateralOffsetCm+
                    decision.RequestedLateralOffsetCm;
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
                decision.Veto=OvertakeVeto::TimedOut;
                ++Aborts;
                decision.EarlyAbortRequested=true;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
            }
            else if(tracked.InCommittedLane &&
                    tracked.SideBySide)
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
                decision.Veto=OvertakeVeto::TimedOut;
                ++Aborts;
                decision.EarlyAbortRequested=true;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
            }
            else if(tracked.InCommittedLane &&
                    tracked.PassedWithFullClearance)
            {
                // The same actual rival must be physically behind by
                // both chassis lengths PLUS a measured rearward gap.
                State=OvertakeState::Clearing;
                RecoverySeconds=Settings.MaximumRecoverySeconds;
                ++Completed;
            }
        }
        const bool laneAuthorized=
            State==OvertakeState::Committed ||
            State==OvertakeState::Alongside;
        if(laneAuthorized)
        {
            const double steeringTarget=std::clamp(
                SafeDivision(
                    CommittedTrackOffsetCm-
                        own.TrackLateralOffsetCm,
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
        // An overtake review can never relax the real six-car collision
        // planner or the original driver command.
        decision.ProposedThrottle=std::min({
            decision.ProposedThrottle,
            sample.Traffic.CurrentThrottle,
            sample.Occupancy.ProposedThrottle});
        decision.ProposedBrake=std::max({
            decision.ProposedBrake,
            sample.Traffic.CurrentBrake,
            sample.Occupancy.ProposedBrake});
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
        Source=own.Identity;
        Roster=roster;
        HasRoster=true;
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
    std::uint64_t TrackedTargetActor() const noexcept
    {return TargetActor;}
    int TrackedTargetSlot() const noexcept{return TargetSlot;}
    double CommittedOffsetCm() const noexcept
    {return CommittedTrackOffsetCm;}
private:
    OvertakeCommitmentSettings Settings{};
    OvertakeDecision Last{};
    OvertakeState State=OvertakeState::Disarmed;
    RaceIdentity Source{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    RivalLaneClass CommittedLane=RivalLaneClass::Center;
    std::uint64_t TargetActor=0;
    int TargetSlot=-1;
    double CommittedTrackOffsetCm=0.0;
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
    bool HasRoster=false;
};
} // namespace latebrake::racecraft::commitment
