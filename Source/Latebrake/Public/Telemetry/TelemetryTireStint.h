#pragma once

#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryFuelStint.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class TireCondition : std::uint8_t
{
    Unknown,
    Cold,
    Working,
    Hot,
    Overheated,
    Worn,
    Critical
};

struct TireStintSettings final
{
    double MinimumColdTemperatureC = 40.0;
    double OptimumMinimumTemperatureC = 72.0;
    double OptimumMaximumTemperatureC = 108.0;
    double OverheatTemperatureC = 130.0;
    double CriticalTemperatureC = 160.0;
    double WarningWearFraction = 0.65;
    double CriticalWearFraction = 0.88;
    double RapidWearPerLap = 0.06;
    double MinimumSensorDeltaSeconds = 0.005;
    double MaximumSensorDeltaSeconds = 0.30;
    double MaximumAllowedWearIncrease = 0.10;
    double MaximumAllowedTemperatureJumpC = 40.0;
    int MinimumReliableLaps = 2;
    bool RequireFourContactSensors = true;
    bool RejectWearReversal = true;
};

inline bool ValidTireStintSettings(
    TireStintSettings s) noexcept
{
    return Finite(s.MinimumColdTemperatureC) &&
        Finite(s.OptimumMinimumTemperatureC) &&
        Finite(s.OptimumMaximumTemperatureC) &&
        Finite(s.OverheatTemperatureC) &&
        Finite(s.CriticalTemperatureC) &&
        s.MinimumColdTemperatureC <
            s.OptimumMinimumTemperatureC &&
        s.OptimumMinimumTemperatureC <
            s.OptimumMaximumTemperatureC &&
        s.OptimumMaximumTemperatureC <
            s.OverheatTemperatureC &&
        s.OverheatTemperatureC <
            s.CriticalTemperatureC &&
        Finite(s.WarningWearFraction) &&
        Finite(s.CriticalWearFraction) &&
        s.WarningWearFraction > 0.0 &&
        s.CriticalWearFraction >
            s.WarningWearFraction &&
        s.CriticalWearFraction <= 1.0 &&
        Finite(s.RapidWearPerLap) &&
        s.RapidWearPerLap > 0.0 &&
        Finite(s.MinimumSensorDeltaSeconds) &&
        s.MinimumSensorDeltaSeconds > 0.0 &&
        Finite(s.MaximumSensorDeltaSeconds) &&
        s.MaximumSensorDeltaSeconds >
            s.MinimumSensorDeltaSeconds &&
        Finite(s.MaximumAllowedWearIncrease) &&
        s.MaximumAllowedWearIncrease > 0.0 &&
        Finite(s.MaximumAllowedTemperatureJumpC) &&
        s.MaximumAllowedTemperatureJumpC > 0.0 &&
        s.MinimumReliableLaps >= 1 &&
        s.MinimumReliableLaps <= 32;
}

struct TireSensorFrame final
{
    std::array<double, 4> TemperatureC{};
    std::array<double, 4> WearFraction{};
    std::array<double, 4> NormalLoadN{};
    std::array<double, 4> SlipRatio{};
    std::array<bool, 4> Contact{};
    double TimeSeconds = 0.0;
    double SpeedCmS = 0.0;
    double TrackWetness = 0.0;
    int CompletedLaps = 0;
    int DriverSlot = -1;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    bool PhysicalSensorsReady = false;
    bool EngineQualified = false;
};

struct TireAxleReview final
{
    double MeanTemperatureC = 0.0;
    double MaximumTemperatureC = 0.0;
    double MinimumTemperatureC = 0.0;
    double MeanWearFraction = 0.0;
    double MaximumWearFraction = 0.0;
    double LoadFraction = 0.0;
    double MeanAbsoluteSlip = 0.0;
    double LeftRightTemperatureDeltaC = 0.0;
    double LeftRightWearDelta = 0.0;
    bool AxleHot = false;
    bool AxleWorn = false;
    bool CompleteContact = false;
    bool Valid = false;
};

