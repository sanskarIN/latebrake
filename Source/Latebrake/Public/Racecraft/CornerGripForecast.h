#pragma once
#include "Racecraft/AerodynamicWakeTraffic.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int CornerForecastCapacity=16;
struct CornerForecastSettings final
{
    double MinimumTrackGrip=0.20;
    double MaximumTrackGrip=2.5;
    double MaximumPlannerSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumBrakingDecelerationCmS2=1500.0;
    double MaximumAccelerationCmS2=900.0;
    double MinimumBrakingDecelerationCmS2=200.0;
    double BrakingReserveFraction=0.15;
    double GripReserveFraction=0.12;
    double UsableDownforceFraction=0.80;
    double MaximumBankAngleRadians=0.30;
    double MinimumRoadHalfWidthCm=300.0;
    double MinimumSafeRoadEdgeCm=160.0;
    double MaximumCurvaturePerCm=0.01;
    double MinimumSensorLoadN=10.0;
    double MaximumSensorLoadN=60000.0;
    double MinimumTotalNormalLoadN=4000.0;
    double MaximumTotalNormalLoadN=90000.0;
    double MaximumForecastDistanceCm=6500.0;
    double MinimumKnotSpacingCm=10.0;
    double MaximumLateralSlipRatio=0.55;
    double TireSlipGripPenalty=0.30;
    double MaximumWearGripPenalty=0.20;
    double MinimumMeasuredWheelGrip=0.15;
    double MaximumMeasuredWheelGrip=2.5;
    double BrakingThrottleCutStartFraction=0.90;
    double BrakeDemandGain=1.10;
    double BrakeFloorAtOverspeed=0.08;
    double ThrottleRecoveryPerSecond=0.75;
    double BrakeReleasePerSecond=2.5;
    double MaximumFrameSeconds=0.10;
    bool RequireFourPhysicalWheels=true;
    bool RequireOriginalRoad=true;
    bool RequireActualAeroSource=true;
    bool RequireVerifiedRouteSamples=true;
};

struct CornerForecastKnot final
{
    double AlongRouteCm=0.0;
    double CurvaturePerCm=0.0;
    double BankRadians=0.0;
    double MeasuredFrictionScale=1.0;
    double RoadHalfWidthCm=800.0;
    double LateralOffsetCm=0.0;
    double DistanceToBarrierCm=1000.0;
    double LocalSpeedLimitCmS=MaximumPhysicalSpeedCmS;
    bool OriginalAsphaltSample=false;
    bool ActualGeometryQualified=false;
    bool RealFrictionQualified=false;
};

struct CornerForecastObservation final
{
    RaceIdentity Identity{};
    Vec2 VehicleVelocityCmS{};
    double HeadingRadians=0.0;
    double MassKg=1300.0;
    double DeltaSeconds=0.0;
    double ProposedThrottle=0.0;
    double ProposedBrake=0.0;
    std::array<WheelObservation,PhysicalWheelCount> Wheels{};
    AeroForces MeasuredAero{};
    WakeSnapshot Wake{};
    std::array<CornerForecastKnot,CornerForecastCapacity> Route{};
    int KnotCount=0;
    bool EngineRaceGreen=false;
    bool OriginalCircuitConfirmed=false;
    bool ExistingAeroCalculatedFromPhysics=false;
    bool WakeRosterMatchesIdentity=false;
};

struct CornerGripKnotResult final
{
    double AlongRouteCm=0.0;
    double CurvaturePerCm=0.0;
    double EffectiveGrip=0.0;
    double AvailableNormalForceN=0.0;
    double MaximumLateralAccelerationCmS2=0.0;
    double SafeCornerSpeedCmS=0.0;
    double BackwardBrakingSpeedCmS=0.0;
    double BrakingDistanceToNextCm=0.0;
    double RoadMarginCm=0.0;
    double BankingAccelerationCmS2=0.0;
    bool CornerIsLimiting=false;
    bool VerifiedOriginalRoad=false;
};

