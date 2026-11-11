#pragma once

#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryLapAnalysis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class HealthCondition : std::uint8_t
{
    Unavailable,
    Healthy,
    Watch,
    Degraded,
    Critical,
    Unverified
};

enum class HealthSystem : std::uint8_t
{
    Engine,
    Brakes,
    Tires,
    Suspension,
    Chassis,
    Fuel,
    Cooling,
    Electronics,
    All
};

enum class ServicePriority : std::uint8_t
{
    None,
    Routine,
    Scheduled,
    NextSafeOpportunity,
    UrgentInspection,
    StopIfSafe
};

struct VehicleHealthSettings final
{
    double EngineWarmThresholdC = 100.0;
    double EngineHotThresholdC = 120.0;
    double EngineCriticalThresholdC = 140.0;
    double BrakeWarmThresholdC = 300.0;
    double BrakeHotThresholdC = 650.0;
    double BrakeCriticalThresholdC = 900.0;
    double TireHotThresholdC = 120.0;
    double TireCriticalThresholdC = 160.0;
    double TireWearWatch = 0.55;
    double TireWearCritical = 0.90;
    double DamageWatch = 0.15;
    double DamageCritical = 0.65;
    double FuelLowFraction = 0.20;
    double FuelCriticalFraction = 0.06;
    double MinimumGripFactor = 0.25;
    double StableWarningSeconds = 0.25;
    double RecoveryCoolSeconds = 0.80;
    double MaximumObservedSeconds = 7200.0;
    bool RequireActualNativePhysics = true;
    bool IgnoreUnloadedWheelTemperatures = true;
};

inline bool ValidVehicleHealthSettings(
    const VehicleHealthSettings& s) noexcept
{
    return Finite(s.EngineWarmThresholdC) &&
           Finite(s.EngineHotThresholdC) &&
           Finite(s.EngineCriticalThresholdC) &&
           s.EngineWarmThresholdC < s.EngineHotThresholdC &&
           s.EngineHotThresholdC < s.EngineCriticalThresholdC &&
           Finite(s.BrakeWarmThresholdC) &&
           Finite(s.BrakeHotThresholdC) &&
           Finite(s.BrakeCriticalThresholdC) &&
           s.BrakeWarmThresholdC < s.BrakeHotThresholdC &&
           s.BrakeHotThresholdC < s.BrakeCriticalThresholdC &&
           Finite(s.TireHotThresholdC) &&
           Finite(s.TireCriticalThresholdC) &&
           s.TireHotThresholdC < s.TireCriticalThresholdC &&
           Finite(s.TireWearWatch) && s.TireWearWatch >= 0.0 &&
           Finite(s.TireWearCritical) &&
           s.TireWearCritical > s.TireWearWatch &&
           s.TireWearCritical <= 1.0 &&
           Finite(s.DamageWatch) && s.DamageWatch >= 0.0 &&
           Finite(s.DamageCritical) &&
           s.DamageCritical > s.DamageWatch &&
           s.DamageCritical <= 1.0 &&
           Finite(s.FuelLowFraction) && s.FuelLowFraction >= 0.0 &&
           Finite(s.FuelCriticalFraction) &&
           s.FuelCriticalFraction < s.FuelLowFraction &&
           Finite(s.MinimumGripFactor) &&
           s.MinimumGripFactor > 0.0 &&
           Finite(s.StableWarningSeconds) &&
           s.StableWarningSeconds > 0.0 &&
           Finite(s.RecoveryCoolSeconds) &&
           s.RecoveryCoolSeconds > 0.0 &&
           Finite(s.MaximumObservedSeconds) &&
           s.MaximumObservedSeconds > 0.0;
}

