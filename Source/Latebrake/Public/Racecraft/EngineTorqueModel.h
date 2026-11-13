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
inline constexpr int TorqueCurveMaximumPoints = 16;
struct TorqueCurvePoint final
{
    double Rpm = 0.0;
    double TorqueNm = 0.0;
    double EfficiencyFraction = 0.0;
};
struct EnginePowerParameters final
{
    std::array<TorqueCurvePoint, TorqueCurveMaximumPoints> Curve{};
    int CurvePoints = 0;
    double IdleRpm = 850.0;
    double RedlineRpm = 8500.0;
    double RevLimiterRpm = 8800.0;
    double StallingRpm = 450.0;
    double MinimumStartingRpm = 750.0;
    double MaximumTorqueNm = 800.0;
    double RotationalInertiaKgM2 = 0.28;
    double EngineFrictionNm = 23.0;
    double EnginePumpingLossNm = 15.0;
    double FrictionSlopeNmPer1000Rpm = 3.8;
    double ThrottleRisePerSecond = 4.0;
    double ThrottleFallPerSecond = 7.0;
    double EngineBrakingNm = 85.0;
    double TurboSpoolSeconds = 0.55;
    double TurboBoostMultiplier = 1.0;
    double TurboMaximumOverboostFraction = 0.18;
    double TemperatureOptimumC = 95.0;
    double TemperatureDerateStartC = 115.0;
    double TemperatureCriticalC = 145.0;
    double ColdDerateStartC = 35.0;
    double FuelCutThresholdFraction = 0.001;
    double MaximumOutputPowerKw = 800.0;
    double MaximumRpmDerivativePerSecond = 150000.0;
    double FuelDensityKgPerLiter = 0.745;
    double FuelTankCapacityLiters = 55.0;
    double BrakeSpecificFuelConsumptionKgPerKwh = 0.29;
};
struct EngineState final
{
    double Rpm = 0.0;
    double ThrottleOpening = 0.0;
    double TurboSpoolFraction = 0.0;
    double TemperatureC = 70.0;
    double FuelFraction = 1.0;
    double OutputTorqueNm = 0.0;
    double OutputPowerKw = 0.0;
    double BrakeTorqueNm = 0.0;
    double AccumulatedFuelUsedLiters = 0.0;
    double LimiterSeconds = 0.0;
    double UnderPowerSeconds = 0.0;
    double OverTemperatureSeconds = 0.0;
    bool EngineRunning = false;
    bool OnLimiter = false;
    bool TorqueAvailable = false;
};
struct EngineFrameInput final
{
    double PedalThrottle = 0.0;
    double ClutchCouplingFraction = 0.0;
    double LoadedEngineTorqueNm = 0.0;
    double ExternalEngineSpeedRpm = 0.0;
    double AmbientTemperatureC = 25.0;
    double MeasuredEngineTemperatureC = 95.0;
    double MeasuredFuelFraction = 1.0;
    double DeltaSeconds = 0.0;
    bool IgnitionEnabled = true;
    bool OriginalEngineObservation = false;
    bool EnginePhysicalReadoutAvailable = false;
};
struct EngineFrameOutput final
{
    double TorqueNm = 0.0;
    double EffectiveTorqueNm = 0.0;
    double BrakeTorqueNm = 0.0;
    double Rpm = 0.0;
    double PowerKw = 0.0;
    double FuelUseLiters = 0.0;
    double FuelUseLitersPerHour = 0.0;
    double ThermalDerateFraction = 0.0;
    double TurboMultiplier = 1.0;
    double ThrottleResponse = 0.0;
    double MechanicalLossNm = 0.0;
    double RotationalAccelerationRadS2 = 0.0;
    double EfficiencyFraction = 0.0;
    bool RevLimiterActive = false;
    bool FuelStarved = false;
    bool OverTemperature = false;
    bool Stalled = false;
    bool Running = false;
    bool OriginalMeasuredRpm = false;
    bool EngineAuthority = false;
    bool Valid = false;
};
inline EnginePowerParameters DefaultTouringEngine() noexcept
{
    EnginePowerParameters p;
    p.Curve = {{
        {800.0,125.0,0.22},
        {1500.0,175.0,0.27},
        {2500.0,225.0,0.31},
        {3500.0,260.0,0.33},
        {4500.0,275.0,0.34},
        {5500.0,267.0,0.31},
        {6500.0,244.0,0.29},
        {7500.0,205.0,0.26},
        {8500.0,155.0,0.23}
    }};
    p.CurvePoints = 9;
    return p;
}
inline bool ValidEngineParameters(EnginePowerParameters s) noexcept
{
    if (s.CurvePoints < 2 ||
        s.CurvePoints > TorqueCurveMaximumPoints ||
        !Finite(s.IdleRpm) || s.IdleRpm <= 0.0 ||
        !Finite(s.RedlineRpm) || s.RedlineRpm <= s.IdleRpm ||
        !Finite(s.RevLimiterRpm) ||
        s.RevLimiterRpm <= s.RedlineRpm ||
        !Finite(s.StallingRpm) ||
        s.StallingRpm >= s.IdleRpm ||
        !Finite(s.MaximumTorqueNm) ||
        s.MaximumTorqueNm <= 0.0 ||
        !Finite(s.RotationalInertiaKgM2) ||
        s.RotationalInertiaKgM2 <= 0.0 ||
        !Finite(s.EngineFrictionNm) ||
        s.EngineFrictionNm < 0.0 ||
        !Finite(s.EnginePumpingLossNm) ||
        s.EnginePumpingLossNm < 0.0 ||
        !Finite(s.ThrottleRisePerSecond) ||
        s.ThrottleRisePerSecond <= 0.0 ||
        !Finite(s.ThrottleFallPerSecond) ||
        s.ThrottleFallPerSecond <= 0.0 ||
        !Finite(s.TurboSpoolSeconds) ||
        s.TurboSpoolSeconds <= 0.0 ||
        !Finite(s.TurboBoostMultiplier) ||
        s.TurboBoostMultiplier < 0.0 ||
        !Finite(s.TemperatureOptimumC) ||
        !Finite(s.TemperatureDerateStartC) ||
        !Finite(s.TemperatureCriticalC) ||
        s.TemperatureCriticalC <= s.TemperatureDerateStartC ||
        !Finite(s.FuelTankCapacityLiters) ||
        s.FuelTankCapacityLiters <= 0.0 ||
        !Finite(s.BrakeSpecificFuelConsumptionKgPerKwh) ||
        s.BrakeSpecificFuelConsumptionKgPerKwh <= 0.0)
        return false;
    double previousRpm = -1.0;
    for (int i = 0; i < s.CurvePoints; ++i)
    {
        const TorqueCurvePoint point =
            s.Curve[static_cast<std::size_t>(i)];
        if (!Finite(point.Rpm) || !Finite(point.TorqueNm) ||
            !Finite(point.EfficiencyFraction) ||
            point.Rpm <= previousRpm ||
            point.TorqueNm < 0.0 ||
            point.TorqueNm > s.MaximumTorqueNm ||
            point.EfficiencyFraction < 0.0 ||
            point.EfficiencyFraction > 1.0)
            return false;
        previousRpm = point.Rpm;
    }
    return true;
}
inline TorqueCurvePoint SampleTorqueCurve(
    const EnginePowerParameters& p, double rpm) noexcept
{
    TorqueCurvePoint out;
    if (!ValidEngineParameters(p) ||
        !Finite(rpm) || rpm < 0.0)
        return out;
    if (rpm <= p.Curve[0].Rpm) return p.Curve[0];
    for (int i = 1; i < p.CurvePoints; ++i)
    {
        const auto before =
            p.Curve[static_cast<std::size_t>(i - 1)];
        const auto after =
            p.Curve[static_cast<std::size_t>(i)];
        if (rpm > after.Rpm) continue;
        const double t = Clamp01(SafeDivision(
            rpm - before.Rpm,
            after.Rpm - before.Rpm));
        out.Rpm = rpm;
        out.TorqueNm = Lerp(before.TorqueNm, after.TorqueNm, t);
        out.EfficiencyFraction = Lerp(
            before.EfficiencyFraction,
            after.EfficiencyFraction, t);
        return out;
    }
    return p.Curve[static_cast<std::size_t>(
        p.CurvePoints - 1)];
}
inline double EngineThermalDerate(
    double temperatureC, EnginePowerParameters s) noexcept
{
    if (!Finite(temperatureC)) return 0.0;
    const double hot = Smoothstep(
        s.TemperatureDerateStartC,
        s.TemperatureCriticalC, temperatureC);
    const double cold = 1.0 - Smoothstep(
        s.ColdDerateStartC,
        s.TemperatureOptimumC, temperatureC);
    return std::clamp(
        1.0 - 0.75 * hot - 0.25 * cold,
        0.12, 1.0);
}
inline double FrictionTorqueNm(
    double rpm, EnginePowerParameters s) noexcept
{
    if (!Finite(rpm) || rpm < 0.0) return 0.0;
    return s.EngineFrictionNm +
        s.EnginePumpingLossNm +
        s.FrictionSlopeNmPer1000Rpm *
        rpm / 1000.0;
}
inline EngineFrameOutput ResolveEngineOutput(
    EngineFrameInput input, EngineState state,
    const EnginePowerParameters& params) noexcept
{
    EngineFrameOutput out;
    if (!ValidEngineParameters(params) ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        !Finite(input.PedalThrottle) ||
        input.PedalThrottle < 0.0 ||
        input.PedalThrottle > 1.0 ||
        !Finite(input.ClutchCouplingFraction) ||
        input.ClutchCouplingFraction < 0.0 ||
        input.ClutchCouplingFraction > 1.0 ||
        !Finite(input.LoadedEngineTorqueNm) ||
        !Finite(input.ExternalEngineSpeedRpm) ||
        !Finite(input.AmbientTemperatureC) ||
        !Finite(input.MeasuredEngineTemperatureC) ||
        !Finite(input.MeasuredFuelFraction) ||
        input.MeasuredFuelFraction < 0.0 ||
        input.MeasuredFuelFraction > 1.0 ||
        !input.OriginalEngineObservation ||
        !input.EnginePhysicalReadoutAvailable)
        return out;
    out.Rpm = input.ExternalEngineSpeedRpm;
    out.OriginalMeasuredRpm = true;
    out.ThermalDerateFraction =
        EngineThermalDerate(input.MeasuredEngineTemperatureC,
                            params);
    out.ThrottleResponse = RateLimited(
        state.ThrottleOpening, input.PedalThrottle,
        params.ThrottleRisePerSecond,
        params.ThrottleFallPerSecond,
        input.DeltaSeconds);
    const double turboTarget =
        input.PedalThrottle >= 0.2 ? 1.0 : 0.0;
    const double spool = ExponentialResponse(
        state.TurboSpoolFraction,
        turboTarget,
        params.TurboSpoolSeconds,
        input.DeltaSeconds);
    out.TurboMultiplier =
        1.0 + params.TurboMaximumOverboostFraction *
        spool * params.TurboBoostMultiplier;
    out.RevLimiterActive =
        out.Rpm >= params.RevLimiterRpm;
    out.FuelStarved =
        input.MeasuredFuelFraction <=
            params.FuelCutThresholdFraction;
    out.Stalled =
        !input.IgnitionEnabled ||
        out.Rpm < params.StallingRpm;
    out.Running = !out.Stalled && !out.FuelStarved;
    const TorqueCurvePoint mapped =
        SampleTorqueCurve(params, out.Rpm);
    out.MechanicalLossNm =
        FrictionTorqueNm(out.Rpm, params);
    out.TorqueNm = out.Running && !out.RevLimiterActive ?
        mapped.TorqueNm * out.ThrottleResponse *
        out.ThermalDerateFraction *
        out.TurboMultiplier : 0.0;
    out.BrakeTorqueNm = input.PedalThrottle < 0.1 ?
        params.EngineBrakingNm *
        Clamp01(out.Rpm / params.RedlineRpm) : 0.0;
    out.EffectiveTorqueNm =
        out.TorqueNm -
        out.MechanicalLossNm -
        out.BrakeTorqueNm;
    out.RotationalAccelerationRadS2 = SafeDivision(
        out.EffectiveTorqueNm -
        input.LoadedEngineTorqueNm *
        input.ClutchCouplingFraction,
        params.RotationalInertiaKgM2);
    out.PowerKw = out.TorqueNm *
        out.Rpm * TwoPi / 60.0 / 1000.0;
    out.PowerKw = std::min(
        params.MaximumOutputPowerKw, out.PowerKw);
    out.EfficiencyFraction = mapped.EfficiencyFraction;
    out.FuelUseLitersPerHour = out.Running ?
        out.PowerKw *
        params.BrakeSpecificFuelConsumptionKgPerKwh /
        params.FuelDensityKgPerLiter : 0.0;
    out.FuelUseLiters =
        out.FuelUseLitersPerHour *
        input.DeltaSeconds / 3600.0;
    out.OverTemperature =
        input.MeasuredEngineTemperatureC >=
            params.TemperatureCriticalC;
    out.EngineAuthority = false;
    out.Valid = Finite(out.PowerKw) &&
        Finite(out.EffectiveTorqueNm) &&
        Finite(out.FuelUseLiters) &&
        Finite(out.RotationalAccelerationRadS2);
    return out;
}
class EngineTorqueEvaluator final
{
public:
    bool Configure(EnginePowerParameters settings) noexcept
    {
        if (!ValidEngineParameters(settings)) return false;
        Parameters = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        State = {};
        Output = {};
        Frames = 0;
        Rejected = 0;
        FuelUsedLiters = 0.0;
        PeakOutputKw = 0.0;
        TotalLimiterSeconds = 0.0;
        ThermalStressSeconds = 0.0;
    }
    EngineFrameOutput Step(EngineFrameInput input) noexcept
    {
        Output = ResolveEngineOutput(
            input, State, Parameters);
        if (!Output.Valid)
        {
            ++Rejected;
            return Output;
        }
        State.Rpm = Output.Rpm;
        State.ThrottleOpening = Output.ThrottleResponse;
        State.TurboSpoolFraction = Clamp01(
            SafeDivision(
                Output.TurboMultiplier - 1.0,
                std::max(PhysicsEpsilon,
                    Parameters.TurboMaximumOverboostFraction *
                    Parameters.TurboBoostMultiplier)));
        State.TemperatureC = input.MeasuredEngineTemperatureC;
        State.FuelFraction = input.MeasuredFuelFraction;
        State.OutputTorqueNm = Output.TorqueNm;
        State.OutputPowerKw = Output.PowerKw;
        State.BrakeTorqueNm = Output.BrakeTorqueNm;
        State.EngineRunning = Output.Running;
        State.OnLimiter = Output.RevLimiterActive;
        State.TorqueAvailable = Output.TorqueNm > 0.0;
        State.AccumulatedFuelUsedLiters +=
            Output.FuelUseLiters;
        State.LimiterSeconds +=
            Output.RevLimiterActive ? input.DeltaSeconds : 0.0;
        State.OverTemperatureSeconds +=
            Output.OverTemperature ? input.DeltaSeconds : 0.0;
        if (Output.TorqueNm > 0.0)
            State.UnderPowerSeconds += input.DeltaSeconds;
        FuelUsedLiters += Output.FuelUseLiters;
        PeakOutputKw = std::max(PeakOutputKw, Output.PowerKw);
        TotalLimiterSeconds = State.LimiterSeconds;
        ThermalStressSeconds = State.OverTemperatureSeconds;
        ++Frames;
        return Output;
    }
    EngineState Snapshot() const noexcept { return State; }
    EngineFrameOutput LastOutput() const noexcept { return Output; }
    std::uint64_t QualifiedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    double ConsumptionLiters() const noexcept { return FuelUsedLiters; }
    double MaximumPowerKw() const noexcept { return PeakOutputKw; }
    double SecondsOnLimiter() const noexcept
    {
        return TotalLimiterSeconds;
    }
    double SecondsOverTemperature() const noexcept
    {
        return ThermalStressSeconds;
    }
private:
    EnginePowerParameters Parameters = DefaultTouringEngine();
    EngineState State{};
    EngineFrameOutput Output{};
    std::uint64_t Frames = 0;
    std::uint64_t Rejected = 0;
    double FuelUsedLiters = 0.0;
    double PeakOutputKw = 0.0;
    double TotalLimiterSeconds = 0.0;
    double ThermalStressSeconds = 0.0;
};
} // namespace latebrake::racecraft