struct CornerGripDecision final
{
    std::array<CornerGripKnotResult,CornerForecastCapacity> Knots{};
    double MeasuredSpeedCmS=0.0;
    double EffectiveTireGrip=0.0;
    double AvailableBrakingCmS2=0.0;
    double AvailableDownforceN=0.0;
    double WakeDragRelief=0.0;
    double WakeDownforcePenalty=0.0;
    double PlannedEntrySpeedCmS=0.0;
    double MinimumUpcomingCornerSpeedCmS=0.0;
    double ClosestRestrictiveCurveCm=0.0;
    double RequiredDecelerationCmS2=0.0;
    double ThrottleCeiling=0.0;
    double BrakeFloor=1.0;
    double CommandThrottle=0.0;
    double CommandBrake=1.0;
    int KnotCount=0;
    int LimitingKnot=-1;
    bool EmergencySlowdown=false;
    bool DirtyAirGripLoss=false;
    bool EdgesVerified=false;
    bool NoUnrealForceIssued=true;
    bool Valid=false;
};

inline bool ValidCornerForecastSettings(
    CornerForecastSettings p) noexcept
{
    return Finite(p.MinimumTrackGrip) &&
        p.MinimumTrackGrip>0.0 &&
        Finite(p.MaximumTrackGrip) &&
        p.MaximumTrackGrip>=p.MinimumTrackGrip &&
        Finite(p.MaximumPlannerSpeedCmS) &&
        p.MaximumPlannerSpeedCmS>0.0 &&
        p.MaximumPlannerSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MaximumBrakingDecelerationCmS2) &&
        p.MaximumBrakingDecelerationCmS2>0.0 &&
        Finite(p.MinimumBrakingDecelerationCmS2) &&
        p.MinimumBrakingDecelerationCmS2>0.0 &&
        p.MinimumBrakingDecelerationCmS2<=
            p.MaximumBrakingDecelerationCmS2 &&
        Finite(p.MaximumAccelerationCmS2) &&
        p.MaximumAccelerationCmS2>=0.0 &&
        Finite(p.BrakingReserveFraction) &&
        p.BrakingReserveFraction>=0.0 &&
        p.BrakingReserveFraction<0.7 &&
        Finite(p.GripReserveFraction) &&
        p.GripReserveFraction>=0.0 &&
        p.GripReserveFraction<0.7 &&
        Finite(p.UsableDownforceFraction) &&
        p.UsableDownforceFraction>=0.0 &&
        p.UsableDownforceFraction<=1.0 &&
        Finite(p.MaximumBankAngleRadians) &&
        p.MaximumBankAngleRadians>=0.0 &&
        p.MaximumBankAngleRadians<Pi*0.5 &&
        Finite(p.MinimumRoadHalfWidthCm) &&
        p.MinimumRoadHalfWidthCm>0.0 &&
        Finite(p.MinimumSafeRoadEdgeCm) &&
        p.MinimumSafeRoadEdgeCm>=0.0 &&
        Finite(p.MaximumCurvaturePerCm) &&
        p.MaximumCurvaturePerCm>0.0 &&
        Finite(p.MinimumSensorLoadN) &&
        p.MinimumSensorLoadN>0.0 &&
        Finite(p.MaximumSensorLoadN) &&
        p.MaximumSensorLoadN>p.MinimumSensorLoadN &&
        Finite(p.MinimumTotalNormalLoadN) &&
        p.MinimumTotalNormalLoadN>0.0 &&
        Finite(p.MaximumTotalNormalLoadN) &&
        p.MaximumTotalNormalLoadN>p.MinimumTotalNormalLoadN &&
        Finite(p.MaximumForecastDistanceCm) &&
        p.MaximumForecastDistanceCm>0.0 &&
        Finite(p.MinimumKnotSpacingCm) &&
        p.MinimumKnotSpacingCm>0.0 &&
        p.MinimumKnotSpacingCm<p.MaximumForecastDistanceCm &&
        Finite(p.MaximumLateralSlipRatio) &&
        p.MaximumLateralSlipRatio>0.0 &&
        Finite(p.TireSlipGripPenalty) &&
        p.TireSlipGripPenalty>=0.0 &&
        p.TireSlipGripPenalty<1.0 &&
        Finite(p.MaximumWearGripPenalty) &&
        p.MaximumWearGripPenalty>=0.0 &&
        p.MaximumWearGripPenalty<1.0 &&
        Finite(p.MinimumMeasuredWheelGrip) &&
        p.MinimumMeasuredWheelGrip>0.0 &&
        Finite(p.MaximumMeasuredWheelGrip) &&
        p.MaximumMeasuredWheelGrip>=p.MinimumMeasuredWheelGrip &&
        Finite(p.BrakingThrottleCutStartFraction) &&
        p.BrakingThrottleCutStartFraction>0.0 &&
        p.BrakingThrottleCutStartFraction<1.0 &&
        Finite(p.BrakeDemandGain) &&
        p.BrakeDemandGain>0.0 &&
        Finite(p.BrakeFloorAtOverspeed) &&
        p.BrakeFloorAtOverspeed>=0.0 &&
        p.BrakeFloorAtOverspeed<=1.0 &&
        Finite(p.ThrottleRecoveryPerSecond) &&
        p.ThrottleRecoveryPerSecond>0.0 &&
        Finite(p.BrakeReleasePerSecond) &&
        p.BrakeReleasePerSecond>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds;
}