struct HealthAssessment final
{
    HealthSystem System = HealthSystem::All;
    HealthCondition Condition = HealthCondition::Unavailable;
    ServicePriority Service = ServicePriority::None;
    double ObservationSeconds = 0.0;
    double LatestValue = 0.0;
    double SmoothedValue = 0.0;
    double PeakValue = 0.0;
    double RelativeMarginToCritical = 0.0;
    double EstimatedTrendPerSecond = 0.0;
    double TimeToThresholdSeconds =
        std::numeric_limits<double>::infinity();
    double TimeInWarningSeconds = 0.0;
    double TimeInCriticalSeconds = 0.0;
    double Confidence = 0.0;
    std::uint64_t QualifiedSamples = 0;
    std::uint64_t RejectedSamples = 0;
    bool CurrentlyWarning = false;
    bool CurrentlyCritical = false;
    bool Verified = false;
};

inline ServicePriority HealthServicePriority(
    HealthCondition condition) noexcept
{
    switch (condition)
    {
    case HealthCondition::Healthy:
        return ServicePriority::Routine;
    case HealthCondition::Watch:
        return ServicePriority::Scheduled;
    case HealthCondition::Degraded:
        return ServicePriority::NextSafeOpportunity;
    case HealthCondition::Critical:
        return ServicePriority::StopIfSafe;
    case HealthCondition::Unverified:
        return ServicePriority::UrgentInspection;
    case HealthCondition::Unavailable:
        return ServicePriority::None;
    }
    return ServicePriority::None;
}

class HealthChannelMonitor final
{
public:
    bool Configure(
        HealthSystem system,
        double watchThreshold, double degradedThreshold,
        double criticalThreshold,
        bool higherIsWorse = true) noexcept
    {
        if (!Finite(watchThreshold) ||
            !Finite(degradedThreshold) ||
            !Finite(criticalThreshold) ||
            (higherIsWorse ?
                 !(watchThreshold < degradedThreshold &&
                   degradedThreshold < criticalThreshold) :
                 !(watchThreshold > degradedThreshold &&
                   degradedThreshold > criticalThreshold)))
        {
            return false;
        }
        System = system;
        Watch = watchThreshold;
        Degraded = degradedThreshold;
        Critical = criticalThreshold;
        HigherIsWorse = higherIsWorse;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Output = {};
        Output.System = System;
        ValueFilter.Reset();
        TrendFilter.Reset();
        PreviousValue = 0.0;
        HasPrevious = false;
        WarningStable = 0.0;
        CriticalStable = 0.0;
        RecoveryStable = 0.0;
    }

