#pragma once
#include "Racecraft/AIOvertakePlanner.h"
#include "Racecraft/RaceMarshalFlags.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class DefensivePhase : std::uint8_t
{
    Normal, ThreatDetected, OneMoveCommitted,
    HoldingLine, AllowingOverlap, Returning,
    Completed, Unsafe
};
struct DefensiveParameters final
{
    double MaximumOneMoveOffsetCm=220.0;
    double MinimumRoadMarginCm=200.0;
    double MinimumOpponentRearDistanceCm=320.0;
    double MaximumOpponentRearDistanceCm=2200.0;
    double MinimumClosingSpeedCmS=180.0;
    double MinimumDefendingSpeedCmS=850.0;
    double MinimumPhysicalGripG=0.45;
    double MaximumDefensiveLateralSpeedCmS=380.0;
    double MaximumDefensiveYawErrorRadians=0.25;
    double MaximumLegalMoveSeconds=0.9;
    double MinimumLineHoldSeconds=1.0;
    double MaximumDefensiveTimeSeconds=6.0;
    double ReturnToLineSeconds=1.1;
    double MaximumThrottleCeiling=0.90;
    double MaximumBrakeFraction=0.75;
    double OverlapSafetyMarginCm=160.0;
    double MinimumSpaceForOpponentCm=260.0;
    double MaximumChassisHalfWidthCm=145.0;
    double MaximumFrameSeconds=0.1;
    int MaximumDefensiveMovesPerStraight=1;
    bool RequireActualOpposingVehicle=true;
    bool RequireOriginalRaceDirectorGreen=true;
    bool ProhibitReactiveWeaving=true;
    bool NeverForceOpposingCarOffPavement=true;
    bool AllowDriverToRejoinRacingLine=true;
};
struct DefensiveObservation final
{
    OvertakeActor Defender{};
    OvertakeActor Challenger{};
    MarshalDirective Marshal{};
    double RoadHalfWidthCm=800.0;
    double DefendingCarLateralCm=0.0;
    double ChallengerLateralCm=0.0;
    double ClockSeconds=0.0;
    double DeltaSeconds=0.0;
    int CurrentOriginalSegment=-1;
    bool RaceGreen=true;
    bool ValidPhysicalTrack=true;
    bool ActualOpponentOverlapping=false;
    bool DefenderAlreadyMoved=false;
    bool NoPriorBlockingMove=true;
};
struct DefensiveDecision final
{
    DefensivePhase Phase=DefensivePhase::Normal;
    double RecommendedLateralOffsetCm=0.0;
    double MaximumThrottleCeiling=1.0;
    double MinimumBrakeFloor=0.0;
    double OpponentClosingSpeedCmS=0.0;
    double OpponentRearDistanceCm=0.0;
    double RemainingRoadForOpponentCm=0.0;
    double TimeUntilOverlapSeconds=0.0;
    double LegalSingleMoveCm=0.0;
    double LateralMoveDurationSeconds=0.0;
    double RemainingDefenseSeconds=0.0;
    bool DefendingAllowed=false;
    bool MustHoldChosenLine=false;
    bool MustYieldForOverlap=false;
    bool IllegalSecondMove=false;
    bool OriginalRoadClearanceVerified=false;
    bool ActualSteeringCommandIssued=false;
    bool Valid=false;
};
inline bool ValidDefensiveParameters(
    DefensiveParameters p) noexcept
{
    return Finite(p.MaximumOneMoveOffsetCm)&&
        p.MaximumOneMoveOffsetCm>0.0 &&
        Finite(p.MinimumRoadMarginCm)&&
        p.MinimumRoadMarginCm>=0.0 &&
        Finite(p.MinimumOpponentRearDistanceCm)&&
        p.MinimumOpponentRearDistanceCm>0.0 &&
        Finite(p.MaximumOpponentRearDistanceCm)&&
        p.MaximumOpponentRearDistanceCm>
            p.MinimumOpponentRearDistanceCm &&
        Finite(p.MinimumClosingSpeedCmS)&&
        p.MinimumClosingSpeedCmS>0.0 &&
        Finite(p.MinimumDefendingSpeedCmS)&&
        p.MinimumDefendingSpeedCmS>0.0 &&
        Finite(p.MinimumPhysicalGripG)&&
        p.MinimumPhysicalGripG>0.0 &&
        Finite(p.MaximumDefensiveLateralSpeedCmS)&&
        p.MaximumDefensiveLateralSpeedCmS>0.0 &&
        Finite(p.MaximumLegalMoveSeconds)&&
        p.MaximumLegalMoveSeconds>0.0 &&
        Finite(p.MinimumLineHoldSeconds)&&
        p.MinimumLineHoldSeconds>0.0 &&
        Finite(p.MaximumDefensiveTimeSeconds)&&
        p.MaximumDefensiveTimeSeconds>
            p.MinimumLineHoldSeconds &&
        Finite(p.ReturnToLineSeconds)&&
        p.ReturnToLineSeconds>0.0 &&
        Finite(p.MaximumThrottleCeiling)&&
        p.MaximumThrottleCeiling>0.0 &&
        p.MaximumThrottleCeiling<=1.0 &&
        Finite(p.MaximumFrameSeconds)&&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds &&
        p.MaximumDefensiveMovesPerStraight>=1 &&
        p.MaximumDefensiveMovesPerStraight<=2;
}
inline bool ValidDefensiveObservation(
    DefensiveObservation s,DefensiveParameters p) noexcept
{
    return ValidOvertakeActor(s.Defender,{}) &&
        ValidOvertakeActor(s.Challenger,{}) &&
        s.Defender.Body.Identity.Actor!=
            s.Challenger.Body.Identity.Actor &&
        s.Defender.Body.Identity.Session==
            s.Challenger.Body.Identity.Session &&
        s.ValidPhysicalTrack &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.RoadHalfWidthCm)&&
        Finite(s.DefendingCarLateralCm)&&
        Finite(s.ChallengerLateralCm)&&
        Finite(s.ClockSeconds)&&
        s.RoadHalfWidthCm>=
            p.MaximumOneMoveOffsetCm+
            p.MinimumRoadMarginCm &&
        s.ClockSeconds>=0.0 &&
        s.CurrentOriginalSegment>=0 &&
        s.CurrentOriginalSegment<OriginalSegmentCount;
}
inline DefensiveDecision ReviewDefensiveDriving(
    DefensiveObservation input,
    DefensiveParameters params={}) noexcept
{
    DefensiveDecision out;
    if (!ValidDefensiveParameters(params) ||
        !ValidDefensiveObservation(input,params))
        return out;
    out.Valid=true;
    out.RecommendedLateralOffsetCm=
        input.DefendingCarLateralCm;
    if (!input.RaceGreen ||
        input.Marshal.NoOvertake ||
        input.Defender.SpeedCmS<
            params.MinimumDefendingSpeedCmS)
        return out;
    const Vec2 direction=HeadingVector(
        input.Defender.Body.Pose.HeadingRadians);
    const Vec2 delta=
        input.Challenger.Body.Pose.Position-
        input.Defender.Body.Pose.Position;
    const double behind=-Dot(delta,direction);
    const double closing=
        input.Challenger.SpeedCmS-
        input.Defender.SpeedCmS;
    out.OpponentRearDistanceCm=behind;
    out.OpponentClosingSpeedCmS=closing;
    out.TimeUntilOverlapSeconds=
        closing>PhysicsEpsilon ?
        behind/closing:
        std::numeric_limits<double>::infinity();
    if (behind<params.MinimumOpponentRearDistanceCm ||
        behind>params.MaximumOpponentRearDistanceCm ||
        closing<params.MinimumClosingSpeedCmS ||
        input.ActualOpponentOverlapping)
    {
        if (input.ActualOpponentOverlapping)
        {
            out.MustYieldForOverlap=true;
            out.Phase=DefensivePhase::AllowingOverlap;
        }
        return out;
    }
    const double vehicleWidth=
        input.Defender.Body.HalfWidthCm+
        input.Challenger.Body.HalfWidthCm+
        params.MinimumSpaceForOpponentCm;
    const double leftFree=input.RoadHalfWidthCm-
        input.DefendingCarLateralCm-
        vehicleWidth-
        params.MinimumRoadMarginCm;
    const double rightFree=input.RoadHalfWidthCm+
        input.DefendingCarLateralCm-
        vehicleWidth-
        params.MinimumRoadMarginCm;
    out.RemainingRoadForOpponentCm=
        std::max(leftFree,rightFree);
    if (out.RemainingRoadForOpponentCm<=0.0)
    {
        out.Phase=DefensivePhase::Unsafe;
        return out;
    }
    if (input.DefenderAlreadyMoved ||
        !input.NoPriorBlockingMove)
    {
        out.Phase=DefensivePhase::HoldingLine;
        out.IllegalSecondMove=true;
        out.MustHoldChosenLine=true;
        return out;
    }
    const double side=input.ChallengerLateralCm>
        input.DefendingCarLateralCm?-1.0:1.0;
    const double allowed=std::min(
        params.MaximumOneMoveOffsetCm,
        std::max(0.0,
            input.RoadHalfWidthCm-
            params.MinimumRoadMarginCm-
            input.Defender.Body.HalfWidthCm-
            std::abs(input.DefendingCarLateralCm)));
    out.LegalSingleMoveCm=allowed;
    out.RecommendedLateralOffsetCm=std::clamp(
        input.DefendingCarLateralCm+side*allowed,
        -input.RoadHalfWidthCm+
            params.MinimumRoadMarginCm+
            input.Defender.Body.HalfWidthCm,
        input.RoadHalfWidthCm-
            params.MinimumRoadMarginCm-
            input.Defender.Body.HalfWidthCm);
    out.LateralMoveDurationSeconds=std::max(
        params.MaximumLegalMoveSeconds,
        SafeDivision(allowed,
            params.MaximumDefensiveLateralSpeedCmS));
    out.MaximumThrottleCeiling=
        params.MaximumThrottleCeiling;
    out.MinimumBrakeFloor=0.0;
    out.DefendingAllowed=true;
    out.OriginalRoadClearanceVerified=true;
    out.Phase=DefensivePhase::OneMoveCommitted;
    out.ActualSteeringCommandIssued=false;
    return out;
}
class DefensiveLineLedger final
{
public:
    bool Configure(DefensiveParameters params) noexcept
    {
        if (!ValidDefensiveParameters(params))return false;
        Params=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        CurrentSegment=-1;
        Phase=DefensivePhase::Normal;
        Session=0;
        Actor=0;
        Target=0;
        MovesThisStraight=0;
        Frames=0;
        Rejected=0;
        IllegalSecondMoveVetoes=0;
        TimeCommittedSeconds=0.0;
    }
    DefensiveDecision Observe(DefensiveObservation source) noexcept
    {
        auto review=ReviewDefensiveDriving(source,Params);
        if (!review.Valid)
        {
            ++Rejected;
            return {};
        }
        if (Session!=0 &&
            (Session!=source.Defender.Body.Identity.Session ||
             Actor!=source.Defender.Body.Identity.Actor))
        {
            Reset();
        }
        Session=source.Defender.Body.Identity.Session;
        Actor=source.Defender.Body.Identity.Actor;
        Target=source.Challenger.Body.Identity.Actor;
        if (CurrentSegment!=source.CurrentOriginalSegment)
        {
            CurrentSegment=source.CurrentOriginalSegment;
            MovesThisStraight=0;
            Phase=DefensivePhase::Normal;
            TimeCommittedSeconds=0.0;
        }
        if (review.DefendingAllowed &&
            MovesThisStraight>=
                Params.MaximumDefensiveMovesPerStraight)
        {
            review.DefendingAllowed=false;
            review.IllegalSecondMove=true;
            review.MustHoldChosenLine=true;
            review.RecommendedLateralOffsetCm=
                Last.RecommendedLateralOffsetCm;
            review.Phase=DefensivePhase::HoldingLine;
            ++IllegalSecondMoveVetoes;
        }
        if (review.DefendingAllowed &&
            Phase==DefensivePhase::Normal)
        {
            Phase=DefensivePhase::OneMoveCommitted;
            ++MovesThisStraight;
            TimeCommittedSeconds=0.0;
        }
        if (Phase==DefensivePhase::OneMoveCommitted)
        {
            TimeCommittedSeconds+=source.DeltaSeconds;
            if (TimeCommittedSeconds>=
                Params.MaximumLegalMoveSeconds)
                Phase=DefensivePhase::HoldingLine;
        }
        if (Phase==DefensivePhase::HoldingLine &&
            source.ActualOpponentOverlapping)
            Phase=DefensivePhase::AllowingOverlap;
        if (Phase==DefensivePhase::AllowingOverlap &&
            !source.ActualOpponentOverlapping)
            Phase=DefensivePhase::Returning;
        if (Phase==DefensivePhase::Returning)
        {
            TimeCommittedSeconds+=source.DeltaSeconds;
            if (TimeCommittedSeconds>=
                Params.MaximumDefensiveTimeSeconds)
                Phase=DefensivePhase::Completed;
        }
        review.Phase=Phase;
        review.MustHoldChosenLine=
            Phase==DefensivePhase::HoldingLine;
        ++Frames;
        Last=review;
        return Last;
    }
    DefensiveDecision Snapshot() const noexcept{return Last;}
    int MovesOnCurrentStraight() const noexcept
    {return MovesThisStraight;}
    std::uint64_t RejectedSecondMoves() const noexcept
    {return IllegalSecondMoveVetoes;}
private:
    DefensiveParameters Params{};
    DefensiveDecision Last{};
    DefensivePhase Phase=DefensivePhase::Normal;
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Target=0;
    std::uint64_t Frames=0;
    std::uint64_t Rejected=0;
    std::uint64_t IllegalSecondMoveVetoes=0;
    int CurrentSegment=-1;
    int MovesThisStraight=0;
    double TimeCommittedSeconds=0.0;
};
} // namespace latebrake::racecraft