inline bool ValidCornerForecastKnot(
    CornerForecastKnot k,CornerForecastSettings p) noexcept
{
    return (!p.RequireVerifiedRouteSamples ||
            (k.ActualGeometryQualified &&
             k.RealFrictionQualified)) &&
        (!p.RequireOriginalRoad || k.OriginalAsphaltSample) &&
        Finite(k.AlongRouteCm) &&
        k.AlongRouteCm>=0.0 &&
        k.AlongRouteCm<=p.MaximumForecastDistanceCm &&
        Finite(k.CurvaturePerCm) &&
        std::abs(k.CurvaturePerCm)<=
            p.MaximumCurvaturePerCm &&
        Finite(k.BankRadians) &&
        std::abs(k.BankRadians)<=
            p.MaximumBankAngleRadians &&
        Finite(k.MeasuredFrictionScale) &&
        k.MeasuredFrictionScale>=p.MinimumTrackGrip &&
        k.MeasuredFrictionScale<=p.MaximumTrackGrip &&
        Finite(k.RoadHalfWidthCm) &&
        k.RoadHalfWidthCm>=p.MinimumRoadHalfWidthCm &&
        Finite(k.LateralOffsetCm) &&
        std::abs(k.LateralOffsetCm)<k.RoadHalfWidthCm &&
        Finite(k.DistanceToBarrierCm) &&
        k.DistanceToBarrierCm>=0.0 &&
        Finite(k.LocalSpeedLimitCmS) &&
        k.LocalSpeedLimitCmS>0.0 &&
        k.LocalSpeedLimitCmS<=p.MaximumPlannerSpeedCmS;
}

inline bool ValidCornerForecastObservation(
    const CornerForecastObservation& sample,
    CornerForecastSettings p) noexcept
{
    if (!ValidCornerForecastSettings(p) ||
        !PhysicalActorIdentityValid(sample.Identity) ||
        !sample.EngineRaceGreen ||
        !sample.OriginalCircuitConfirmed ||
        !sample.ExistingAeroCalculatedFromPhysics ||
        !Finite(sample.VehicleVelocityCmS) ||
        !Finite(sample.HeadingRadians) ||
        !Finite(sample.MassKg) ||
        sample.MassKg<300.0 || sample.MassKg>9000.0 ||
        Length(sample.VehicleVelocityCmS)>
            p.MaximumPlannerSpeedCmS ||
        !Finite(sample.DeltaSeconds) ||
        sample.DeltaSeconds<=0.0 ||
        sample.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(sample.ProposedThrottle) ||
        !Finite(sample.ProposedBrake) ||
        sample.ProposedThrottle<0.0 ||
        sample.ProposedThrottle>1.0 ||
        sample.ProposedBrake<0.0 ||
        sample.ProposedBrake>1.0 ||
        sample.KnotCount<2 ||
        sample.KnotCount>CornerForecastCapacity ||
        !sample.MeasuredAero.Valid ||
        !Finite(sample.MeasuredAero.TotalDownforceN) ||
        !Finite(sample.MeasuredAero.FrontDownforceN) ||
        !Finite(sample.MeasuredAero.RearDownforceN) ||
        sample.MeasuredAero.TotalDownforceN<0.0 ||
        !Finite(sample.MeasuredAero.DragForceWorldN))
        return false;
    if (sample.WakeRosterMatchesIdentity &&
        (!sample.Wake.Valid ||
         !sample.Wake.ProvenOriginalRoster ||
         !Finite(sample.Wake.CombinedWakeFraction)))
        return false;
    if (sample.Wake.Valid &&
        !sample.WakeRosterMatchesIdentity)
        return false;
    double normalSum=0.0;
    for (const WheelObservation& wheel:sample.Wheels)
    {
        if (!ValidWheelObservation(wheel) ||
            (p.RequireFourPhysicalWheels &&
             (!wheel.ContactVerified || !wheel.PhysicalContact)) ||
            !Finite(wheel.NormalLoadN) ||
            wheel.NormalLoadN<p.MinimumSensorLoadN ||
            wheel.NormalLoadN>p.MaximumSensorLoadN ||
            !Finite(wheel.SurfaceGrip) ||
            wheel.SurfaceGrip<p.MinimumMeasuredWheelGrip ||
            wheel.SurfaceGrip>p.MaximumMeasuredWheelGrip ||
            !Finite(wheel.SlipAngleRadians) ||
            !Finite(wheel.SlipRatio))
            return false;
        normalSum+=wheel.NormalLoadN;
    }
    if (normalSum<p.MinimumTotalNormalLoadN ||
        normalSum>p.MaximumTotalNormalLoadN)
        return false;
    double prior=-p.MinimumKnotSpacingCm;
    for (int i=0;i<sample.KnotCount;++i)
    {
        const CornerForecastKnot& knot=sample.Route[
            static_cast<std::size_t>(i)];
        if (!ValidCornerForecastKnot(knot,p) ||
            knot.AlongRouteCm-prior<p.MinimumKnotSpacingCm)
            return false;
        prior=knot.AlongRouteCm;
    }
    return sample.Route[0].AlongRouteCm<=
        p.MinimumKnotSpacingCm*2.0;
}