    bool Observe(
        double value, double dt, bool nativeVerified,
        double confidence) noexcept
    {
        if (!Finite(value) || !Finite(dt) ||
            !Finite(confidence) || dt <= 0.0 || dt > 0.5 ||
            confidence < 0.0 || confidence > 1.0)
        {
            Output.RejectedSamples =
                SaturatingIncrement(Output.RejectedSamples);
            return false;
        }
        Output.QualifiedSamples =
            SaturatingIncrement(Output.QualifiedSamples);
        Output.ObservationSeconds += dt;
        Output.LatestValue = value;
        const double filtered = ValueFilter.Observe(value, dt, 0.22);
        Output.SmoothedValue = filtered;
        Output.PeakValue = Output.QualifiedSamples == 1 ?
            value : HigherIsWorse ?
                std::max(Output.PeakValue, value) :
                std::min(Output.PeakValue, value);
        const double trend = HasPrevious ?
            (value - PreviousValue) / dt : 0.0;
        Output.EstimatedTrendPerSecond =
            TrendFilter.Observe(trend, dt, 0.80);
        Output.Confidence = confidence;
        Output.Verified = nativeVerified;
        const auto exceeds = [this](double v, double threshold) noexcept
        {
            return HigherIsWorse ? v >= threshold : v <= threshold;
        };
        const bool watch = exceeds(filtered, Watch);
        const bool degraded = exceeds(filtered, Degraded);
        const bool critical = exceeds(filtered, Critical);
        WarningStable = watch ? WarningStable + dt : 0.0;
        CriticalStable = critical ? CriticalStable + dt : 0.0;
        RecoveryStable = !watch ? RecoveryStable + dt : 0.0;
        Output.TimeInWarningSeconds += watch ? dt : 0.0;
        Output.TimeInCriticalSeconds += critical ? dt : 0.0;
        Output.CurrentlyWarning = watch;
        Output.CurrentlyCritical = critical;
        if (!nativeVerified || confidence < 0.5)
        {
            Output.Condition = HealthCondition::Unverified;
        }
        else if (critical && CriticalStable > 0.10)
        {
            Output.Condition = HealthCondition::Critical;
        }
        else if (degraded && WarningStable > 0.15)
        {
            Output.Condition = HealthCondition::Degraded;
        }
        else if (watch && WarningStable > 0.15)
        {
            Output.Condition = HealthCondition::Watch;
        }
        else if (RecoveryStable > 0.25 ||
                 Output.Condition == HealthCondition::Unavailable)
        {
            Output.Condition = HealthCondition::Healthy;
        }
        Output.Service =
            HealthServicePriority(Output.Condition);
        const double gap = HigherIsWorse ?
            Critical - filtered : filtered - Critical;
        Output.RelativeMarginToCritical =
            SafeRatio(gap, std::max(1.0, std::abs(Critical)));
        const double velocity = HigherIsWorse ?
            Output.EstimatedTrendPerSecond :
            -Output.EstimatedTrendPerSecond;
        Output.TimeToThresholdSeconds = velocity > Tiny &&
            gap > 0.0 ? gap / velocity :
            std::numeric_limits<double>::infinity();
        PreviousValue = value;
        HasPrevious = true;
        return true;
    }

    HealthAssessment Snapshot() const noexcept { return Output; }
    bool HasQualifiedSamples() const noexcept
    {
        return Output.QualifiedSamples > 0;
    }

private:
    HealthSystem System = HealthSystem::All;
    double Watch = 0.0;
    double Degraded = 0.0;
    double Critical = 0.0;
    bool HigherIsWorse = true;
    HealthAssessment Output{};
    ExponentialFilter ValueFilter;
    ExponentialFilter TrendFilter;
    double PreviousValue = 0.0;
    double WarningStable = 0.0;
    double CriticalStable = 0.0;
    double RecoveryStable = 0.0;
    bool HasPrevious = false;
};

struct VehicleHealthSnapshot final
{
    std::array<HealthAssessment, 9> Channels{};
    HealthCondition Overall = HealthCondition::Unavailable;
    ServicePriority HighestPriority = ServicePriority::None;
    double MaximumObservedDamageFraction = 0.0;
    double MaximumObservedTireWear = 0.0;
    double EstimatedTotalHeatLoad = 0.0;
    double TotalQualifiedSeconds = 0.0;
    double CombinedConfidence = 0.0;
    double FuelFraction = 0.0;
    double GripFactor = 0.0;
    double EngineTemperatureC = 0.0;
    double BrakeTemperatureC = 0.0;
    int WarningSystems = 0;
    int DegradedSystems = 0;
    int CriticalSystems = 0;
    int UnverifiedSystems = 0;
    bool AnyCritical = false;
    bool SensorsComplete = false;
    bool Qualified = false;
};

class VehicleHealthMonitor final
{
public:
    bool Configure(VehicleHealthSettings settings) noexcept
    {
        if (!ValidVehicleHealthSettings(settings))
        {
            return false;
        }
        Settings = settings;
        ConfigureChannels();
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        for (HealthChannelMonitor& channel : Channels)
        {
            channel.Reset();
        }
        Last = {};
        Accepted = 0;
        Rejected = 0;
        Session = 0;
        Actor = 0;
    }

    void Suspend() noexcept
    {
        Last = {};
    }

