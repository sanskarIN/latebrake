#pragma once
#include "Racecraft/RaceMarshalFlags.h"
#include "Racecraft/PhysicalCarFollowing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class YieldPhase : std::uint8_t
{
    None, BlueFlagObserved, SeekingSpace,
    Decelerating, HoldingLine, Yielding,
    AllowingPass, Rejoining, Complete, Unsafe
};
struct YieldParameters final
{
    double MinimumLappingCarClosingCmS=180.0;
    double MaximumRearDetectionDistanceCm=2600.0;
    double MinimumRearClearanceCm=480.0;
    double MinimumSideRoadMarginCm=200.0;
    double MaximumYieldLateralOffsetCm=280.0;
    double MinimumPhysicalGripG=0.40;
    double BlueFlagYieldTimeSeconds=2.5;
    double MaximumYieldTimeSeconds=9.0;
    double MaximumApproachTimeSeconds=5.0;
    double MaximumAllowedDecelerationCmS2=750.0;
    double MaximumYieldBrakeFraction=0.65;
    double MaximumThrottleCeilingWhileYielding=0.55;
    double MinimumYieldSpeedCmS=450.0;
    double MaximumYieldSpeedCmS=3200.0;
    double MaximumLateralSpeedCmS=450.0;
    double MaximumSafeHeadingErrorRadians=0.18;
    double MinimumFollowingDistanceAfterPassCm=700.0;
    double PostPassRejoinWaitSeconds=0.8;
    double RejoinAccelerationCeiling=0.68;
    double ReturnToNormalSeconds=1.2;
    double MaximumFrameSeconds=0.10;
    bool RequireBlueFlagFromActualRaceDirector=true;
    bool RequireLappingCarPhysicalIdentity=true;
    bool NeverForceLappingCarBraking=true;
    bool DoNotChangeOfficialPosition=true;
};
struct YieldObservation final
{
    FollowingActor YieldingCar{};
    FollowingActor LappingCar{};
    MarshalDirective BlueFlag{};
    double CurrentClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double CurrentRoadMarginCm=0.0;
    double LocalRoadHalfWidthCm=800.0;
    double ActualLateralOffsetCm=0.0;
    int OwnCompletedLaps=0;
    int LappingCarCompletedLaps=0;
    bool OriginalPhysicalRoadVerified=false;
    bool OriginalEngineRosterVerified=false;
    bool ActualDirectorBlueFlag=false;
    bool RaceGreen=false;
    bool HeavyTrafficAhead=false;
    bool OffPavement=false;
};
struct YieldDecision final
{
    YieldPhase Phase=YieldPhase::None;
    double ThrottleCeiling=1.0;
    double BrakeFloor=0.0;
    double SuggestedLateralOffsetCm=0.0;
    double MaximumAllowedSpeedCmS=MaximumPhysicalSpeedCmS;
    double DistanceToLappingCarCm=0.0;
    double ClosingSpeedCmS=0.0;
    double TimeToPassSeconds=0.0;
    double MinimumRoadMarginCm=0.0;
    double LateralMoveDemandCm=0.0;
    double YieldElapsedSeconds=0.0;
    double TimeSinceCarPassedSeconds=0.0;
    bool LappingCarIsBehind=false;
    bool ActuallyUnderBlueFlag=false;
    bool YieldOpportunityFound=false;
    bool PassedByLappingCar=false;
    bool YieldNotRequired=false;
    bool SafetyVeto=false;
    bool ControlsOriginalVehicle=false;
    bool OfficialPositionChanged=false;
    bool Valid=false;
};
inline bool ValidYieldParameters(YieldParameters p) noexcept
{
    return Finite(p.MinimumLappingCarClosingCmS) &&
        p.MinimumLappingCarClosingCmS>0.0 &&
        Finite(p.MaximumRearDetectionDistanceCm) &&
        p.MaximumRearDetectionDistanceCm>0.0 &&
        Finite(p.MinimumRearClearanceCm) &&
        p.MinimumRearClearanceCm>0.0 &&
        Finite(p.MinimumSideRoadMarginCm) &&
        p.MinimumSideRoadMarginCm>=0.0 &&
        Finite(p.MaximumYieldLateralOffsetCm) &&
        p.MaximumYieldLateralOffsetCm>0.0 &&
        Finite(p.MinimumPhysicalGripG) &&
        p.MinimumPhysicalGripG>0.0 &&
        Finite(p.BlueFlagYieldTimeSeconds) &&
        p.BlueFlagYieldTimeSeconds>0.0 &&
        Finite(p.MaximumYieldTimeSeconds) &&
        p.MaximumYieldTimeSeconds>
            p.BlueFlagYieldTimeSeconds &&
        Finite(p.MaximumAllowedDecelerationCmS2) &&
        p.MaximumAllowedDecelerationCmS2>0.0 &&
        Finite(p.MaximumYieldBrakeFraction) &&
        p.MaximumYieldBrakeFraction>0.0 &&
        p.MaximumYieldBrakeFraction<=1.0 &&
        Finite(p.MaximumThrottleCeilingWhileYielding) &&
        p.MaximumThrottleCeilingWhileYielding>0.0 &&
        p.MaximumThrottleCeilingWhileYielding<=1.0 &&
        Finite(p.MinimumYieldSpeedCmS) &&
        p.MinimumYieldSpeedCmS>=0.0 &&
        Finite(p.MaximumYieldSpeedCmS) &&
        p.MaximumYieldSpeedCmS>p.MinimumYieldSpeedCmS &&
        Finite(p.MaximumLateralSpeedCmS) &&
        p.MaximumLateralSpeedCmS>0.0 &&
        Finite(p.MaximumSafeHeadingErrorRadians) &&
        p.MaximumSafeHeadingErrorRadians>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidYieldObservation(
    YieldObservation s,YieldParameters p) noexcept
{
    return ValidFollowingActor(s.YieldingCar,{}) &&
        ValidFollowingActor(s.LappingCar,{}) &&
        s.YieldingCar.Identity.Actor!=
            s.LappingCar.Identity.Actor &&
        s.YieldingCar.Identity.Session==
            s.LappingCar.Identity.Session &&
        s.OriginalPhysicalRoadVerified &&
        s.OriginalEngineRosterVerified &&
        s.RaceGreen &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.CurrentClockSeconds) &&
        Finite(s.CurrentRoadMarginCm) &&
        Finite(s.LocalRoadHalfWidthCm) &&
        Finite(s.ActualLateralOffsetCm) &&
        s.CurrentClockSeconds>=0.0 &&
        s.LocalRoadHalfWidthCm>
            p.MinimumSideRoadMarginCm &&
        s.OwnCompletedLaps>=0 &&
        s.LappingCarCompletedLaps>=0;
}
inline YieldDecision EvaluateBlueFlagYield(
    YieldObservation s,YieldParameters p={}) noexcept
{
    YieldDecision out;
    if (!ValidYieldParameters(p) ||
        !ValidYieldObservation(s,p))
        return out;
    out.Valid=true;
    if (!s.ActualDirectorBlueFlag ||
        s.BlueFlag.DominantFlag!=RaceFlag::Blue ||
        s.LappingCarCompletedLaps<=s.OwnCompletedLaps)
    {
        out.YieldNotRequired=true;
        return out;
    }
    out.ActuallyUnderBlueFlag=true;
    const Vec2 heading=HeadingVector(
        s.YieldingCar.Pose.HeadingRadians);
    const Vec2 relative=s.LappingCar.Pose.Position-
        s.YieldingCar.Pose.Position;
    const double along=Dot(relative,heading);
    out.LappingCarIsBehind=along<0.0;
    out.DistanceToLappingCarCm=std::abs(along);
    out.MinimumRoadMarginCm=s.CurrentRoadMarginCm;
    const double otherSpeed=ForwardSpeed(
        s.LappingCar.VelocityCmS,
        s.LappingCar.Pose.HeadingRadians);
    const double selfSpeed=ForwardSpeed(
        s.YieldingCar.VelocityCmS,
        s.YieldingCar.Pose.HeadingRadians);
    out.ClosingSpeedCmS=otherSpeed-selfSpeed;
    out.TimeToPassSeconds=
        out.ClosingSpeedCmS>PhysicsEpsilon ?
        out.DistanceToLappingCarCm/
            out.ClosingSpeedCmS :
        std::numeric_limits<double>::infinity();
    if (!out.LappingCarIsBehind)
    {
        out.PassedByLappingCar=true;
        out.Phase=YieldPhase::Rejoining;
        return out;
    }
    if (out.DistanceToLappingCarCm>
        p.MaximumRearDetectionDistanceCm ||
        out.ClosingSpeedCmS<
            p.MinimumLappingCarClosingCmS)
    {
        out.Phase=YieldPhase::BlueFlagObserved;
        return out;
    }
    const bool enoughSpace=
        s.CurrentRoadMarginCm>=
            p.MinimumSideRoadMarginCm+
            s.YieldingCar.HalfWidthCm &&
        !s.HeavyTrafficAhead &&
        !s.OffPavement &&
        s.YieldingCar.GripG>=p.MinimumPhysicalGripG;
    out.YieldOpportunityFound=enoughSpace;
    out.Phase=enoughSpace ?
        YieldPhase::Yielding :
        YieldPhase::HoldingLine;
    if (!enoughSpace)
    {
        out.ThrottleCeiling=0.82;
        return out;
    }
    const double direction=
        s.ActualLateralOffsetCm>0.0?1.0:-1.0;
    out.SuggestedLateralOffsetCm=
        std::clamp(
            s.ActualLateralOffsetCm+
            direction*p.MaximumYieldLateralOffsetCm,
            -s.LocalRoadHalfWidthCm+
                p.MinimumSideRoadMarginCm,
            s.LocalRoadHalfWidthCm-
                p.MinimumSideRoadMarginCm);
    out.LateralMoveDemandCm=
        out.SuggestedLateralOffsetCm-
        s.ActualLateralOffsetCm;
    out.ThrottleCeiling=
        p.MaximumThrottleCeilingWhileYielding;
    out.MaximumAllowedSpeedCmS=std::clamp(
        selfSpeed*0.78,p.MinimumYieldSpeedCmS,
        p.MaximumYieldSpeedCmS);
    out.BrakeFloor=std::clamp(
        SafeDivision(selfSpeed-
            out.MaximumAllowedSpeedCmS,
            p.MaximumAllowedDecelerationCmS2),
        0.0,p.MaximumYieldBrakeFraction);
    out.ControlsOriginalVehicle=false;
    out.OfficialPositionChanged=false;
    return out;
}
class BlueFlagYieldMonitor final
{
public:
    bool Configure(YieldParameters params) noexcept
    {
        if (!ValidYieldParameters(params))return false;
        Params=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Session=0;
        Actor=0;
        Lapper=0;
        YieldSeconds=0.0;
        RejoinSeconds=0.0;
        QualifiedFrames=0;
        InvalidFrames=0;
        YieldEpisodes=0;
        CompletedYieldEpisodes=0;
        AbortedYieldEpisodes=0;
        WasYielding=false;
    }
    YieldDecision Observe(YieldObservation input) noexcept
    {
        auto result=EvaluateBlueFlagYield(input,Params);
        if (!result.Valid)
        {
            ++InvalidFrames;
            return {};
        }
        if (Session!=0 &&
            (input.YieldingCar.Identity.Session!=Session ||
             input.YieldingCar.Identity.Actor!=Actor ||
             input.LappingCar.Identity.Actor!=Lapper))
        {
            if (WasYielding)++AbortedYieldEpisodes;
            YieldSeconds=0.0;
            RejoinSeconds=0.0;
            WasYielding=false;
        }
        Session=input.YieldingCar.Identity.Session;
        Actor=input.YieldingCar.Identity.Actor;
        Lapper=input.LappingCar.Identity.Actor;
        const bool yielding=
            result.Phase==YieldPhase::Yielding ||
            result.Phase==YieldPhase::HoldingLine;
        if (yielding)
        {
            if (!WasYielding)++YieldEpisodes;
            WasYielding=true;
            YieldSeconds+=input.DeltaSeconds;
            result.YieldElapsedSeconds=YieldSeconds;
            if (YieldSeconds>Params.MaximumYieldTimeSeconds)
            {
                result.Phase=YieldPhase::Unsafe;
                result.SafetyVeto=true;
                result.ThrottleCeiling=0.0;
                result.BrakeFloor=1.0;
                ++AbortedYieldEpisodes;
                WasYielding=false;
            }
        }
        if (result.PassedByLappingCar && WasYielding)
        {
            RejoinSeconds+=input.DeltaSeconds;
            result.TimeSinceCarPassedSeconds=RejoinSeconds;
            if (RejoinSeconds>=Params.PostPassRejoinWaitSeconds)
            {
                result.Phase=YieldPhase::Complete;
                ++CompletedYieldEpisodes;
                WasYielding=false;
            }
        }
        ++QualifiedFrames;
        Last=result;
        return Last;
    }
    YieldDecision Snapshot() const noexcept{return Last;}
    std::uint64_t CompletedYields() const noexcept
    {return CompletedYieldEpisodes;}
    std::uint64_t AbortedYields() const noexcept
    {return AbortedYieldEpisodes;}
private:
    YieldParameters Params{};
    YieldDecision Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Lapper=0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t InvalidFrames=0;
    std::uint64_t YieldEpisodes=0;
    std::uint64_t CompletedYieldEpisodes=0;
    std::uint64_t AbortedYieldEpisodes=0;
    double YieldSeconds=0.0;
    double RejoinSeconds=0.0;
    bool WasYielding=false;
};
} // namespace latebrake::racecraft