inline double ObservedTireGrip(
    const CornerForecastObservation& sample,
    CornerForecastSettings p) noexcept
{
    double weighted=0.0;
    double weight=0.0;
    for (const WheelObservation& wheel:sample.Wheels)
    {
        const double slip=std::max(
            std::abs(wheel.SlipRatio),
            std::abs(wheel.SlipAngleRadians));
        const double slipPenalty=p.TireSlipGripPenalty*
            Clamp01(SafeDivision(slip,
                            p.MaximumLateralSlipRatio));
        const double wearPenalty=p.MaximumWearGripPenalty*
            Clamp01(wheel.WearFraction);
        const double grip=wheel.SurfaceGrip*
            (1.0-slipPenalty)*(1.0-wearPenalty);
        weighted+=grip*wheel.NormalLoadN;
        weight+=wheel.NormalLoadN;
    }
    return std::clamp(
        SafeDivision(weighted,weight),
        p.MinimumTrackGrip,p.MaximumTrackGrip);
}

inline CornerGripDecision ForecastPhysicalCornerGrip(
    const CornerForecastObservation& sample,
    CornerForecastSettings p={}) noexcept
{
    CornerGripDecision out;
    if (!ValidCornerForecastObservation(sample,p))
        return out;
    out.MeasuredSpeedCmS=
        Length(sample.VehicleVelocityCmS);
    out.EffectiveTireGrip=ObservedTireGrip(sample,p);
    out.AvailableDownforceN=
        sample.MeasuredAero.TotalDownforceN;
    if (sample.WakeRosterMatchesIdentity)
    {
        out.WakeDragRelief=sample.Wake.DragReliefFraction;
        out.WakeDownforcePenalty=std::max(
            sample.Wake.FrontDownforceLoss,
            sample.Wake.RearDownforceLoss);
        out.AvailableDownforceN=
            sample.MeasuredAero.FrontDownforceN*
                (1.0-Clamp01(
                    sample.Wake.FrontDownforceLoss))+
            sample.MeasuredAero.RearDownforceN*
                (1.0-Clamp01(
                    sample.Wake.RearDownforceLoss));
        out.DirtyAirGripLoss=
            out.WakeDownforcePenalty>PhysicsEpsilon;
    }
    const double loadG=GravityCmS2+
        SafeDivision(out.AvailableDownforceN*
                     CentimetersPerMeter,sample.MassKg);
    const double grip=std::max(p.MinimumTrackGrip,
        out.EffectiveTireGrip*(1.0-p.GripReserveFraction));
    out.AvailableBrakingCmS2=std::clamp(
        grip*loadG*(1.0-p.BrakingReserveFraction),
        p.MinimumBrakingDecelerationCmS2,
        p.MaximumBrakingDecelerationCmS2);
    out.MinimumUpcomingCornerSpeedCmS=
        p.MaximumPlannerSpeedCmS;
    for (int i=0;i<sample.KnotCount;++i)
    {
        const CornerForecastKnot& knot=
            sample.Route[static_cast<std::size_t>(i)];
        CornerGripKnotResult& result=
            out.Knots[static_cast<std::size_t>(i)];
        result.AlongRouteCm=knot.AlongRouteCm;
        result.CurvaturePerCm=knot.CurvaturePerCm;
        result.EffectiveGrip=std::clamp(
            grip*knot.MeasuredFrictionScale,
            p.MinimumTrackGrip,p.MaximumTrackGrip);
        result.RoadMarginCm=std::min(
            knot.RoadHalfWidthCm-
                std::abs(knot.LateralOffsetCm),
            knot.DistanceToBarrierCm);
        const double bankSupport=
            GravityCmS2*std::sin(knot.BankRadians);
        result.BankingAccelerationCmS2=
            bankSupport*(knot.CurvaturePerCm>=0.0?1.0:-1.0);
        result.MaximumLateralAccelerationCmS2=std::max(
            0.0,result.EffectiveGrip*loadG*
                std::cos(knot.BankRadians)+
                result.BankingAccelerationCmS2);
        const double maxByCurve=
            std::abs(knot.CurvaturePerCm)>PhysicsEpsilon ?
            std::sqrt(SafeDivision(
                result.MaximumLateralAccelerationCmS2,
                std::abs(knot.CurvaturePerCm))) :
            p.MaximumPlannerSpeedCmS;
        result.SafeCornerSpeedCmS=std::clamp(
            maxByCurve,0.0,std::min(
                p.MaximumPlannerSpeedCmS,
                knot.LocalSpeedLimitCmS));
        if (result.RoadMarginCm<p.MinimumSafeRoadEdgeCm)
            result.SafeCornerSpeedCmS=0.0;
        result.BackwardBrakingSpeedCmS=
            result.SafeCornerSpeedCmS;
        result.VerifiedOriginalRoad=true;
        out.MinimumUpcomingCornerSpeedCmS=std::min(
            out.MinimumUpcomingCornerSpeedCmS,
            result.SafeCornerSpeedCmS);
    }
    // Integrate the maximum safe arrival speed backward, not forward.
    // A distant sharp bend must constrain the current throttle and brake.
    int limiting=sample.KnotCount-1;
    for (int i=sample.KnotCount-2;i>=0;--i)
    {
        auto& current=out.Knots[
            static_cast<std::size_t>(i)];
        const auto& next=out.Knots[
            static_cast<std::size_t>(i+1)];
        const double distance=
            next.AlongRouteCm-current.AlongRouteCm;
        current.BrakingDistanceToNextCm=distance;
        const double vmax=std::sqrt(
            next.BackwardBrakingSpeedCmS*
                next.BackwardBrakingSpeedCmS+
            2.0*out.AvailableBrakingCmS2*distance);
        if (current.SafeCornerSpeedCmS>vmax)
        {
            current.BackwardBrakingSpeedCmS=vmax;
            limiting=(limiting==sample.KnotCount-1)?
                i+1:limiting;
            current.CornerIsLimiting=true;
        }
    }
    out.PlannedEntrySpeedCmS=
        out.Knots[0].BackwardBrakingSpeedCmS;
    out.LimitingKnot=limiting;
    out.ClosestRestrictiveCurveCm=
        out.Knots[static_cast<std::size_t>(limiting)].
            AlongRouteCm;
    const double speed=out.MeasuredSpeedCmS;
    const double ceiling=out.PlannedEntrySpeedCmS;
    const double ratio=SafeDivision(
        speed,std::max(100.0,ceiling));
    out.ThrottleCeiling=Clamp01(
        SafeDivision(1.0-ratio,
            1.0-p.BrakingThrottleCutStartFraction));
    if (ratio<p.BrakingThrottleCutStartFraction)
        out.ThrottleCeiling=1.0;
    out.RequiredDecelerationCmS2=
        std::max(0.0,SafeDivision(
            speed*speed-ceiling*ceiling,
            2.0*std::max(
                p.MinimumKnotSpacingCm,
                out.ClosestRestrictiveCurveCm)));
    const double demand=Clamp01(
        SafeDivision(out.RequiredDecelerationCmS2,
                     out.AvailableBrakingCmS2)*
        p.BrakeDemandGain);
    out.BrakeFloor=speed>ceiling ?
        std::max(p.BrakeFloorAtOverspeed,demand):0.0;
    if (out.Knots[0].RoadMarginCm<
        p.MinimumSafeRoadEdgeCm ||
        ceiling<PhysicsEpsilon)
    {
        out.EmergencySlowdown=true;
        out.BrakeFloor=1.0;
        out.ThrottleCeiling=0.0;
    }
    out.CommandThrottle=std::min(
        sample.ProposedThrottle,out.ThrottleCeiling);
    out.CommandBrake=std::max(
        sample.ProposedBrake,out.BrakeFloor);
    if (out.CommandBrake>0.05)
        out.CommandThrottle=0.0;
    out.KnotCount=sample.KnotCount;
    out.EdgesVerified=true;
    out.NoUnrealForceIssued=true;
    out.Valid=Finite(out.PlannedEntrySpeedCmS) &&
        Finite(out.RequiredDecelerationCmS2) &&
        Finite(out.CommandThrottle) &&
        Finite(out.CommandBrake);
    return out;
}