    bool Observe(
        const TelemetryFrame& frame,
        const GripTelemetry& grip,
        double dt) noexcept
    {
        if (!ValidVehicleHealthSettings(Settings) ||
            !Finite(dt) || dt <= 0.0 ||
            dt > MaximumFrameDeltaSeconds ||
            !ValidTelemetryDynamics(frame.Dynamics,
                                    TelemetryGateSettings{}) ||
            !ValidTelemetryWheels(frame.Wheels) ||
            (Settings.RequireActualNativePhysics &&
             (!frame.Race.EnginePhysicsReady ||
              !IsEngineObservation(frame.Identity.Trust))) ||
            frame.Identity.SessionToken == 0 ||
            frame.Identity.ActorToken == 0)
        {
            Rejected = SaturatingIncrement(Rejected);
            Suspend();
            return false;
        }
        if (Session != 0 &&
            (Session != frame.Identity.SessionToken ||
             Actor != frame.Identity.ActorToken))
        {
            Reset();
        }
        Session = frame.Identity.SessionToken;
        Actor = frame.Identity.ActorToken;
        const double maxWear = std::max({
            frame.Wheels.TireWearFraction[0],
            frame.Wheels.TireWearFraction[1],
            frame.Wheels.TireWearFraction[2],
            frame.Wheels.TireWearFraction[3]
        });
        const double temperature = WheelMeanTemperature(frame.Wheels);
        const double physicsConfidence =
            frame.Race.EnginePhysicsReady ? 1.0 : 0.0;
        const double gripConfidence =
            grip.Valid ? grip.Confidence : 0.0;
        const double values[9] = {
            frame.Dynamics.EngineTemperatureCelsius,
            frame.Dynamics.BrakeTemperatureCelsius,
            maxWear,
            frame.Dynamics.SuspensionCompressionCm,
            frame.Dynamics.ChassisDamageFraction,
            frame.Dynamics.FuelFraction,
            frame.Dynamics.EngineTemperatureCelsius -
                frame.Environment.AirTemperatureCelsius,
            grip.Valid ? grip.LoadWeightedGripFactor : 0.0,
            temperature
        };
        const double confidences[9] = {
            physicsConfidence,
            physicsConfidence,
            gripConfidence,
            physicsConfidence,
            physicsConfidence,
            physicsConfidence,
            physicsConfidence,
            gripConfidence,
            gripConfidence
        };
        VehicleHealthSnapshot out;
        out.SensorsComplete = grip.Valid &&
            grip.ContactFraction >= 0.75;
        out.CombinedConfidence = 1.0;
        out.Overall = HealthCondition::Healthy;
        for (std::size_t i = 0; i < Channels.size(); ++i)
        {
            const bool qualified = confidences[i] >= 0.5;
            if (!Channels[i].Observe(
                    values[i], dt, qualified, confidences[i]))
            {
                Rejected = SaturatingIncrement(Rejected);
                return false;
            }
            out.Channels[i] = Channels[i].Snapshot();
            out.CombinedConfidence = std::min(
                out.CombinedConfidence, confidences[i]);
            out.HighestPriority = static_cast<int>(
                out.Channels[i].Service) >
                    static_cast<int>(out.HighestPriority) ?
                        out.Channels[i].Service :
                        out.HighestPriority;
            switch (out.Channels[i].Condition)
            {
            case HealthCondition::Critical:
                ++out.CriticalSystems;
                break;
            case HealthCondition::Degraded:
                ++out.DegradedSystems;
                break;
            case HealthCondition::Watch:
                ++out.WarningSystems;
                break;
            case HealthCondition::Unverified:
                ++out.UnverifiedSystems;
                break;
            case HealthCondition::Healthy:
            case HealthCondition::Unavailable:
                break;
            }
            if (static_cast<int>(out.Channels[i].Condition) >
                static_cast<int>(out.Overall))
            {
                out.Overall = out.Channels[i].Condition;
            }
        }
        if (out.CriticalSystems)
        {
            out.Overall = HealthCondition::Critical;
        }
        else if (out.DegradedSystems)
        {
            out.Overall = HealthCondition::Degraded;
        }
        else if (out.WarningSystems)
        {
            out.Overall = HealthCondition::Watch;
        }
        else if (out.UnverifiedSystems)
        {
            out.Overall = HealthCondition::Unverified;
        }
        out.AnyCritical = out.CriticalSystems > 0;
        out.MaximumObservedDamageFraction = std::max(
            Last.MaximumObservedDamageFraction,
            frame.Dynamics.ChassisDamageFraction);
        out.MaximumObservedTireWear = std::max(
            Last.MaximumObservedTireWear, maxWear);
        out.EstimatedTotalHeatLoad = Last.EstimatedTotalHeatLoad +
            (std::max(0.0, frame.Dynamics.EngineTemperatureCelsius -
                            Settings.EngineWarmThresholdC) +
             std::max(0.0, frame.Dynamics.BrakeTemperatureCelsius -
                            Settings.BrakeWarmThresholdC)) * dt;
        out.TotalQualifiedSeconds = Last.TotalQualifiedSeconds + dt;
        out.FuelFraction = frame.Dynamics.FuelFraction;
        out.GripFactor = grip.Valid ?
            grip.LoadWeightedGripFactor : 0.0;
        out.EngineTemperatureC =
            frame.Dynamics.EngineTemperatureCelsius;
        out.BrakeTemperatureC =
            frame.Dynamics.BrakeTemperatureCelsius;
        out.Qualified = out.CombinedConfidence >= 0.5;
        Last = out;
        Accepted = SaturatingIncrement(Accepted);
        return true;
    }

