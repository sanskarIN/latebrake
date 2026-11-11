#pragma once
#include "Racecraft/CornerGripForecast.h"
#include "Racecraft/PhysicalCarFollowing.h"
#include "Racecraft/VehicleStabilityAssist.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
enum class SafetyControlReason : std::uint8_t
{
    Unqualified,
    FreeRunning,
    CornerPace,
    TrafficProximity,
    StabilityIntervention,
    CompoundRestriction,
    EmergencyStop,
    RosterReset,
    ReplayedFrame
};

struct SafetyControlSettings final
{
    double MaximumFrameTimeSeconds=0.1;
    double MaximumPositionDisagreementCm=25.0;
    double MaximumVelocityDisagreementCmS=30.0;
    double MaximumYawDisagreementRadians=0.03;
    double MaximumDeltaDisagreementSeconds=0.0001;
    double MaximumGripDisagreementG=1.5;
    double MaximumThrottleRecoveryPerSecond=1.0;
    double MaximumBrakeReleasePerSecond=2.0;
    double MaximumSteeringInput=1.0;
    double MaximumSteeringRecoveryPerSecond=3.0;
    double BrakingThrottleInterlock=0.05;
    double MinimumEmergencyBrake=1.0;
    double MaximumSteerDuringEmergency=1.0;
    double RosterChangeHoldSeconds=0.28;
    double MaximumUnqualifiedHoldSeconds=0.65;
    double MinimumControlConfidence=0.65;
    bool RequireAllFiveRivals=true;
    bool RequireActualEngineSource=true;
    bool RequireStableRoster=true;
    bool AlwaysPreserveDriverBrake=true;
    bool NeverIncreaseDriverThrottle=true;
    bool NeverIssueChaosForces=true;
};

struct SafetyControlFrame final
{
    CornerForecastObservation Corner{};
    FollowingObservation Following{};
    StabilityIntervention Stability{};
    double DriverThrottle=0.0;
    double DriverBrake=0.0;
    double DriverSteering=0.0;
    double Confidence=1.0;
    bool StabilitySensorAvailable=false;
    bool EngineAuthorityQualified=false;
    bool ActualRivalActorConfirmed=false;
    bool EngineRaceRunning=false;
    bool OriginalMapConfirmed=false;
};

struct SafetyControlOutput final
{
    RaceIdentity Identity{};
    SafetyControlReason Reason=SafetyControlReason::Unqualified;
    double RequestedThrottle=0.0;
    double RequestedBrake=1.0;
    double RequestedSteering=0.0;
    double GrantedThrottle=0.0;
    double GrantedBrake=1.0;
    double GrantedSteering=0.0;
    double CornerThrottleCeiling=0.0;
    double TrafficThrottleCeiling=0.0;
    double StabilityThrottleCeiling=0.0;
    double CornerBrakeFloor=1.0;
    double TrafficBrakeFloor=1.0;
    double StabilityBrakeFloor=0.0;
    double EffectiveSpeedLimitCmS=0.0;
    double ObservedClosingSpeedCmS=0.0;
    double DirtyAirDownforceLoss=0.0;
    double VerifiedCornerMarginCm=0.0;
    int PhysicalRivalsChecked=0;
    int LimitingTrafficSlot=-1;
    int LimitingCornerKnot=-1;
    bool BrakeOverridesThrottle=false;
    bool EmergencyStop=false;
    bool RosterTransition=false;
    bool CornerQualified=false;
    bool TrafficQualified=false;
    bool StabilityQualified=false;
    bool ActualForceApplied=false;
    bool SourceOnly=true;
    bool Valid=false;
};

