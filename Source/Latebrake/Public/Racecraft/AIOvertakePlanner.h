#pragma once
#include "Racecraft/PhysicalCarFollowing.h"
#include "Racecraft/SmoothRacingLine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class OvertakeSide : std::uint8_t
{
    None, Left, Right
};
enum class OvertakePhase : std::uint8_t
{
    Inactive, Assessment, Prepare, MoveOut,
    Overlap, Clear, ReturnToLine, Aborted
};
enum class OvertakeFailure : std::uint8_t
{
    None, MissingPhysics, YellowFlag, InsufficientSpeed,
    NoClearance, UnsafeGap, MissingOriginalRoad,
    DifferentSession, RearConflict, CountersteerRisk,
    OffTrack, AlreadyOverlapping, NoLegalPassingSide
};
struct OvertakeParameters final
{
    double MinimumClosingSpeedCmS=140.0;
    double MinimumOvertakeSpeedCmS=900.0;
    double MinimumTargetSpeedCmS=400.0;
    double MaximumTimeToPassSeconds=6.0;
    double MinimumPreparationSeconds=0.25;
    double MinimumGapBeforeMoveCm=450.0;
    double MaximumGapBeforeMoveCm=1900.0;
    double MinimumSideClearanceCm=90.0;
    double MaximumLateralOffsetCm=390.0;
    double LateralTransitionSeconds=0.75;
    double LateralReturnSeconds=1.0;
    double MinimumRearGapCm=800.0;
    double TargetFrontClearanceCm=550.0;
    double MaximumLateralDriftCmS=650.0;
    double MaximumSteeringHeadingErrorRadians=0.35;
    double MinimumPhysicalGripG=0.40;
    double MaximumLateralGripDemandG=0.75;
    double WetWeatherLateralReduction=0.35;
    double MinimumRoadMarginCm=170.0;
    double RoadBodyHalfWidthCm=100.0;
    double MaximumLongitudinalAccelerationCmS2=850.0;
    double AbortContactTimeSeconds=1.5;
    double RosterChangeHoldSeconds=0.6;
    int MaximumSuccessiveAttemptsPerLap=8;
    bool AllowPassLeft=true;
    bool AllowPassRight=true;
    bool RequireRaceGreen=true;
    bool RequirePhysicalRearCar=true;
    bool ProhibitPassingUnderCaution=true;
};
struct OvertakeActor final
{
    FollowingActor Body{};
    double UnwrappedTrackCm=0.0;
    double SpeedCmS=0.0;
    double LongitudinalAccelerationCmS2=0.0;
    double LateralSpeedCmS=0.0;
    double YawRateRadS=0.0;
    double TrackRoadHalfWidthCm=800.0;
    double CurrentRoadMarginCm=0.0;
    double TireGripG=1.0;
    bool VerifiedRealChaosActor=false;
    bool OriginalCircuitPosition=false;
};
struct OvertakeObservation final
{
    OvertakeActor Self{};
    OvertakeActor Target{};
    OvertakeActor Rear{};
    bool RearExists=false;
    bool RaceGreen=false;
    bool YellowFlag=false;
    bool RedFlag=false;
    bool LeaderCompletedRace=false;
    bool SameTimingEpoch=false;
    bool AuthoredPavementVerified=false;
    double DeltaSeconds=0.0;
    double RaceClockSeconds=0.0;
    int CurrentLap=0;
};
struct OvertakePlan final
{
    OvertakePhase Phase=OvertakePhase::Inactive;
    OvertakeFailure Failure=OvertakeFailure::MissingPhysics;
    OvertakeSide Side=OvertakeSide::None;
    double TargetLateralOffsetCm=0.0;
    double InterpolationFraction=0.0;
    double TimeToReachLaneSeconds=0.0;
    double PredictedPassSeconds=0.0;
    double AvailableLeftMarginCm=0.0;
    double AvailableRightMarginCm=0.0;
    double LongitudinalGapCm=0.0;
    double ClosingSpeedCmS=0.0;
    double RequiredLateralShiftCm=0.0;
    double RequiredYawRadians=0.0;
    double LateralAccelerationDemandG=0.0;
    double RecommendedThrottleCeiling=0.0;
    double RecommendedBrakeFloor=0.0;
    double SafetyTimeToContactSeconds=0.0;
    double RearClearanceCm=0.0;
    double RequiredFrontClearanceCm=0.0;
    bool MeasuredPhysicalRoster=true;
    bool LateralPlanInsideRoad=false;
    bool LegallyAllowed=false;
    bool ThreatBehind=false;
    bool CommandsChaosSteering=false;
    bool Valid=false;
};
inline bool ValidOvertakeParameters(OvertakeParameters p) noexcept
{
    return Finite(p.MinimumClosingSpeedCmS)&&
        p.MinimumClosingSpeedCmS>0.0 &&
        Finite(p.MinimumOvertakeSpeedCmS)&&
        p.MinimumOvertakeSpeedCmS>0.0 &&
        Finite(p.MaximumTimeToPassSeconds)&&
        p.MaximumTimeToPassSeconds>0.0 &&
        Finite(p.MinimumPreparationSeconds)&&
        p.MinimumPreparationSeconds>=0.0 &&
        Finite(p.MinimumGapBeforeMoveCm)&&
        p.MinimumGapBeforeMoveCm>0.0 &&
        Finite(p.MaximumGapBeforeMoveCm)&&
        p.MaximumGapBeforeMoveCm>p.MinimumGapBeforeMoveCm &&
        Finite(p.MinimumSideClearanceCm)&&
        p.MinimumSideClearanceCm>=0.0 &&
        Finite(p.MaximumLateralOffsetCm)&&
        p.MaximumLateralOffsetCm>p.MinimumSideClearanceCm &&
        Finite(p.LateralTransitionSeconds)&&
        p.LateralTransitionSeconds>0.0 &&
        Finite(p.LateralReturnSeconds)&&
        p.LateralReturnSeconds>0.0 &&
        Finite(p.MinimumRearGapCm)&&
        p.MinimumRearGapCm>0.0 &&
        Finite(p.TargetFrontClearanceCm)&&
        p.TargetFrontClearanceCm>0.0 &&
        Finite(p.MaximumLateralDriftCmS)&&
        p.MaximumLateralDriftCmS>0.0 &&
        Finite(p.MaximumSteeringHeadingErrorRadians)&&
        p.MaximumSteeringHeadingErrorRadians>0.0 &&
        Finite(p.MinimumPhysicalGripG)&&
        p.MinimumPhysicalGripG>0.0 &&
        Finite(p.MaximumLateralGripDemandG)&&
        p.MaximumLateralGripDemandG>0.0 &&
        Finite(p.MinimumRoadMarginCm)&&
        p.MinimumRoadMarginCm>=0.0 &&
        Finite(p.RoadBodyHalfWidthCm)&&
        p.RoadBodyHalfWidthCm>0.0 &&
        p.MaximumSuccessiveAttemptsPerLap>=1 &&
        p.MaximumSuccessiveAttemptsPerLap<=100;
}
inline bool ValidOvertakeActor(OvertakeActor a,
                               OvertakeParameters p) noexcept
{
    return a.VerifiedRealChaosActor &&
        a.OriginalCircuitPosition &&
        ValidFollowingActor(a.Body,{}) &&
        Finite(a.UnwrappedTrackCm) &&
        Finite(a.SpeedCmS) &&
        Finite(a.LongitudinalAccelerationCmS2) &&
        Finite(a.LateralSpeedCmS) &&
        Finite(a.YawRateRadS) &&
        Finite(a.TrackRoadHalfWidthCm) &&
        Finite(a.CurrentRoadMarginCm) &&
        Finite(a.TireGripG) &&
        a.SpeedCmS>=0.0 &&
        a.SpeedCmS<=MaximumPhysicalSpeedCmS &&
        a.TireGripG>=p.MinimumPhysicalGripG &&
        a.TireGripG<=4.0 &&
        a.TrackRoadHalfWidthCm>=
            a.Body.HalfWidthCm+p.MinimumRoadMarginCm;
}
inline bool ValidOvertakeObservation(
    const OvertakeObservation& input,
    OvertakeParameters p) noexcept
{
    if (!ValidFrameDelta(input.DeltaSeconds) ||
        !Finite(input.RaceClockSeconds) ||
        input.RaceClockSeconds<0.0 ||
        !input.SameTimingEpoch ||
        !input.AuthoredPavementVerified ||
        !ValidOvertakeActor(input.Self,p) ||
        !ValidOvertakeActor(input.Target,p) ||
        input.Self.Body.Identity.Actor==
            input.Target.Body.Identity.Actor ||
        input.Self.Body.Identity.Session!=
            input.Target.Body.Identity.Session ||
        input.CurrentLap<0) return false;
    if (input.RearExists)
    {
        if (!ValidOvertakeActor(input.Rear,p) ||
            input.Rear.Body.Identity.Actor==
                input.Self.Body.Identity.Actor ||
            input.Rear.Body.Identity.Actor==
                input.Target.Body.Identity.Actor ||
            input.Rear.Body.Identity.Session!=
                input.Self.Body.Identity.Session)
            return false;
    }
    return true;
}
inline double RequiredPassingOffsetCm(
    OvertakeActor self,OvertakeActor other,
    OvertakeParameters p) noexcept
{
    return self.Body.HalfWidthCm +
        other.Body.HalfWidthCm +
        p.MinimumSideClearanceCm;
}
inline OvertakePlan EvaluateOvertakeOpportunity(
    const OvertakeObservation& input,
    OvertakeParameters params={}) noexcept
{
    OvertakePlan out;
    if (!ValidOvertakeParameters(params) ||
        !ValidOvertakeObservation(input,params))
        return out;
    out.Failure=OvertakeFailure::None;
    out.Valid=true;
    if (!input.RaceGreen ||
        input.YellowFlag ||
        input.RedFlag ||
        input.LeaderCompletedRace)
    {
        out.Failure=OvertakeFailure::YellowFlag;
        return out;
    }
    const Vec2 heading=HeadingVector(
        input.Self.Body.Pose.HeadingRadians);
    const Vec2 delta=input.Target.Body.Pose.Position-
        input.Self.Body.Pose.Position;
    const double along=Dot(delta,heading);
    const double lateral=Cross(heading,delta);
    const double speedDifference=
        input.Self.SpeedCmS-input.Target.SpeedCmS;
    const double availableLeft=
        input.Self.TrackRoadHalfWidthCm-
        input.Self.Body.HalfWidthCm-
        params.MinimumRoadMarginCm-
        input.Self.Body.LateralDeviationCm;
    const double availableRight=
        input.Self.TrackRoadHalfWidthCm-
        input.Self.Body.HalfWidthCm-
        params.MinimumRoadMarginCm+
        input.Self.Body.LateralDeviationCm;
    const double requiredShift=
        RequiredPassingOffsetCm(
            input.Self,input.Target,params)+
        std::abs(lateral);
    out.LongitudinalGapCm=along-
        input.Self.Body.HalfLengthCm-
        input.Target.Body.HalfLengthCm;
    out.ClosingSpeedCmS=speedDifference;
    out.AvailableLeftMarginCm=availableLeft;
    out.AvailableRightMarginCm=availableRight;
    out.RequiredLateralShiftCm=requiredShift;
    out.RequiredFrontClearanceCm=
        params.TargetFrontClearanceCm;
    if (input.Self.SpeedCmS<
            params.MinimumOvertakeSpeedCmS ||
        speedDifference<
            params.MinimumClosingSpeedCmS)
    {
        out.Failure=OvertakeFailure::InsufficientSpeed;
        return out;
    }
    if (out.LongitudinalGapCm<
            params.MinimumGapBeforeMoveCm ||
        out.LongitudinalGapCm>
            params.MaximumGapBeforeMoveCm)
    {
        out.Failure=OvertakeFailure::UnsafeGap;
        return out;
    }
    if (input.RearExists)
    {
        const Vec2 behind=
            input.Self.Body.Pose.Position-
            input.Rear.Body.Pose.Position;
        const double behindGap=Dot(behind,heading)-
            input.Self.Body.HalfLengthCm-
            input.Rear.Body.HalfLengthCm;
        out.RearClearanceCm=behindGap;
        if (behindGap<params.MinimumRearGapCm)
        {
            out.Failure=OvertakeFailure::RearConflict;
            out.ThreatBehind=true;
            return out;
        }
    }
    const double leftShift=
        input.Target.Body.LateralDeviationCm+
        RequiredPassingOffsetCm(
            input.Self,input.Target,params);
    const double rightShift=
        input.Target.Body.LateralDeviationCm-
        RequiredPassingOffsetCm(
            input.Self,input.Target,params);
    const bool leftLegal=params.AllowPassLeft &&
        leftShift<=availableLeft &&
        leftShift<=params.MaximumLateralOffsetCm;
    const bool rightLegal=params.AllowPassRight &&
        -rightShift<=availableRight &&
        -rightShift<=params.MaximumLateralOffsetCm;
    if (!leftLegal && !rightLegal)
    {
        out.Failure=OvertakeFailure::NoLegalPassingSide;
        return out;
    }
    const double leftCost=std::abs(leftShift-
        input.Self.Body.LateralDeviationCm);
    const double rightCost=std::abs(rightShift-
        input.Self.Body.LateralDeviationCm);
    const bool chooseLeft=leftLegal &&
        (!rightLegal ||leftCost<=rightCost);
    out.Side=chooseLeft?OvertakeSide::Left:
        OvertakeSide::Right;
    out.TargetLateralOffsetCm=chooseLeft ?
        leftShift:rightShift;
    out.RequiredLateralShiftCm=std::abs(
        out.TargetLateralOffsetCm-
        input.Self.Body.LateralDeviationCm);
    out.TimeToReachLaneSeconds=std::max(
        params.LateralTransitionSeconds,
        out.RequiredLateralShiftCm/
            params.MaximumLateralDriftCmS);
    const double passDistance=
        out.LongitudinalGapCm+
        input.Target.Body.HalfLengthCm*2.0+
        params.TargetFrontClearanceCm;
    out.PredictedPassSeconds=SafeDivision(
        passDistance,speedDifference,
        std::numeric_limits<double>::infinity());
    out.RequiredYawRadians=std::atan2(
        out.RequiredLateralShiftCm,
        input.Self.SpeedCmS*
            out.TimeToReachLaneSeconds);
    out.LateralAccelerationDemandG=SafeDivision(
        2.0*out.RequiredLateralShiftCm,
        Square(out.TimeToReachLaneSeconds)*
            GravityCmS2);
    out.LateralPlanInsideRoad=true;
    out.SafetyTimeToContactSeconds=SafeDivision(
        out.LongitudinalGapCm,speedDifference,
        std::numeric_limits<double>::infinity());
    if (out.PredictedPassSeconds>
        params.MaximumTimeToPassSeconds ||
        out.LateralAccelerationDemandG>
            params.MaximumLateralGripDemandG*
            input.Self.TireGripG ||
        out.RequiredYawRadians>
            params.MaximumSteeringHeadingErrorRadians)
    {
        out.Failure=OvertakeFailure::CountersteerRisk;
        return out;
    }
    out.Phase=OvertakePhase::Prepare;
    out.LegallyAllowed=true;
    out.RecommendedThrottleCeiling=1.0;
    out.RecommendedBrakeFloor=0.0;
    out.CommandsChaosSteering=false;
    return out;
}
inline double OvertakeLateralProfile(
    double fromCm,double targetCm,
    double timeSeconds,double durationSeconds) noexcept
{
    if (!Finite(fromCm)||!Finite(targetCm)||
        !Finite(timeSeconds)||!Finite(durationSeconds)||
        durationSeconds<=0.0)return fromCm;
    return Lerp(fromCm,targetCm,
        EaseInOutQuint(timeSeconds/durationSeconds));
}
inline bool OvertakeClearOfTarget(
    OvertakeActor self,OvertakeActor target,
    OvertakeParameters settings) noexcept
{
    if (!ValidOvertakeActor(self,settings) ||
        !ValidOvertakeActor(target,settings))return false;
    const Vec2 direction=HeadingVector(
        target.Body.Pose.HeadingRadians);
    return Dot(self.Body.Pose.Position-
        target.Body.Pose.Position,direction)>
        self.Body.HalfLengthCm+
        target.Body.HalfLengthCm+
        settings.TargetFrontClearanceCm;
}
class OvertakeDecisionLedger final
{
public:
    bool Configure(OvertakeParameters params) noexcept
    {
        if (!ValidOvertakeParameters(params))return false;
        Settings=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Phase=OvertakePhase::Inactive;
        Session=0;
        Actor=0;
        Target=0;
        Lap=-1;
        AttemptsThisLap=0;
        Accepted=0;
        Rejected=0;
        Successful=0;
        Aborted=0;
        PhaseSeconds=0.0;
        PreviousOffset=0.0;
    }
    OvertakePlan Observe(const OvertakeObservation& input) noexcept
    {
        OvertakePlan opportunity=
            EvaluateOvertakeOpportunity(input,Settings);
        if (!opportunity.Valid)
        {
            ++Rejected;
            Phase=OvertakePhase::Aborted;
            return {};
        }
        if (Session!=0 &&
            (Session!=input.Self.Body.Identity.Session ||
             Actor!=input.Self.Body.Identity.Actor))
        {
            Phase=OvertakePhase::Aborted;
            ++Aborted;
        }
        Session=input.Self.Body.Identity.Session;
        Actor=input.Self.Body.Identity.Actor;
        Target=input.Target.Body.Identity.Actor;
        if (Lap!=input.CurrentLap)
        {
            Lap=input.CurrentLap;
            AttemptsThisLap=0;
        }
        if (Phase==OvertakePhase::Inactive &&
            opportunity.LegallyAllowed &&
            AttemptsThisLap<Settings.MaximumSuccessiveAttemptsPerLap)
        {
            Phase=OvertakePhase::Prepare;
            ++AttemptsThisLap;
            PhaseSeconds=0.0;
        }
        PhaseSeconds+=input.DeltaSeconds;
        if (Phase==OvertakePhase::Prepare &&
            PhaseSeconds>=Settings.MinimumPreparationSeconds)
        {
            Phase=OvertakePhase::MoveOut;
            PhaseSeconds=0.0;
        }
        if (Phase==OvertakePhase::MoveOut &&
            PhaseSeconds>=Settings.LateralTransitionSeconds)
        {
            Phase=OvertakePhase::Overlap;
            PhaseSeconds=0.0;
        }
        if (Phase==OvertakePhase::Overlap &&
            OvertakeClearOfTarget(input.Self,input.Target,Settings))
        {
            Phase=OvertakePhase::Clear;
            PhaseSeconds=0.0;
        }
        if (Phase==OvertakePhase::Clear)
        {
            Phase=OvertakePhase::ReturnToLine;
            PhaseSeconds=0.0;
        }
        if (Phase==OvertakePhase::ReturnToLine &&
            PhaseSeconds>=Settings.LateralReturnSeconds)
        {
            Phase=OvertakePhase::Inactive;
            ++Successful;
            PhaseSeconds=0.0;
        }
        if (!opportunity.LegallyAllowed &&
            Phase!=OvertakePhase::Inactive)
        {
            Phase=OvertakePhase::Aborted;
            ++Aborted;
            PhaseSeconds=0.0;
        }
        opportunity.Phase=Phase;
        if (Phase==OvertakePhase::MoveOut)
        {
            opportunity.InterpolationFraction=Clamp01(
                PhaseSeconds/Settings.LateralTransitionSeconds);
            opportunity.TargetLateralOffsetCm=
                OvertakeLateralProfile(
                    PreviousOffset,
                    opportunity.TargetLateralOffsetCm,
                    PhaseSeconds,
                    Settings.LateralTransitionSeconds);
        }
        else if (Phase==OvertakePhase::ReturnToLine)
        {
            opportunity.TargetLateralOffsetCm=
                OvertakeLateralProfile(
                    PreviousOffset,0.0,PhaseSeconds,
                    Settings.LateralReturnSeconds);
        }
        PreviousOffset=opportunity.TargetLateralOffsetCm;
        ++Accepted;
        Last=opportunity;
        return Last;
    }
    OvertakePlan Snapshot() const noexcept {return Last;}
    std::uint64_t QualifiedFrames() const noexcept{return Accepted;}
    std::uint64_t RejectedFrames() const noexcept{return Rejected;}
    std::uint64_t CompletedPasses() const noexcept{return Successful;}
    std::uint64_t AbortedPasses() const noexcept{return Aborted;}
private:
    OvertakeParameters Settings{};
    OvertakePlan Last{};
    OvertakePhase Phase=OvertakePhase::Inactive;
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Target=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t Successful=0;
    std::uint64_t Aborted=0;
    int Lap=-1;
    int AttemptsThisLap=0;
    double PhaseSeconds=0.0;
    double PreviousOffset=0.0;
};
} // namespace latebrake::racecraft