    VehicleHealthSnapshot Snapshot() const noexcept { return Last; }
    HealthAssessment SystemHealth(HealthSystem system) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(system);
        return index < Channels.size() ?
            Channels[index].Snapshot() : HealthAssessment{};
    }
    bool AnyWarning() const noexcept
    {
        return Last.WarningSystems > 0 ||
               Last.DegradedSystems > 0 ||
               Last.CriticalSystems > 0;
    }
    bool ImmediateInspectionSuggested() const noexcept
    {
        return Last.AnyCritical;
    }
    std::uint64_t AcceptedSamples() const noexcept { return Accepted; }
    std::uint64_t RejectedSamples() const noexcept { return Rejected; }

private:
    void ConfigureChannels() noexcept
    {
        Channels[0].Configure(
            HealthSystem::Engine,
            Settings.EngineWarmThresholdC,
            Settings.EngineHotThresholdC,
            Settings.EngineCriticalThresholdC);
        Channels[1].Configure(
            HealthSystem::Brakes,
            Settings.BrakeWarmThresholdC,
            Settings.BrakeHotThresholdC,
            Settings.BrakeCriticalThresholdC);
        Channels[2].Configure(
            HealthSystem::Tires,
            Settings.TireWearWatch, 0.75,
            Settings.TireWearCritical);
        Channels[3].Configure(
            HealthSystem::Suspension, 100.0, 170.0, 250.0);
        Channels[4].Configure(
            HealthSystem::Chassis,
            Settings.DamageWatch, 0.35,
            Settings.DamageCritical);
        Channels[5].Configure(
            HealthSystem::Fuel,
            Settings.FuelLowFraction,
            0.10,
            Settings.FuelCriticalFraction, false);
        Channels[6].Configure(
            HealthSystem::Cooling, 90.0, 110.0, 125.0);
        Channels[7].Configure(
            HealthSystem::Electronics,
            0.45, 0.35, 0.20, false);
        Channels[8].Configure(
            HealthSystem::All,
            Settings.TireHotThresholdC - 20.0,
            Settings.TireHotThresholdC,
            Settings.TireCriticalThresholdC);
    }

    VehicleHealthSettings Settings{};
    std::array<HealthChannelMonitor, 9> Channels{};
    VehicleHealthSnapshot Last{};
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
};
} // namespace latebrake::engineering