inline bool ValidSafetyControlSettings(
    SafetyControlSettings p) noexcept
{
    return Finite(p.MaximumFrameTimeSeconds) &&
        p.MaximumFrameTimeSeconds>0.0 &&
        p.MaximumFrameTimeSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumPositionDisagreementCm) &&
        p.MaximumPositionDisagreementCm>=0.0 &&
        Finite(p.MaximumVelocityDisagreementCmS) &&
        p.MaximumVelocityDisagreementCmS>=0.0 &&
        Finite(p.MaximumYawDisagreementRadians) &&
        p.MaximumYawDisagreementRadians>=0.0 &&
        p.MaximumYawDisagreementRadians<=Pi &&
        Finite(p.MaximumDeltaDisagreementSeconds) &&
        p.MaximumDeltaDisagreementSeconds>=0.0 &&
        Finite(p.MaximumGripDisagreementG) &&
        p.MaximumGripDisagreementG>=0.0 &&
        Finite(p.MaximumThrottleRecoveryPerSecond) &&
        p.MaximumThrottleRecoveryPerSecond>0.0 &&
        Finite(p.MaximumBrakeReleasePerSecond) &&
        p.MaximumBrakeReleasePerSecond>0.0 &&
        Finite(p.MaximumSteeringInput) &&
        p.MaximumSteeringInput>0.0 &&
        p.MaximumSteeringInput<=1.0 &&
        Finite(p.MaximumSteeringRecoveryPerSecond) &&
        p.MaximumSteeringRecoveryPerSecond>0.0 &&
        Finite(p.BrakingThrottleInterlock) &&
        p.BrakingThrottleInterlock>=0.0 &&
        p.BrakingThrottleInterlock<1.0 &&
        Finite(p.MinimumEmergencyBrake) &&
        p.MinimumEmergencyBrake>0.0 &&
        p.MinimumEmergencyBrake<=1.0 &&
        Finite(p.MaximumSteerDuringEmergency) &&
        p.MaximumSteerDuringEmergency>=0.0 &&
        p.MaximumSteerDuringEmergency<=1.0 &&
        Finite(p.RosterChangeHoldSeconds) &&
        p.RosterChangeHoldSeconds>=0.0 &&
        Finite(p.MaximumUnqualifiedHoldSeconds) &&
        p.MaximumUnqualifiedHoldSeconds>0.0 &&
        Finite(p.MinimumControlConfidence) &&
        p.MinimumControlConfidence>0.0 &&
        p.MinimumControlConfidence<=1.0;
}

inline bool SameSourceActor(
    RaceIdentity a,RaceIdentity b) noexcept
{
    return PhysicalActorIdentityValid(a) &&
        PhysicalActorIdentityValid(b) &&
        a.Session==b.Session &&
        a.SessionRevision==b.SessionRevision &&
        a.Actor==b.Actor &&
        a.Slot==b.Slot &&
        a.Frame==b.Frame &&
        a.Frame>0;
}

