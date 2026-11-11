#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct FollowingParameters final
{
    double DesiredTimeHeadwaySeconds=1.40;
    double MinimumStandstillGapCm=580.0;
    double MaximumInteractionDistanceCm=5000.0;
    double MaximumLateralInteractionCm=350.0;
    double ComfortableAccelerationCmS2=480.0;
    double ComfortableDecelerationCmS2=700.0;
    double EmergencyDecelerationCmS2=1200.0;
    double MinimumEstimatedGripG=0.15;
    double MaximumEstimatedGripG=2.5;
    double MaximumReactionSeconds=0.30;
    double RearwardIgnoreDistanceCm=500.0;
    double MinimumApproachSpeedCmS=100.0;
    double MaximumRelativeVelocityCmS=8000.0;
    double RateLimitedThrottleRecoveryPerSecond=1.0;
    double BrakeReleasePerSecond=2.2;
    double ClosingSeverityGain=0.9;
    double StoppingSafetyMarginCm=180.0;
    double PhysicalBodyHalfLengthCm=220.0;
    double LaneWidthCm=720.0;
    double MinimumOverlapFraction=0.12;
    double RecoveryHazardHoldSeconds=0.30;
    double MaximumTimeToContactSeconds=5.0;
    bool RequireSameSession=true;
    bool RequireActualRigidBodies=true;
    bool RequireVerifiedRoad=true;
    bool NeverIncreaseThrottle=true;
    bool NeverReducePriorBraking=true;
};

struct FollowingActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    Vec2 AccelerationCmS2{};
    double HalfLengthCm=220.0;
    double HalfWidthCm=95.0;
    double GripG=1.0;
    double TrackDistanceCm=0.0;
    double LateralDeviationCm=0.0;
    bool OriginalCollisionBody=true;
    bool OnOriginalAsphalt=true;
    bool ContactPhysicsReady=true;
};

struct FollowingObservation final
{
    FollowingActor Self{};
    std::array<FollowingActor,5> Opponents{};
    int OpponentCount=0;
    double DeltaSeconds=0.0;
    double ProposedThrottle=0.0;
    double ProposedBrake=0.0;
    bool EnginePhysicsQualified=false;
    bool RaceRunning=false;
};

struct FollowingThreat final
{
    int Slot=-1;
    std::uint64_t Actor=0;
    double LongitudinalGapCm=0.0;
    double LateralSeparationCm=0.0;
    double RelativeForwardSpeedCmS=0.0;
    double ClosingSpeedCmS=0.0;
    double DesiredGapCm=0.0;
    double StoppingDistanceCm=0.0;
    double TimeToContactSeconds=0.0;
    double RequiredDecelerationCmS2=0.0;
    double ThrottleCeiling=1.0;
    double BrakeFloor=0.0;
    double Severity=0.0;
    bool SameLane=false;
    bool PhysicallyClosing=false;
    bool StopImmediately=false;
    bool Valid=false;
};

struct FollowingDecision final
{
    FollowingThreat Limiting{};
    double ThrottleCeiling=0.0;
    double BrakeFloor=1.0;
    double ProposedThrottle=0.0;
    double ProposedBrake=1.0;
    double MaximumClosingSpeedCmS=0.0;
    double NearestLongitudinalGapCm=0.0;
    double MinimumTimeToContactSeconds=0.0;
    double RequestedSpacingCm=0.0;
    int VerifiedOpponents=0;
    int SameLaneOpponents=0;
    int ClosingOpponents=0;
    bool EmergencyBrake=false;
    bool HoldingForRosterChange=false;
    bool NoActorWasInvented=true;
    bool DirectVehicleCommandIssued=false;
    bool Valid=false;
};

