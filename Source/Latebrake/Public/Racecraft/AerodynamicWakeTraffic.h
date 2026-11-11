#pragma once
#include "Racecraft/VehicleAerodynamics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// A measured, source-only aerodynamic wake; it NEVER applies Chaos forces.
// The roster must come from existing physical actors in the original session.
struct WakeParameters final
{
    double MinimumLeaderSpeedCmS=650.0;
    double MaximumVehicleSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumRangeCm=5500.0;
    double MinimumFollowingClearanceCm=90.0;
    double WakeLengthCm=2000.0;
    double WakeHalfWidthAtSourceCm=150.0;
    double WakeExpansionPerCm=0.065;
    double MaximumWakeHalfWidthCm=520.0;
    double MaximumHeadingDifferenceRadians=0.60;
    double MaximumCrosswindCmS=2800.0;
    double WindDriftScale=0.35;
    double MaximumDragReliefFraction=0.29;
    double MaximumFrontDownforceLoss=0.21;
    double MaximumRearDownforceLoss=0.14;
    double MaximumBrakeCoolingLoss=0.16;
    double MaximumEngineCoolingLoss=0.12;
    double MaximumTireCoolingLoss=0.17;
    double TurbulenceLateralGain=0.045;
    double WakeBuildSeconds=0.12;
    double WakeDecaySeconds=0.24;
    double MaximumFrameDeltaSeconds=0.10;
    double MinimumRoadGripG=0.15;
    double MaximumRoadGripG=2.5;
    double MinimumSourceCertainty=0.60;
    bool RequireOriginalRoad=true;
    bool RequireSimulatingBodies=true;
    bool RequireSameSession=true;
};

struct WakeActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    Vec2 WindVelocityCmS{};
    double HalfLengthCm=220.0;
    double HalfWidthCm=100.0;
    double GripG=1.0;
    double SourceCertainty=1.0;
    double ClockSeconds=0.0;
    bool OriginalAsphaltQualified=false;
    bool ContactPhysicsQualified=false;
};

struct WakeFleetObservation final
{
    WakeActor FollowingCar{};
    std::array<WakeActor,PhysicalFleetCapacity-1> OtherCars{};
    int OtherCount=0;
    double DeltaSeconds=0.0;
    bool EngineRaceRunning=false;
    bool OriginalMapConfirmed=false;
    bool CompletePhysicalRoster=false;
};

struct WakeContribution final
{
    int LeaderSlot=-1;
    std::uint64_t LeaderActor=0;
    double LongitudinalSeparationCm=0.0;
    double SideSeparationCm=0.0;
    double WakeHalfWidthCm=0.0;
    double LeaderSpeedCmS=0.0;
    double HeadingDifferenceRadians=0.0;
    double TailwindDriftCm=0.0;
    double LongitudinalAttenuation=0.0;
    double LateralAttenuation=0.0;
    double RelativeWindFactor=0.0;
    double Strength=0.0;
    double DragReliefFraction=0.0;
    double FrontDownforceLoss=0.0;
    double RearDownforceLoss=0.0;
    double BrakeCoolingLoss=0.0;
    double EngineCoolingLoss=0.0;
    double TireCoolingLoss=0.0;
    double PredictedSideForceFraction=0.0;
    bool LocatedBehindLeader=false;
    bool Qualified=false;
};

struct WakeSnapshot final
{
    std::array<WakeContribution,PhysicalFleetCapacity-1> Contributions{};
    double CombinedWakeFraction=0.0;
    double DragReliefFraction=0.0;
    double FrontDownforceLoss=0.0;
    double RearDownforceLoss=0.0;
    double BrakeCoolingLoss=0.0;
    double EngineCoolingLoss=0.0;
    double TireCoolingLoss=0.0;
    double PredictedSideForceFraction=0.0;
    double ClosestLeaderCm=std::numeric_limits<double>::infinity();
    int StrongestLeaderSlot=-1;
    int QualifiedLeaders=0;
    int WakeLeaders=0;
    bool ProvenOriginalRoster=false;
    bool HasUsableWake=false;
    bool AppliedToChaos=false;
    bool Valid=false;
};

