#pragma once

#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class FuelTrend : std::uint8_t
{
    Unknown,
    Stable,
    Saving,
    IncreasingConsumption,
    SuspectedRefuel,
    SensorDiscontinuity
};

struct FuelSample final
{
    double TimeSeconds = 0.0;
    double FuelFraction = 1.0;
    double TrackDistanceCm = 0.0;
    double ElapsedLapSeconds = 0.0;
    double Throttle = 0.0;
    double SpeedCmS = 0.0;
    int CompletedLaps = 0;
    int DriverSlot = -1;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    bool PhysicalSensor = false;
    bool InPitLane = false;
    bool EngineQualified = false;
};

struct FuelStintSettings final
{
    double TankCapacityLiters = 60.0;
    double MinimumFuelDropFraction = 0.00001;
    double MaximumFuelDropPerSecond = 0.015;
    double RefuelIncreaseThreshold = 0.005;
    double SensorNoiseAllowance = 0.0005;
    double MaximumSampleGapSeconds = 5.0;
    double MinimumTravelForEstimateCm = 500.0;
    double ReserveFuelFraction = 0.08;
    double ReliableConsumptionWindowSeconds = 5.0;
    double LowFuelWarningFraction = 0.15;
    double CriticalFuelFraction = 0.05;
    int MinimumLapSamplesForForecast = 2;
    bool AllowPitLaneRefuel = true;
    bool RequireOriginalPhysicalSensor = true;
};

inline bool ValidFuelStintSettings(
    FuelStintSettings settings) noexcept
{
    return Finite(settings.TankCapacityLiters) &&
        settings.TankCapacityLiters > 1.0 &&
        settings.TankCapacityLiters <= 300.0 &&
        Finite(settings.MinimumFuelDropFraction) &&
        settings.MinimumFuelDropFraction >= 0.0 &&
        Finite(settings.MaximumFuelDropPerSecond) &&
        settings.MaximumFuelDropPerSecond > 0.0 &&
        Finite(settings.RefuelIncreaseThreshold) &&
        settings.RefuelIncreaseThreshold > 0.0 &&
        Finite(settings.SensorNoiseAllowance) &&
        settings.SensorNoiseAllowance >= 0.0 &&
        Finite(settings.MaximumSampleGapSeconds) &&
        settings.MaximumSampleGapSeconds > 0.0 &&
        Finite(settings.MinimumTravelForEstimateCm) &&
        settings.MinimumTravelForEstimateCm >= 0.0 &&
        Finite(settings.ReserveFuelFraction) &&
        settings.ReserveFuelFraction >= 0.0 &&
        settings.ReserveFuelFraction < 0.5 &&
        Finite(settings.ReliableConsumptionWindowSeconds) &&
        settings.ReliableConsumptionWindowSeconds > 0.0 &&
        Finite(settings.LowFuelWarningFraction) &&
        settings.LowFuelWarningFraction > settings.CriticalFuelFraction &&
        settings.LowFuelWarningFraction < 1.0 &&
        Finite(settings.CriticalFuelFraction) &&
        settings.CriticalFuelFraction > 0.0 &&
        settings.MinimumLapSamplesForForecast >= 1 &&
        settings.MinimumLapSamplesForForecast <= 32;
}

struct FuelStintReview final
{
    double CurrentFuelFraction = 1.0;
    double RemainingLiters = 0.0;
    double ReserveLiters = 0.0;
    double UsableLiters = 0.0;
    double ConsumedLiters = 0.0;
    double RefuelAddedLiters = 0.0;
    double AverageLitersPerSecond = 0.0;
    double AverageLitersPerKm = 0.0;
    double AverageLitersPerLap = 0.0;
    double EstimatedLapsRemaining = 0.0;
    double EstimatedDistanceRemainingKm = 0.0;
    double EstimatedSecondsRemaining = 0.0;
    double EstimatedFinishFuelFraction = 0.0;
    double RequiredFuelSavingFraction = 0.0;
    double ConsumptionTrendLitersPerLap = 0.0;
    double MaximumObservedConsumptionRateLps = 0.0;
    double MinimumObservedConsumptionRateLps = 0.0;
    double LastLapLiters = 0.0;
    int CompletedFuelLaps = 0;
    int Samples = 0;
    int RejectedSamples = 0;
    int RefuelEvents = 0;
    int SensorFaults = 0;
    FuelTrend Trend = FuelTrend::Unknown;
    bool LowFuel = false;
    bool CriticalFuel = false;
    bool PhysicalSensorTrusted = false;
    bool HasConsumptionEstimate = false;
    bool AuthoritativeFuelLevel = false;
};

class FuelStintMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Previous = {};
        HasPrevious = false;
        Session = 0;
        Actor = 0;
        DriverSlot = -1;
        LastCompletedLap = -1;
        LapStartFuelFraction = 1.0;
        LapConsumption.fill(0.0);
        LapCount = 0;
        LapHead = 0;
        Current = {};
        TotalConsumedLiters = 0.0;
        TotalAddedLiters = 0.0;
        TotalTimeSeconds = 0.0;
        TotalDistanceCm = 0.0;
        TotalThrottleSeconds = 0.0;
        TotalCoastSeconds = 0.0;
        MaximumRate = 0.0;
        MinimumRate = std::numeric_limits<double>::infinity();
        Samples = 0;
        Rejections = 0;
        Refuels = 0;
        Faults = 0;
        LastTrend = FuelTrend::Unknown;
    }

    bool Configure(FuelStintSettings settings) noexcept
    {
        if (!ValidFuelStintSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(FuelSample sample) noexcept
    {
        if (!ValidSample(sample))
        {
            ++Rejections;
            return false;
        }
        if (!HasPrevious ||
            sample.Session != Session ||
            sample.Actor != Actor ||
            sample.DriverSlot != DriverSlot)
        {
            Begin(sample);
            return true;
        }
        const double dt =
            sample.TimeSeconds - Previous.TimeSeconds;
        const double traveled =
            sample.TrackDistanceCm - Previous.TrackDistanceCm;
        if (dt <= 0.0 ||
            dt > Settings.MaximumSampleGapSeconds ||
            !Finite(traveled) ||
            traveled < -1000.0 ||
            traveled > MaximumPlanarSpeedCmS * dt + 150.0)
        {
            ++Faults;
            ++Rejections;
            Previous = sample;
            LastTrend = FuelTrend::SensorDiscontinuity;
            return false;
        }
        const double fuelDelta =
            sample.FuelFraction - Previous.FuelFraction;
        if (fuelDelta >
            Settings.RefuelIncreaseThreshold)
        {
            if (!Settings.AllowPitLaneRefuel ||
                !sample.InPitLane)
            {
                ++Faults;
                ++Rejections;
                Previous = sample;
                LastTrend = FuelTrend::SensorDiscontinuity;
                return false;
            }
            TotalAddedLiters +=
                fuelDelta * Settings.TankCapacityLiters;
            ++Refuels;
            LastTrend = FuelTrend::SuspectedRefuel;
            LapStartFuelFraction = sample.FuelFraction;
        }
        else if (fuelDelta <
                 -Settings.MinimumFuelDropFraction)
        {
            const double consumed = -fuelDelta;
            const double rate = consumed / dt;
            if (rate >
                Settings.MaximumFuelDropPerSecond)
            {
                ++Faults;
                ++Rejections;
                Previous = sample;
                LastTrend = FuelTrend::SensorDiscontinuity;
                return false;
            }
            TotalConsumedLiters +=
                consumed * Settings.TankCapacityLiters;
            const double litersPerSecond =
                consumed * Settings.TankCapacityLiters / dt;
            MaximumRate = std::max(
                MaximumRate, litersPerSecond);
            MinimumRate = std::min(
                MinimumRate, litersPerSecond);
        }
        else if (fuelDelta >
                 Settings.SensorNoiseAllowance)
        {
            ++Faults;
        }
        TotalTimeSeconds += dt;
        TotalDistanceCm += std::max(0.0, traveled);
        TotalThrottleSeconds +=
            sample.Throttle >= 0.18 ? dt : 0.0;
        TotalCoastSeconds +=
            sample.Throttle < 0.18 ? dt : 0.0;
        if (sample.CompletedLaps >
            LastCompletedLap)
        {
            const double lapFuel =
                (LapStartFuelFraction -
                 sample.FuelFraction) *
                Settings.TankCapacityLiters;
            if (lapFuel > Settings.MinimumFuelDropFraction *
                Settings.TankCapacityLiters &&
                sample.CompletedLaps == LastCompletedLap + 1)
            {
                RecordLapFuel(lapFuel);
            }
            LastCompletedLap = sample.CompletedLaps;
            LapStartFuelFraction = sample.FuelFraction;
        }
        Previous = sample;
        ++Samples;
        UpdateReview();
        return true;
    }

    FuelStintReview EvaluateRequiredFinish(
        int remainingLaps,
        double remainingDistanceKm) const noexcept
    {
        FuelStintReview review = Current;
        if (!review.HasConsumptionEstimate ||
            remainingLaps < 0 ||
            !Finite(remainingDistanceKm) ||
            remainingDistanceKm < 0.0)
        {
            return review;
        }
        const double requiredByLaps =
            remainingLaps * review.AverageLitersPerLap;
        const double requiredByDistance =
            remainingDistanceKm *
            review.AverageLitersPerKm;
        const double required = std::max(
            requiredByLaps, requiredByDistance);
        const double available =
            review.UsableLiters;
        review.RequiredFuelSavingFraction =
            available >= required ? 0.0 :
            Clamp01(SafeRatio(required - available, required));
        review.EstimatedFinishFuelFraction =
            SafeRatio(review.RemainingLiters - required,
                      Settings.TankCapacityLiters);
        return review;
    }

    FuelStintReview Snapshot() const noexcept
    {
        return Current;
    }

    std::uint64_t RejectedObservations() const noexcept
    {
        return Rejections;
    }

    double TotalFuelUsedLiters() const noexcept
    {
        return TotalConsumedLiters;
    }

    double TotalFuelAddedLiters() const noexcept
    {
        return TotalAddedLiters;
    }

    int RecordedLapConsumptions() const noexcept
    {
        return LapCount;
    }

    double LapFuelFromOldest(int index) const noexcept
    {
        return index >= 0 && index < LapCount ?
            LapConsumption[static_cast<std::size_t>(
                (LapHead + index) %
                static_cast<int>(LapConsumption.size()))] : 0.0;
    }

private:
    bool ValidSample(const FuelSample& s) const noexcept
    {
        return s.EngineQualified &&
            (!Settings.RequireOriginalPhysicalSensor ||
             s.PhysicalSensor) &&
            s.Session != 0 && s.Actor != 0 &&
            s.DriverSlot >= 0 &&
            s.DriverSlot < MaximumTelemetryDrivers &&
            Finite(s.TimeSeconds) &&
            Finite(s.FuelFraction) &&
            Finite(s.TrackDistanceCm) &&
            Finite(s.ElapsedLapSeconds) &&
            Finite(s.Throttle) &&
            Finite(s.SpeedCmS) &&
            s.TimeSeconds >= 0.0 &&
            s.FuelFraction >= 0.0 &&
            s.FuelFraction <= 1.0 &&
            s.TrackDistanceCm >= 0.0 &&
            s.ElapsedLapSeconds >= 0.0 &&
            s.Throttle >= 0.0 &&
            s.Throttle <= 1.0 &&
            s.SpeedCmS >= 0.0 &&
            s.SpeedCmS <= MaximumPlanarSpeedCmS &&
            s.CompletedLaps >= 0 &&
            s.CompletedLaps <= MaximumTelemetryLaps;
    }

    void Begin(FuelSample sample) noexcept
    {
        Previous = sample;
        Session = sample.Session;
        Actor = sample.Actor;
        DriverSlot = sample.DriverSlot;
        LastCompletedLap = sample.CompletedLaps;
        LapStartFuelFraction = sample.FuelFraction;
        HasPrevious = true;
        Current.CurrentFuelFraction =
            sample.FuelFraction;
        Current.RemainingLiters =
            sample.FuelFraction *
            Settings.TankCapacityLiters;
        Current.PhysicalSensorTrusted =
            sample.PhysicalSensor &&
            sample.EngineQualified;
        Current.AuthoritativeFuelLevel = false;
        ++Samples;
    }

    void RecordLapFuel(double liters) noexcept
    {
        if (LapCount == static_cast<int>(LapConsumption.size()))
        {
            LapHead = (LapHead + 1) %
                static_cast<int>(LapConsumption.size());
        }
        else
        {
            ++LapCount;
        }
        LapConsumption[static_cast<std::size_t>(
            (LapHead + LapCount - 1) %
            static_cast<int>(LapConsumption.size()))] =
            liters;
    }

    void UpdateReview() noexcept
    {
        Current.CurrentFuelFraction =
            Previous.FuelFraction;
        Current.RemainingLiters =
            Previous.FuelFraction *
            Settings.TankCapacityLiters;
        Current.ReserveLiters =
            Settings.ReserveFuelFraction *
            Settings.TankCapacityLiters;
        Current.UsableLiters = std::max(
            0.0, Current.RemainingLiters -
                 Current.ReserveLiters);
        Current.ConsumedLiters =
            TotalConsumedLiters;
        Current.RefuelAddedLiters =
            TotalAddedLiters;
        Current.AverageLitersPerSecond =
            SafeRatio(TotalConsumedLiters, TotalTimeSeconds);
        Current.AverageLitersPerKm =
            SafeRatio(TotalConsumedLiters,
                      TotalDistanceCm / CmPerKilometer);
        double lapFuel = 0.0;
        for (int i = 0; i < LapCount; ++i)
        {
            lapFuel += LapFuelFromOldest(i);
        }
        Current.AverageLitersPerLap =
            SafeRatio(lapFuel, LapCount);
        Current.LastLapLiters =
            LapCount > 0 ?
            LapFuelFromOldest(LapCount - 1) : 0.0;
        Current.CompletedFuelLaps =
            LapCount;
        Current.EstimatedLapsRemaining =
            SafeRatio(Current.UsableLiters,
                      Current.AverageLitersPerLap);
        Current.EstimatedDistanceRemainingKm =
            SafeRatio(Current.UsableLiters,
                      Current.AverageLitersPerKm);
        Current.EstimatedSecondsRemaining =
            SafeRatio(Current.UsableLiters,
                      Current.AverageLitersPerSecond);
        Current.MaximumObservedConsumptionRateLps =
            MaximumRate;
        Current.MinimumObservedConsumptionRateLps =
            Finite(MinimumRate) ? MinimumRate : 0.0;
        Current.Samples = Samples;
        Current.RejectedSamples =
            static_cast<int>(Rejections);
        Current.RefuelEvents = Refuels;
        Current.SensorFaults = Faults;
        Current.LowFuel =
            Current.CurrentFuelFraction <=
            Settings.LowFuelWarningFraction;
        Current.CriticalFuel =
            Current.CurrentFuelFraction <=
            Settings.CriticalFuelFraction;
        Current.PhysicalSensorTrusted =
            Previous.PhysicalSensor &&
            Previous.EngineQualified;
        Current.HasConsumptionEstimate =
            TotalDistanceCm >=
                Settings.MinimumTravelForEstimateCm &&
            TotalTimeSeconds >=
                Settings.ReliableConsumptionWindowSeconds &&
            TotalConsumedLiters > 0.0 &&
            LapCount >=
                Settings.MinimumLapSamplesForForecast;
        if (LapCount >= 4)
        {
            double early = 0.0;
            double late = 0.0;
            const int half = LapCount / 2;
            for (int i = 0; i < LapCount; ++i)
            {
                if (i < half)
                {
                    early += LapFuelFromOldest(i);
                }
                else
                {
                    late += LapFuelFromOldest(i);
                }
            }
            const double earlyMean =
                early / static_cast<double>(half);
            const double lateMean =
                late / static_cast<double>(LapCount - half);
            Current.ConsumptionTrendLitersPerLap =
                lateMean - earlyMean;
            LastTrend =
                std::abs(lateMean - earlyMean) <
                    earlyMean * 0.03 ?
                FuelTrend::Stable :
                lateMean < earlyMean ?
                    FuelTrend::Saving :
                    FuelTrend::IncreasingConsumption;
        }
        Current.Trend = LastTrend;
        Current.AuthoritativeFuelLevel = false;
    }

    FuelStintSettings Settings{};
    FuelSample Previous{};
    FuelStintReview Current{};
    std::array<double, 32> LapConsumption{};
    int LapCount = 0;
    int LapHead = 0;
    int LastCompletedLap = -1;
    int DriverSlot = -1;
    int Samples = 0;
    int Refuels = 0;
    int Faults = 0;
    std::uint64_t Rejections = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    double LapStartFuelFraction = 1.0;
    double TotalConsumedLiters = 0.0;
    double TotalAddedLiters = 0.0;
    double TotalTimeSeconds = 0.0;
    double TotalDistanceCm = 0.0;
    double TotalThrottleSeconds = 0.0;
    double TotalCoastSeconds = 0.0;
    double MaximumRate = 0.0;
    double MinimumRate =
        std::numeric_limits<double>::infinity();
    FuelTrend LastTrend = FuelTrend::Unknown;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