inline bool ValidSafetyControlFrame(
    const SafetyControlFrame& s,
    SafetyControlSettings p) noexcept
{
    if (!ValidSafetyControlSettings(p) ||
        !s.EngineRaceRunning ||
        !s.EngineAuthorityQualified ||
        !s.ActualRivalActorConfirmed ||
        !s.OriginalMapConfirmed ||
        !Finite(s.DriverThrottle) ||
        !Finite(s.DriverBrake) ||
        !Finite(s.DriverSteering) ||
        !Finite(s.Confidence) ||
        s.DriverThrottle<0.0 ||
        s.DriverThrottle>1.0 ||
        s.DriverBrake<0.0 ||
        s.DriverBrake>1.0 ||
        std::abs(s.DriverSteering)>p.MaximumSteeringInput ||
        s.Confidence<p.MinimumControlConfidence ||
        s.Confidence>1.0 ||
        !SameSourceActor(s.Corner.Identity,
                         s.Following.Self.Identity) ||
        s.Corner.Identity.Slot==0 ||
        !ValidCornerForecastObservation(s.Corner,{}) ||
        !ValidFollowingObservation(s.Following,{}) ||
        (p.RequireAllFiveRivals &&
         s.Following.OpponentCount!=PhysicalFleetCapacity-1) ||
        std::abs(s.Corner.DeltaSeconds-
                 s.Following.DeltaSeconds)>
            p.MaximumDeltaDisagreementSeconds ||
        s.Corner.DeltaSeconds>p.MaximumFrameTimeSeconds ||
        std::abs(s.DriverThrottle-
                 s.Corner.ProposedThrottle)>1e-6 ||
        std::abs(s.DriverBrake-
                 s.Corner.ProposedBrake)>1e-6 ||
        std::abs(s.DriverThrottle-
                 s.Following.ProposedThrottle)>1e-6 ||
        std::abs(s.DriverBrake-
                 s.Following.ProposedBrake)>1e-6 ||
        Length(s.Corner.VehicleVelocityCmS-
               s.Following.Self.VelocityCmS)>
            p.MaximumVelocityDisagreementCmS ||
        std::abs(AngleDifference(
            s.Corner.HeadingRadians,
            s.Following.Self.Pose.HeadingRadians))>
                p.MaximumYawDisagreementRadians ||
        s.Following.Self.GripG<0.15 ||
        s.Following.Self.GripG>2.5)
        return false;
    if (s.StabilitySensorAvailable)
    {
        if (!s.Stability.Valid ||
            s.Stability.SentActualChaosForce ||
            !Finite(s.Stability.ThrottleCeiling) ||
            !Finite(s.Stability.CombinedBrakeFloor) ||
            !Finite(s.Stability.SteeringCeilingFraction) ||
            s.Stability.ThrottleCeiling<0.0 ||
            s.Stability.ThrottleCeiling>1.0 ||
            s.Stability.CombinedBrakeFloor<0.0 ||
            s.Stability.CombinedBrakeFloor>1.0 ||
            s.Stability.SteeringCeilingFraction<0.0 ||
            s.Stability.SteeringCeilingFraction>1.0)
            return false;
    }
    return true;
}

