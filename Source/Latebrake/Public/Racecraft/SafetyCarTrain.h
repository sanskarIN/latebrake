#pragma once
#include "Racecraft/RaceMarshalFlags.h"
#include "Racecraft/PhysicalCarFollowing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct SafetyCarParameters final
{
    double TargetFollowingGapCm=900.0;
    double MinimumFollowingGapCm=550.0;
    double MaximumFollowingGapCm=2400.0;
    double MaximumSafetyCarSpeedCmS=1250.0;
    double MaximumCautionAccelerationCmS2=300.0;
    double MaximumCautionDecelerationCmS2=550.0;
    double EmergencyCautionDecelerationCmS2=1050.0;
    double MaximumCautionThrottle=0.55;
    double MaximumCautionBrake=0.75;
    double MaximumHitchSeconds=0.10;
    double RestitutionGapMultiplier=1.15;
    double MinimumTireGripG=0.15;
    double MaximumTireGripG=2.5;
    double MinimumPhysicalProgressCm=0.0;
    double MaximumReverseProgressCm=40.0;
    double CautionOnsetHoldSeconds=0.4;
    double RestartHoldSeconds=0.5;
    double SpeedLimitTransitionSeconds=0.7;
    double MaximumFollowingDeltaSeconds=0.10;
    bool RequireActualSafetyCar=false;
    bool RequireMarshalCautionFlag=true;
    bool RequireExactSixCars=true;
    bool PreservePositionOnRestart=true;
    bool ProhibitOvertaking=true;
    bool NeverIncreaseExistingThrottle=true;
};
struct SafetyCarActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    double UnwrappedProgressCm=0.0;
    double GripG=1.0;
    double HalfLengthCm=210.0;
    double HalfWidthCm=95.0;
    int PreviousOfficialPosition=0;
    bool RealChaosBody=true;
    bool OriginalAsphaltContact=true;
};
struct SafetyCarObservation final
{
    std::array<SafetyCarActor,PhysicalFleetCapacity> Cars{};
    MarshalDirective Flags{};
    double RaceClockSeconds=0.0;
    double DeltaSeconds=0.0;
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    bool ActualMarshalCaution=true;
    bool OriginalGameModeRacing=true;
    bool PhysicalRosterVerified=true;
    bool TrackConfirmed=true;
};
struct SafetyCarCommand final
{
    int Slot=-1;
    double ObservedSpeedCmS=0.0;
    double LeaderGapCm=0.0;
    double RecommendedSpeedCmS=0.0;
    double MaximumThrottleCeiling=0.0;
    double MinimumBrakeFloor=0.0;
    double GapErrorCm=0.0;
    double ClosingSpeedCmS=0.0;
    double MinimumSafeGapCm=0.0;
    double RequiredDecelerationCmS2=0.0;
    bool PhysicallyQualified=false;
    bool StopForUnsafeGap=false;
    bool CannotOvertake=true;
    bool SendsActualCarControls=false;
};
struct SafetyCarReport final
{
    std::array<SafetyCarCommand,PhysicalFleetCapacity> BySlot{};
    double MeanSpacingCm=0.0;
    double MinimumActualGapCm=0.0;
    double MaximumActualGapCm=0.0;
    double MaximumClosingSpeedCmS=0.0;
    double PackLengthCm=0.0;
    double TargetTrainSpeedCmS=0.0;
    double RequiredCautionSeconds=0.0;
    int ViolatedMinimumGapCars=0;
    int UnsafePassingObservations=0;
    int QualifiedCars=0;
    bool AllCarsSlowed=false;
    bool RaceClockModified=false;
    bool OfficialRankModified=false;
    bool Valid=false;
};
inline bool ValidSafetyCarParameters(
    SafetyCarParameters p) noexcept
{
    return Finite(p.TargetFollowingGapCm) &&
        p.TargetFollowingGapCm>0.0 &&
        Finite(p.MinimumFollowingGapCm) &&
        p.MinimumFollowingGapCm>0.0 &&
        p.MinimumFollowingGapCm<p.TargetFollowingGapCm &&
        Finite(p.MaximumFollowingGapCm) &&
        p.MaximumFollowingGapCm>
            p.TargetFollowingGapCm &&
        Finite(p.MaximumSafetyCarSpeedCmS) &&
        p.MaximumSafetyCarSpeedCmS>0.0 &&
        Finite(p.MaximumCautionAccelerationCmS2) &&
        p.MaximumCautionAccelerationCmS2>0.0 &&
        Finite(p.MaximumCautionDecelerationCmS2) &&
        p.MaximumCautionDecelerationCmS2>0.0 &&
        Finite(p.EmergencyCautionDecelerationCmS2) &&
        p.EmergencyCautionDecelerationCmS2>=
            p.MaximumCautionDecelerationCmS2 &&
        Finite(p.MaximumCautionThrottle) &&
        p.MaximumCautionThrottle>0.0 &&
        p.MaximumCautionThrottle<=1.0 &&
        Finite(p.MaximumCautionBrake) &&
        p.MaximumCautionBrake>0.0 &&
        p.MaximumCautionBrake<=1.0 &&
        Finite(p.MaximumHitchSeconds) &&
        p.MaximumHitchSeconds>0.0 &&
        Finite(p.MinimumTireGripG) &&
        p.MinimumTireGripG>0.0 &&
        Finite(p.MaximumTireGripG) &&
        p.MaximumTireGripG>=p.MinimumTireGripG &&
        Finite(p.CautionOnsetHoldSeconds) &&
        p.CautionOnsetHoldSeconds>=0.0 &&
        Finite(p.RestartHoldSeconds) &&
        p.RestartHoldSeconds>=0.0;
}
inline bool ValidSafetyCarActor(
    SafetyCarActor a,SafetyCarParameters p) noexcept
{
    return PhysicalActorIdentityValid(a.Identity) &&
        a.RealChaosBody &&
        a.OriginalAsphaltContact &&
        Finite(a.Pose) &&
        Finite(a.VelocityCmS) &&
        Finite(a.UnwrappedProgressCm) &&
        Finite(a.GripG) &&
        Finite(a.HalfLengthCm) &&
        Finite(a.HalfWidthCm) &&
        a.GripG>=p.MinimumTireGripG &&
        a.GripG<=p.MaximumTireGripG &&
        a.HalfLengthCm>0.0 &&
        a.HalfLengthCm<=450.0 &&
        a.HalfWidthCm>0.0 &&
        a.HalfWidthCm<=240.0 &&
        Length(a.VelocityCmS)<=MaximumPhysicalSpeedCmS &&
        a.PreviousOfficialPosition>=1 &&
        a.PreviousOfficialPosition<=PhysicalFleetCapacity;
}
inline bool ValidSafetyCarObservation(
    SafetyCarObservation input,
    SafetyCarParameters p) noexcept
{
    if (!input.OriginalGameModeRacing ||
        !input.PhysicalRosterVerified ||
        !input.TrackConfirmed ||
        !input.ActualMarshalCaution ||
        !input.Flags.Valid ||
        !input.Flags.NoOvertake ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        input.DeltaSeconds>p.MaximumHitchSeconds ||
        !Finite(input.RaceClockSeconds) ||
        input.RaceClockSeconds<0.0 ||
        input.Session==0)
        return false;
    for (int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& car=input.Cars[static_cast<std::size_t>(i)];
        if (!ValidSafetyCarActor(car,p) ||
            car.Identity.Session!=input.Session ||
            car.Identity.SessionRevision!=input.Revision ||
            car.Identity.Slot!=i)
            return false;
        for (int j=0;j<i;++j)
        {
            const auto& earlier=input.Cars[
                static_cast<std::size_t>(j)];
            if (earlier.Identity.Actor==car.Identity.Actor ||
                earlier.PreviousOfficialPosition==
                    car.PreviousOfficialPosition)
                return false;
        }
    }
    return true;
}
inline SafetyCarReport EvaluateSafetyCarTrain(
    SafetyCarObservation input,
    SafetyCarParameters settings={}) noexcept
{
    SafetyCarReport report;
    if (!ValidSafetyCarParameters(settings) ||
        !ValidSafetyCarObservation(input,settings))
        return report;
    report.MinimumActualGapCm=
        std::numeric_limits<double>::infinity();
    report.TargetTrainSpeedCmS=
        settings.MaximumSafetyCarSpeedCmS*
        input.Flags.SpeedCeilingFraction;
    for (int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& car=input.Cars[static_cast<std::size_t>(i)];
        const double speed=Length(car.VelocityCmS);
        SafetyCarCommand command;
        command.Slot=i;
        command.ObservedSpeedCmS=speed;
        command.RecommendedSpeedCmS=
            report.TargetTrainSpeedCmS;
        command.MaximumThrottleCeiling=
            std::min(settings.MaximumCautionThrottle,
                input.Flags.ThrottleCeiling);
        command.MinimumBrakeFloor=
            input.Flags.BrakeFloor;
        command.PhysicallyQualified=true;
        if (speed>command.RecommendedSpeedCmS+100.0)
        {
            command.MaximumThrottleCeiling=0.0;
            command.MinimumBrakeFloor=std::max(
                command.MinimumBrakeFloor,
                std::clamp((speed-
                    command.RecommendedSpeedCmS)/
                    settings.MaximumCautionDecelerationCmS2,
                    0.0,settings.MaximumCautionBrake));
        }
        int ahead=-1;
        for (int j=0;j<PhysicalFleetCapacity;++j)
        {
            if (input.Cars[static_cast<std::size_t>(j)].
                PreviousOfficialPosition==
                car.PreviousOfficialPosition-1)
            {
                ahead=j;
                break;
            }
        }
        if (ahead>=0)
        {
            const auto& leader=input.Cars[
                static_cast<std::size_t>(ahead)];
            const double gap=leader.UnwrappedProgressCm-
                car.UnwrappedProgressCm-
                car.HalfLengthCm-leader.HalfLengthCm;
            const double closing=speed-
                Length(leader.VelocityCmS);
            command.LeaderGapCm=gap;
            command.ClosingSpeedCmS=closing;
            command.GapErrorCm=gap-
                settings.TargetFollowingGapCm;
            command.MinimumSafeGapCm=
                settings.MinimumFollowingGapCm+
                std::max(0.0,closing)*0.4;
            report.MinimumActualGapCm=std::min(
                report.MinimumActualGapCm,gap);
            report.MaximumActualGapCm=std::max(
                report.MaximumActualGapCm,gap);
            report.MaximumClosingSpeedCmS=std::max(
                report.MaximumClosingSpeedCmS,closing);
            report.MeanSpacingCm+=gap;
            if (gap<command.MinimumSafeGapCm)
            {
                command.StopForUnsafeGap=true;
                command.MaximumThrottleCeiling=0.0;
                command.MinimumBrakeFloor=1.0;
                ++report.ViolatedMinimumGapCars;
            }
            else if (gap<
                     settings.TargetFollowingGapCm)
            {
                command.MaximumThrottleCeiling=std::min(
                    command.MaximumThrottleCeiling,
                    Clamp01(
                        SafeDivision(gap,
                            settings.TargetFollowingGapCm)));
            }
            if (gap<0.0)++report.UnsafePassingObservations;
            command.RequiredDecelerationCmS2=closing>0.0 ?
                SafeDivision(Square(closing),
                    2.0*std::max(gap,50.0)):0.0;
        }
        command.CannotOvertake=true;
        command.SendsActualCarControls=false;
        report.BySlot[static_cast<std::size_t>(i)]=command;
        ++report.QualifiedCars;
    }
    report.MeanSpacingCm/=5.0;
    report.AllCarsSlowed=true;
    for (const auto& cmd:report.BySlot)
    {
        if (cmd.ObservedSpeedCmS>
            settings.MaximumSafetyCarSpeedCmS+200.0)
            report.AllCarsSlowed=false;
    }
    report.RaceClockModified=false;
    report.OfficialRankModified=false;
    report.Valid=true;
    return report;
}
class SafetyCarTrainMonitor final
{
public:
    bool Configure(SafetyCarParameters settings) noexcept
    {
        if (!ValidSafetyCarParameters(settings))return false;
        Params=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Session=0;
        Revision=0;
        QualifiedFrames=0;
        RejectedFrames=0;
        GapViolationFrames=0;
        SafetyCarSeconds=0.0;
        WorstGapCm=std::numeric_limits<double>::infinity();
    }
    SafetyCarReport Observe(SafetyCarObservation sample) noexcept
    {
        auto result=EvaluateSafetyCarTrain(sample,Params);
        if (!result.Valid)
        {
            ++RejectedFrames;
            return {};
        }
        if (Session!=0 &&
            (sample.Session!=Session ||
             sample.Revision!=Revision))
        {
            Reset();
        }
        Session=sample.Session;
        Revision=sample.Revision;
        Last=result;
        ++QualifiedFrames;
        SafetyCarSeconds+=sample.DeltaSeconds;
        if (result.ViolatedMinimumGapCars>0)
            ++GapViolationFrames;
        WorstGapCm=std::min(
            WorstGapCm,result.MinimumActualGapCm);
        return Last;
    }
    SafetyCarReport Snapshot() const noexcept{return Last;}
    std::uint64_t QualifiedSamples() const noexcept
    {return QualifiedFrames;}
    std::uint64_t GapViolationSamples() const noexcept
    {return GapViolationFrames;}
private:
    SafetyCarParameters Params{};
    SafetyCarReport Last{};
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t GapViolationFrames=0;
    double SafetyCarSeconds=0.0;
    double WorstGapCm=0.0;
};
} // namespace latebrake::racecraft
