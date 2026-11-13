#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class ContactPatchCondition : std::uint8_t
{
    Unknown, Healthy, Underinflated, Overinflated,
    Worn, Overheated, StandingWater, GravelContamination,
    KerbUnloaded, Saturated
};
struct ContactPatchParameters final
{
    double NominalPressureKpa=210.0;
    double MinimumPressureKpa=115.0;
    double MaximumPressureKpa=350.0;
    double NominalLoadN=3200.0;
    double MinimumObservedLoadN=50.0;
    double MaximumObservedLoadN=26000.0;
    double NominalPatchAreaCm2=165.0;
    double MaximumPatchAreaCm2=450.0;
    double MinimumPatchAreaCm2=60.0;
    double NominalAspectRatio=1.45;
    double MinimumAspectRatio=0.65;
    double MaximumAspectRatio=2.6;
    double MaximumContactDistortion=0.45;
    double LoadSensitivityExponent=0.92;
    double PressureGripPenalty=0.16;
    double PressureAreaExponent=0.7;
    double WearGripPenalty=0.22;
    double ThermalGripPenalty=0.32;
    double KerbGripPenalty=0.25;
    double GravelContaminationPenalty=0.30;
    double ContactConfidenceMinimum=0.65;
    double MinimumPhysicalGrip=0.15;
    double MaximumPhysicalGrip=2.5;
    double MinimumTireTemperatureC=-20.0;
    double OptimumTireTemperatureC=85.0;
    double MaximumTireTemperatureC=175.0;
    double MaximumAllowedSlipAngleRadians=0.75;
    double MaximumAllowedSlipRatio=1.4;
    double WetContactPenalty=0.22;
    double MinimumStandingWaterMm=1.0;
    double MaximumStandingWaterMm=70.0;
    double WaterEvacuationMmS=1.6;
    double TreadDepthNewMm=7.2;
    double TreadDepthMinimumMm=1.1;
    double HydroplaningSpeedGain=950.0;
    double HydroplaningLoadRecoveryFraction=0.20;
    double HydroplaningPressureExponent=0.5;
    double MaximumWaterRisk=1.0;
    double FrictionEllipseExponent=2.0;
    double TireHeatingGain=0.00004;
    double TireCoolingGain=0.10;
    double MaximumFrameSeconds=0.10;
    double MaximumVehicleSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumRoadNormalZ=0.65;
    bool RequireActualGroundContact=true;
    bool RequireOriginalCollisionBody=true;
    bool RequirePhysicalWeatherProbes=true;
};
struct ContactPatchWheelSample final
{
    WheelObservation Wheel{};
    RaceIdentity Identity{};
    Vec3 RoadNormal{0.0,0.0,1.0};
    double MeasuredPressureKpa=210.0;
    double MeasuredTreadDepthMm=7.0;
    double ActualWaterDepthMm=0.0;
    double ActualForwardSpeedCmS=0.0;
    double ActualLateralSpeedCmS=0.0;
    double MeasuredLongitudinalForceN=0.0;
    double MeasuredLateralForceN=0.0;
    double MeasuredRoadFriction=1.0;
    double SensorConfidence=1.0;
    double AmbientTemperatureC=25.0;
    double DeltaSeconds=0.0;
    bool OriginalWheelObserved=false;
    bool PhysicalRoadFrictionReady=false;
    bool PhysicalWaterSensorReady=false;
    bool ActualChaosBodyReady=false;
};
struct ContactPatchEstimate final
{
    RaceIdentity Identity{};
    ContactPatchCondition Condition=ContactPatchCondition::Unknown;
    double PatchAreaCm2=0.0;
    double PatchLengthCm=0.0;
    double PatchWidthCm=0.0;
    double EffectiveGrip=0.0;
    double LongitudinalGrip=0.0;
    double LateralGrip=0.0;
    double PressureDeviation=0.0;
    double WearFraction=0.0;
    double EffectiveTreadDepthMm=0.0;
    double ThermalPenalty=0.0;
    double PressurePenalty=0.0;
    double SurfacePenalty=0.0;
    double ContactDistortion=0.0;
    double LoadSensitivityMultiplier=0.0;
    double WaterFilmMm=0.0;
    double WaterEvacuationCapacityMmS=0.0;
    double HydroplaningOnsetSpeedCmS=0.0;
    double HydroplaningSeverity=0.0;
    double PredictedNormalLoadN=0.0;
    double AvailableLongitudinalForceN=0.0;
    double AvailableLateralForceN=0.0;
    double ObservedLongitudinalForceN=0.0;
    double ObservedLateralForceN=0.0;
    double LongitudinalTractionUse=0.0;
    double LateralTractionUse=0.0;
    double CombinedTractionUse=0.0;
    double RemainingFrictionBudget=0.0;
    double SlipPowerWatts=0.0;
    double HeatDeltaC=0.0;
    double Confidence=0.0;
    bool HydroplaningRisk=false;
    bool CombinedGripSaturated=false;
    bool GroundQualified=false;
    bool AppliesChaosForce=false;
    bool Valid=false;
};
inline bool ValidContactPatchParameters(
    ContactPatchParameters p) noexcept
{
    return Finite(p.NominalPressureKpa) && p.NominalPressureKpa>0.0 &&
        Finite(p.MinimumPressureKpa) && p.MinimumPressureKpa>0.0 &&
        p.MinimumPressureKpa<p.NominalPressureKpa &&
        Finite(p.MaximumPressureKpa) &&
        p.MaximumPressureKpa>p.NominalPressureKpa &&
        Finite(p.NominalLoadN) && p.NominalLoadN>0.0 &&
        Finite(p.MinimumObservedLoadN) &&
        p.MinimumObservedLoadN>=0.0 &&
        Finite(p.MaximumObservedLoadN) &&
        p.MaximumObservedLoadN>p.MinimumObservedLoadN &&
        Finite(p.NominalPatchAreaCm2) &&
        p.NominalPatchAreaCm2>0.0 &&
        Finite(p.MaximumPatchAreaCm2) &&
        p.MaximumPatchAreaCm2>p.NominalPatchAreaCm2 &&
        Finite(p.MinimumPatchAreaCm2) &&
        p.MinimumPatchAreaCm2>0.0 &&
        p.MinimumPatchAreaCm2<p.NominalPatchAreaCm2 &&
        Finite(p.NominalAspectRatio) &&
        p.NominalAspectRatio>0.0 &&
        Finite(p.MinimumAspectRatio) &&
        p.MinimumAspectRatio>0.0 &&
        Finite(p.MaximumAspectRatio) &&
        p.MaximumAspectRatio>p.MinimumAspectRatio &&
        Finite(p.MaximumContactDistortion) &&
        p.MaximumContactDistortion>0.0 &&
        Finite(p.LoadSensitivityExponent) &&
        p.LoadSensitivityExponent>0.0 &&
        p.LoadSensitivityExponent<=1.5 &&
        Finite(p.PressureGripPenalty) &&
        p.PressureGripPenalty>=0.0 &&
        p.PressureGripPenalty<1.0 &&
        Finite(p.PressureAreaExponent) &&
        p.PressureAreaExponent>0.0 &&
        Finite(p.WearGripPenalty) &&
        p.WearGripPenalty>=0.0 && p.WearGripPenalty<1.0 &&
        Finite(p.ThermalGripPenalty) &&
        p.ThermalGripPenalty>=0.0 && p.ThermalGripPenalty<1.0 &&
        Finite(p.KerbGripPenalty) &&
        p.KerbGripPenalty>=0.0 && p.KerbGripPenalty<1.0 &&
        Finite(p.GravelContaminationPenalty) &&
        p.GravelContaminationPenalty>=0.0 &&
        p.GravelContaminationPenalty<1.0 &&
        Finite(p.ContactConfidenceMinimum) &&
        p.ContactConfidenceMinimum>0.0 &&
        p.ContactConfidenceMinimum<=1.0 &&
        Finite(p.MinimumPhysicalGrip) &&
        p.MinimumPhysicalGrip>0.0 &&
        Finite(p.MaximumPhysicalGrip) &&
        p.MaximumPhysicalGrip>=p.MinimumPhysicalGrip &&
        Finite(p.MinimumTireTemperatureC) &&
        Finite(p.OptimumTireTemperatureC) &&
        Finite(p.MaximumTireTemperatureC) &&
        p.MinimumTireTemperatureC<p.OptimumTireTemperatureC &&
        p.OptimumTireTemperatureC<p.MaximumTireTemperatureC &&
        Finite(p.MaximumAllowedSlipAngleRadians) &&
        p.MaximumAllowedSlipAngleRadians>0.0 &&
        Finite(p.MaximumAllowedSlipRatio) &&
        p.MaximumAllowedSlipRatio>0.0 &&
        Finite(p.WetContactPenalty) &&
        p.WetContactPenalty>=0.0 &&
        p.WetContactPenalty<1.0 &&
        Finite(p.MinimumStandingWaterMm) &&
        p.MinimumStandingWaterMm>=0.0 &&
        Finite(p.MaximumStandingWaterMm) &&
        p.MaximumStandingWaterMm>p.MinimumStandingWaterMm &&
        Finite(p.WaterEvacuationMmS) &&
        p.WaterEvacuationMmS>0.0 &&
        Finite(p.TreadDepthNewMm) &&
        p.TreadDepthNewMm>0.0 &&
        Finite(p.TreadDepthMinimumMm) &&
        p.TreadDepthMinimumMm>0.0 &&
        p.TreadDepthMinimumMm<p.TreadDepthNewMm &&
        Finite(p.HydroplaningSpeedGain) &&
        p.HydroplaningSpeedGain>0.0 &&
        Finite(p.HydroplaningLoadRecoveryFraction) &&
        p.HydroplaningLoadRecoveryFraction>=0.0 &&
        p.HydroplaningLoadRecoveryFraction<1.0 &&
        Finite(p.HydroplaningPressureExponent) &&
        p.HydroplaningPressureExponent>0.0 &&
        Finite(p.MaximumWaterRisk) &&
        p.MaximumWaterRisk>0.0 &&
        Finite(p.FrictionEllipseExponent) &&
        p.FrictionEllipseExponent>0.0 &&
        Finite(p.TireHeatingGain) &&
        p.TireHeatingGain>=0.0 &&
        Finite(p.TireCoolingGain) &&
        p.TireCoolingGain>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumVehicleSpeedCmS) &&
        p.MaximumVehicleSpeedCmS>0.0 &&
        p.MaximumVehicleSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MinimumRoadNormalZ) &&
        p.MinimumRoadNormalZ>0.0 &&
        p.MinimumRoadNormalZ<=1.0;
}
inline bool ValidContactPatchWheelSample(
    ContactPatchWheelSample s,ContactPatchParameters p) noexcept
{
    return ValidContactPatchParameters(p) &&
        PhysicalActorIdentityValid(s.Identity) &&
        s.Identity.Frame>0 &&
        ValidWheelObservation(s.Wheel) &&
        (!p.RequireActualGroundContact ||
         (s.Wheel.PhysicalContact &&
          s.Wheel.ContactVerified)) &&
        (!p.RequireOriginalCollisionBody ||
         (s.OriginalWheelObserved &&
          s.ActualChaosBodyReady)) &&
        (!p.RequirePhysicalWeatherProbes ||
         (s.PhysicalRoadFrictionReady &&
          s.PhysicalWaterSensorReady)) &&
        Finite(s.RoadNormal) &&
        s.RoadNormal.Z>=p.MinimumRoadNormalZ &&
        Finite(s.MeasuredPressureKpa) &&
        s.MeasuredPressureKpa>=p.MinimumPressureKpa &&
        s.MeasuredPressureKpa<=p.MaximumPressureKpa &&
        Finite(s.MeasuredTreadDepthMm) &&
        s.MeasuredTreadDepthMm>=0.0 &&
        s.MeasuredTreadDepthMm<=p.TreadDepthNewMm &&
        Finite(s.ActualWaterDepthMm) &&
        s.ActualWaterDepthMm>=0.0 &&
        s.ActualWaterDepthMm<=p.MaximumStandingWaterMm &&
        Finite(s.ActualForwardSpeedCmS) &&
        std::abs(s.ActualForwardSpeedCmS)<=p.MaximumVehicleSpeedCmS &&
        Finite(s.ActualLateralSpeedCmS) &&
        std::abs(s.ActualLateralSpeedCmS)<=p.MaximumVehicleSpeedCmS &&
        Finite(s.MeasuredLongitudinalForceN) &&
        std::abs(s.MeasuredLongitudinalForceN)<=
            p.MaximumObservedLoadN*p.MaximumPhysicalGrip &&
        Finite(s.MeasuredLateralForceN) &&
        std::abs(s.MeasuredLateralForceN)<=
            p.MaximumObservedLoadN*p.MaximumPhysicalGrip &&
        Finite(s.MeasuredRoadFriction) &&
        s.MeasuredRoadFriction>=p.MinimumPhysicalGrip &&
        s.MeasuredRoadFriction<=p.MaximumPhysicalGrip &&
        Finite(s.SensorConfidence) &&
        s.SensorConfidence>=p.ContactConfidenceMinimum &&
        s.SensorConfidence<=1.0 &&
        Finite(s.AmbientTemperatureC) &&
        s.AmbientTemperatureC>=p.MinimumTireTemperatureC &&
        s.AmbientTemperatureC<=p.MaximumTireTemperatureC &&
        Finite(s.DeltaSeconds) &&
        s.DeltaSeconds>0.0 &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        s.Wheel.NormalLoadN>=p.MinimumObservedLoadN &&
        s.Wheel.NormalLoadN<=p.MaximumObservedLoadN &&
        std::abs(s.Wheel.SlipRatio)<=p.MaximumAllowedSlipRatio &&
        std::abs(s.Wheel.SlipAngleRadians)<=
            p.MaximumAllowedSlipAngleRadians;
}
inline double ContactPatchWaterOnsetSpeed(
    ContactPatchWheelSample s,ContactPatchParameters p) noexcept
{
    if (!ValidContactPatchWheelSample(s,p))
        return 0.0;
    const double treadFraction=Clamp01(SafeDivision(
        s.MeasuredTreadDepthMm,p.TreadDepthNewMm));
    const double pressure=std::pow(
        SafeDivision(s.MeasuredPressureKpa,p.NominalPressureKpa),
        p.HydroplaningPressureExponent);
    return p.HydroplaningSpeedGain*
        pressure*std::sqrt(0.3+0.7*treadFraction);
}
inline double ContactPatchHydroplaningRisk(
    ContactPatchWheelSample s,ContactPatchParameters p) noexcept
{
    const double onset=ContactPatchWaterOnsetSpeed(s,p);
    if (onset<=0.0 || s.ActualWaterDepthMm<
        p.MinimumStandingWaterMm)
        return 0.0;
    const double excess=Clamp01(
        SafeDivision(std::abs(s.ActualForwardSpeedCmS)-onset,
                     std::max(1.0,onset)));
    const double depth=Clamp01(SafeDivision(
        s.ActualWaterDepthMm-p.MinimumStandingWaterMm,
        p.MaximumStandingWaterMm-
            p.MinimumStandingWaterMm));
    const double tread=1.0-Clamp01(SafeDivision(
        s.MeasuredTreadDepthMm,p.TreadDepthNewMm));
    return std::clamp(
        excess*(0.5+0.5*depth)*(0.7+0.3*tread),
        0.0,p.MaximumWaterRisk);
}
inline ContactPatchEstimate EvaluateContactPatch(
    ContactPatchWheelSample s,ContactPatchParameters p={}) noexcept
{
    ContactPatchEstimate out;
    if (!ValidContactPatchWheelSample(s,p))
        return out;
    out.Identity=s.Identity;
    const double load=s.Wheel.NormalLoadN;
    const double pressureFraction=
        SafeDivision(s.MeasuredPressureKpa,p.NominalPressureKpa);
    const double loadFraction=
        SafeDivision(load,p.NominalLoadN);
    out.PressureDeviation=pressureFraction-1.0;
    out.WearFraction=s.Wheel.WearFraction;
    out.EffectiveTreadDepthMm=s.MeasuredTreadDepthMm;
    out.PatchAreaCm2=std::clamp(
        p.NominalPatchAreaCm2*
            std::pow(std::max(PhysicsEpsilon,loadFraction),
                     p.LoadSensitivityExponent)/
            std::pow(pressureFraction,p.PressureAreaExponent),
        p.MinimumPatchAreaCm2,p.MaximumPatchAreaCm2);
    const double widthRatio=std::clamp(
        p.NominalAspectRatio*(1.0+
            0.5*out.PressureDeviation),
        p.MinimumAspectRatio,p.MaximumAspectRatio);
    out.PatchLengthCm=std::sqrt(
        out.PatchAreaCm2/widthRatio);
    out.PatchWidthCm=std::sqrt(
        out.PatchAreaCm2*widthRatio);
    const double pressureLoss=Clamp01(
        std::abs(out.PressureDeviation));
    out.PressurePenalty=p.PressureGripPenalty*pressureLoss;
    const double tempOffset=std::abs(
        s.Wheel.TemperatureC-p.OptimumTireTemperatureC);
    const double tempRange=std::max(
        p.OptimumTireTemperatureC-p.MinimumTireTemperatureC,
        p.MaximumTireTemperatureC-p.OptimumTireTemperatureC);
    out.ThermalPenalty=p.ThermalGripPenalty*
        Clamp01(SafeDivision(tempOffset,tempRange));
    out.SurfacePenalty=
        s.Wheel.Surface==ContactSurface::PaintedKerb ?
            p.KerbGripPenalty :
        s.Wheel.Surface==ContactSurface::Gravel ?
            p.GravelContaminationPenalty :
        s.Wheel.Surface==ContactSurface::WetAsphalt ?
            p.WetContactPenalty:0.0;
    out.HydroplaningOnsetSpeedCmS=
        ContactPatchWaterOnsetSpeed(s,p);
    out.WaterEvacuationCapacityMmS=
        p.WaterEvacuationMmS*
        Clamp01(SafeDivision(s.MeasuredTreadDepthMm,
                             p.TreadDepthNewMm));
    out.WaterFilmMm=std::max(
        0.0,s.ActualWaterDepthMm-
            out.WaterEvacuationCapacityMmS*s.DeltaSeconds);
    out.HydroplaningSeverity=
        ContactPatchHydroplaningRisk(s,p);
    const double waterPenalty=
        p.WetContactPenalty*
        Clamp01(SafeDivision(
            out.WaterFilmMm,p.MaximumStandingWaterMm));
    out.LoadSensitivityMultiplier=std::pow(
        std::max(0.05,loadFraction),
        p.LoadSensitivityExponent-1.0);
    const double gripRaw=s.MeasuredRoadFriction*
        s.Wheel.SurfaceGrip*
        out.LoadSensitivityMultiplier*
        (1.0-out.PressurePenalty)*
        (1.0-p.WearGripPenalty*s.Wheel.WearFraction)*
        (1.0-out.ThermalPenalty)*
        (1.0-out.SurfacePenalty)*
        (1.0-waterPenalty);
    out.EffectiveGrip=std::clamp(
        gripRaw,p.MinimumPhysicalGrip,
        p.MaximumPhysicalGrip);
    out.HydroplaningRisk=
        out.HydroplaningSeverity>0.10;
    const double contactScale=1.0-
        out.HydroplaningSeverity*
            (1.0-p.HydroplaningLoadRecoveryFraction);
    out.PredictedNormalLoadN=load*contactScale;
    out.LongitudinalGrip=out.EffectiveGrip*
        (1.0-0.30*out.HydroplaningSeverity);
    out.LateralGrip=out.EffectiveGrip*
        (1.0-0.55*out.HydroplaningSeverity);
    out.AvailableLongitudinalForceN=
        out.PredictedNormalLoadN*out.LongitudinalGrip;
    out.AvailableLateralForceN=
        out.PredictedNormalLoadN*out.LateralGrip;
    out.ObservedLongitudinalForceN=
        s.MeasuredLongitudinalForceN;
    out.ObservedLateralForceN=
        s.MeasuredLateralForceN;
    out.LongitudinalTractionUse=std::abs(
        SafeDivision(out.ObservedLongitudinalForceN,
            std::max(PhysicsEpsilon,
                     out.AvailableLongitudinalForceN)));
    out.LateralTractionUse=std::abs(
        SafeDivision(out.ObservedLateralForceN,
            std::max(PhysicsEpsilon,
                     out.AvailableLateralForceN)));
    const double exponent=p.FrictionEllipseExponent;
    out.CombinedTractionUse=std::pow(
        std::pow(out.LongitudinalTractionUse,exponent)+
        std::pow(out.LateralTractionUse,exponent),
        1.0/exponent);
    out.RemainingFrictionBudget=std::max(
        0.0,1.0-out.CombinedTractionUse);
    const double measuredSlip=std::max(
        std::abs(s.Wheel.SlipRatio),
        std::abs(s.Wheel.SlipAngleRadians));
    out.ContactDistortion=std::min(
        p.MaximumContactDistortion,
        measuredSlip*0.35+
        out.HydroplaningSeverity*0.25);
    out.SlipPowerWatts=
        (std::abs(s.MeasuredLongitudinalForceN)+
         std::abs(s.MeasuredLateralForceN))*
        std::hypot(
            s.ActualForwardSpeedCmS,
            s.ActualLateralSpeedCmS)/100.0*
        Clamp01(measuredSlip);
    out.HeatDeltaC=
        out.SlipPowerWatts*
        p.TireHeatingGain*s.DeltaSeconds-
        (s.Wheel.TemperatureC-s.AmbientTemperatureC)*
        p.TireCoolingGain*s.DeltaSeconds;
    out.Confidence=s.SensorConfidence*
        (1.0-0.3*out.ContactDistortion);
    out.CombinedGripSaturated=
        out.CombinedTractionUse>1.0;
    out.GroundQualified=s.Wheel.ContactVerified &&
        s.Wheel.PhysicalContact;
    out.Condition=out.HydroplaningRisk ?
        ContactPatchCondition::StandingWater :
        out.CombinedGripSaturated ?
            ContactPatchCondition::Saturated :
        s.Wheel.TemperatureC>p.OptimumTireTemperatureC+40.0 ?
            ContactPatchCondition::Overheated :
        s.Wheel.WearFraction>0.75 ?
            ContactPatchCondition::Worn :
        s.MeasuredPressureKpa<p.NominalPressureKpa*0.85 ?
            ContactPatchCondition::Underinflated :
        s.MeasuredPressureKpa>p.NominalPressureKpa*1.15 ?
            ContactPatchCondition::Overinflated :
        s.Wheel.Surface==ContactSurface::Gravel ?
            ContactPatchCondition::GravelContamination :
        s.Wheel.Surface==ContactSurface::PaintedKerb ?
            ContactPatchCondition::KerbUnloaded :
            ContactPatchCondition::Healthy;
    out.AppliesChaosForce=false;
    out.Valid=Finite(out.PatchAreaCm2) &&
        Finite(out.EffectiveGrip) &&
        Finite(out.CombinedTractionUse) &&
        Finite(out.HeatDeltaC) &&
        Finite(out.HydroplaningSeverity);
    return out;
}
struct FourPatchObservation final
{
    RaceIdentity Identity{};
    std::array<ContactPatchWheelSample,PhysicalWheelCount> Wheels{};
    double FrameSeconds=0.0;
    bool OriginalFourWheelVehicle=true;
    bool EnginePhysicsQualified=true;
    bool RaceInProgress=true;
};
struct FourPatchSnapshot final
{
    RaceIdentity Identity{};
    std::array<ContactPatchEstimate,PhysicalWheelCount> Wheels{};
    double TotalContactAreaCm2=0.0;
    double TotalPredictedNormalLoadN=0.0;
    double MinimumAvailableLateralForceN=0.0;
    double MaximumHydroplaningSeverity=0.0;
    double MeanEffectiveGrip=0.0;
    double MinimumConfidence=0.0;
    double TotalSlipPowerWatts=0.0;
    double FrontContactBiasFraction=0.0;
    double LeftContactBiasFraction=0.0;
    double MaximumTractionUse=0.0;
    int SaturatedWheels=0;
    int HydroplaningWheels=0;
    int GroundedWheels=0;
    bool SupportsFullPhysicalVehicle=true;
    bool ActualChaosActuation=false;
    bool Valid=false;
};
inline FourPatchSnapshot EvaluateFourContactPatches(
    const FourPatchObservation& sample,
    ContactPatchParameters p={}) noexcept
{
    FourPatchSnapshot out;
    if (!ValidContactPatchParameters(p) ||
        !sample.OriginalFourWheelVehicle ||
        !sample.EnginePhysicsQualified ||
        !sample.RaceInProgress ||
        !PhysicalActorIdentityValid(sample.Identity) ||
        !ValidFrameDelta(sample.FrameSeconds))
        return out;
    out.Identity=sample.Identity;
    out.MinimumConfidence=1.0;
    out.MinimumAvailableLateralForceN=
        std::numeric_limits<double>::infinity();
    for(std::size_t i=0;i<PhysicalWheelCount;++i)
    {
        const auto& w=sample.Wheels[i];
        if (w.Identity.Session!=sample.Identity.Session ||
            w.Identity.SessionRevision!=
                sample.Identity.SessionRevision ||
            w.Identity.Actor!=sample.Identity.Actor ||
            w.Identity.Frame!=sample.Identity.Frame ||
            w.Identity.Slot!=sample.Identity.Slot ||
            std::abs(w.DeltaSeconds-sample.FrameSeconds)>1e-6)
            return {};
        const ContactPatchEstimate patch=
            EvaluateContactPatch(w,p);
        if (!patch.Valid)return {};
        out.Wheels[i]=patch;
        out.TotalContactAreaCm2+=patch.PatchAreaCm2;
        out.TotalPredictedNormalLoadN+=
            patch.PredictedNormalLoadN;
        out.MinimumAvailableLateralForceN=std::min(
            out.MinimumAvailableLateralForceN,
            patch.AvailableLateralForceN);
        out.MaximumHydroplaningSeverity=std::max(
            out.MaximumHydroplaningSeverity,
            patch.HydroplaningSeverity);
        out.MinimumConfidence=std::min(
            out.MinimumConfidence,patch.Confidence);
        out.TotalSlipPowerWatts+=patch.SlipPowerWatts;
        out.MaximumTractionUse=std::max(
            out.MaximumTractionUse,
            patch.CombinedTractionUse);
        out.MeanEffectiveGrip+=
            patch.EffectiveGrip*0.25;
        out.SaturatedWheels+=
            patch.CombinedGripSaturated?1:0;
        out.HydroplaningWheels+=
            patch.HydroplaningRisk?1:0;
        out.GroundedWheels+=patch.GroundQualified?1:0;
    }
    const auto& w=out.Wheels;
    out.FrontContactBiasFraction=SafeDivision(
        w[0].PredictedNormalLoadN+
        w[1].PredictedNormalLoadN,
        out.TotalPredictedNormalLoadN);
    out.LeftContactBiasFraction=SafeDivision(
        w[0].PredictedNormalLoadN+
        w[2].PredictedNormalLoadN,
        out.TotalPredictedNormalLoadN);
    out.ActualChaosActuation=false;
    out.Valid=out.GroundedWheels==PhysicalWheelCount &&
        Finite(out.TotalContactAreaCm2) &&
        Finite(out.TotalPredictedNormalLoadN) &&
        Finite(out.MinimumConfidence) &&
        Finite(out.TotalSlipPowerWatts);
    return out;
}
class PhysicalContactPatchSupervisor final
{
public:
    bool Configure(ContactPatchParameters p) noexcept
    {
        if (!ValidContactPatchParameters(p))
            return false;
        Settings=p;
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
        Qualified=0;
        Rejected=0;
        WetFrames=0;
        SaturationFrames=0;
        PeakHydroplaning=0.0;
        PeakTractionUse=0.0;
        AccumulatedSlipEnergyJ=0.0;
    }
    FourPatchSnapshot Step(
        const FourPatchObservation& observation) noexcept
    {
        const FourPatchSnapshot proposed=
            EvaluateFourContactPatches(observation,Settings);
        const bool sameEpoch=Session!=0 &&
            Session==observation.Identity.Session &&
            Revision==observation.Identity.SessionRevision &&
            Actor==observation.Identity.Actor;
        if (!proposed.Valid ||
            (sameEpoch && observation.Identity.Frame<=Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        Last=proposed;
        Session=observation.Identity.Session;
        Revision=observation.Identity.SessionRevision;
        Actor=observation.Identity.Actor;
        Frame=observation.Identity.Frame;
        ++Qualified;
        WetFrames+=proposed.HydroplaningWheels>0?1U:0U;
        SaturationFrames+=proposed.SaturatedWheels>0?1U:0U;
        PeakHydroplaning=std::max(
            PeakHydroplaning,proposed.MaximumHydroplaningSeverity);
        PeakTractionUse=std::max(
            PeakTractionUse,proposed.MaximumTractionUse);
        AccumulatedSlipEnergyJ+=
            proposed.TotalSlipPowerWatts*
            observation.FrameSeconds;
        return Last;
    }
    FourPatchSnapshot Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t WetContactFrames() const noexcept
    {return WetFrames;}
    std::uint64_t SaturatedContactFrames() const noexcept
    {return SaturationFrames;}
    double PeakMeasuredHydroplaning() const noexcept
    {return PeakHydroplaning;}
    double MaximumTractionUse() const noexcept
    {return PeakTractionUse;}
    double TotalSlipEnergyJ() const noexcept
    {return AccumulatedSlipEnergyJ;}
private:
    ContactPatchParameters Settings{};
    FourPatchSnapshot Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t Frame=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t WetFrames=0;
    std::uint64_t SaturationFrames=0;
    std::uint32_t Revision=0;
    double PeakHydroplaning=0.0;
    double PeakTractionUse=0.0;
    double AccumulatedSlipEnergyJ=0.0;
};
} // namespace latebrake::racecraft