inline SafetyControlOutput ResolveSafetyControl(
    const SafetyControlFrame& sample,
    SafetyControlSettings p={}) noexcept
{
    SafetyControlOutput out;
    if (!ValidSafetyControlFrame(sample,p))
        return out;
    const CornerGripDecision corner=
        ForecastPhysicalCornerGrip(sample.Corner);
    const FollowingDecision traffic=
        RestrictPhysicalCarFollowing(sample.Following);
    if (!corner.Valid || !traffic.Valid)
        return out;
    out.Identity=sample.Corner.Identity;
    out.RequestedThrottle=sample.DriverThrottle;
    out.RequestedBrake=sample.DriverBrake;
    out.RequestedSteering=sample.DriverSteering;
    out.CornerThrottleCeiling=corner.ThrottleCeiling;
    out.TrafficThrottleCeiling=traffic.ThrottleCeiling;
    out.StabilityThrottleCeiling=
        sample.StabilitySensorAvailable ?
            sample.Stability.ThrottleCeiling:1.0;
    out.CornerBrakeFloor=corner.BrakeFloor;
    out.TrafficBrakeFloor=traffic.BrakeFloor;
    out.StabilityBrakeFloor=
        sample.StabilitySensorAvailable ?
            sample.Stability.CombinedBrakeFloor:0.0;
    out.EffectiveSpeedLimitCmS=
        corner.PlannedEntrySpeedCmS;
    out.ObservedClosingSpeedCmS=
        traffic.MaximumClosingSpeedCmS;
    out.DirtyAirDownforceLoss=
        corner.WakeDownforcePenalty;
    out.VerifiedCornerMarginCm=
        corner.Knots[0].RoadMarginCm;
    out.PhysicalRivalsChecked=traffic.VerifiedOpponents;
    out.LimitingTrafficSlot=traffic.Limiting.Slot;
    out.LimitingCornerKnot=corner.LimitingKnot;
    out.CornerQualified=true;
    out.TrafficQualified=true;
    out.StabilityQualified=
        !sample.StabilitySensorAvailable ||
        sample.Stability.Valid;
    const double ceiling=std::min({
        out.CornerThrottleCeiling,
        out.TrafficThrottleCeiling,
        out.StabilityThrottleCeiling});
    const double floor=std::max({
        out.CornerBrakeFloor,
        out.TrafficBrakeFloor,
        out.StabilityBrakeFloor});
    out.GrantedThrottle=std::min(
        sample.DriverThrottle,ceiling);
    out.GrantedBrake=std::max(
        sample.DriverBrake,floor);
    const double steeringAllowance=
        sample.StabilitySensorAvailable ?
        sample.Stability.SteeringCeilingFraction:1.0;
    out.GrantedSteering=std::clamp(
        sample.DriverSteering,
        -steeringAllowance,steeringAllowance);
    out.EmergencyStop=
        traffic.EmergencyBrake ||
        corner.EmergencySlowdown;
    if (out.EmergencyStop)
    {
        out.GrantedThrottle=0.0;
        out.GrantedBrake=std::max(
            out.GrantedBrake,p.MinimumEmergencyBrake);
        out.GrantedSteering=std::clamp(
            out.GrantedSteering,
            -p.MaximumSteerDuringEmergency,
            p.MaximumSteerDuringEmergency);
    }
    out.BrakeOverridesThrottle=
        out.GrantedBrake>p.BrakingThrottleInterlock;
    if (out.BrakeOverridesThrottle)
        out.GrantedThrottle=0.0;
    const bool limitedCorner=
        corner.ThrottleCeiling<sample.DriverThrottle ||
        corner.BrakeFloor>sample.DriverBrake;
    const bool limitedTraffic=
        traffic.ThrottleCeiling<sample.DriverThrottle ||
        traffic.BrakeFloor>sample.DriverBrake;
    const bool limitedStability=
        sample.StabilitySensorAvailable &&
        (sample.Stability.ThrottleCeiling<sample.DriverThrottle ||
         sample.Stability.CombinedBrakeFloor>sample.DriverBrake);
    const int count=
        static_cast<int>(limitedCorner)+
        static_cast<int>(limitedTraffic)+
        static_cast<int>(limitedStability);
    out.Reason=out.EmergencyStop ?
        SafetyControlReason::EmergencyStop :
        count>1 ? SafetyControlReason::CompoundRestriction :
        limitedCorner ? SafetyControlReason::CornerPace :
        limitedTraffic ? SafetyControlReason::TrafficProximity :
        limitedStability ? SafetyControlReason::StabilityIntervention :
        SafetyControlReason::FreeRunning;
    out.ActualForceApplied=false;
    out.SourceOnly=true;
    out.Valid=Finite(out.GrantedThrottle) &&
        Finite(out.GrantedBrake) &&
        Finite(out.GrantedSteering) &&
        out.GrantedThrottle>=0.0 &&
        out.GrantedBrake>=0.0 &&
        out.GrantedThrottle<=sample.DriverThrottle &&
        out.GrantedBrake>=sample.DriverBrake;
    return out;
}