inline bool ValidWakeParameters(WakeParameters s) noexcept
{
    return Finite(s.MinimumLeaderSpeedCmS) &&
        s.MinimumLeaderSpeedCmS>=0.0 &&
        Finite(s.MaximumVehicleSpeedCmS) &&
        s.MaximumVehicleSpeedCmS>s.MinimumLeaderSpeedCmS &&
        s.MaximumVehicleSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(s.MaximumRangeCm) && s.MaximumRangeCm>500.0 &&
        Finite(s.MinimumFollowingClearanceCm) &&
        s.MinimumFollowingClearanceCm>=0.0 &&
        s.MinimumFollowingClearanceCm<s.MaximumRangeCm &&
        Finite(s.WakeLengthCm) && s.WakeLengthCm>0.0 &&
        Finite(s.WakeHalfWidthAtSourceCm) &&
        s.WakeHalfWidthAtSourceCm>0.0 &&
        Finite(s.WakeExpansionPerCm) &&
        s.WakeExpansionPerCm>=0.0 &&
        Finite(s.MaximumWakeHalfWidthCm) &&
        s.MaximumWakeHalfWidthCm>=s.WakeHalfWidthAtSourceCm &&
        Finite(s.MaximumHeadingDifferenceRadians) &&
        s.MaximumHeadingDifferenceRadians>0.0 &&
        s.MaximumHeadingDifferenceRadians<=Pi &&
        Finite(s.MaximumCrosswindCmS) &&
        s.MaximumCrosswindCmS>=0.0 &&
        Finite(s.WindDriftScale) &&
        s.WindDriftScale>=0.0 && s.WindDriftScale<=1.0 &&
        Finite(s.MaximumDragReliefFraction) &&
        s.MaximumDragReliefFraction>=0.0 &&
        s.MaximumDragReliefFraction<1.0 &&
        Finite(s.MaximumFrontDownforceLoss) &&
        s.MaximumFrontDownforceLoss>=0.0 &&
        s.MaximumFrontDownforceLoss<1.0 &&
        Finite(s.MaximumRearDownforceLoss) &&
        s.MaximumRearDownforceLoss>=0.0 &&
        s.MaximumRearDownforceLoss<1.0 &&
        Finite(s.MaximumBrakeCoolingLoss) &&
        s.MaximumBrakeCoolingLoss>=0.0 &&
        s.MaximumBrakeCoolingLoss<1.0 &&
        Finite(s.MaximumEngineCoolingLoss) &&
        s.MaximumEngineCoolingLoss>=0.0 &&
        s.MaximumEngineCoolingLoss<1.0 &&
        Finite(s.MaximumTireCoolingLoss) &&
        s.MaximumTireCoolingLoss>=0.0 &&
        s.MaximumTireCoolingLoss<1.0 &&
        Finite(s.TurbulenceLateralGain) &&
        s.TurbulenceLateralGain>=0.0 &&
        Finite(s.WakeBuildSeconds) &&
        s.WakeBuildSeconds>0.0 &&
        Finite(s.WakeDecaySeconds) &&
        s.WakeDecaySeconds>0.0 &&
        Finite(s.MaximumFrameDeltaSeconds) &&
        s.MaximumFrameDeltaSeconds>0.0 &&
        s.MaximumFrameDeltaSeconds<=MaximumFrameTimeSeconds &&
        Finite(s.MinimumRoadGripG) && s.MinimumRoadGripG>0.0 &&
        Finite(s.MaximumRoadGripG) &&
        s.MaximumRoadGripG>=s.MinimumRoadGripG &&
        Finite(s.MinimumSourceCertainty) &&
        s.MinimumSourceCertainty>0.0 &&
        s.MinimumSourceCertainty<=1.0;
}

inline bool ValidWakeActor(WakeActor actor,WakeParameters p) noexcept
{
    return PhysicalActorIdentityValid(actor.Identity) &&
        (!p.RequireOriginalRoad || actor.OriginalAsphaltQualified) &&
        (!p.RequireSimulatingBodies || actor.ContactPhysicsQualified) &&
        Finite(actor.Pose) && Finite(actor.VelocityCmS) &&
        Finite(actor.WindVelocityCmS) &&
        Finite(actor.HalfLengthCm) &&
        actor.HalfLengthCm>50.0 && actor.HalfLengthCm<500.0 &&
        Finite(actor.HalfWidthCm) &&
        actor.HalfWidthCm>20.0 && actor.HalfWidthCm<250.0 &&
        Finite(actor.GripG) &&
        actor.GripG>=p.MinimumRoadGripG &&
        actor.GripG<=p.MaximumRoadGripG &&
        Finite(actor.SourceCertainty) &&
        actor.SourceCertainty>=p.MinimumSourceCertainty &&
        actor.SourceCertainty<=1.0 &&
        Finite(actor.ClockSeconds) && actor.ClockSeconds>=0.0 &&
        Length(actor.VelocityCmS)<=p.MaximumVehicleSpeedCmS &&
        Length(actor.WindVelocityCmS)<=p.MaximumCrosswindCmS;
}