inline bool ValidFollowingParameters(FollowingParameters s) noexcept
{
    return Finite(s.DesiredTimeHeadwaySeconds) &&
        s.DesiredTimeHeadwaySeconds>0.0 &&
        Finite(s.MinimumStandstillGapCm) &&
        s.MinimumStandstillGapCm>0.0 &&
        Finite(s.MaximumInteractionDistanceCm) &&
        s.MaximumInteractionDistanceCm>
            s.MinimumStandstillGapCm &&
        Finite(s.MaximumLateralInteractionCm) &&
        s.MaximumLateralInteractionCm>0.0 &&
        Finite(s.ComfortableAccelerationCmS2) &&
        s.ComfortableAccelerationCmS2>0.0 &&
        Finite(s.ComfortableDecelerationCmS2) &&
        s.ComfortableDecelerationCmS2>0.0 &&
        Finite(s.EmergencyDecelerationCmS2) &&
        s.EmergencyDecelerationCmS2>=
            s.ComfortableDecelerationCmS2 &&
        Finite(s.MinimumEstimatedGripG) &&
        s.MinimumEstimatedGripG>0.0 &&
        Finite(s.MaximumEstimatedGripG) &&
        s.MaximumEstimatedGripG>=s.MinimumEstimatedGripG &&
        Finite(s.MaximumReactionSeconds) &&
        s.MaximumReactionSeconds>=0.0 &&
        Finite(s.RearwardIgnoreDistanceCm) &&
        s.RearwardIgnoreDistanceCm>=0.0 &&
        Finite(s.PhysicalBodyHalfLengthCm) &&
        s.PhysicalBodyHalfLengthCm>0.0 &&
        Finite(s.MaximumTimeToContactSeconds) &&
        s.MaximumTimeToContactSeconds>0.0 &&
        Finite(s.RateLimitedThrottleRecoveryPerSecond) &&
        s.RateLimitedThrottleRecoveryPerSecond>0.0 &&
        Finite(s.BrakeReleasePerSecond) &&
        s.BrakeReleasePerSecond>0.0;
}

inline bool ValidFollowingActor(
    FollowingActor a,FollowingParameters s) noexcept
{
    return PhysicalActorIdentityValid(a.Identity) &&
        (!s.RequireActualRigidBodies ||
         (a.OriginalCollisionBody && a.ContactPhysicsReady)) &&
        (!s.RequireVerifiedRoad || a.OnOriginalAsphalt) &&
        Finite(a.Pose) &&
        Finite(a.VelocityCmS) &&
        Finite(a.AccelerationCmS2) &&
        Finite(a.HalfLengthCm) &&
        Finite(a.HalfWidthCm) &&
        Finite(a.GripG) &&
        Finite(a.TrackDistanceCm) &&
        Finite(a.LateralDeviationCm) &&
        a.HalfLengthCm>0.0 &&
        a.HalfLengthCm<500.0 &&
        a.HalfWidthCm>0.0 &&
        a.HalfWidthCm<250.0 &&
        a.GripG>=s.MinimumEstimatedGripG &&
        a.GripG<=s.MaximumEstimatedGripG &&
        Length(a.VelocityCmS)<=MaximumPhysicalSpeedCmS;
}

inline bool ValidFollowingObservation(
    const FollowingObservation& input,
    FollowingParameters s) noexcept
{
    if (!input.EnginePhysicsQualified ||
        !input.RaceRunning ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        !Finite(input.ProposedThrottle) ||
        !Finite(input.ProposedBrake) ||
        input.ProposedThrottle<0.0 ||
        input.ProposedThrottle>1.0 ||
        input.ProposedBrake<0.0 ||
        input.ProposedBrake>1.0 ||
        input.OpponentCount<1 ||
        input.OpponentCount>5 ||
        !ValidFollowingActor(input.Self,s) ||
        input.Self.Identity.Slot==0)
        return false;
    for (int i=0;i<input.OpponentCount;++i)
    {
        const auto& opponent=input.Opponents[
            static_cast<std::size_t>(i)];
        if (!ValidFollowingActor(opponent,s) ||
            opponent.Identity.Actor==input.Self.Identity.Actor ||
            opponent.Identity.Slot==input.Self.Identity.Slot ||
            (s.RequireSameSession &&
             opponent.Identity.Session!=input.Self.Identity.Session))
            return false;
        for (int j=0;j<i;++j)
        {
            const auto& earlier=input.Opponents[
                static_cast<std::size_t>(j)];
            if (earlier.Identity.Actor==opponent.Identity.Actor ||
                earlier.Identity.Slot==opponent.Identity.Slot)
                return false;
        }
    }
    return true;
}