struct TireStintReview final
{
    std::array<TireCondition, 4> WheelConditions{};
    std::array<double, 4> TemperatureC{};
    std::array<double, 4> WearFraction{};
    std::array<double, 4> WearPerLap{};
    TireAxleReview Front{};
    TireAxleReview Rear{};
    double MeanTemperatureC = 0.0;
    double MeanWear = 0.0;
    double HighestTemperatureC = 0.0;
    double HighestWear = 0.0;
    double EstimatedLapsUntilWarning = 0.0;
    double EstimatedLapsUntilCritical = 0.0;
    double EstimatedLapsUntilWearLimit = 0.0;
    double EstimatedGripRetentionFraction = 0.0;
    double MaximumObservedSlip = 0.0;
    double TemperatureSpreadC = 0.0;
    double WearSpread = 0.0;
    double HighTemperatureSeconds = 0.0;
    double LowTemperatureSeconds = 0.0;
    double ContactLostSeconds = 0.0;
    double CumulativeSlipSeconds = 0.0;
    int CompletedLaps = 0;
    int Samples = 0;
    int Rejected = 0;
    int SensorFaults = 0;
    int HotWheelCount = 0;
    int CriticalWheelCount = 0;
    bool WearForecastQualified = false;
    bool FourContactSensorsQualified = false;
    bool TireChangeRecommended = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline TireCondition EvaluateTireCondition(
    double temperatureC, double wear,
    TireStintSettings settings) noexcept
{
    if (!Finite(temperatureC) || !Finite(wear) ||
        wear < 0.0 || wear > 1.0)
    {
        return TireCondition::Unknown;
    }
    if (wear >= settings.CriticalWearFraction ||
        temperatureC >= settings.CriticalTemperatureC)
    {
        return TireCondition::Critical;
    }
    if (temperatureC >= settings.OverheatTemperatureC)
    {
        return TireCondition::Overheated;
    }
    if (wear >= settings.WarningWearFraction)
    {
        return TireCondition::Worn;
    }
    if (temperatureC >= settings.OptimumMaximumTemperatureC)
    {
        return TireCondition::Hot;
    }
    if (temperatureC < settings.MinimumColdTemperatureC)
    {
        return TireCondition::Cold;
    }
    return TireCondition::Working;
}

class TireStintMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Previous = {};
        Last = {};
        HasPrevious = false;
        Session = 0;
        Actor = 0;
        DriverSlot = -1;
        StartingLap = 0;
        CompletedLapCount = 0;
        InitialWear.fill(0.0);
        PreviousLapWear.fill(0.0);
        AccumulatedWear.fill(0.0);
        LapWearSamples.fill(0);
        HighHeatSeconds = 0.0;
        LowHeatSeconds = 0.0;
        ContactLossSeconds = 0.0;
        SlipSeconds = 0.0;
        MaximumSlip = 0.0;
        Rejected = 0;
        Faults = 0;
        Samples = 0;
    }

    bool Configure(TireStintSettings settings) noexcept
    {
        if (!ValidTireStintSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(TireSensorFrame sample) noexcept
    {
        if (!ValidSample(sample))
        {
            ++Rejected;
            return false;
        }
        if (!HasPrevious || Session != sample.Session ||
            Actor != sample.Actor ||
            DriverSlot != sample.DriverSlot)
        {
            StartNewStint(sample);
            return true;
        }
        const double dt =
            sample.TimeSeconds - Previous.TimeSeconds;
        if (dt < Settings.MinimumSensorDeltaSeconds ||
            dt > Settings.MaximumSensorDeltaSeconds)
        {
            ++Rejected;
            ++Faults;
            Previous = sample;
            return false;
        }
        for (std::size_t i = 0; i < 4; ++i)
        {
            const double wearDelta =
                sample.WearFraction[i] -
                Previous.WearFraction[i];
            const double temperatureDelta =
                std::abs(sample.TemperatureC[i] -
                         Previous.TemperatureC[i]);
            if ((Settings.RejectWearReversal &&
                 wearDelta < -0.001) ||
                wearDelta >
                    Settings.MaximumAllowedWearIncrease ||
                temperatureDelta >
                    Settings.MaximumAllowedTemperatureJumpC)
            {
                ++Rejected;
                ++Faults;
                Previous = sample;
                return false;
            }
        }
        bool hot = false;
        bool cold = false;
        bool lost = false;
        double maxSlip = 0.0;
        for (std::size_t i = 0; i < 4; ++i)
        {
            hot = hot ||
                sample.TemperatureC[i] >=
                    Settings.OverheatTemperatureC;
            cold = cold ||
                sample.TemperatureC[i] <
                    Settings.MinimumColdTemperatureC;
            lost = lost || !sample.Contact[i];
            maxSlip = std::max(
                maxSlip, std::abs(sample.SlipRatio[i]));
        }
        if (hot)
        {
            HighHeatSeconds += dt;
        }
        if (cold)
        {
            LowHeatSeconds += dt;
        }
        if (lost)
        {
            ContactLossSeconds += dt;
        }
        SlipSeconds += maxSlip > 0.25 ? dt : 0.0;
        MaximumSlip = std::max(
            MaximumSlip, maxSlip);
        if (sample.CompletedLaps >
            Previous.CompletedLaps)
        {
            if (sample.CompletedLaps ==
                Previous.CompletedLaps + 1)
            {
                for (std::size_t i = 0; i < 4; ++i)
                {
                    const double used = std::max(
                        0.0, sample.WearFraction[i] -
                             PreviousLapWear[i]);
                    AccumulatedWear[i] += used;
                    ++LapWearSamples[i];
                    PreviousLapWear[i] =
                        sample.WearFraction[i];
                }
                ++CompletedLapCount;
            }
            else
            {
                ++Faults;
            }
        }
        Previous = sample;
        ++Samples;
        UpdateReview();
        return true;
    }

    TireStintReview Snapshot() const noexcept
    {
        return Last;
    }

    double MaxMeanWearPerLap() const noexcept
    {
        double maximum = 0.0;
        for (std::size_t i = 0; i < 4; ++i)
        {
            maximum = std::max(
                maximum, SafeRatio(
                    AccumulatedWear[i],
                    LapWearSamples[i]));
        }
        return maximum;
    }

    std::uint64_t RejectedFrames() const noexcept
    {
        return Rejected;
    }

private:
    bool ValidSample(const TireSensorFrame& s) const noexcept
    {
        if (!s.EngineQualified ||
            !s.PhysicalSensorsReady ||
            s.Session == 0 || s.Actor == 0 ||
            s.DriverSlot < 0 ||
            s.DriverSlot >= MaximumTelemetryDrivers ||
            !Finite(s.TimeSeconds) ||
            !Finite(s.SpeedCmS) ||
            !Finite(s.TrackWetness) ||
            s.TimeSeconds < 0.0 ||
            s.SpeedCmS < 0.0 ||
            s.SpeedCmS > MaximumPlanarSpeedCmS ||
            s.TrackWetness < 0.0 ||
            s.TrackWetness > 1.0 ||
            s.CompletedLaps < 0 ||
            s.CompletedLaps > MaximumTelemetryLaps)
        {
            return false;
        }
        for (std::size_t i = 0; i < 4; ++i)
        {
            if (!Finite(s.TemperatureC[i]) ||
                !Finite(s.WearFraction[i]) ||
                !Finite(s.NormalLoadN[i]) ||
                !Finite(s.SlipRatio[i]) ||
                s.TemperatureC[i] < -100.0 ||
                s.TemperatureC[i] > 300.0 ||
                s.WearFraction[i] < 0.0 ||
                s.WearFraction[i] > 1.0 ||
                s.NormalLoadN[i] < 0.0 ||
                s.NormalLoadN[i] > 250000.0 ||
                std::abs(s.SlipRatio[i]) > 12.0)
            {
                return false;
            }
        }
        return true;
    }

    void StartNewStint(TireSensorFrame sample) noexcept
    {
        Previous = sample;
        Session = sample.Session;
        Actor = sample.Actor;
        DriverSlot = sample.DriverSlot;
        StartingLap = sample.CompletedLaps;
        for (std::size_t i = 0; i < 4; ++i)
        {
            InitialWear[i] = sample.WearFraction[i];
            PreviousLapWear[i] = sample.WearFraction[i];
        }
        HasPrevious = true;
        ++Samples;
        UpdateReview();
    }

    void UpdateReview() noexcept
    {
        TireStintReview result;
        result.TemperatureC = Previous.TemperatureC;
        result.WearFraction = Previous.WearFraction;
        result.CompletedLaps = CompletedLapCount;
        result.Samples = Samples;
        result.Rejected = static_cast<int>(Rejected);
        result.SensorFaults = Faults;
        result.HighTemperatureSeconds = HighHeatSeconds;
        result.LowTemperatureSeconds = LowHeatSeconds;
        result.ContactLostSeconds = ContactLossSeconds;
        result.CumulativeSlipSeconds = SlipSeconds;
        result.MaximumObservedSlip = MaximumSlip;
        result.HighestTemperatureC =
            -std::numeric_limits<double>::infinity();
        result.HighestWear = 0.0;
        double minimumTemperature =
            std::numeric_limits<double>::infinity();
        double minimumWear =
            std::numeric_limits<double>::infinity();
        double temperatureSum = 0.0;
        double wearSum = 0.0;
        int contacts = 0;
        for (std::size_t i = 0; i < 4; ++i)
        {
            const double temperature =
                Previous.TemperatureC[i];
            const double wear =
                Previous.WearFraction[i];
            result.WheelConditions[i] =
                EvaluateTireCondition(
                    temperature, wear, Settings);
            result.WearPerLap[i] = SafeRatio(
                AccumulatedWear[i], LapWearSamples[i]);
            temperatureSum += temperature;
            wearSum += wear;
            minimumTemperature = std::min(
                minimumTemperature, temperature);
            minimumWear = std::min(
                minimumWear, wear);
            result.HighestTemperatureC = std::max(
                result.HighestTemperatureC, temperature);
            result.HighestWear = std::max(
                result.HighestWear, wear);
            contacts += Previous.Contact[i] ? 1 : 0;
            const TireCondition condition =
                result.WheelConditions[i];
            result.HotWheelCount +=
                condition == TireCondition::Hot ||
                condition == TireCondition::Overheated ||
                condition == TireCondition::Critical ? 1 : 0;
            result.CriticalWheelCount +=
                condition == TireCondition::Critical ? 1 : 0;
        }
        result.MeanTemperatureC =
            temperatureSum / 4.0;
        result.MeanWear = wearSum / 4.0;
        result.TemperatureSpreadC =
            result.HighestTemperatureC -
            minimumTemperature;
        result.WearSpread =
            result.HighestWear - minimumWear;
        result.Front = EvaluateAxle(0, 1);
        result.Rear = EvaluateAxle(2, 3);
        result.FourContactSensorsQualified =
            contacts == 4;
        const double worstWearPerLap =
            MaxMeanWearPerLap();
        result.WearForecastQualified =
            CompletedLapCount >= Settings.MinimumReliableLaps &&
            worstWearPerLap > Tiny;
        if (result.WearForecastQualified)
        {
            result.EstimatedLapsUntilWarning =
                std::max(0.0, Settings.WarningWearFraction -
                         result.HighestWear) /
                worstWearPerLap;
            result.EstimatedLapsUntilCritical =
                std::max(0.0, Settings.CriticalWearFraction -
                         result.HighestWear) /
                worstWearPerLap;
            result.EstimatedLapsUntilWearLimit =
                std::max(0.0, 1.0 -
                         result.HighestWear) /
                worstWearPerLap;
        }
        result.EstimatedGripRetentionFraction = Clamp01(
            1.0 - 0.58 * result.HighestWear -
            0.25 * Clamp01(
                (result.HighestTemperatureC -
                 Settings.OptimumMaximumTemperatureC) /
                std::max(1.0,
                    Settings.CriticalTemperatureC -
                    Settings.OptimumMaximumTemperatureC)));
        result.TireChangeRecommended =
            result.CriticalWheelCount > 0 ||
            (result.WearForecastQualified &&
             result.EstimatedLapsUntilCritical < 2.0);
        result.Valid = Previous.EngineQualified &&
            Previous.PhysicalSensorsReady &&
            (!Settings.RequireFourContactSensors ||
             result.FourContactSensorsQualified);
        Last = result;
    }

    TireAxleReview EvaluateAxle(
        std::size_t left, std::size_t right) const noexcept
    {
        TireAxleReview axle;
        const double leftTemperature =
            Previous.TemperatureC[left];
        const double rightTemperature =
            Previous.TemperatureC[right];
        axle.MeanTemperatureC =
            0.5 * (leftTemperature + rightTemperature);
        axle.MaximumTemperatureC =
            std::max(leftTemperature, rightTemperature);
        axle.MinimumTemperatureC =
            std::min(leftTemperature, rightTemperature);
        axle.MeanWearFraction = 0.5 * (
            Previous.WearFraction[left] +
            Previous.WearFraction[right]);
        axle.MaximumWearFraction = std::max(
            Previous.WearFraction[left],
            Previous.WearFraction[right]);
        axle.LeftRightTemperatureDeltaC =
            leftTemperature - rightTemperature;
        axle.LeftRightWearDelta =
            Previous.WearFraction[left] -
            Previous.WearFraction[right];
        const double total =
            Previous.NormalLoadN[0] +
            Previous.NormalLoadN[1] +
            Previous.NormalLoadN[2] +
            Previous.NormalLoadN[3];
        axle.LoadFraction = SafeRatio(
            Previous.NormalLoadN[left] +
            Previous.NormalLoadN[right], total);
        axle.MeanAbsoluteSlip = 0.5 * (
            std::abs(Previous.SlipRatio[left]) +
            std::abs(Previous.SlipRatio[right]));
        axle.AxleHot =
            axle.MaximumTemperatureC >=
            Settings.OverheatTemperatureC;
        axle.AxleWorn =
            axle.MaximumWearFraction >=
            Settings.WarningWearFraction;
        axle.CompleteContact =
            Previous.Contact[left] &&
            Previous.Contact[right];
        axle.Valid = Previous.PhysicalSensorsReady;
        return axle;
    }

    TireStintSettings Settings{};
    TireSensorFrame Previous{};
    TireStintReview Last{};
    std::array<double, 4> InitialWear{};
    std::array<double, 4> PreviousLapWear{};
    std::array<double, 4> AccumulatedWear{};
    std::array<int, 4> LapWearSamples{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int DriverSlot = -1;
    int StartingLap = 0;
    int CompletedLapCount = 0;
    int Samples = 0;
    int Faults = 0;
    std::uint64_t Rejected = 0;
    double HighHeatSeconds = 0.0;
    double LowHeatSeconds = 0.0;
    double ContactLossSeconds = 0.0;
    double SlipSeconds = 0.0;
    double MaximumSlip = 0.0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