inline bool ValidateWakeFleet(
    const WakeFleetObservation& input,WakeParameters p) noexcept
{
    if (!ValidWakeParameters(p) ||
        !input.EngineRaceRunning ||
        !input.OriginalMapConfirmed ||
        !input.CompletePhysicalRoster ||
        !Finite(input.DeltaSeconds) ||
        input.DeltaSeconds<=0.0 ||
        input.DeltaSeconds>p.MaximumFrameDeltaSeconds ||
        input.OtherCount!=PhysicalFleetCapacity-1 ||
        !ValidWakeActor(input.FollowingCar,p))
        return false;
    std::array<std::uint64_t,PhysicalFleetCapacity> actors{};
    actors[static_cast<std::size_t>(
        input.FollowingCar.Identity.Slot)]=
        input.FollowingCar.Identity.Actor;
    for (int i=0;i<input.OtherCount;++i)
    {
        const WakeActor& leader=input.OtherCars[
            static_cast<std::size_t>(i)];
        if (!ValidWakeActor(leader,p) ||
            (p.RequireSameSession &&
             leader.Identity.Session!=
                 input.FollowingCar.Identity.Session) ||
            leader.Identity.SessionRevision!=
                input.FollowingCar.Identity.SessionRevision ||
            std::abs(leader.ClockSeconds-
                     input.FollowingCar.ClockSeconds)>
                p.MaximumFrameDeltaSeconds ||
            leader.Identity.Frame!=input.FollowingCar.Identity.Frame ||
            actors[static_cast<std::size_t>(leader.Identity.Slot)]!=0)
            return false;
        for (int prior=0;prior<i;++prior)
        {
            if (input.OtherCars[
                  static_cast<std::size_t>(prior)].Identity.Actor==
                leader.Identity.Actor)
                return false;
        }
        if (leader.Identity.Actor==
            input.FollowingCar.Identity.Actor)
            return false;
        actors[static_cast<std::size_t>(leader.Identity.Slot)]=
            leader.Identity.Actor;
    }
    return std::all_of(actors.begin(),actors.end(),
        [](std::uint64_t actor){return actor!=0;});
}

