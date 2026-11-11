#pragma once
#include "Racecraft/RaceStartGridSafety.h"
#include "Racecraft/RaceClockDirector.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class StartFault : std::uint8_t
{
    None, MissingEngineStart, MissingCars,
    RosterMismatch, EarlyThrottle, PreGreenMovement,
    ChassisUnsupported, WrongPhase, StaleCountdown,
    OverlappingChassis, UnrealAuthorityUnconfirmed
};
struct StartLaunchSettings final
{
    double MaximumAllowedCountdownThrottle=0.02;
    double MaximumCountdownTravelCm=22.0;
    double MaximumCountdownSpeedCmS=18.0;
    double MaximumLaunchJumpCm=120.0;
    double MaximumForwardHeadingErrorRadians=0.28;
    double MaximumAllowedLaunchYawRateRadS=0.75;
    double MaximumVerticalDisplacementCm=40.0;
    double MaximumFrameSeconds=0.10;
    double MinimumVerifiedCountdownSeconds=0.5;
    double MaximumLaunchWindowSeconds=2.0;
    double MaximumGreenReactionSeconds=2.0;
    double MinimumValidReactionSeconds=0.0;
    double FalseStartThrottleThreshold=0.06;
    double FalseStartSpeedThresholdCmS=60.0;
    double ValidLaunchAccelerationCmS2=1200.0;
    int MinimumConsecutivePhysicalFrames=3;
    bool RequireOriginalSixCarGrid=true;
    bool RequireAllCarsOnWorldStatic=true;
    bool RequireOfficialGreenLight=true;
    bool RecordFalseStartEvidenceOnly=true;
};
struct StartLaunchObservation final
{
    FleetFrame Fleet{};
    RaceClockReceipt Clock{};
    double DeltaSeconds=0.0;
    double ActualMonotonicSeconds=0.0;
    double GreenEventMonotonicSeconds=0.0;
    bool ActualLightRigGreen=false;
    bool OfficialRaceStarted=false;
    bool OriginalStartGridStillIntact=false;
    bool PhysicalRigidBodyObservationsReady=false;
};
struct StartLaunchDecision final
{
    StartFault Fault=StartFault::MissingEngineStart;
    double CountdownProgressSeconds=0.0;
    double EarliestThrottleTimeSeconds=0.0;
    double LaunchReactionSeconds=0.0;
    double HighestPreGreenSpeedCmS=0.0;
    double GreatestPreGreenDisplacementCm=0.0;
    double LargestLaunchYawRateRadS=0.0;
    std::array<double,PhysicalFleetCapacity> LaunchSpeedsCmS{};
    std::array<double,PhysicalFleetCapacity> ReactionSeconds{};
    int PhysicallyQualifiedCars=0;
    int PreGreenMotionCars=0;
    int EarlyThrottleCars=0;
    int CarsWithWheelContacts=0;
    int CarsReadyToGo=0;
    bool FalseStartEvidence=false;
    bool CountdownFullyVerified=false;
    bool GreenIssuedByRealGameMode=false;
    bool RaceClockModified=false;
    bool CanAuthorizeGo=false;
    bool Valid=false;
};
inline bool ValidStartLaunchSettings(StartLaunchSettings p) noexcept
{
    return Finite(p.MaximumAllowedCountdownThrottle) &&
        p.MaximumAllowedCountdownThrottle>=0.0 &&
        p.MaximumAllowedCountdownThrottle<=1.0 &&
        Finite(p.MaximumCountdownTravelCm) &&
        p.MaximumCountdownTravelCm>=0.0 &&
        Finite(p.MaximumCountdownSpeedCmS) &&
        p.MaximumCountdownSpeedCmS>=0.0 &&
        Finite(p.MaximumLaunchJumpCm) &&
        p.MaximumLaunchJumpCm>0.0 &&
        Finite(p.MaximumForwardHeadingErrorRadians) &&
        p.MaximumForwardHeadingErrorRadians>0.0 &&
        Finite(p.MaximumAllowedLaunchYawRateRadS) &&
        p.MaximumAllowedLaunchYawRateRadS>0.0 &&
        Finite(p.MaximumVerticalDisplacementCm) &&
        p.MaximumVerticalDisplacementCm>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds &&
        Finite(p.MinimumVerifiedCountdownSeconds) &&
        p.MinimumVerifiedCountdownSeconds>=0.0 &&
        Finite(p.MaximumLaunchWindowSeconds) &&
        p.MaximumLaunchWindowSeconds>0.0 &&
        Finite(p.FalseStartThrottleThreshold) &&
        p.FalseStartThrottleThreshold>=0.0 &&
        p.FalseStartThrottleThreshold<=1.0 &&
        p.MinimumConsecutivePhysicalFrames>=1 &&
        p.MinimumConsecutivePhysicalFrames<=300;
}
inline bool ValidStartLaunchObservation(
    StartLaunchObservation sample,
    StartLaunchSettings s) noexcept
{
    return sample.PhysicalRigidBodyObservationsReady &&
        sample.Fleet.EngineRosterVerified &&
        sample.Fleet.OriginalMapVerified &&
        sample.Fleet.Count==PhysicalFleetCapacity &&
        sample.Clock.SessionToken==sample.Fleet.Session &&
        ValidFrameDelta(sample.DeltaSeconds) &&
        sample.DeltaSeconds<=s.MaximumFrameSeconds &&
        Finite(sample.ActualMonotonicSeconds) &&
        Finite(sample.GreenEventMonotonicSeconds) &&
        sample.ActualMonotonicSeconds>=0.0 &&
        ValidateFleetFrame(sample.Fleet,true)==SnapshotFault::None;
}
inline StartLaunchDecision ReviewRaceStart(
    StartLaunchObservation observation,
    StartLaunchSettings s={}) noexcept
{
    StartLaunchDecision out;
    if (!ValidStartLaunchSettings(s) ||
        !ValidStartLaunchObservation(observation,s))
        return out;
    out.Fault=StartFault::None;
    out.Valid=true;
    const bool countdown=
        observation.Clock.Phase==RacePhase::Countdown;
    const bool green=
        observation.Clock.Phase==RacePhase::Green;
    if (!countdown && !green)
    {
        out.Fault=StartFault::WrongPhase;
        return out;
    }
    for (int i=0;i<observation.Fleet.Count;++i)
    {
        const auto& car=observation.Fleet.Members[
            static_cast<std::size_t>(i)];
        ++out.PhysicallyQualifiedCars;
        const double speed=Length(car.Chassis.VelocityCmS);
        int contacts=0;
        for (const auto& wheel:car.Wheels)
        {
            if (wheel.ContactVerified &&
                wheel.PhysicalContact)
                ++contacts;
        }
        if (contacts>=3)++out.CarsWithWheelContacts;
        if (countdown)
        {
            out.HighestPreGreenSpeedCmS=std::max(
                out.HighestPreGreenSpeedCmS,speed);
            if (speed>s.MaximumCountdownSpeedCmS)
                ++out.PreGreenMotionCars;
            if (car.Controls.Throttle>
                s.MaximumAllowedCountdownThrottle)
                ++out.EarlyThrottleCars;
        }
        out.LaunchSpeedsCmS[static_cast<std::size_t>(i)]=
            speed;
        if (contacts>=3 &&
            car.ChassisOnWorldStatic &&
            car.Identity.BodySimulating &&
            car.Track.OnOriginalPavement)
            ++out.CarsReadyToGo;
    }
    out.FalseStartEvidence=
        out.PreGreenMotionCars>0 ||
        out.EarlyThrottleCars>0;
    if (out.CarsReadyToGo!=PhysicalFleetCapacity)
    {
        out.Fault=StartFault::ChassisUnsupported;
        return out;
    }
    if (countdown)
    {
        if (out.FalseStartEvidence)
        {
            out.Fault=out.PreGreenMotionCars>0 ?
                StartFault::PreGreenMovement :
                StartFault::EarlyThrottle;
            return out;
        }
        out.CountdownFullyVerified=
            observation.Clock.CountdownRemainingSeconds>0.0;
    }
    if (green)
    {
        out.GreenIssuedByRealGameMode=
            observation.OfficialRaceStarted &&
            observation.ActualLightRigGreen &&
            observation.Clock.PhysicalRosterAtGo;
        if (!out.GreenIssuedByRealGameMode)
        {
            out.Fault=StartFault::UnrealAuthorityUnconfirmed;
            return out;
        }
        out.LaunchReactionSeconds=std::max(
            0.0,observation.ActualMonotonicSeconds-
                observation.GreenEventMonotonicSeconds);
    }
    out.CanAuthorizeGo=false;
    out.RaceClockModified=false;
    return out;
}
class RaceStartLaunchMonitor final
{
public:
    bool Configure(StartLaunchSettings params) noexcept
    {
        if (!ValidStartLaunchSettings(params))
            return false;
        Settings=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        FirstPositions.fill({});
        HasBaseline=false;
        HasGreen=false;
        CountdownFrames=0;
        QualifiedFrames=0;
        RejectedFrames=0;
        FalseStartFrames=0;
        LaunchFrames=0;
        Session=0;
        Revision=0;
        FirstGreenSeconds=0.0;
    }
    StartLaunchDecision Observe(
        StartLaunchObservation sample) noexcept
    {
        auto result=ReviewRaceStart(sample,Settings);
        if (!result.Valid)
        {
            ++RejectedFrames;
            HasBaseline=false;
            return {};
        }
        if (Session!=0 &&
            (sample.Fleet.Session!=Session ||
             sample.Fleet.Revision!=Revision))
        {
            HasBaseline=false;
            CountdownFrames=0;
            HasGreen=false;
        }
        Session=sample.Fleet.Session;
        Revision=sample.Fleet.Revision;
        if (!HasBaseline)
        {
            for (int i=0;i<PhysicalFleetCapacity;++i)
            {
                const auto& p=sample.Fleet.Members[
                    static_cast<std::size_t>(i)].Chassis;
                FirstPositions[static_cast<std::size_t>(i)]=
                    p.PlanarPose.Position;
            }
            HasBaseline=true;
        }
        if (sample.Clock.Phase==RacePhase::Countdown)
        {
            ++CountdownFrames;
            for (int i=0;i<PhysicalFleetCapacity;++i)
            {
                const Vec2 now=sample.Fleet.Members[
                    static_cast<std::size_t>(i)].
                    Chassis.PlanarPose.Position;
                const double travel=Distance(
                    now,FirstPositions[
                        static_cast<std::size_t>(i)]);
                result.GreatestPreGreenDisplacementCm=std::max(
                    result.GreatestPreGreenDisplacementCm,
                    travel);
                if (travel>Settings.MaximumCountdownTravelCm)
                {
                    result.FalseStartEvidence=true;
                    result.Fault=StartFault::PreGreenMovement;
                }
            }
        }
        if (sample.Clock.Phase==RacePhase::Green)
        {
            if (!HasGreen)
            {
                HasGreen=true;
                FirstGreenSeconds=
                    sample.ActualMonotonicSeconds;
            }
            ++LaunchFrames;
        }
        if (result.FalseStartEvidence)
            ++FalseStartFrames;
        result.CountdownFullyVerified=
            CountdownFrames>=
            Settings.MinimumConsecutivePhysicalFrames;
        ++QualifiedFrames;
        Last=result;
        return Last;
    }
    StartLaunchDecision Snapshot() const noexcept {return Last;}
    std::uint64_t CountdownPhysicalFrames() const noexcept
    {return CountdownFrames;}
    std::uint64_t FalseStartObservationFrames() const noexcept
    {return FalseStartFrames;}
private:
    StartLaunchSettings Settings{};
    StartLaunchDecision Last{};
    std::array<Vec2,PhysicalFleetCapacity> FirstPositions{};
    std::uint64_t Session=0;
    std::uint32_t Revision=0;
    std::uint64_t CountdownFrames=0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t FalseStartFrames=0;
    std::uint64_t LaunchFrames=0;
    double FirstGreenSeconds=0.0;
    bool HasBaseline=false;
    bool HasGreen=false;
};
} // namespace latebrake::racecraft
