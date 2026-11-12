#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Engine-independent rotor heat/cooling estimate for future telemetry.
// This does NOT modify Chaos brake force, service/handbrake authority,
// existing ArcadeBrakeHeat tuning, or any driver control.
struct PortableBrakeThermalFrame final
{
    double StepSeconds = 1.0 / 60.0;
    double RotorCelsius = 25.0;
    double AmbientCelsius = 25.0;
    double BrakingHeatKilojoules = 0.0;
    double EffectiveHeatCapacityKilojoulesPerCelsius = 3.0;
    double PassiveCoolingRatePerSecond = 0.015;
    double AirflowCoolingRatePerSecondAt100Mps = 0.10;
    double AirSpeedMetersPerSecond = 0.0;
};

struct PortableBrakeThermalResult final
{
    bool Valid = false;
    double PreviousCelsius = 0.0;
    double HeatAddedCelsius = 0.0;
    double HeatAfterInputCelsius = 0.0;
    double CoolingRatePerSecond = 0.0;
    double NextCelsius = 0.0;
    double EstimatedPadEfficiency = 0.0;
    bool HeatClamped = false;
    bool EstimatedSevereFade = false;
};

inline bool ValidPortableBrakeThermalFrame(
    PortableBrakeThermalFrame sample) noexcept
{
    const auto finite = [](double value) noexcept
    {
        return std::isfinite(value);
    };
    return finite(sample.StepSeconds) &&
        sample.StepSeconds > 0.0 && sample.StepSeconds <= 0.50 &&
        finite(sample.AmbientCelsius) &&
        sample.AmbientCelsius >= -40.0 &&
        sample.AmbientCelsius <= 100.0 &&
        finite(sample.RotorCelsius) &&
        sample.RotorCelsius >= sample.AmbientCelsius &&
        sample.RotorCelsius <= 1200.0 &&
        finite(sample.BrakingHeatKilojoules) &&
        sample.BrakingHeatKilojoules >= 0.0 &&
        sample.BrakingHeatKilojoules <= 3000.0 &&
        finite(sample.EffectiveHeatCapacityKilojoulesPerCelsius) &&
        sample.EffectiveHeatCapacityKilojoulesPerCelsius >= 0.1 &&
        sample.EffectiveHeatCapacityKilojoulesPerCelsius <= 1000.0 &&
        finite(sample.PassiveCoolingRatePerSecond) &&
        sample.PassiveCoolingRatePerSecond >= 0.0 &&
        sample.PassiveCoolingRatePerSecond <= 2.0 &&
        finite(sample.AirflowCoolingRatePerSecondAt100Mps) &&
        sample.AirflowCoolingRatePerSecondAt100Mps >= 0.0 &&
        sample.AirflowCoolingRatePerSecondAt100Mps <= 2.0 &&
        finite(sample.AirSpeedMetersPerSecond) &&
        sample.AirSpeedMetersPerSecond >= 0.0 &&
        sample.AirSpeedMetersPerSecond <= 120.0;
}

inline PortableBrakeThermalResult EstimatePortableBrakeThermalStep(
    PortableBrakeThermalFrame sample) noexcept
{
    if (!ValidPortableBrakeThermalFrame(sample))
    {
        return {};
    }
    PortableBrakeThermalResult result;
    result.Valid = true;
    result.PreviousCelsius = sample.RotorCelsius;
    result.HeatAddedCelsius = sample.BrakingHeatKilojoules /
        sample.EffectiveHeatCapacityKilojoulesPerCelsius;
    const double hot = sample.RotorCelsius + result.HeatAddedCelsius;
    result.HeatClamped = hot > 1200.0;
    result.HeatAfterInputCelsius = std::min(1200.0, hot);

    // Exponential decay keeps pure cooling independent of frame
    // subdivision: N steps of dt/N match one step of dt (no heating).
    const double cooling = sample.PassiveCoolingRatePerSecond +
        sample.AirflowCoolingRatePerSecondAt100Mps *
        (sample.AirSpeedMetersPerSecond / 100.0);
    result.CoolingRatePerSecond = cooling;
    result.NextCelsius = sample.AmbientCelsius +
        (result.HeatAfterInputCelsius - sample.AmbientCelsius) *
        std::exp(-cooling * sample.StepSeconds);
    result.NextCelsius = std::clamp(result.NextCelsius,
        sample.AmbientCelsius, 1200.0);
    if (!std::isfinite(result.NextCelsius))
    {
        return {};
    }

    // Bounded, illustrative pad-efficiency estimate, *not* a
    // certification of real compounds, heat flux or fade curve.
    constexpr double FadeStartsCelsius = 450.0;
    constexpr double FadeEndsCelsius = 950.0;
    constexpr double MinimumEfficiency = 0.45;
    const double fadeFraction = std::clamp(
        (result.NextCelsius - FadeStartsCelsius) /
        (FadeEndsCelsius - FadeStartsCelsius), 0.0, 1.0);
    result.EstimatedPadEfficiency =
        1.0 - fadeFraction * (1.0 - MinimumEfficiency);
    result.EstimatedSevereFade = result.NextCelsius >= FadeEndsCelsius;
    return result;
}
} // namespace latebrake