inline WakeContribution CalculateWakeContribution(
    WakeActor follower,WakeActor leader,WakeParameters p) noexcept
{
    WakeContribution out;
    if (!ValidWakeParameters(p) ||
        !ValidWakeActor(follower,p) ||
        !ValidWakeActor(leader,p) ||
        follower.Identity.Actor==leader.Identity.Actor ||
        follower.Identity.Slot==leader.Identity.Slot ||
        (p.RequireSameSession &&
         follower.Identity.Session!=leader.Identity.Session))
        return out;
    out.Qualified=true;
    out.LeaderSlot=leader.Identity.Slot;
    out.LeaderActor=leader.Identity.Actor;
    const Vec2 forward=HeadingVector(leader.Pose.HeadingRadians);
    const Vec2 right=PerpendicularRight(forward);
    const Vec2 separation=follower.Pose.Position-
                          leader.Pose.Position;
    const double behind=-Dot(separation,forward);
    const double offset=Dot(separation,right);
    const double halfBodies=follower.HalfLengthCm+
                            leader.HalfLengthCm;
    out.LongitudinalSeparationCm=behind-halfBodies;
    out.SideSeparationCm=offset;
    out.LeaderSpeedCmS=std::max(0.0,
        Dot(leader.VelocityCmS,forward));
    out.HeadingDifferenceRadians=std::abs(
        AngleDifference(follower.Pose.HeadingRadians,
                        leader.Pose.HeadingRadians));
    if (behind<=halfBodies+p.MinimumFollowingClearanceCm ||
        behind>=p.MaximumRangeCm ||
        out.LeaderSpeedCmS<p.MinimumLeaderSpeedCmS ||
        out.HeadingDifferenceRadians>
            p.MaximumHeadingDifferenceRadians)
        return out;
    out.LocatedBehindLeader=true;
    const double length=behind-halfBodies;
    out.WakeHalfWidthCm=std::min(
        p.MaximumWakeHalfWidthCm,
        p.WakeHalfWidthAtSourceCm+
            p.WakeExpansionPerCm*length);
    const Vec2 relativeWind=leader.WindVelocityCmS;
    const double crosswind=Dot(relativeWind,right);
    out.TailwindDriftCm=std::clamp(
        crosswind*p.WindDriftScale*
            SafeDivision(length,
                std::max(p.MinimumLeaderSpeedCmS,
                         out.LeaderSpeedCmS)),
        -p.MaximumWakeHalfWidthCm,p.MaximumWakeHalfWidthCm);
    const double lateral=offset-out.TailwindDriftCm;
    const double lateralRatio=SafeDivision(
        lateral,out.WakeHalfWidthCm);
    out.LongitudinalAttenuation=std::exp(
        -length/p.WakeLengthCm);
    out.LateralAttenuation=std::exp(
        -1.5*lateralRatio*lateralRatio);
    const double alignment=std::max(
        0.0,std::cos(out.HeadingDifferenceRadians));
    const double velocityGain=std::clamp(
        (out.LeaderSpeedCmS-p.MinimumLeaderSpeedCmS)/
        std::max(1.0,p.MaximumVehicleSpeedCmS-
                   p.MinimumLeaderSpeedCmS),0.0,1.0);
    out.RelativeWindFactor=std::sqrt(velocityGain);
    out.Strength=Clamp01(
        out.LongitudinalAttenuation*
        out.LateralAttenuation*
        alignment*out.RelativeWindFactor*
        leader.SourceCertainty*follower.SourceCertainty);
    out.DragReliefFraction=
        p.MaximumDragReliefFraction*out.Strength;
    out.FrontDownforceLoss=
        p.MaximumFrontDownforceLoss*out.Strength;
    out.RearDownforceLoss=
        p.MaximumRearDownforceLoss*out.Strength;
    out.BrakeCoolingLoss=
        p.MaximumBrakeCoolingLoss*out.Strength;
    out.EngineCoolingLoss=
        p.MaximumEngineCoolingLoss*out.Strength;
    out.TireCoolingLoss=
        p.MaximumTireCoolingLoss*out.Strength;
    out.PredictedSideForceFraction=
        p.TurbulenceLateralGain*out.Strength*
        std::clamp(lateralRatio,-1.0,1.0);
    if (!Finite(out.Strength) ||
        !Finite(out.PredictedSideForceFraction))
        return {};
    return out;
}

inline WakeSnapshot EvaluateAerodynamicWake(
    const WakeFleetObservation& input,
    WakeParameters p={}) noexcept
{
    WakeSnapshot out;
    if (!ValidateWakeFleet(input,p))
        return out;
    out.ProvenOriginalRoster=true;
    double complement=1.0;
    double best=-1.0;
    for (int i=0;i<input.OtherCount;++i)
    {
        const WakeContribution sample=CalculateWakeContribution(
            input.FollowingCar,input.OtherCars[
                static_cast<std::size_t>(i)],p);
        if (!sample.Qualified)
            return {};
        out.Contributions[static_cast<std::size_t>(i)]=sample;
        ++out.QualifiedLeaders;
        if (sample.Strength<=0.0)
            continue;
        ++out.WakeLeaders;
        complement*=1.0-sample.Strength;
        out.ClosestLeaderCm=std::min(
            out.ClosestLeaderCm,sample.LongitudinalSeparationCm);
        if (sample.Strength>best ||
            (sample.Strength==best &&
             sample.LeaderSlot<out.StrongestLeaderSlot))
        {
            best=sample.Strength;
            out.StrongestLeaderSlot=sample.LeaderSlot;
        }
        out.PredictedSideForceFraction+=
            sample.PredictedSideForceFraction;
    }
    out.CombinedWakeFraction=Clamp01(1.0-complement);
    out.DragReliefFraction=
        p.MaximumDragReliefFraction*out.CombinedWakeFraction;
    out.FrontDownforceLoss=
        p.MaximumFrontDownforceLoss*out.CombinedWakeFraction;
    out.RearDownforceLoss=
        p.MaximumRearDownforceLoss*out.CombinedWakeFraction;
    out.BrakeCoolingLoss=
        p.MaximumBrakeCoolingLoss*out.CombinedWakeFraction;
    out.EngineCoolingLoss=
        p.MaximumEngineCoolingLoss*out.CombinedWakeFraction;
    out.TireCoolingLoss=
        p.MaximumTireCoolingLoss*out.CombinedWakeFraction;
    out.PredictedSideForceFraction=std::clamp(
        out.PredictedSideForceFraction,
        -p.TurbulenceLateralGain,p.TurbulenceLateralGain);
    out.HasUsableWake=out.CombinedWakeFraction>PhysicsEpsilon;
    out.Valid=Finite(out.CombinedWakeFraction) &&
        Finite(out.DragReliefFraction) &&
        Finite(out.PredictedSideForceFraction);
    return out;
}