class SafetyControlSupervisor final
{
public:
    bool Configure(SafetyControlSettings settings) noexcept
    {
        if (!ValidSafetyControlSettings(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        LastThrottle=0.0;
        LastBrake=0.0;
        LastSteering=0.0;
        HoldSeconds=0.0;
        LastIdentity={};
        Roster.fill(0);
        HasRoster=false;
        Accepted=0;
        Rejected=0;
        EmergencyFrames=0;
        RosterChangeFrames=0;
    }
    SafetyControlOutput Step(
        const SafetyControlFrame& sample) noexcept
    {
        SafetyControlOutput decision=
            ResolveSafetyControl(sample,Settings);
        if (!decision.Valid ||
            (HasRoster &&
             SameSourceActor(
                 LastIdentity,sample.Corner.Identity)))
        {
            ++Rejected;
            Last={};
            LastThrottle=0.0;
            LastBrake=1.0;
            LastSteering=0.0;
            HoldSeconds=Settings.MaximumUnqualifiedHoldSeconds;
            return {};
        }
        std::array<std::uint64_t,PhysicalFleetCapacity> now{};
        now[static_cast<std::size_t>(
            sample.Corner.Identity.Slot)]=
            sample.Corner.Identity.Actor;
        for (int i=0;i<sample.Following.OpponentCount;++i)
        {
            const auto& other=sample.Following.Opponents[
                static_cast<std::size_t>(i)];
            now[static_cast<std::size_t>(
                other.Identity.Slot)]=other.Identity.Actor;
        }
        const bool changed=HasRoster &&
            (now!=Roster ||
             sample.Corner.Identity.Session!=
                 LastIdentity.Session ||
             sample.Corner.Identity.SessionRevision!=
                 LastIdentity.SessionRevision ||
             sample.Corner.Identity.Frame<=LastIdentity.Frame);
        if (changed)
        {
            ++RosterChangeFrames;
            HoldSeconds=std::max(
                HoldSeconds,Settings.RosterChangeHoldSeconds);
        }
        if (decision.EmergencyStop)
            HoldSeconds=std::max(
                HoldSeconds,Settings.RosterChangeHoldSeconds);
        else
            HoldSeconds=std::max(
                0.0,HoldSeconds-sample.Corner.DeltaSeconds);
        if (HoldSeconds>0.0)
        {
            decision.GrantedThrottle=0.0;
            decision.GrantedBrake=1.0;
            decision.RosterTransition=true;
            decision.Reason=SafetyControlReason::RosterReset;
        }
        else
        {
            decision.GrantedThrottle=std::min(
                decision.GrantedThrottle,
                LastThrottle+
                Settings.MaximumThrottleRecoveryPerSecond*
                    sample.Corner.DeltaSeconds);
            decision.GrantedBrake=std::max(
                decision.GrantedBrake,
                LastBrake-
                Settings.MaximumBrakeReleasePerSecond*
                    sample.Corner.DeltaSeconds);
            decision.GrantedSteering=RateLimited(
                LastSteering,decision.GrantedSteering,
                Settings.MaximumSteeringRecoveryPerSecond,
                Settings.MaximumSteeringRecoveryPerSecond,
                sample.Corner.DeltaSeconds);
            if (decision.GrantedBrake>
                Settings.BrakingThrottleInterlock)
                decision.GrantedThrottle=0.0;
        }
        decision.GrantedThrottle=std::min(
            decision.GrantedThrottle,
            sample.DriverThrottle);
        decision.GrantedBrake=std::max(
            decision.GrantedBrake,sample.DriverBrake);
        LastIdentity=sample.Corner.Identity;
        Roster=now;
        HasRoster=true;
        LastThrottle=decision.GrantedThrottle;
        LastBrake=decision.GrantedBrake;
        LastSteering=decision.GrantedSteering;
        ++Accepted;
        if (decision.EmergencyStop)++EmergencyFrames;
        Last=decision;
        return Last;
    }
    SafetyControlOutput Snapshot() const noexcept {return Last;}
    std::uint64_t Frames() const noexcept {return Accepted;}
    std::uint64_t Rejections() const noexcept {return Rejected;}
    std::uint64_t EmergencyStops() const noexcept
    {return EmergencyFrames;}
    std::uint64_t RosterTransitions() const noexcept
    {return RosterChangeFrames;}
private:
    SafetyControlSettings Settings{};
    SafetyControlOutput Last{};
    RaceIdentity LastIdentity{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t EmergencyFrames=0;
    std::uint64_t RosterChangeFrames=0;
    double LastThrottle=0.0;
    double LastBrake=0.0;
    double LastSteering=0.0;
    double HoldSeconds=0.0;
    bool HasRoster=false;
};
} // namespace latebrake::racecraft