inline FollowingThreat EvaluateFollowingThreat(
    FollowingActor self,FollowingActor other,
    FollowingParameters p) noexcept
{
    FollowingThreat out;
    if (!ValidFollowingActor(self,p) ||
        !ValidFollowingActor(other,p) ||
        self.Identity.Actor==other.Identity.Actor ||
        self.Identity.Slot==other.Identity.Slot)
        return out;
    const Vec2 longitudinal=
        HeadingVector(self.Pose.HeadingRadians);
    const Vec2 relativePosition=
        other.Pose.Position-self.Pose.Position;
    const Vec2 relativeVelocity=
        other.VelocityCmS-self.VelocityCmS;
    const double along=Dot(
        relativePosition,longitudinal);
    const double lateral=Cross(
        longitudinal,relativePosition);
    const double closing=-
        Dot(relativeVelocity,longitudinal);
    const double bodyGap=along-
        self.HalfLengthCm-other.HalfLengthCm;
    out.Slot=other.Identity.Slot;
    out.Actor=other.Identity.Actor;
    out.LongitudinalGapCm=bodyGap;
    out.LateralSeparationCm=lateral;
    out.RelativeForwardSpeedCmS = -closing;
    out.ClosingSpeedCmS=std::max(0.0,closing);
    out.SameLane=std::abs(lateral)<
        p.MaximumLateralInteractionCm+
        self.HalfWidthCm+other.HalfWidthCm;
    out.PhysicallyClosing=closing>
        p.MinimumApproachSpeedCmS;
    if (along<-p.RearwardIgnoreDistanceCm ||
        along>p.MaximumInteractionDistanceCm ||
        !out.SameLane)
    {
        out.Valid=true;
        return out;
    }
    const double speed=std::max(
        0.0,ForwardSpeed(
            self.VelocityCmS,self.Pose.HeadingRadians));
    out.DesiredGapCm=
        p.MinimumStandstillGapCm+
        speed*p.DesiredTimeHeadwaySeconds+
        SafeDivision(
            speed*std::max(0.0,closing),
            2.0*std::sqrt(
                p.ComfortableAccelerationCmS2*
                p.ComfortableDecelerationCmS2));
    const double deceleration=
        std::min(p.EmergencyDecelerationCmS2,
            self.GripG*GravityCmS2);
    out.StoppingDistanceCm=StoppingEnvelope(
        speed,std::max(0.0,speed-closing),
        deceleration,p.MaximumReactionSeconds).DistanceCm+
        p.StoppingSafetyMarginCm;
    out.TimeToContactSeconds=
        closing>PhysicsEpsilon ?
        std::max(0.0,bodyGap/closing) :
        std::numeric_limits<double>::infinity();
    out.RequiredDecelerationCmS2=
        closing>0.0 ?
        SafeDivision(Square(closing),
            2.0*std::max(1.0,bodyGap-p.StoppingSafetyMarginCm)):
        0.0;
    const double gapFraction=Clamp01(
        SafeDivision(bodyGap,out.DesiredGapCm));
    const double closeness=1.0-gapFraction;
    const double brakeRisk=Clamp01(
        SafeDivision(out.RequiredDecelerationCmS2,
                     deceleration));
    out.Severity=std::max(closeness,brakeRisk);
    out.ThrottleCeiling=std::clamp(
        1.0-0.9*out.Severity,0.0,1.0);
    out.BrakeFloor=Clamp01(
        std::max(0.0,brakeRisk-0.25)*1.2);
    out.StopImmediately=
        bodyGap<=0.0 ||
        (out.PhysicallyClosing &&
         out.StoppingDistanceCm>bodyGap);
    if (out.StopImmediately)
    {
        out.ThrottleCeiling=0.0;
        out.BrakeFloor=1.0;
    }
    out.Valid=Finite(out.LongitudinalGapCm) &&
        Finite(out.Severity) &&
        Finite(out.BrakeFloor);
    return out;
}

inline FollowingDecision RestrictPhysicalCarFollowing(
    const FollowingObservation& input,
    FollowingParameters params={}) noexcept
{
    FollowingDecision out;
    if (!ValidFollowingParameters(params) ||
        !ValidFollowingObservation(input,params))
        return out;
    out.ThrottleCeiling=1.0;
    out.BrakeFloor=0.0;
    out.NearestLongitudinalGapCm=
        std::numeric_limits<double>::infinity();
    out.MinimumTimeToContactSeconds=
        std::numeric_limits<double>::infinity();
    double severity=-1.0;
    for (int i=0;i<input.OpponentCount;++i)
    {
        const FollowingThreat threat=EvaluateFollowingThreat(
            input.Self,input.Opponents[
                static_cast<std::size_t>(i)],params);
        if (!threat.Valid)return {};
        ++out.VerifiedOpponents;
        if (!threat.SameLane)continue;
        ++out.SameLaneOpponents;
        if (threat.PhysicallyClosing)++out.ClosingOpponents;
        out.MaximumClosingSpeedCmS=std::max(
            out.MaximumClosingSpeedCmS,threat.ClosingSpeedCmS);
        out.NearestLongitudinalGapCm=std::min(
            out.NearestLongitudinalGapCm,
            threat.LongitudinalGapCm);
        out.MinimumTimeToContactSeconds=std::min(
            out.MinimumTimeToContactSeconds,
            threat.TimeToContactSeconds);
        out.ThrottleCeiling=std::min(
            out.ThrottleCeiling,threat.ThrottleCeiling);
        out.BrakeFloor=std::max(
            out.BrakeFloor,threat.BrakeFloor);
        if (threat.Severity>severity ||
            (threat.Severity==severity &&
             threat.Slot<out.Limiting.Slot))
        {
            severity=threat.Severity;
            out.Limiting=threat;
        }
        if (threat.StopImmediately)
            out.EmergencyBrake=true;
    }
    out.ProposedThrottle=std::min(
        input.ProposedThrottle,out.ThrottleCeiling);
    out.ProposedBrake=std::max(
        input.ProposedBrake,out.BrakeFloor);
    if (out.EmergencyBrake)
    {
        out.ProposedThrottle=0.0;
        out.ProposedBrake=1.0;
    }
    if (out.ProposedBrake>0.05)
        out.ProposedThrottle=0.0;
    out.RequestedSpacingCm=
        out.Limiting.DesiredGapCm;
    out.DirectVehicleCommandIssued=false;
    out.NoActorWasInvented=true;
    out.Valid=true;
    return out;
}