inline WakeSnapshot ScaleWake(
    WakeSnapshot sample,double progress) noexcept
{
    if (!sample.Valid || !Finite(progress))
        return {};
    const double factor=Clamp01(progress);
    sample.CombinedWakeFraction*=factor;
    sample.DragReliefFraction*=factor;
    sample.FrontDownforceLoss*=factor;
    sample.RearDownforceLoss*=factor;
    sample.BrakeCoolingLoss*=factor;
    sample.EngineCoolingLoss*=factor;
    sample.TireCoolingLoss*=factor;
    sample.PredictedSideForceFraction*=factor;
    sample.HasUsableWake=
        sample.CombinedWakeFraction>PhysicsEpsilon;
    return sample;
}

struct WakeAeroAdjustment final
{
    AeroForces Base{};
    AeroForces Adjusted{};
    double DragRemovedN=0.0;
    double DownforceRemovedN=0.0;
    double CoolingRateRemoved=0.0;
    bool SourceOnly=true;
    bool AppliedToChaos=false;
    bool Valid=false;
};

inline WakeAeroAdjustment ApplyWakeToModeledAero(
    AeroForces freeAir,WakeSnapshot wake) noexcept
{
    WakeAeroAdjustment out;
    if (!freeAir.Valid || !wake.Valid ||
        !Finite(freeAir.DragForceWorldN) ||
        !Finite(freeAir.TotalDownforceN) ||
        !Finite(freeAir.BrakeCoolingCoefficient) ||
        !Finite(freeAir.EngineCoolingCoefficient) ||
        !Finite(freeAir.TireCoolingCoefficient))
        return out;
    out.Base=freeAir;
    out.Adjusted=freeAir;
    const double dragScale=1.0-
        Clamp01(wake.DragReliefFraction);
    const double frontScale=1.0-
        Clamp01(wake.FrontDownforceLoss);
    const double rearScale=1.0-
        Clamp01(wake.RearDownforceLoss);
    out.Adjusted.DragForceWorldN=
        freeAir.DragForceWorldN*dragScale;
    out.Adjusted.LongitudinalDragN*=
        dragScale;
    out.Adjusted.LateralDragN*=
        dragScale;
    out.Adjusted.EffectiveDragCoefficient*=
        dragScale;
    out.Adjusted.AerodynamicPowerLossKw*=
        dragScale;
    out.Adjusted.FrontDownforceN*=
        frontScale;
    out.Adjusted.RearDownforceN*=
        rearScale;
    out.Adjusted.TotalDownforceN=
        out.Adjusted.FrontDownforceN+
        out.Adjusted.RearDownforceN;
    out.Adjusted.AeroBalanceFrontFraction=SafeDivision(
        out.Adjusted.FrontDownforceN,
        out.Adjusted.TotalDownforceN);
    out.Adjusted.BrakeCoolingCoefficient*=
        1.0-Clamp01(wake.BrakeCoolingLoss);
    out.Adjusted.EngineCoolingCoefficient*=
        1.0-Clamp01(wake.EngineCoolingLoss);
    out.Adjusted.TireCoolingCoefficient*=
        1.0-Clamp01(wake.TireCoolingLoss);
    out.DragRemovedN=std::max(0.0,
        Length(freeAir.DragForceWorldN)-
        Length(out.Adjusted.DragForceWorldN));
    out.DownforceRemovedN=std::max(0.0,
        freeAir.TotalDownforceN-
        out.Adjusted.TotalDownforceN);
    out.CoolingRateRemoved=
        freeAir.BrakeCoolingCoefficient-
            out.Adjusted.BrakeCoolingCoefficient+
        freeAir.EngineCoolingCoefficient-
            out.Adjusted.EngineCoolingCoefficient+
        freeAir.TireCoolingCoefficient-
            out.Adjusted.TireCoolingCoefficient;
    out.Adjusted.AuthoritativeChaosForce=false;
    out.Valid=Finite(out.DragRemovedN) &&
        Finite(out.DownforceRemovedN) &&
        Finite(out.Adjusted.DragForceWorldN) &&
        Finite(out.Adjusted.TotalDownforceN);
    return out;
}