class CornerGripSupervisor final
{
public:
    bool Configure(CornerForecastSettings p) noexcept
    {
        if (!ValidCornerForecastSettings(p))
            return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Session=0;
        Actor=0;
        Frame=0;
        PreviousThrottle=0.0;
        PreviousBrake=0.0;
        Accepted=0;
        Rejected=0;
        Emergency=0;
        DirtyAir=0;
        PeakRequiredDeceleration=0.0;
    }
    CornerGripDecision Step(
        const CornerForecastObservation& observation) noexcept
    {
        CornerGripDecision output=
            ForecastPhysicalCornerGrip(observation,Settings);
        if (!output.Valid ||
            (Actor==observation.Identity.Actor &&
             Session==observation.Identity.Session &&
             observation.Identity.Frame<=Frame))
        {
            ++Rejected;
            PreviousThrottle=0.0;
            PreviousBrake=1.0;
            Last={};
            return {};
        }
        if (Actor!=observation.Identity.Actor ||
            Session!=observation.Identity.Session)
        {
            PreviousThrottle=0.0;
            PreviousBrake=0.0;
        }
        Session=observation.Identity.Session;
        Actor=observation.Identity.Actor;
        Frame=observation.Identity.Frame;
        output.CommandThrottle=std::min(
            output.CommandThrottle,
            PreviousThrottle+
            Settings.ThrottleRecoveryPerSecond*
                observation.DeltaSeconds);
        output.CommandBrake=std::max(
            output.CommandBrake,
            PreviousBrake-
            Settings.BrakeReleasePerSecond*
                observation.DeltaSeconds);
        output.CommandThrottle=std::min(
            output.CommandThrottle,
            observation.ProposedThrottle);
        output.CommandBrake=std::max(
            output.CommandBrake,
            observation.ProposedBrake);
        if (output.CommandBrake>0.05)
            output.CommandThrottle=0.0;
        PreviousThrottle=output.CommandThrottle;
        PreviousBrake=output.CommandBrake;
        PeakRequiredDeceleration=std::max(
            PeakRequiredDeceleration,
            output.RequiredDecelerationCmS2);
        ++Accepted;
        if (output.EmergencySlowdown)++Emergency;
        if (output.DirtyAirGripLoss)++DirtyAir;
        Last=output;
        return Last;
    }
    CornerGripDecision Snapshot() const noexcept {return Last;}
    std::uint64_t Frames() const noexcept {return Accepted;}
    std::uint64_t Rejections() const noexcept {return Rejected;}
    std::uint64_t EmergencyFrames() const noexcept
    {return Emergency;}
    std::uint64_t DirtyAirFrames() const noexcept
    {return DirtyAir;}
    double PeakBrakingDemandCmS2() const noexcept
    {return PeakRequiredDeceleration;}
private:
    CornerForecastSettings Settings{};
    CornerGripDecision Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t Emergency=0;
    std::uint64_t DirtyAir=0;
    double PreviousThrottle=0.0;
    double PreviousBrake=0.0;
    double PeakRequiredDeceleration=0.0;
};
} // namespace latebrake::racecraft