class PhysicalFollowingSupervisor final
{
public:
    bool Configure(FollowingParameters settings) noexcept
    {
        if (!ValidFollowingParameters(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Previous={};
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        HoldSeconds=0.0;
        Session=0;
        Actor=0;
        Accepted=0;
        Rejected=0;
        EmergencyFrames=0;
        RosterChangeFrames=0;
        MaxClosingSpeedCmS=0.0;
        HasRoster=false;
        Roster.fill(0);
    }
    FollowingDecision Step(
        const FollowingObservation& input) noexcept
    {
        FollowingDecision decision=
            RestrictPhysicalCarFollowing(input,Settings);
        if (!decision.Valid)
        {
            ++Rejected;
            HasRoster=false;
            return {};
        }
        std::array<std::uint64_t,PhysicalFleetCapacity> now{};
        now[0]=input.Self.Identity.Actor;
        for (int i=0;i<input.OpponentCount;++i)
        {
            const auto& other=input.Opponents[
                static_cast<std::size_t>(i)];
            now[static_cast<std::size_t>(other.Identity.Slot)]=
                other.Identity.Actor;
        }
        if (HasRoster && now!=Roster)
        {
            HoldSeconds=Settings.RecoveryHazardHoldSeconds;
            ++RosterChangeFrames;
        }
        if (HasRoster &&
            (Session!=input.Self.Identity.Session ||
             Actor!=input.Self.Identity.Actor))
            HoldSeconds=Settings.RecoveryHazardHoldSeconds;
        Roster=now;
        HasRoster=true;
        Session=input.Self.Identity.Session;
        Actor=input.Self.Identity.Actor;
        if (decision.EmergencyBrake)
            HoldSeconds=std::max(
                HoldSeconds,Settings.RecoveryHazardHoldSeconds);
        else
            HoldSeconds=std::max(0.0,
                HoldSeconds-input.DeltaSeconds);
        if (HoldSeconds>0.0)
        {
            decision.ThrottleCeiling=0.0;
            decision.BrakeFloor=1.0;
            decision.ProposedThrottle=0.0;
            decision.ProposedBrake=1.0;
            decision.HoldingForRosterChange=true;
        }
        else
        {
            decision.ProposedThrottle=RateLimited(
                PreviousThrottle,decision.ProposedThrottle,
                Settings.RateLimitedThrottleRecoveryPerSecond,
                1000.0,input.DeltaSeconds);
            decision.ProposedBrake=RateLimited(
                PreviousBrake,decision.ProposedBrake,
                1000.0,Settings.BrakeReleasePerSecond,
                input.DeltaSeconds);
            if (decision.ProposedBrake>0.05)
                decision.ProposedThrottle=0.0;
        }
        PreviousThrottle=decision.ProposedThrottle;
        PreviousBrake=decision.ProposedBrake;
        if (decision.EmergencyBrake)++EmergencyFrames;
        MaxClosingSpeedCmS=std::max(
            MaxClosingSpeedCmS,decision.MaximumClosingSpeedCmS);
        ++Accepted;
        Previous=decision;
        return Previous;
    }
    FollowingDecision Snapshot() const noexcept
    {return Previous;}
    std::uint64_t QualifiedFrames() const noexcept
    {return Accepted;}
    std::uint64_t EmergencyBrakeFrames() const noexcept
    {return EmergencyFrames;}
    std::uint64_t RejectedFrames() const noexcept
    {return Rejected;}
    double MaximumObservedClosingSpeedCmS() const noexcept
    {return MaxClosingSpeedCmS;}
private:
    FollowingParameters Settings{};
    FollowingDecision Previous{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t EmergencyFrames=0;
    std::uint64_t RosterChangeFrames=0;
    double HoldSeconds=0.0;
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    double MaxClosingSpeedCmS=0.0;
    bool HasRoster=false;
};
} // namespace latebrake::racecraft