class AerodynamicWakeSupervisor final
{
public:
    bool Configure(WakeParameters settings) noexcept
    {
        if (!ValidWakeParameters(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        SmoothStrength=0.0;
        SourceSession=0;
        SourceActor=0;
        LastFrame=0;
        Roster.fill(0);
        HasRoster=false;
        QualifiedFrames=0;
        RejectedFrames=0;
        RosterResetFrames=0;
        PeakWake=0.0;
        DraftingSeconds=0.0;
    }
    WakeSnapshot Step(
        const WakeFleetObservation& sample) noexcept
    {
        const WakeSnapshot target=
            EvaluateAerodynamicWake(sample,Settings);
        if (!target.Valid)
        {
            ++RejectedFrames;
            HasRoster=false;
            SmoothStrength=0.0;
            Last={};
            return {};
        }
        std::array<std::uint64_t,PhysicalFleetCapacity> now{};
        now[static_cast<std::size_t>(
            sample.FollowingCar.Identity.Slot)]=
            sample.FollowingCar.Identity.Actor;
        for (int i=0;i<sample.OtherCount;++i)
        {
            const WakeActor& observed=sample.OtherCars[
                static_cast<std::size_t>(i)];
            now[static_cast<std::size_t>(
                observed.Identity.Slot)]=
                observed.Identity.Actor;
        }
        const bool changed=HasRoster &&
            (now!=Roster ||
             SourceSession!=sample.FollowingCar.Identity.Session ||
             SourceActor!=sample.FollowingCar.Identity.Actor ||
             sample.FollowingCar.Identity.Frame<=LastFrame);
        if (changed)
        {
            ++RosterResetFrames;
            SmoothStrength=0.0;
        }
        Roster=now;
        HasRoster=true;
        SourceSession=sample.FollowingCar.Identity.Session;
        SourceActor=sample.FollowingCar.Identity.Actor;
        LastFrame=sample.FollowingCar.Identity.Frame;
        const double tau=target.CombinedWakeFraction>SmoothStrength ?
            Settings.WakeBuildSeconds:Settings.WakeDecaySeconds;
        SmoothStrength=ExponentialResponse(
            SmoothStrength,target.CombinedWakeFraction,
            tau,sample.DeltaSeconds);
        Last=ScaleWake(target,SafeDivision(
            SmoothStrength,target.CombinedWakeFraction));
        // On departure, the target has zero wake: aerodynamic benefit
        // must be removed immediately; no fictitious persistent draft.
        if (!target.HasUsableWake)
        {
            Last=target;
            SmoothStrength=0.0;
        }
        ++QualifiedFrames;
        PeakWake=std::max(PeakWake,
                          Last.CombinedWakeFraction);
        if (Last.HasUsableWake)
            DraftingSeconds+=sample.DeltaSeconds;
        return Last;
    }
    WakeSnapshot Snapshot() const noexcept {return Last;}
    std::uint64_t Frames() const noexcept {return QualifiedFrames;}
    std::uint64_t Rejects() const noexcept {return RejectedFrames;}
    std::uint64_t RosterResets() const noexcept
    {return RosterResetFrames;}
    double PeakDraftFraction() const noexcept {return PeakWake;}
    double TimeInWakeSeconds() const noexcept
    {return DraftingSeconds;}
private:
    WakeParameters Settings{};
    WakeSnapshot Last{};
    std::array<std::uint64_t,PhysicalFleetCapacity> Roster{};
    std::uint64_t SourceSession=0;
    std::uint64_t SourceActor=0;
    std::uint64_t LastFrame=0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t RosterResetFrames=0;
    double SmoothStrength=0.0;
    double PeakWake=0.0;
    double DraftingSeconds=0.0;
    bool HasRoster=false;
};
} // namespace latebrake::racecraft
