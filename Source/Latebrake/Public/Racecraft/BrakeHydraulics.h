#pragma once
#include "Racecraft/TireForceModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
struct BrakeHydraulicParameters final
{
    double MaximumLinePressureBar = 120.0;
    double MaximumPedalInput = 1.0;
    double MasterCylinderAreaCm2 = 3.14;
    double FrontCaliperAreaCm2 = 22.0;
    double RearCaliperAreaCm2 = 16.0;
    double DiscEffectiveRadiusCm = 14.0;
    double PadFriction = 0.42;
    double FrontBiasFraction = 0.62;
    double PressureRisePerSecondBar = 550.0;
    double PressureReleasePerSecondBar = 850.0;
    double BoosterMultiplier = 2.0;
    double MinimumOperatingTemperatureC = -10.0;
    double MaximumOperatingTemperatureC = 950.0;
    double FadeOnsetTemperatureC = 540.0;
    double FadeCriticalTemperatureC = 810.0;
    double MaximumPadTemperatureRiseCPerSecond = 45.0;
    double BrakeCoolingTimeConstantSeconds = 28.0;
    double AbsSlipThreshold = -0.17;
    double AbsReleaseSlipThreshold = -0.22;
    double AbsHoldSlipThreshold = -0.12;
    double AbsReapplyFractionPerSecond = 5.0;
    double AbsReleaseFractionPerSecond = 16.0;
    double HandbrakeMaximumTorqueNm = 1650.0;
    double MaximumSingleWheelTorqueNm = 9000.0;
};
struct BrakeWheelHydraulics final
{
    double LinePressureBar = 0.0;
    double TorqueNm = 0.0;
    double TemperatureC = 30.0;
    double FadeFraction = 0.0;
    double AbsDutyFraction = 1.0;
    double PadWearFraction = 0.0;
    double DissipatedEnergyJ = 0.0;
    bool AbsCycling = false;
    bool WheelLocked = false;
    bool Overheated = false;
};
struct BrakeHydraulicInput final
{
    double PedalFraction = 0.0;
    double ParkingBrakeFraction = 0.0;
    double ForwardSpeedCmS = 0.0;
    double AmbientTemperatureC = 20.0;
    double DeltaSeconds = 0.0;
    std::array<double, PhysicalWheelCount> WheelSlipRatios{};
    std::array<double, PhysicalWheelCount> WheelRotationsRadS{};
    std::array<double, PhysicalWheelCount> WheelNormalLoadsN{};
    std::array<double, PhysicalWheelCount> DiscTemperaturesC{};
    std::array<bool, PhysicalWheelCount> ContactVerified{};
    bool OriginalEngineWheelSignals = false;
    bool AbsEnabled = true;
    bool HandbrakeEnabled = true;
    bool VehiclePhysicsReady = false;
};
struct BrakeHydraulicOutput final
{
    std::array<BrakeWheelHydraulics, PhysicalWheelCount> Wheels{};
    double FrontBrakeTorqueNm = 0.0;
    double RearBrakeTorqueNm = 0.0;
    double TotalCommandedTorqueNm = 0.0;
    double MeanLinePressureBar = 0.0;
    double PeakTemperatureC = 0.0;
    double MeanFadeFraction = 0.0;
    double MeanAbsDutyFraction = 0.0;
    double HeatEnergyJ = 0.0;
    int AbsActiveWheels = 0;
    int LockedWheels = 0;
    int ContactWheels = 0;
    bool RealHydraulicHardwareVerified = false;
    bool OutputLimitedByGrip = false;
    bool SourceOnly = true;
    bool Valid = false;
};
inline bool ValidBrakeHydraulicParameters(
    BrakeHydraulicParameters s) noexcept
{
    return Finite(s.MaximumLinePressureBar) &&
        s.MaximumLinePressureBar > 0.0 &&
        Finite(s.MasterCylinderAreaCm2) &&
        s.MasterCylinderAreaCm2 > 0.0 &&
        Finite(s.FrontCaliperAreaCm2) &&
        s.FrontCaliperAreaCm2 > 0.0 &&
        Finite(s.RearCaliperAreaCm2) &&
        s.RearCaliperAreaCm2 > 0.0 &&
        Finite(s.DiscEffectiveRadiusCm) &&
        s.DiscEffectiveRadiusCm > 0.0 &&
        Finite(s.PadFriction) && s.PadFriction > 0.0 &&
        s.PadFriction <= 1.0 &&
        Finite(s.FrontBiasFraction) &&
        s.FrontBiasFraction >= 0.35 &&
        s.FrontBiasFraction <= 0.85 &&
        Finite(s.PressureRisePerSecondBar) &&
        s.PressureRisePerSecondBar > 0.0 &&
        Finite(s.PressureReleasePerSecondBar) &&
        s.PressureReleasePerSecondBar > 0.0 &&
        Finite(s.BoosterMultiplier) &&
        s.BoosterMultiplier >= 1.0 &&
        Finite(s.FadeOnsetTemperatureC) &&
        Finite(s.FadeCriticalTemperatureC) &&
        s.FadeCriticalTemperatureC >
            s.FadeOnsetTemperatureC &&
        Finite(s.BrakeCoolingTimeConstantSeconds) &&
        s.BrakeCoolingTimeConstantSeconds > 0.0 &&
        Finite(s.AbsSlipThreshold) &&
        s.AbsSlipThreshold < 0.0 &&
        Finite(s.AbsReleaseSlipThreshold) &&
        s.AbsReleaseSlipThreshold <
            s.AbsSlipThreshold &&
        Finite(s.AbsHoldSlipThreshold) &&
        s.AbsHoldSlipThreshold >
            s.AbsSlipThreshold &&
        Finite(s.AbsReapplyFractionPerSecond) &&
        s.AbsReapplyFractionPerSecond > 0.0 &&
        Finite(s.AbsReleaseFractionPerSecond) &&
        s.AbsReleaseFractionPerSecond > 0.0 &&
        Finite(s.HandbrakeMaximumTorqueNm) &&
        s.HandbrakeMaximumTorqueNm >= 0.0 &&
        Finite(s.MaximumSingleWheelTorqueNm) &&
        s.MaximumSingleWheelTorqueNm > 0.0;
}
inline bool ValidBrakeHydraulicInput(
    BrakeHydraulicInput s) noexcept
{
    if (!ValidFrameDelta(s.DeltaSeconds) ||
        !Finite(s.PedalFraction) ||
        !Finite(s.ParkingBrakeFraction) ||
        !Finite(s.ForwardSpeedCmS) ||
        !Finite(s.AmbientTemperatureC) ||
        s.PedalFraction < 0.0 ||
        s.PedalFraction > 1.0 ||
        s.ParkingBrakeFraction < 0.0 ||
        s.ParkingBrakeFraction > 1.0 ||
        std::abs(s.ForwardSpeedCmS) > MaximumPhysicalSpeedCmS ||
        !s.VehiclePhysicsReady ||
        !s.OriginalEngineWheelSignals)
        return false;
    for (std::size_t i = 0; i < PhysicalWheelCount; ++i)
    {
        if (!Finite(s.WheelSlipRatios[i]) ||
            !Finite(s.WheelRotationsRadS[i]) ||
            !Finite(s.WheelNormalLoadsN[i]) ||
            !Finite(s.DiscTemperaturesC[i]) ||
            std::abs(s.WheelSlipRatios[i]) > 5.0 ||
            s.WheelNormalLoadsN[i] < 0.0 ||
            s.WheelNormalLoadsN[i] > 300000.0 ||
            s.DiscTemperaturesC[i] < -80.0 ||
            s.DiscTemperaturesC[i] > 1200.0)
            return false;
    }
    return true;
}
inline double FadeFromTemperature(double discC,
                                  BrakeHydraulicParameters s) noexcept
{
    if (!Finite(discC)) return 1.0;
    return Smoothstep(s.FadeOnsetTemperatureC,
                      s.FadeCriticalTemperatureC, discC);
}
inline double BrakeTorqueFromPressure(
    double pressureBar, double caliperAreaCm2,
    BrakeHydraulicParameters s) noexcept
{
    if (!Finite(pressureBar) || pressureBar < 0.0 ||
        !Finite(caliperAreaCm2) || caliperAreaCm2 <= 0.0)
        return 0.0;
    const double barToPascal = 100000.0;
    const double cm2ToM2 = 0.0001;
    const double radiusM =
        s.DiscEffectiveRadiusCm / CentimetersPerMeter;
    const double clampForceN =
        pressureBar * barToPascal *
        caliperAreaCm2 * cm2ToM2;
    return 2.0 * clampForceN * s.PadFriction * radiusM;
}
inline BrakeWheelHydraulics ResolveBrakeWheel(
    BrakeWheelHydraulics previous, double targetBar,
    double padAreaCm2, double discC, double wheelSlip,
    double forwardSpeedCmS, double normalLoadN,
    double parkingBrake, double dt,
    bool allowAbs, bool physicallyContacting,
    BrakeHydraulicParameters parameters) noexcept
{
    BrakeWheelHydraulics out = previous;
    const double targetPressure = std::clamp(
        targetBar, 0.0, parameters.MaximumLinePressureBar);
    out.LinePressureBar = RateLimited(
        previous.LinePressureBar, targetPressure,
        parameters.PressureRisePerSecondBar,
        parameters.PressureReleasePerSecondBar, dt);
    out.FadeFraction =
        FadeFromTemperature(discC, parameters);
    out.TemperatureC = discC;
    const bool absNeedsRelease =
        allowAbs && physicallyContacting &&
        normalLoadN > 5.0 &&
        std::abs(forwardSpeedCmS) > 180.0 &&
        wheelSlip <= parameters.AbsReleaseSlipThreshold;
    if (absNeedsRelease)
    {
        out.AbsDutyFraction = std::max(
            0.15, previous.AbsDutyFraction -
            parameters.AbsReleaseFractionPerSecond * dt);
    }
    else if (wheelSlip >= parameters.AbsHoldSlipThreshold)
    {
        out.AbsDutyFraction = std::min(
            1.0, previous.AbsDutyFraction +
            parameters.AbsReapplyFractionPerSecond * dt);
    }
    out.AbsCycling = out.AbsDutyFraction < 0.99 &&
        allowAbs;
    out.WheelLocked = wheelSlip <= -0.8 &&
        std::abs(forwardSpeedCmS) > 300.0;
    const double rawNm = BrakeTorqueFromPressure(
        out.LinePressureBar, padAreaCm2, parameters);
    const double rearParking = parkingBrake *
        parameters.HandbrakeMaximumTorqueNm;
    out.TorqueNm = std::min(
        parameters.MaximumSingleWheelTorqueNm,
        (rawNm * (1.0 - 0.72 * out.FadeFraction) +
         rearParking) * out.AbsDutyFraction);
    out.Overheated =
        discC >= parameters.FadeCriticalTemperatureC;
    out.DissipatedEnergyJ +=
        out.TorqueNm * std::abs(
            forwardSpeedCmS / CentimetersPerMeter) * dt;
    out.PadWearFraction = Clamp01(
        out.PadWearFraction +
        0.0000000003 * out.DissipatedEnergyJ * dt);
    return out;
}
class FourWheelBrakeController final
{
public:
    bool Configure(BrakeHydraulicParameters settings) noexcept
    {
        if (!ValidBrakeHydraulicParameters(settings)) return false;
        Parameters = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Wheels.fill({});
        Last = {};
        QualifiedFrames = 0;
        RejectedFrames = 0;
        TotalBrakeEnergyJ = 0.0;
        MaxFade = 0.0;
        AbsEvents = 0;
    }
    BrakeHydraulicOutput Step(BrakeHydraulicInput input) noexcept
    {
        BrakeHydraulicOutput out;
        if (!ValidBrakeHydraulicInput(input))
        {
            ++RejectedFrames;
            Last = out;
            return out;
        }
        for (std::size_t i = 0; i < PhysicalWheelCount; ++i)
        {
            const bool front = i < 2;
            const double bias = front ?
                Parameters.FrontBiasFraction :
                1.0 - Parameters.FrontBiasFraction;
            const double linePressure =
                Parameters.MaximumLinePressureBar *
                input.PedalFraction * bias *
                Parameters.BoosterMultiplier;
            const double rearHandbrake = !front &&
                input.HandbrakeEnabled ?
                input.ParkingBrakeFraction : 0.0;
            const double caliperArea = front ?
                Parameters.FrontCaliperAreaCm2 :
                Parameters.RearCaliperAreaCm2;
            Wheels[i] = ResolveBrakeWheel(
                Wheels[i], linePressure, caliperArea,
                input.DiscTemperaturesC[i],
                input.WheelSlipRatios[i],
                input.ForwardSpeedCmS,
                input.WheelNormalLoadsN[i],
                rearHandbrake, input.DeltaSeconds,
                input.AbsEnabled, input.ContactVerified[i],
                Parameters);
            out.Wheels[i] = Wheels[i];
            out.TotalCommandedTorqueNm += Wheels[i].TorqueNm;
            out.MeanLinePressureBar += Wheels[i].LinePressureBar;
            out.PeakTemperatureC = std::max(
                out.PeakTemperatureC, Wheels[i].TemperatureC);
            out.MeanFadeFraction += Wheels[i].FadeFraction;
            out.MeanAbsDutyFraction += Wheels[i].AbsDutyFraction;
            if (front) out.FrontBrakeTorqueNm += Wheels[i].TorqueNm;
            else out.RearBrakeTorqueNm += Wheels[i].TorqueNm;
            out.AbsActiveWheels += Wheels[i].AbsCycling ? 1 : 0;
            out.LockedWheels += Wheels[i].WheelLocked ? 1 : 0;
            out.ContactWheels +=
                input.ContactVerified[i] ? 1 : 0;
            out.HeatEnergyJ +=
                Wheels[i].DissipatedEnergyJ;
            MaxFade = std::max(MaxFade, Wheels[i].FadeFraction);
            if (Wheels[i].AbsCycling) ++AbsEvents;
        }
        out.MeanLinePressureBar *= 0.25;
        out.MeanFadeFraction *= 0.25;
        out.MeanAbsDutyFraction *= 0.25;
        out.RealHydraulicHardwareVerified = false;
        out.OutputLimitedByGrip = out.AbsActiveWheels > 0 ||
            out.LockedWheels > 0;
        out.SourceOnly = true;
        out.Valid = Finite(out.TotalCommandedTorqueNm);
        Last = out;
        ++QualifiedFrames;
        TotalBrakeEnergyJ += out.HeatEnergyJ;
        return out;
    }
    BrakeHydraulicOutput Snapshot() const noexcept { return Last; }
    std::uint64_t Frames() const noexcept { return QualifiedFrames; }
    std::uint64_t Rejects() const noexcept { return RejectedFrames; }
    std::uint64_t AbsSamples() const noexcept { return AbsEvents; }
    double MaximumFadeFraction() const noexcept { return MaxFade; }
    double AccumulatedBrakeEnergyJ() const noexcept
    {
        return TotalBrakeEnergyJ;
    }
private:
    BrakeHydraulicParameters Parameters{};
    std::array<BrakeWheelHydraulics, PhysicalWheelCount> Wheels{};
    BrakeHydraulicOutput Last{};
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t AbsEvents = 0;
    double TotalBrakeEnergyJ = 0.0;
    double MaxFade = 0.0;
};
struct BrakeBiasReview final
{
    double FrontTorqueFraction = 0.0;
    double RearTorqueFraction = 0.0;
    double PeakDiscTemperatureC = 0.0;
    double MeanPadFade = 0.0;
    double AbsInterventionFraction = 0.0;
    double StoppingDistanceEstimateCm = 0.0;
    bool FrontBiased = false;
    bool RearBiased = false;
    bool ThermalCritical = false;
    bool Valid = false;
};
inline BrakeBiasReview EvaluateBrakeBalance(
    BrakeHydraulicOutput output,
    double currentCmS, double estimatedGripG) noexcept
{
    BrakeBiasReview result;
    if (!output.Valid || !Finite(currentCmS) ||
        !Finite(estimatedGripG) || currentCmS < 0.0 ||
        estimatedGripG <= 0.0) return result;
    const double sum = output.FrontBrakeTorqueNm +
        output.RearBrakeTorqueNm;
    result.FrontTorqueFraction =
        SafeDivision(output.FrontBrakeTorqueNm, sum);
    result.RearTorqueFraction =
        SafeDivision(output.RearBrakeTorqueNm, sum);
    result.PeakDiscTemperatureC =
        output.PeakTemperatureC;
    result.MeanPadFade =
        output.MeanFadeFraction;
    result.AbsInterventionFraction =
        output.AbsActiveWheels / 4.0;
    const BrakeEnvelope stop = StoppingEnvelope(
        currentCmS, 0.0,
        estimatedGripG * GravityCmS2, 0.20);
    result.StoppingDistanceEstimateCm =
        stop.Valid ? stop.DistanceCm : 0.0;
    result.FrontBiased =
        result.FrontTorqueFraction > 0.70;
    result.RearBiased =
        result.RearTorqueFraction > 0.47;
    result.ThermalCritical =
        result.PeakDiscTemperatureC > 800.0;
    result.Valid = stop.Valid;
    return result;
}
} // namespace latebrake::racecraft
