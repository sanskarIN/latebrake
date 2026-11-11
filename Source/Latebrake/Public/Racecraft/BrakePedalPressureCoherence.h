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
// A supplementary diagnostic for ACTUAL hydraulic pressure observations.
// This model cannot command the UE Chaos brake or simulate missing sensors.
inline constexpr int HydraulicWheelCount=PhysicalWheelCount;
enum class BrakePressureFault : std::uint8_t
{
    None, NoPhysicalFrame, CrossEpoch, ReplayedFrame,
    InsufficientPressure, PressureMismatch, LockedWheel,
    ThermalFade, HydraulicLeak, UnqualifiedWheel
};
struct BrakeCoherenceParameters final
{
    double MaximumPressureBar=175.0;
    double MinimumPressureBar=0.0;
    double MasterCylinderPressureRiseBarS=850.0;
    double MasterCylinderPressureFallBarS=1200.0;
    double MaximumLineDelaySeconds=0.12;
    double BrakeBiasFront=0.62;
    double MinimumBrakeBiasFront=0.48;
    double MaximumBrakeBiasFront=0.80;
    double MaximumWheelPressureMismatchBar=35.0;
    double MaximumMeasuredPressureJumpBar=45.0;
    double MaximumPadTemperatureC=950.0;
    double FadeOnsetTemperatureC=580.0;
    double FadeCriticalTemperatureC=800.0;
    double PadCoefficientDry=0.41;
    double PadCoefficientWet=0.29;
    double MinimumPadCoefficient=0.10;
    double DiscRadiusCm=13.5;
    double FrontPistonAreaCm2=23.0;
    double RearPistonAreaCm2=16.0;
    double MaximumExpectedWheelTorqueNm=11000.0;
    double PedalDeadzoneFraction=0.025;
    double PedalForceGain=1.2;
    double MaximumPedalTravelCm=9.0;
    double MinimumPedalTravelCm=0.0;
    double MaximumPedalStrokeSpeedCmS=50.0;
    double MaximumPedalForceN=1800.0;
    double MaximumBoosterGain=6.0;
    double BoosterResponseSeconds=0.06;
    double BrakeBalanceResponseSeconds=0.1;
    double MinimumGripG=0.15;
    double MaximumGripG=2.5;
    double MinimumWheelLoadN=50.0;
    double MaximumWheelLoadN=25000.0;
    double MinimumForwardSpeedCmS=0.0;
    double MaximumForwardSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.03;
    double MinimumSensorConfidence=0.75;
    double MaximumPressureResidualBar=50.0;
    double LeakPressureLossBarS=10.0;
    double BrakeThermalRecoverySeconds=45.0;
    double MaximumQualifiedSlipRatio=1.0;
    double WheelLockSlipThreshold=-0.22;
    double MaximumBrakeIntervention=1.0;
    bool RequireAllFourHydraulicCircuits=true;
    bool RequireOriginalBrakeComponents=true;
    bool NeverOverrideDriverPedal=true;
};
struct BrakeCircuitWheelObservation final
{
    WheelObservation Wheel{};
    double MeasuredLinePressureBar=0.0;
    double MeasuredDiscTemperatureC=30.0;
    double CaliperPistonTravelCm=0.0;
    double PadWearFraction=0.0;
    double MeasuredWheelBrakeTorqueNm=0.0;
    double SourceClockSeconds=0.0;
    double SensorConfidence=1.0;
    bool OriginalCaliperPresent=false;
    bool PressureSensorQualified=false;
    bool MeasuredTorqueQualified=false;
};
struct BrakePressureObservation final
{
    RaceIdentity Identity{};
    std::array<BrakeCircuitWheelObservation,HydraulicWheelCount> Circuits{};
    double DriverPedalFraction=0.0;
    double MasterCylinderPressureBar=0.0;
    double PedalTravelCm=0.0;
    double PedalTravelSpeedCmS=0.0;
    double MeasuredPedalForceN=0.0;
    double MeasuredBoosterGain=1.0;
    double LongitudinalSpeedCmS=0.0;
    double RoadGripG=1.0;
    double DeltaSeconds=0.0;
    double PhysicsClockSeconds=0.0;
    bool RaceRunning=false;
    bool EngineBrakePhysicsQualified=false;
    bool OriginalMapVerified=false;
    bool AbsActualSensorReady=false;
};
struct BrakeCircuitEstimate final
{
    double ActualPressureBar=0.0;
    double ExpectedPressureBar=0.0;
    double EffectivePadFriction=0.0;
    double ExpectedWheelTorqueNm=0.0;
    double MeasuredWheelTorqueNm=0.0;
    double ExpectedHydraulicForceN=0.0;
    double MeasuredNormalLoadN=0.0;
    double GripLimitedWheelTorqueNm=0.0;
    double TorqueDisagreementNm=0.0;
    double PressureResidualBar=0.0;
    double ThermalFadeFraction=0.0;
    double RequiredAbsReleaseFraction=0.0;
    double HydraulicDelaySeconds=0.0;
    double AvailablePressureReserveBar=0.0;
    double EstimatedLeakSeverity=0.0;
    double PadWearFraction=0.0;
    double WheelSlipRatio=0.0;
    bool BrakePressureQualified=false;
    bool MayBeLeaking=false;
    bool AbsWouldIntervene=false;
    bool Overheated=false;
    bool WheelTorqueGripLimited=false;
    bool Valid=false;
};
struct BrakeCoherenceDecision final
{
    RaceIdentity Identity{};
    std::array<BrakeCircuitEstimate,HydraulicWheelCount> Circuits{};
    double DriverPedal=0.0;
    double MasterCylinderPressureBar=0.0;
    double ExpectedFrontPressureBar=0.0;
    double ExpectedRearPressureBar=0.0;
    double MeanFrontPressureBar=0.0;
    double MeanRearPressureBar=0.0;
    double MeasuredFrontTorqueNm=0.0;
    double MeasuredRearTorqueNm=0.0;
    double ExpectedTotalTorqueNm=0.0;
    double MeasuredTotalTorqueNm=0.0;
    double EstimatedFrontBiasFraction=0.0;
    double MinimumEffectiveFriction=0.0;
    double MaximumObservedFade=0.0;
    double MaximumPressureResidualBar=0.0;
    double TotalPressureImbalanceBar=0.0;
    double ExpectedBrakingForceN=0.0;
    double MeasuredBrakingForceN=0.0;
    double MaximumEstimatedDecelerationCmS2=0.0;
    double ConservativeStopDistanceCm=0.0;
    double EstimatedPedalTravelCm=0.0;
    double HydraulicConfidence=0.0;
    double BrakeInterventionCeiling=0.0;
    int QualifiedCircuits=0;
    int LeakingCircuits=0;
    int AbsActiveCircuits=0;
    int OverheatedCircuits=0;
    int GripLimitedCircuits=0;
    BrakePressureFault Fault=BrakePressureFault::None;
    bool ActualChaosCommandIssued=false;
    bool MayRequirePitInspection=false;
    bool Valid=false;
};
inline bool ValidBrakeCoherenceParameters(
    BrakeCoherenceParameters p) noexcept
{
    return Finite(p.MaximumPressureBar) &&
        p.MaximumPressureBar>0.0 &&
        Finite(p.MinimumPressureBar) &&
        p.MinimumPressureBar>=0.0 &&
        p.MinimumPressureBar<p.MaximumPressureBar &&
        Finite(p.MasterCylinderPressureRiseBarS) &&
        p.MasterCylinderPressureRiseBarS>0.0 &&
        Finite(p.MasterCylinderPressureFallBarS) &&
        p.MasterCylinderPressureFallBarS>0.0 &&
        Finite(p.MaximumLineDelaySeconds) &&
        p.MaximumLineDelaySeconds>0.0 &&
        Finite(p.BrakeBiasFront) &&
        p.BrakeBiasFront>0.0 &&
        p.BrakeBiasFront<1.0 &&
        Finite(p.MinimumBrakeBiasFront) &&
        p.MinimumBrakeBiasFront>0.0 &&
        p.MinimumBrakeBiasFront<p.BrakeBiasFront &&
        Finite(p.MaximumBrakeBiasFront) &&
        p.MaximumBrakeBiasFront>p.BrakeBiasFront &&
        p.MaximumBrakeBiasFront<=1.0 &&
        Finite(p.MaximumWheelPressureMismatchBar) &&
        p.MaximumWheelPressureMismatchBar>0.0 &&
        Finite(p.MaximumMeasuredPressureJumpBar) &&
        p.MaximumMeasuredPressureJumpBar>0.0 &&
        Finite(p.MaximumPadTemperatureC) &&
        p.MaximumPadTemperatureC>0.0 &&
        Finite(p.FadeOnsetTemperatureC) &&
        p.FadeOnsetTemperatureC>0.0 &&
        p.FadeOnsetTemperatureC<p.MaximumPadTemperatureC &&
        Finite(p.FadeCriticalTemperatureC) &&
        p.FadeCriticalTemperatureC>p.FadeOnsetTemperatureC &&
        p.FadeCriticalTemperatureC<=p.MaximumPadTemperatureC &&
        Finite(p.PadCoefficientDry) &&
        p.PadCoefficientDry>0.0 &&
        Finite(p.PadCoefficientWet) &&
        p.PadCoefficientWet>0.0 &&
        Finite(p.MinimumPadCoefficient) &&
        p.MinimumPadCoefficient>0.0 &&
        Finite(p.DiscRadiusCm) && p.DiscRadiusCm>0.0 &&
        Finite(p.FrontPistonAreaCm2) &&
        p.FrontPistonAreaCm2>0.0 &&
        Finite(p.RearPistonAreaCm2) &&
        p.RearPistonAreaCm2>0.0 &&
        Finite(p.MaximumExpectedWheelTorqueNm) &&
        p.MaximumExpectedWheelTorqueNm>0.0 &&
        Finite(p.PedalDeadzoneFraction) &&
        p.PedalDeadzoneFraction>=0.0 &&
        p.PedalDeadzoneFraction<1.0 &&
        Finite(p.PedalForceGain) && p.PedalForceGain>0.0 &&
        Finite(p.MaximumPedalTravelCm) &&
        p.MaximumPedalTravelCm>0.0 &&
        Finite(p.MinimumPedalTravelCm) &&
        p.MinimumPedalTravelCm>=0.0 &&
        Finite(p.MaximumPedalStrokeSpeedCmS) &&
        p.MaximumPedalStrokeSpeedCmS>0.0 &&
        Finite(p.MaximumPedalForceN) &&
        p.MaximumPedalForceN>0.0 &&
        Finite(p.MaximumBoosterGain) &&
        p.MaximumBoosterGain>=1.0 &&
        Finite(p.BoosterResponseSeconds) &&
        p.BoosterResponseSeconds>0.0 &&
        Finite(p.BrakeBalanceResponseSeconds) &&
        p.BrakeBalanceResponseSeconds>0.0 &&
        Finite(p.MinimumGripG) && p.MinimumGripG>0.0 &&
        Finite(p.MaximumGripG) &&
        p.MaximumGripG>p.MinimumGripG &&
        Finite(p.MinimumWheelLoadN) &&
        p.MinimumWheelLoadN>=0.0 &&
        Finite(p.MaximumWheelLoadN) &&
        p.MaximumWheelLoadN>p.MinimumWheelLoadN &&
        Finite(p.MaximumForwardSpeedCmS) &&
        p.MaximumForwardSpeedCmS>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MaximumPressureResidualBar) &&
        p.MaximumPressureResidualBar>0.0 &&
        Finite(p.LeakPressureLossBarS) &&
        p.LeakPressureLossBarS>0.0 &&
        Finite(p.BrakeThermalRecoverySeconds) &&
        p.BrakeThermalRecoverySeconds>0.0 &&
        Finite(p.MaximumQualifiedSlipRatio) &&
        p.MaximumQualifiedSlipRatio>0.0 &&
        Finite(p.WheelLockSlipThreshold) &&
        p.WheelLockSlipThreshold<0.0 &&
        Finite(p.MaximumBrakeIntervention) &&
        p.MaximumBrakeIntervention>0.0 &&
        p.MaximumBrakeIntervention<=1.0;
}
inline bool ValidBrakeCircuitWheelObservation(
    BrakeCircuitWheelObservation w,
    BrakeCoherenceParameters p) noexcept
{
    return ValidWheelObservation(w.Wheel) &&
        (!p.RequireOriginalBrakeComponents ||
         (w.OriginalCaliperPresent &&
          w.PressureSensorQualified &&
          w.MeasuredTorqueQualified)) &&
        (!p.RequireAllFourHydraulicCircuits ||
         (w.Wheel.ContactVerified &&
          w.Wheel.PhysicalContact)) &&
        Finite(w.MeasuredLinePressureBar) &&
        w.MeasuredLinePressureBar>=p.MinimumPressureBar &&
        w.MeasuredLinePressureBar<=p.MaximumPressureBar &&
        Finite(w.MeasuredDiscTemperatureC) &&
        w.MeasuredDiscTemperatureC>=-30.0 &&
        w.MeasuredDiscTemperatureC<=p.MaximumPadTemperatureC &&
        Finite(w.CaliperPistonTravelCm) &&
        w.CaliperPistonTravelCm>=0.0 &&
        w.CaliperPistonTravelCm<=p.MaximumPedalTravelCm &&
        Finite(w.PadWearFraction) &&
        w.PadWearFraction>=0.0 &&
        w.PadWearFraction<=1.0 &&
        Finite(w.MeasuredWheelBrakeTorqueNm) &&
        std::abs(w.MeasuredWheelBrakeTorqueNm)<=
            p.MaximumExpectedWheelTorqueNm &&
        Finite(w.SourceClockSeconds) &&
        w.SourceClockSeconds>=0.0 &&
        Finite(w.SensorConfidence) &&
        w.SensorConfidence>=p.MinimumSensorConfidence &&
        w.SensorConfidence<=1.0 &&
        w.Wheel.NormalLoadN>=p.MinimumWheelLoadN &&
        w.Wheel.NormalLoadN<=p.MaximumWheelLoadN;
}
inline bool ValidBrakePressureObservation(
    const BrakePressureObservation& o,
    BrakeCoherenceParameters p) noexcept
{
    if (!ValidBrakeCoherenceParameters(p) ||
        !PhysicalActorIdentityValid(o.Identity) ||
        o.Identity.Frame==0 ||
        !o.EngineBrakePhysicsQualified ||
        !o.OriginalMapVerified ||
        !o.RaceRunning ||
        !o.AbsActualSensorReady ||
        !Finite(o.DriverPedalFraction) ||
        o.DriverPedalFraction<0.0 ||
        o.DriverPedalFraction>1.0 ||
        !Finite(o.MasterCylinderPressureBar) ||
        o.MasterCylinderPressureBar<p.MinimumPressureBar ||
        o.MasterCylinderPressureBar>p.MaximumPressureBar ||
        !Finite(o.PedalTravelCm) ||
        o.PedalTravelCm<p.MinimumPedalTravelCm ||
        o.PedalTravelCm>p.MaximumPedalTravelCm ||
        !Finite(o.PedalTravelSpeedCmS) ||
        std::abs(o.PedalTravelSpeedCmS)>
            p.MaximumPedalStrokeSpeedCmS ||
        !Finite(o.MeasuredPedalForceN) ||
        o.MeasuredPedalForceN<0.0 ||
        o.MeasuredPedalForceN>p.MaximumPedalForceN ||
        !Finite(o.MeasuredBoosterGain) ||
        o.MeasuredBoosterGain<1.0 ||
        o.MeasuredBoosterGain>p.MaximumBoosterGain ||
        !Finite(o.LongitudinalSpeedCmS) ||
        o.LongitudinalSpeedCmS<p.MinimumForwardSpeedCmS ||
        o.LongitudinalSpeedCmS>p.MaximumForwardSpeedCmS ||
        !Finite(o.RoadGripG) ||
        o.RoadGripG<p.MinimumGripG ||
        o.RoadGripG>p.MaximumGripG ||
        !Finite(o.DeltaSeconds) ||
        o.DeltaSeconds<=0.0 ||
        o.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(o.PhysicsClockSeconds) ||
        o.PhysicsClockSeconds<0.0)
        return false;
    for(const auto& w:o.Circuits)
    {
        if (!ValidBrakeCircuitWheelObservation(w,p) ||
            std::abs(w.SourceClockSeconds-
                     o.PhysicsClockSeconds)>
                p.MaximumClockSkewSeconds)
            return false;
    }
    return true;
}
inline double BrakeFadeAtTemperature(
    double measuredC,BrakeCoherenceParameters p) noexcept
{
    if (!Finite(measuredC))return 1.0;
    return Clamp01(SafeDivision(
        measuredC-p.FadeOnsetTemperatureC,
        p.FadeCriticalTemperatureC-p.FadeOnsetTemperatureC));
}
inline double PressureToTorqueNm(
    double pressureBar,double pistonAreaCm2,
    double padCoefficient,double discRadiusCm) noexcept
{
    if (!Finite(pressureBar) || pressureBar<0.0 ||
        !Finite(pistonAreaCm2) || pistonAreaCm2<0.0 ||
        !Finite(padCoefficient) || padCoefficient<0.0 ||
        !Finite(discRadiusCm) || discRadiusCm<0.0)
        return 0.0;
    const double pascal=pressureBar*100000.0;
    const double pistonAreaM2=pistonAreaCm2/10000.0;
    const double frictionRadiusM=discRadiusCm/100.0;
    return pascal*pistonAreaM2*
        padCoefficient*frictionRadiusM;
}
inline BrakeCircuitEstimate EvaluateBrakeCircuit(
    BrakeCircuitWheelObservation sample,
    double expectedPressureBar,double tireGripG,
    bool front,BrakeCoherenceParameters p) noexcept
{
    BrakeCircuitEstimate out;
    if (!ValidBrakeCircuitWheelObservation(sample,p) ||
        !Finite(expectedPressureBar) ||
        expectedPressureBar<0.0 ||
        expectedPressureBar>p.MaximumPressureBar ||
        !Finite(tireGripG) ||
        tireGripG<p.MinimumGripG ||
        tireGripG>p.MaximumGripG)
        return out;
    const double fade=BrakeFadeAtTemperature(
        sample.MeasuredDiscTemperatureC,p);
    const double coefficient=std::max(
        p.MinimumPadCoefficient,
        p.PadCoefficientDry*(1.0-0.65*fade)*
        (1.0-0.20*sample.PadWearFraction));
    const double area=front ?
        p.FrontPistonAreaCm2:p.RearPistonAreaCm2;
    out.ActualPressureBar=sample.MeasuredLinePressureBar;
    out.ExpectedPressureBar=expectedPressureBar;
    out.EffectivePadFriction=coefficient;
    out.ExpectedHydraulicForceN=
        expectedPressureBar*100000.0*area/10000.0;
    out.ExpectedWheelTorqueNm=std::clamp(
        PressureToTorqueNm(
            expectedPressureBar,area,coefficient,p.DiscRadiusCm),
        0.0,p.MaximumExpectedWheelTorqueNm);
    out.MeasuredWheelTorqueNm=
        sample.MeasuredWheelBrakeTorqueNm;
    out.MeasuredNormalLoadN=sample.Wheel.NormalLoadN;
    out.GripLimitedWheelTorqueNm=
        sample.Wheel.NormalLoadN*tireGripG*
        sample.Wheel.RadiusCm/100.0;
    out.TorqueDisagreementNm=std::abs(
        out.ExpectedWheelTorqueNm-
        out.MeasuredWheelTorqueNm);
    out.PressureResidualBar=
        sample.MeasuredLinePressureBar-expectedPressureBar;
    out.ThermalFadeFraction=fade;
    out.WheelSlipRatio=sample.Wheel.SlipRatio;
    out.AvailablePressureReserveBar=std::max(
        0.0,p.MaximumPressureBar-
        sample.MeasuredLinePressureBar);
    out.HydraulicDelaySeconds=SafeDivision(
        std::abs(out.PressureResidualBar),
        p.MasterCylinderPressureRiseBarS);
    out.EstimatedLeakSeverity=Clamp01(SafeDivision(
        -out.PressureResidualBar,p.MaximumPressureResidualBar));
    out.PadWearFraction=sample.PadWearFraction;
    out.MayBeLeaking=
        -out.PressureResidualBar>
        p.MaximumWheelPressureMismatchBar;
    out.AbsWouldIntervene=
        sample.Wheel.SlipRatio<p.WheelLockSlipThreshold;
    out.RequiredAbsReleaseFraction=out.AbsWouldIntervene ?
        Clamp01(SafeDivision(
            p.WheelLockSlipThreshold-sample.Wheel.SlipRatio,
            p.MaximumQualifiedSlipRatio)):0.0;
    out.Overheated=fade>0.5;
    out.WheelTorqueGripLimited=
        out.ExpectedWheelTorqueNm>
        out.GripLimitedWheelTorqueNm;
    out.BrakePressureQualified=
        sample.PressureSensorQualified &&
        sample.MeasuredTorqueQualified;
    out.Valid=Finite(out.ExpectedWheelTorqueNm) &&
        Finite(out.GripLimitedWheelTorqueNm) &&
        Finite(out.TorqueDisagreementNm) &&
        Finite(out.EstimatedLeakSeverity);
    return out;
}
inline BrakeCoherenceDecision EvaluateBrakePressureCoherence(
    const BrakePressureObservation& o,
    BrakeCoherenceParameters p={}) noexcept
{
    BrakeCoherenceDecision out;
    if (!ValidBrakePressureObservation(o,p))
        return out;
    out.Identity=o.Identity;
    out.DriverPedal=o.DriverPedalFraction;
    out.MasterCylinderPressureBar=o.MasterCylinderPressureBar;
    const double active=Clamp01(SafeDivision(
        o.DriverPedalFraction-p.PedalDeadzoneFraction,
        1.0-p.PedalDeadzoneFraction));
    const double idealPressure=std::min(
        p.MaximumPressureBar,
        o.MasterCylinderPressureBar*
        std::min(p.MaximumBoosterGain,
                 o.MeasuredBoosterGain)*
        active*p.PedalForceGain);
    out.ExpectedFrontPressureBar=
        idealPressure*p.BrakeBiasFront;
    out.ExpectedRearPressureBar=
        idealPressure*(1.0-p.BrakeBiasFront);
    out.MinimumEffectiveFriction=
        std::numeric_limits<double>::infinity();
    out.HydraulicConfidence=1.0;
    for(std::size_t i=0;i<HydraulicWheelCount;++i)
    {
        const bool front=i<2;
        const auto& sample=o.Circuits[i];
        const double target=front ?
            out.ExpectedFrontPressureBar :
            out.ExpectedRearPressureBar;
        const auto wheel=EvaluateBrakeCircuit(
            sample,target,o.RoadGripG,front,p);
        if (!wheel.Valid)return {};
        out.Circuits[i]=wheel;
        ++out.QualifiedCircuits;
        out.LeakingCircuits+=wheel.MayBeLeaking?1:0;
        out.AbsActiveCircuits+=wheel.AbsWouldIntervene?1:0;
        out.OverheatedCircuits+=wheel.Overheated?1:0;
        out.GripLimitedCircuits+=
            wheel.WheelTorqueGripLimited?1:0;
        out.ExpectedTotalTorqueNm+=wheel.ExpectedWheelTorqueNm;
        out.MeasuredTotalTorqueNm+=wheel.MeasuredWheelTorqueNm;
        out.MaximumObservedFade=std::max(
            out.MaximumObservedFade,
            wheel.ThermalFadeFraction);
        out.MaximumPressureResidualBar=std::max(
            out.MaximumPressureResidualBar,
            std::abs(wheel.PressureResidualBar));
        out.TotalPressureImbalanceBar+=
            std::abs(wheel.PressureResidualBar);
        out.MinimumEffectiveFriction=std::min(
            out.MinimumEffectiveFriction,
            wheel.EffectivePadFriction);
        out.HydraulicConfidence=std::min(
            out.HydraulicConfidence,
            sample.SensorConfidence);
        if(front)
        {
            out.MeanFrontPressureBar+=wheel.ActualPressureBar*0.5;
            out.MeasuredFrontTorqueNm+=wheel.MeasuredWheelTorqueNm;
        }
        else
        {
            out.MeanRearPressureBar+=wheel.ActualPressureBar*0.5;
            out.MeasuredRearTorqueNm+=wheel.MeasuredWheelTorqueNm;
        }
        const double radiusM=sample.Wheel.RadiusCm/100.0;
        out.ExpectedBrakingForceN+=SafeDivision(
            std::min(wheel.ExpectedWheelTorqueNm,
                     wheel.GripLimitedWheelTorqueNm),radiusM);
        out.MeasuredBrakingForceN+=SafeDivision(
            std::min(wheel.MeasuredWheelTorqueNm,
                     wheel.GripLimitedWheelTorqueNm),radiusM);
    }
    out.EstimatedFrontBiasFraction=SafeDivision(
        out.MeasuredFrontTorqueNm,out.MeasuredTotalTorqueNm,
        p.BrakeBiasFront);
    // This estimate deliberately does not invent missing actual vehicle
    // mass. The deceleration ceiling is a grip-limited bound, not an
    // assertion that this brake pedal achieves that force in Chaos.
    out.MaximumEstimatedDecelerationCmS2=
        std::min(o.RoadGripG*GravityCmS2,
                 p.MaximumGripG*GravityCmS2);
    out.ConservativeStopDistanceCm=
        SafeDivision(o.LongitudinalSpeedCmS*
                         o.LongitudinalSpeedCmS,
                     2.0*std::max(
                         PhysicsEpsilon,
                         out.MaximumEstimatedDecelerationCmS2));
    out.EstimatedPedalTravelCm=
        o.PedalTravelCm;
    out.BrakeInterventionCeiling=
        1.0-Clamp01(out.MaximumObservedFade*
                    p.MaximumBrakeIntervention);
    out.MayRequirePitInspection=
        out.LeakingCircuits>0 ||
        out.OverheatedCircuits>1;
    out.Fault=out.LeakingCircuits>0 ?
        BrakePressureFault::HydraulicLeak :
        out.AbsActiveCircuits>0 ?
            BrakePressureFault::LockedWheel :
        out.OverheatedCircuits>0 ?
            BrakePressureFault::ThermalFade :
        out.MaximumPressureResidualBar>
            p.MaximumPressureResidualBar ?
            BrakePressureFault::PressureMismatch :
            BrakePressureFault::None;
    out.ActualChaosCommandIssued=false;
    out.Valid=out.QualifiedCircuits==HydraulicWheelCount &&
        Finite(out.MeasuredTotalTorqueNm) &&
        Finite(out.ConservativeStopDistanceCm) &&
        Finite(out.HydraulicConfidence);
    return out;
}
class BrakePressureConsistencySupervisor final
{
public:
    bool Configure(BrakeCoherenceParameters settings) noexcept
    {
        if (!ValidBrakeCoherenceParameters(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Identity={};
        FramesAccepted=0;
        FramesRejected=0;
        LeakSamples=0;
        AbsSamples=0;
        HeatSamples=0;
        MaximumPressureResidual=0.0;
        TotalMeasuredBrakeWork=0.0;
        HasAccepted=false;
    }
    BrakeCoherenceDecision Step(
        const BrakePressureObservation& sample) noexcept
    {
        const BrakeCoherenceDecision result=
            EvaluateBrakePressureCoherence(sample,Settings);
        const bool sameEpoch=HasAccepted &&
            Identity.Session==sample.Identity.Session &&
            Identity.SessionRevision==
                sample.Identity.SessionRevision &&
            Identity.Actor==sample.Identity.Actor;
        if (!result.Valid ||
            (sameEpoch &&
             sample.Identity.Frame<=Identity.Frame))
        {
            ++FramesRejected;
            Last={};
            return {};
        }
        Identity=sample.Identity;
        HasAccepted=true;
        Last=result;
        ++FramesAccepted;
        LeakSamples+=result.LeakingCircuits>0?1U:0U;
        AbsSamples+=result.AbsActiveCircuits>0?1U:0U;
        HeatSamples+=result.OverheatedCircuits>0?1U:0U;
        MaximumPressureResidual=std::max(
            MaximumPressureResidual,
            result.MaximumPressureResidualBar);
        TotalMeasuredBrakeWork+=
            result.MeasuredBrakingForceN*
            sample.LongitudinalSpeedCmS/100.0*
            sample.DeltaSeconds;
        return Last;
    }
    BrakeCoherenceDecision Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return FramesAccepted;}
    std::uint64_t Rejections() const noexcept
    {return FramesRejected;}
    std::uint64_t LeakFrames() const noexcept{return LeakSamples;}
    std::uint64_t AntiLockFrames() const noexcept{return AbsSamples;}
    std::uint64_t HeatWarningFrames() const noexcept
    {return HeatSamples;}
    double MaximumObservedPressureResidualBar() const noexcept
    {return MaximumPressureResidual;}
    double MeasuredBrakeWorkJoules() const noexcept
    {return TotalMeasuredBrakeWork;}
private:
    BrakeCoherenceParameters Settings{};
    BrakeCoherenceDecision Last{};
    RaceIdentity Identity{};
    std::uint64_t FramesAccepted=0;
    std::uint64_t FramesRejected=0;
    std::uint64_t LeakSamples=0;
    std::uint64_t AbsSamples=0;
    std::uint64_t HeatSamples=0;
    double MaximumPressureResidual=0.0;
    double TotalMeasuredBrakeWork=0.0;
    bool HasAccepted=false;
};
} // namespace latebrake::racecraft
