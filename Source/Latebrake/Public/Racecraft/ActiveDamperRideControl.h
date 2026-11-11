#pragma once
#include "Racecraft/RoadVerticalProfileSampler.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// Engine-neutral damper force proposal. The physical Chaos suspension
// is authoritative; the forecast may be compared with observed forces.
enum class DamperRideMode : std::uint8_t
{
    Comfort, Road, Sport, Track, Wet, Recovery
};
struct DamperValveParameters final
{
    double CompressionLowSpeedNsPerM=2300.0;
    double CompressionHighSpeedNsPerM=1300.0;
    double ReboundLowSpeedNsPerM=3500.0;
    double ReboundHighSpeedNsPerM=2100.0;
    double KneeVelocityCmS=12.0;
    double MaximumTravelCm=26.0;
    double MinimumRideHeightCm=6.0;
    double MaximumRideHeightCm=28.0;
    double BumpStopEngagementFraction=0.82;
    double BumpStopStiffnessNPerCm=2300.0;
    double TopOutStiffnessNPerCm=1000.0;
    double CoilSpringStiffnessNPerCm=330.0;
    double SpringPreloadCm=5.0;
    double WheelbaseCm=270.0;
    double FrontTrackCm=160.0;
    double RearTrackCm=158.0;
    double AntiRollStiffnessNPerCm=120.0;
    double MaximumDamperForceN=12000.0;
    double MaximumSpringForceN=22000.0;
    double MaximumAntiRollForceN=6000.0;
    double MaximumShaftVelocityCmS=350.0;
    double MaximumValveTransitionPerSecond=5.0;
    double MinimumValveFraction=0.25;
    double MaximumValveFraction=1.0;
    double ValveResponseSeconds=0.09;
    double MinimumTemperatureC=-30.0;
    double MaximumTemperatureC=180.0;
    double HighTemperatureFadeOnsetC=115.0;
    double MaximumFadeFraction=0.38;
    double HeatCapacityJPerC=9500.0;
    double CoolingSeconds=70.0;
    double AmbientTemperatureC=25.0;
    double MaximumFrameSeconds=0.10;
    double MinimumMeasuredLoadN=80.0;
    double MaximumMeasuredLoadN=25000.0;
    double MaximumBodySpeedCmS=16000.0;
    double MaximumProfileImpactIndex=1.0;
    double PitchResponseGain=0.30;
    double RollResponseGain=0.40;
    double WetSofteningFraction=0.23;
    double RecoverySofteningFraction=0.33;
    bool RequireMeasuredChaosContacts=true;
    bool RequireOriginalRoadProfile=true;
    bool RestrictForceToPhysicalLimits=true;
};
struct DamperWheelSample final
{
    WheelObservation Wheel{};
    double ShaftVelocityCmS=0.0;
    double MeasuredSuspensionForceN=0.0;
    double MeasuredRideHeightCm=12.0;
    double WheelTravelDemandCm=0.0;
    double FluidTemperatureC=40.0;
    double ContactPatchConfidence=1.0;
    bool OriginalSuspensionComponent=true;
    bool SensorQualified=true;
};
struct DamperRideObservation final
{
    RaceIdentity Identity{};
    std::array<DamperWheelSample,PhysicalWheelCount> Wheels{};
    RoadVerticalProfile Road{};
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double CurrentSpeedCmS=0.0;
    double LongitudinalAccelerationG=0.0;
    double LateralAccelerationG=0.0;
    double PitchRateRadiansSecond=0.0;
    double RollRateRadiansSecond=0.0;
    double DriverDemandedStiffness=0.5;
    double MeasuredRoadWetness=0.0;
    bool ActualChaosSuspensionReady=false;
    bool FourPhysicalContactsVerified=false;
    bool OriginalMapVerified=false;
};
struct DamperWheelForce final
{
    double SpringForceN=0.0;
    double CompressionForceN=0.0;
    double ReboundForceN=0.0;
    double BumpStopForceN=0.0;
    double TopOutForceN=0.0;
    double AntiRollForceN=0.0;
    double PredictedTotalForceN=0.0;
    double MeasuredForceN=0.0;
    double ForceDisagreementN=0.0;
    double ValveFraction=0.0;
    double ActualShaftVelocityCmS=0.0;
    double TravelFraction=0.0;
    double RemainingCompressionCm=0.0;
    double BumpEnergyJ=0.0;
    double DissipatedPowerW=0.0;
    double EstimatedTemperatureC=0.0;
    double EffectiveFadeFraction=0.0;
    double ContactLoadReserveN=0.0;
    bool CompressionStroke=false;
    bool ReboundStroke=false;
    bool BumpStopEngaged=false;
    bool FullyExtended=false;
    bool ForceLimited=false;
    bool ContactQualified=false;
    bool Valid=false;
};
struct DamperRideOutput final
{
    RaceIdentity Identity{};
    std::array<DamperWheelForce,PhysicalWheelCount> Wheels{};
    double FrontMeanForceN=0.0;
    double RearMeanForceN=0.0;
    double LeftMeanForceN=0.0;
    double RightMeanForceN=0.0;
    double PredictedPitchMomentNm=0.0;
    double PredictedRollMomentNm=0.0;
    double EstimatedBodyHeaveForceN=0.0;
    double TotalDampingPowerW=0.0;
    double MaximumForceDisagreementN=0.0;
    double MinimumCompressionReserveCm=0.0;
    double MaximumTemperatureC=0.0;
    double MeanValveFraction=0.0;
    double MaximumBottomOutRisk=0.0;
    double MaximumWheelLoadRatio=0.0;
    double MeasuredRoadImpactIndex=0.0;
    int BottomedWheels=0;
    int ContactWheels=0;
    int ForceLimitedWheels=0;
    bool CanRequestSoftening=false;
    bool CanRequestStiffening=false;
    bool ActualChaosForceIssued=false;
    bool Valid=false;
};
inline bool ValidDamperValveParameters(
    DamperValveParameters p) noexcept
{
    return Finite(p.CompressionLowSpeedNsPerM) &&
        p.CompressionLowSpeedNsPerM>0.0 &&
        Finite(p.CompressionHighSpeedNsPerM) &&
        p.CompressionHighSpeedNsPerM>0.0 &&
        Finite(p.ReboundLowSpeedNsPerM) &&
        p.ReboundLowSpeedNsPerM>0.0 &&
        Finite(p.ReboundHighSpeedNsPerM) &&
        p.ReboundHighSpeedNsPerM>0.0 &&
        Finite(p.KneeVelocityCmS) && p.KneeVelocityCmS>0.0 &&
        Finite(p.MaximumTravelCm) && p.MaximumTravelCm>0.0 &&
        Finite(p.MinimumRideHeightCm) &&
        p.MinimumRideHeightCm>0.0 &&
        Finite(p.MaximumRideHeightCm) &&
        p.MaximumRideHeightCm>p.MinimumRideHeightCm &&
        Finite(p.BumpStopEngagementFraction) &&
        p.BumpStopEngagementFraction>0.0 &&
        p.BumpStopEngagementFraction<1.0 &&
        Finite(p.BumpStopStiffnessNPerCm) &&
        p.BumpStopStiffnessNPerCm>=0.0 &&
        Finite(p.TopOutStiffnessNPerCm) &&
        p.TopOutStiffnessNPerCm>=0.0 &&
        Finite(p.CoilSpringStiffnessNPerCm) &&
        p.CoilSpringStiffnessNPerCm>0.0 &&
        Finite(p.SpringPreloadCm) && p.SpringPreloadCm>=0.0 &&
        Finite(p.AntiRollStiffnessNPerCm) &&
        p.AntiRollStiffnessNPerCm>=0.0 &&
        Finite(p.MaximumDamperForceN) &&
        p.MaximumDamperForceN>0.0 &&
        Finite(p.MaximumSpringForceN) &&
        p.MaximumSpringForceN>0.0 &&
        Finite(p.MaximumAntiRollForceN) &&
        p.MaximumAntiRollForceN>0.0 &&
        Finite(p.MaximumShaftVelocityCmS) &&
        p.MaximumShaftVelocityCmS>0.0 &&
        Finite(p.MaximumValveTransitionPerSecond) &&
        p.MaximumValveTransitionPerSecond>0.0 &&
        Finite(p.MinimumValveFraction) &&
        p.MinimumValveFraction>0.0 &&
        Finite(p.MaximumValveFraction) &&
        p.MaximumValveFraction>=p.MinimumValveFraction &&
        p.MaximumValveFraction<=1.0 &&
        Finite(p.ValveResponseSeconds) &&
        p.ValveResponseSeconds>0.0 &&
        Finite(p.MinimumTemperatureC) &&
        Finite(p.MaximumTemperatureC) &&
        p.MaximumTemperatureC>p.MinimumTemperatureC &&
        Finite(p.HighTemperatureFadeOnsetC) &&
        p.HighTemperatureFadeOnsetC>=p.MinimumTemperatureC &&
        p.HighTemperatureFadeOnsetC<p.MaximumTemperatureC &&
        Finite(p.MaximumFadeFraction) &&
        p.MaximumFadeFraction>=0.0 &&
        p.MaximumFadeFraction<1.0 &&
        Finite(p.HeatCapacityJPerC) &&
        p.HeatCapacityJPerC>0.0 &&
        Finite(p.CoolingSeconds) && p.CoolingSeconds>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MinimumMeasuredLoadN) &&
        p.MinimumMeasuredLoadN>=0.0 &&
        Finite(p.MaximumMeasuredLoadN) &&
        p.MaximumMeasuredLoadN>p.MinimumMeasuredLoadN &&
        Finite(p.MaximumBodySpeedCmS) &&
        p.MaximumBodySpeedCmS>0.0 &&
        Finite(p.MaximumProfileImpactIndex) &&
        p.MaximumProfileImpactIndex>0.0 &&
        Finite(p.PitchResponseGain) && p.PitchResponseGain>=0.0 &&
        Finite(p.RollResponseGain) && p.RollResponseGain>=0.0 &&
        Finite(p.WetSofteningFraction) &&
        p.WetSofteningFraction>=0.0 &&
        p.WetSofteningFraction<1.0 &&
        Finite(p.RecoverySofteningFraction) &&
        p.RecoverySofteningFraction>=0.0 &&
        p.RecoverySofteningFraction<1.0;
}
inline bool ValidDamperWheelSample(
    DamperWheelSample s,DamperValveParameters p) noexcept
{
    return ValidWheelObservation(s.Wheel) &&
        (!p.RequireMeasuredChaosContacts ||
         (s.Wheel.PhysicalContact &&
          s.Wheel.ContactVerified &&
          s.SensorQualified &&
          s.OriginalSuspensionComponent)) &&
        Finite(s.ShaftVelocityCmS) &&
        std::abs(s.ShaftVelocityCmS)<=
            p.MaximumShaftVelocityCmS &&
        Finite(s.MeasuredSuspensionForceN) &&
        std::abs(s.MeasuredSuspensionForceN)<=
            p.MaximumSpringForceN+p.MaximumDamperForceN &&
        Finite(s.MeasuredRideHeightCm) &&
        s.MeasuredRideHeightCm>=p.MinimumRideHeightCm &&
        s.MeasuredRideHeightCm<=p.MaximumRideHeightCm &&
        Finite(s.WheelTravelDemandCm) &&
        s.WheelTravelDemandCm>=0.0 &&
        s.WheelTravelDemandCm<=p.MaximumTravelCm &&
        Finite(s.FluidTemperatureC) &&
        s.FluidTemperatureC>=p.MinimumTemperatureC &&
        s.FluidTemperatureC<=p.MaximumTemperatureC &&
        Finite(s.ContactPatchConfidence) &&
        s.ContactPatchConfidence>=0.5 &&
        s.ContactPatchConfidence<=1.0 &&
        s.Wheel.NormalLoadN>=p.MinimumMeasuredLoadN &&
        s.Wheel.NormalLoadN<=p.MaximumMeasuredLoadN;
}
inline bool ValidDamperRideObservation(
    const DamperRideObservation& s,
    DamperValveParameters p) noexcept
{
    if (!ValidDamperValveParameters(p) ||
        !PhysicalActorIdentityValid(s.Identity) ||
        s.Identity.Frame==0 ||
        !s.ActualChaosSuspensionReady ||
        !s.FourPhysicalContactsVerified ||
        !s.OriginalMapVerified ||
        !Finite(s.PhysicsClockSeconds) ||
        s.PhysicsClockSeconds<0.0 ||
        !Finite(s.DeltaSeconds) ||
        s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(s.CurrentSpeedCmS) ||
        s.CurrentSpeedCmS<0.0 ||
        s.CurrentSpeedCmS>p.MaximumBodySpeedCmS ||
        !Finite(s.LongitudinalAccelerationG) ||
        std::abs(s.LongitudinalAccelerationG)>4.0 ||
        !Finite(s.LateralAccelerationG) ||
        std::abs(s.LateralAccelerationG)>4.0 ||
        !Finite(s.PitchRateRadiansSecond) ||
        !Finite(s.RollRateRadiansSecond) ||
        !Finite(s.DriverDemandedStiffness) ||
        s.DriverDemandedStiffness<0.0 ||
        s.DriverDemandedStiffness>1.0 ||
        !Finite(s.MeasuredRoadWetness) ||
        s.MeasuredRoadWetness<0.0 ||
        s.MeasuredRoadWetness>1.0 ||
        (p.RequireOriginalRoadProfile &&
         (!s.Road.Valid ||
          s.Road.Identity.Session!=s.Identity.Session ||
          s.Road.Identity.SessionRevision!=
              s.Identity.SessionRevision ||
          s.Road.Identity.Actor!=s.Identity.Actor ||
          s.Road.Identity.Frame!=s.Identity.Frame)))
        return false;
    for (const auto& wheel:s.Wheels)
        if (!ValidDamperWheelSample(wheel,p))
            return false;
    return true;
}
inline double DamperModeFactor(
    DamperRideMode mode) noexcept
{
    switch(mode)
    {
    case DamperRideMode::Comfort:return 0.45;
    case DamperRideMode::Road:return 0.65;
    case DamperRideMode::Sport:return 0.82;
    case DamperRideMode::Track:return 1.0;
    case DamperRideMode::Wet:return 0.56;
    case DamperRideMode::Recovery:return 0.35;
    }
    return 0.65;
}
inline double DamperProgressiveForce(
    double velocityCmS,double slowNsM,
    double fastNsM,double kneeCmS) noexcept
{
    if (!Finite(velocityCmS) || !Finite(slowNsM) ||
        !Finite(fastNsM) || !Finite(kneeCmS) ||
        kneeCmS<=0.0)
        return 0.0;
    const double speed=std::abs(velocityCmS);
    const double first=std::min(speed,kneeCmS);
    const double after=std::max(0.0,speed-kneeCmS);
    return (slowNsM*first+fastNsM*after)/100.0;
}
inline DamperWheelForce ForecastDamperWheelForce(
    DamperWheelSample sample,double valve,
    double antiRollDeltaCm,double impactIndex,
    double dt,DamperValveParameters p) noexcept
{
    DamperWheelForce out;
    if (!ValidDamperValveParameters(p) ||
        !ValidDamperWheelSample(sample,p) ||
        !Finite(valve) ||
        valve<p.MinimumValveFraction ||
        valve>p.MaximumValveFraction ||
        !Finite(antiRollDeltaCm) ||
        !Finite(impactIndex) ||
        impactIndex<0.0 ||
        impactIndex>p.MaximumProfileImpactIndex ||
        !Finite(dt) || dt<=0.0 ||
        dt>p.MaximumFrameSeconds)
        return out;
    const double travel=sample.WheelTravelDemandCm;
    const double travelFraction=Clamp01(
        SafeDivision(travel,p.MaximumTravelCm));
    const double compression=sample.ShaftVelocityCmS>0.0 ?
        sample.ShaftVelocityCmS:0.0;
    const double rebound=sample.ShaftVelocityCmS<0.0 ?
        -sample.ShaftVelocityCmS:0.0;
    const double faded=Clamp01(SafeDivision(
        sample.FluidTemperatureC-p.HighTemperatureFadeOnsetC,
        p.MaximumTemperatureC-p.HighTemperatureFadeOnsetC));
    const double fade=p.MaximumFadeFraction*faded;
    const double coefficient=valve*(1.0-fade);
    out.CompressionForceN=std::min(
        p.MaximumDamperForceN,
        DamperProgressiveForce(
            compression,p.CompressionLowSpeedNsPerM,
            p.CompressionHighSpeedNsPerM,p.KneeVelocityCmS)*
            coefficient);
    out.ReboundForceN=std::min(
        p.MaximumDamperForceN,
        DamperProgressiveForce(
            rebound,p.ReboundLowSpeedNsPerM,
            p.ReboundHighSpeedNsPerM,p.KneeVelocityCmS)*
            coefficient);
    out.SpringForceN=std::min(
        p.MaximumSpringForceN,
        p.CoilSpringStiffnessNPerCm*
            (travel+p.SpringPreloadCm));
    const double bumpExcess=std::max(
        0.0,travel-
        p.MaximumTravelCm*p.BumpStopEngagementFraction);
    out.BumpStopForceN=
        p.BumpStopStiffnessNPerCm*bumpExcess*bumpExcess/
        std::max(1.0,p.MaximumTravelCm);
    out.TopOutForceN=travel<1.0 ?
        p.TopOutStiffnessNPerCm*(1.0-travel):0.0;
    out.AntiRollForceN=std::clamp(
        p.AntiRollStiffnessNPerCm*antiRollDeltaCm,
        -p.MaximumAntiRollForceN,p.MaximumAntiRollForceN);
    const double damping=out.CompressionForceN-
        out.ReboundForceN;
    const double requested=out.SpringForceN+
        damping+out.BumpStopForceN-out.TopOutForceN+
        out.AntiRollForceN;
    out.PredictedTotalForceN=std::clamp(
        requested,-p.MaximumDamperForceN,
        p.MaximumSpringForceN+p.MaximumDamperForceN);
    out.MeasuredForceN=sample.MeasuredSuspensionForceN;
    out.ForceDisagreementN=std::abs(
        out.PredictedTotalForceN-out.MeasuredForceN);
    out.ValveFraction=valve;
    out.ActualShaftVelocityCmS=sample.ShaftVelocityCmS;
    out.TravelFraction=travelFraction;
    out.RemainingCompressionCm=p.MaximumTravelCm-travel;
    out.DissipatedPowerW=
        (out.CompressionForceN+out.ReboundForceN)*
        std::abs(sample.ShaftVelocityCmS)/100.0;
    out.BumpEnergyJ=out.BumpStopForceN*
        std::max(0.0,compression)/100.0*dt;
    out.EstimatedTemperatureC=std::clamp(
        sample.FluidTemperatureC+
        out.DissipatedPowerW*dt/p.HeatCapacityJPerC-
        (sample.FluidTemperatureC-
         p.AmbientTemperatureC)*
            dt/p.CoolingSeconds,
        p.MinimumTemperatureC,p.MaximumTemperatureC);
    out.EffectiveFadeFraction=fade;
    out.ContactLoadReserveN=sample.Wheel.NormalLoadN-
        std::max(0.0,out.PredictedTotalForceN);
    out.CompressionStroke=compression>PhysicsEpsilon;
    out.ReboundStroke=rebound>PhysicsEpsilon;
    out.BumpStopEngaged=bumpExcess>0.0;
    out.FullyExtended=travel<=PhysicsEpsilon;
    out.ForceLimited=std::abs(
        out.PredictedTotalForceN-requested)>PhysicsEpsilon;
    out.ContactQualified=
        sample.Wheel.ContactVerified &&
        sample.Wheel.PhysicalContact;
    out.Valid=Finite(out.PredictedTotalForceN) &&
        Finite(out.EstimatedTemperatureC) &&
        Finite(out.DissipatedPowerW) &&
        Finite(out.ContactLoadReserveN) &&
        Finite(out.ForceDisagreementN);
    return out;
}
inline DamperRideOutput ForecastDamperRide(
    const DamperRideObservation& observation,
    DamperRideMode mode,
    double activeValve,DamperValveParameters p={}) noexcept
{
    DamperRideOutput out;
    if (!ValidDamperRideObservation(observation,p) ||
        !Finite(activeValve) ||
        activeValve<p.MinimumValveFraction ||
        activeValve>p.MaximumValveFraction)
        return out;
    out.Identity=observation.Identity;
    out.MinimumCompressionReserveCm=p.MaximumTravelCm;
    const double roadImpact=observation.Road.Valid ?
        observation.Road.PeakImpactIndex:0.0;
    out.MeasuredRoadImpactIndex=roadImpact;
    const double frontDifference=
        observation.Wheels[0].WheelTravelDemandCm-
        observation.Wheels[1].WheelTravelDemandCm;
    const double rearDifference=
        observation.Wheels[2].WheelTravelDemandCm-
        observation.Wheels[3].WheelTravelDemandCm;
    const std::array<double,4> rollDeltas{
        -frontDifference,frontDifference,
        -rearDifference,rearDifference};
    for(std::size_t i=0;i<4;++i)
    {
        const DamperWheelForce f=ForecastDamperWheelForce(
            observation.Wheels[i],activeValve,
            rollDeltas[i],roadImpact,observation.DeltaSeconds,p);
        if (!f.Valid)return {};
        out.Wheels[i]=f;
        out.ContactWheels+=f.ContactQualified?1:0;
        out.BottomedWheels+=f.BumpStopEngaged?1:0;
        out.ForceLimitedWheels+=f.ForceLimited?1:0;
        out.TotalDampingPowerW+=f.DissipatedPowerW;
        out.EstimatedBodyHeaveForceN+=
            f.PredictedTotalForceN;
        out.MaximumForceDisagreementN=std::max(
            out.MaximumForceDisagreementN,
            f.ForceDisagreementN);
        out.MinimumCompressionReserveCm=std::min(
            out.MinimumCompressionReserveCm,
            f.RemainingCompressionCm);
        out.MaximumTemperatureC=std::max(
            out.MaximumTemperatureC,
            f.EstimatedTemperatureC);
        out.MeanValveFraction+=f.ValveFraction*0.25;
        out.MaximumBottomOutRisk=std::max(
            out.MaximumBottomOutRisk,
            f.TravelFraction);
        out.MaximumWheelLoadRatio=std::max(
            out.MaximumWheelLoadRatio,
            SafeDivision(f.PredictedTotalForceN,
                         std::max(1.0,
                                  observation.Wheels[i].Wheel.NormalLoadN)));
    }
    const auto& wheel=out.Wheels;
    out.FrontMeanForceN=
        0.5*(wheel[0].PredictedTotalForceN+
             wheel[1].PredictedTotalForceN);
    out.RearMeanForceN=
        0.5*(wheel[2].PredictedTotalForceN+
             wheel[3].PredictedTotalForceN);
    out.LeftMeanForceN=
        0.5*(wheel[0].PredictedTotalForceN+
             wheel[2].PredictedTotalForceN);
    out.RightMeanForceN=
        0.5*(wheel[1].PredictedTotalForceN+
             wheel[3].PredictedTotalForceN);
    out.PredictedPitchMomentNm=
        (out.RearMeanForceN-out.FrontMeanForceN)*
            p.WheelbaseCm/200.0;
    out.PredictedRollMomentNm=
        (out.LeftMeanForceN-out.RightMeanForceN)*
            (p.FrontTrackCm+p.RearTrackCm)/400.0;
    out.CanRequestSoftening=
        roadImpact>0.30 ||
        observation.MeasuredRoadWetness>0.5 ||
        mode==DamperRideMode::Comfort ||
        mode==DamperRideMode::Recovery;
    out.CanRequestStiffening=
        mode==DamperRideMode::Sport ||
        mode==DamperRideMode::Track;
    out.ActualChaosForceIssued=false;
    out.Valid=out.ContactWheels==PhysicalWheelCount &&
        Finite(out.PredictedPitchMomentNm) &&
        Finite(out.PredictedRollMomentNm) &&
        Finite(out.TotalDampingPowerW);
    return out;
}
inline double DamperDesiredValve(
    const DamperRideObservation& sample,
    DamperRideMode mode,DamperValveParameters p) noexcept
{
    if (!ValidDamperRideObservation(sample,p))
        return p.MinimumValveFraction;
    const double acceleration=Clamp01(
        0.5*std::abs(sample.LongitudinalAccelerationG)+
        0.5*std::abs(sample.LateralAccelerationG));
    const double pitchRoll=Clamp01(
        p.PitchResponseGain*
            std::abs(sample.PitchRateRadiansSecond)+
        p.RollResponseGain*
            std::abs(sample.RollRateRadiansSecond));
    const double modeLevel=DamperModeFactor(mode);
    const double driver=sample.DriverDemandedStiffness;
    const double active=Clamp01(
        0.30*modeLevel+0.30*driver+
        0.20*acceleration+0.20*pitchRoll);
    const double wet=1.0-
        p.WetSofteningFraction*sample.MeasuredRoadWetness;
    const double recovery=mode==DamperRideMode::Recovery ?
        1.0-p.RecoverySofteningFraction:1.0;
    const double impact=sample.Road.Valid ?
        1.0-0.4*Clamp01(sample.Road.PeakImpactIndex):1.0;
    return std::clamp(
        p.MinimumValveFraction+
            (p.MaximumValveFraction-p.MinimumValveFraction)*
            active*wet*recovery*impact,
        p.MinimumValveFraction,p.MaximumValveFraction);
}
class ActiveDamperRideSupervisor final
{
public:
    bool Configure(
        DamperValveParameters p,DamperRideMode mode) noexcept
    {
        if (!ValidDamperValveParameters(p))return false;
        Parameters=p;
        Mode=mode;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Session=0;
        Revision=0;
        Actor=0;
        Frame=0;
        Valve=Parameters.MinimumValveFraction;
        Qualified=0;
        Rejected=0;
        BumpStopFrames=0;
        AccumulatedEnergyJ=0.0;
        PeakDampingPowerW=0.0;
        PeakForceDisagreementN=0.0;
    }
    DamperRideOutput Step(
        const DamperRideObservation& input) noexcept
    {
        const bool sameEpoch=Session!=0 &&
            Session==input.Identity.Session &&
            Revision==input.Identity.SessionRevision &&
            Actor==input.Identity.Actor;
        if (!ValidDamperRideObservation(input,Parameters) ||
            (sameEpoch && input.Identity.Frame<=Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        if (!sameEpoch)
            Valve=Parameters.MinimumValveFraction;
        const double desired=DamperDesiredValve(
            input,Mode,Parameters);
        Valve=std::clamp(RateLimited(
            Valve,desired,
            Parameters.MaximumValveTransitionPerSecond,
            Parameters.MaximumValveTransitionPerSecond,
            input.DeltaSeconds),
            Parameters.MinimumValveFraction,
            Parameters.MaximumValveFraction);
        const double next=ExponentialResponse(
            Valve,desired,Parameters.ValveResponseSeconds,
            input.DeltaSeconds);
        Valve=std::clamp(next,
            Parameters.MinimumValveFraction,
            Parameters.MaximumValveFraction);
        const DamperRideOutput out=ForecastDamperRide(
            input,Mode,Valve,Parameters);
        if (!out.Valid)
        {
            ++Rejected;
            Last={};
            return {};
        }
        Last=out;
        Session=input.Identity.Session;
        Revision=input.Identity.SessionRevision;
        Actor=input.Identity.Actor;
        Frame=input.Identity.Frame;
        ++Qualified;
        if (out.BottomedWheels>0)++BumpStopFrames;
        AccumulatedEnergyJ+=
            out.TotalDampingPowerW*input.DeltaSeconds;
        PeakDampingPowerW=std::max(
            PeakDampingPowerW,out.TotalDampingPowerW);
        PeakForceDisagreementN=std::max(
            PeakForceDisagreementN,
            out.MaximumForceDisagreementN);
        return Last;
    }
    DamperRideOutput Snapshot() const noexcept{return Last;}
    double CurrentValveFraction() const noexcept{return Valve;}
    double AccumulatedDissipationJoules() const noexcept
    {return AccumulatedEnergyJ;}
    double MaximumDampingPowerWatts() const noexcept
    {return PeakDampingPowerW;}
    double PeakSourceForceDisagreementN() const noexcept
    {return PeakForceDisagreementN;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t BumpStopSamples() const noexcept
    {return BumpStopFrames;}
private:
    DamperValveParameters Parameters{};
    DamperRideMode Mode=DamperRideMode::Road;
    DamperRideOutput Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t BumpStopFrames=0;
    std::uint32_t Revision=0;
    double Valve=0.25;
    double AccumulatedEnergyJ=0.0;
    double PeakDampingPowerW=0.0;
    double PeakForceDisagreementN=0.0;
};
} // namespace latebrake::racecraft
