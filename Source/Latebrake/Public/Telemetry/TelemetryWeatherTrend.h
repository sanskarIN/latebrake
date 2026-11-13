#pragma once

#include "Telemetry/TelemetryGrip.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class TrackConditionTrend : std::uint8_t
{
    Unknown,
    StableDry,
    Drying,
    Wetting,
    StableWet,
    Unstable,
    SensorFault
};

struct TrackEnvironmentSample final
{
    double TimeSeconds = 0.0;
    double AmbientC = 20.0;
    double TrackSurfaceC = 25.0;
    double WetnessFraction = 0.0;
    double RoadFrictionEstimate = 1.0;
    double VisibilityFraction = 1.0;
    double WindSpeedCmS = 0.0;
    double WindDirectionRadians = 0.0;
    double TireGripEstimate = 1.0;
    double TireTemperatureC = 65.0;
    double AirDensityKgM3 = 1.225;
    double RainfallRateMmHour = 0.0;
    double RelativeHumidityFraction = 0.5;
    double SurfaceWaterDepthMm = 0.0;
    bool SourceEngineQualified = false;
    bool PhysicallyMeasuredWetness = false;
    bool PhysicallyMeasuredFriction = false;
};

struct WeatherTrendSettings final
{
    double MaximumFrameGapSeconds = 20.0;
    double MaximumWetnessRatePerSecond = 0.20;
    double MaximumTemperatureRateCPerSecond = 5.0;
    double WetThreshold = 0.30;
    double HeavyWetThreshold = 0.72;
    double TrendRateThresholdPerSecond = 0.0005;
    double MinimumTrendWindowSeconds = 5.0;
    double TrendSmoothSeconds = 10.0;
    double MinimumGripEstimate = 0.10;
    double MaximumRainfallMmHour = 150.0;
    double WaterDepthWarningMm = 1.5;
    bool RequirePhysicalWetnessSensor = true;
};

inline bool ValidWeatherTrendSettings(
    WeatherTrendSettings s) noexcept
{
    return Finite(s.MaximumFrameGapSeconds) &&
        s.MaximumFrameGapSeconds > 0.0 &&
        Finite(s.MaximumWetnessRatePerSecond) &&
        s.MaximumWetnessRatePerSecond > 0.0 &&
        Finite(s.MaximumTemperatureRateCPerSecond) &&
        s.MaximumTemperatureRateCPerSecond > 0.0 &&
        Finite(s.WetThreshold) &&
        Finite(s.HeavyWetThreshold) &&
        s.WetThreshold > 0.0 &&
        s.HeavyWetThreshold > s.WetThreshold &&
        s.HeavyWetThreshold <= 1.0 &&
        Finite(s.TrendRateThresholdPerSecond) &&
        s.TrendRateThresholdPerSecond > 0.0 &&
        Finite(s.MinimumTrendWindowSeconds) &&
        s.MinimumTrendWindowSeconds > 0.0 &&
        Finite(s.TrendSmoothSeconds) &&
        s.TrendSmoothSeconds > 0.0 &&
        Finite(s.MinimumGripEstimate) &&
        s.MinimumGripEstimate > 0.0 &&
        Finite(s.MaximumRainfallMmHour) &&
        s.MaximumRainfallMmHour > 0.0 &&
        Finite(s.WaterDepthWarningMm) &&
        s.WaterDepthWarningMm >= 0.0;
}

inline bool ValidEnvironmentSample(
    TrackEnvironmentSample s,
    WeatherTrendSettings settings) noexcept
{
    return s.SourceEngineQualified &&
        (!settings.RequirePhysicalWetnessSensor ||
         s.PhysicallyMeasuredWetness) &&
        Finite(s.TimeSeconds) &&
        Finite(s.AmbientC) &&
        Finite(s.TrackSurfaceC) &&
        Finite(s.WetnessFraction) &&
        Finite(s.RoadFrictionEstimate) &&
        Finite(s.VisibilityFraction) &&
        Finite(s.WindSpeedCmS) &&
        Finite(s.WindDirectionRadians) &&
        Finite(s.TireGripEstimate) &&
        Finite(s.TireTemperatureC) &&
        Finite(s.AirDensityKgM3) &&
        Finite(s.RainfallRateMmHour) &&
        Finite(s.RelativeHumidityFraction) &&
        Finite(s.SurfaceWaterDepthMm) &&
        s.TimeSeconds >= 0.0 &&
        s.AmbientC >= -100.0 &&
        s.AmbientC <= 90.0 &&
        s.TrackSurfaceC >= -100.0 &&
        s.TrackSurfaceC <= 150.0 &&
        s.WetnessFraction >= 0.0 &&
        s.WetnessFraction <= 1.0 &&
        s.RoadFrictionEstimate >= 0.0 &&
        s.RoadFrictionEstimate <= 4.0 &&
        s.VisibilityFraction >= 0.0 &&
        s.VisibilityFraction <= 1.0 &&
        s.WindSpeedCmS >= 0.0 &&
        s.WindSpeedCmS <= 10000.0 &&
        s.TireGripEstimate >= 0.0 &&
        s.TireGripEstimate <= 4.0 &&
        s.TireTemperatureC >= -100.0 &&
        s.TireTemperatureC <= 300.0 &&
        s.AirDensityKgM3 >= 0.2 &&
        s.AirDensityKgM3 <= 2.0 &&
        s.RainfallRateMmHour >= 0.0 &&
        s.RainfallRateMmHour <= settings.MaximumRainfallMmHour &&
        s.RelativeHumidityFraction >= 0.0 &&
        s.RelativeHumidityFraction <= 1.0 &&
        s.SurfaceWaterDepthMm >= 0.0 &&
        s.SurfaceWaterDepthMm <= 100.0;
}

struct WeatherTrendReview final
{
    TrackConditionTrend Trend = TrackConditionTrend::Unknown;
    double CurrentWetness = 0.0;
    double MeanWetness = 0.0;
    double MeanSurfaceTemperatureC = 0.0;
    double MeanAmbientTemperatureC = 0.0;
    double MeanRoadFriction = 0.0;
    double MeanGripEstimate = 0.0;
    double CurrentWetnessRatePerSecond = 0.0;
    double SmoothedWetnessRatePerSecond = 0.0;
    double CurrentSurfaceTemperatureRateCPerSecond = 0.0;
    double WetnessStandardDeviation = 0.0;
    double TemperatureStandardDeviationC = 0.0;
    double DryingTimeEstimateSeconds = 0.0;
    double TimeToHeavyWetSeconds = 0.0;
    double MaximumObservedWetness = 0.0;
    double MinimumObservedFriction = 0.0;
    double MaximumObservedWaterDepthMm = 0.0;
    double AccumulatedRainfallMm = 0.0;
    double MeanVisibilityFraction = 0.0;
    double MeanWindSpeedCmS = 0.0;
    double MeanHumidityFraction = 0.0;
    double ObservationSpanSeconds = 0.0;
    double WetConditionsSeconds = 0.0;
    double DryConditionsSeconds = 0.0;
    double LowVisibilitySeconds = 0.0;
    int Observations = 0;
    int Rejected = 0;
    int Discontinuities = 0;
    bool EnoughHistory = false;
    bool OriginalWetnessMeasured = false;
    bool OriginalFrictionMeasured = false;
    bool SafeToRecommendTireChange = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

class TrackWeatherMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Previous = {};
        HasPrevious = false;
        WetnessStats.Reset();
        TrackTemperatureStats.Reset();
        AirTemperatureStats.Reset();
        FrictionStats.Reset();
        GripStats.Reset();
        VisibilityStats.Reset();
        WindStats.Reset();
        HumidityStats.Reset();
        WetRateFilter.Reset();
        History.fill({});
        HistoryHead = 0;
        HistoryCount = 0;
        TotalSpan = 0.0;
        WetSeconds = 0.0;
        DrySeconds = 0.0;
        LowVisibilitySeconds = 0.0;
        MaximumWetness = 0.0;
        MinimumFriction =
            std::numeric_limits<double>::infinity();
        MaximumWaterDepth = 0.0;
        RainfallMm = 0.0;
        Rejections = 0;
        Discontinuities = 0;
        Last = {};
    }

    bool Configure(WeatherTrendSettings settings) noexcept
    {
        if (!ValidWeatherTrendSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(TrackEnvironmentSample sample) noexcept
    {
        if (!ValidEnvironmentSample(sample, Settings))
        {
            ++Rejections;
            Last.Trend = TrackConditionTrend::SensorFault;
            return false;
        }
        double dt = 0.0;
        double wetRate = 0.0;
        double temperatureRate = 0.0;
        if (HasPrevious)
        {
            dt = sample.TimeSeconds -
                Previous.TimeSeconds;
            if (dt <= 0.0 ||
                dt > Settings.MaximumFrameGapSeconds)
            {
                ++Discontinuities;
                ++Rejections;
                Previous = sample;
                Last.Trend = TrackConditionTrend::SensorFault;
                return false;
            }
            wetRate = (
                sample.WetnessFraction -
                Previous.WetnessFraction) / dt;
            temperatureRate = (
                sample.TrackSurfaceC -
                Previous.TrackSurfaceC) / dt;
            if (std::abs(wetRate) >
                    Settings.MaximumWetnessRatePerSecond ||
                std::abs(temperatureRate) >
                    Settings.MaximumTemperatureRateCPerSecond)
            {
                ++Rejections;
                Previous = sample;
                Last.Trend = TrackConditionTrend::SensorFault;
                return false;
            }
            WetRateFilter.Observe(
                wetRate, dt,
                Settings.TrendSmoothSeconds);
            TotalSpan += dt;
            WetSeconds +=
                sample.WetnessFraction >=
                    Settings.WetThreshold ? dt : 0.0;
            DrySeconds +=
                sample.WetnessFraction <
                    Settings.WetThreshold ? dt : 0.0;
            LowVisibilitySeconds +=
                sample.VisibilityFraction < 0.5 ?
                dt : 0.0;
            RainfallMm +=
                sample.RainfallRateMmHour * dt / 3600.0;
        }
        Previous = sample;
        HasPrevious = true;
        WetnessStats.Observe(sample.WetnessFraction);
        TrackTemperatureStats.Observe(sample.TrackSurfaceC);
        AirTemperatureStats.Observe(sample.AmbientC);
        FrictionStats.Observe(sample.RoadFrictionEstimate);
        GripStats.Observe(sample.TireGripEstimate);
        VisibilityStats.Observe(sample.VisibilityFraction);
        WindStats.Observe(sample.WindSpeedCmS);
        HumidityStats.Observe(sample.RelativeHumidityFraction);
        MaximumWetness = std::max(
            MaximumWetness, sample.WetnessFraction);
        MinimumFriction = std::min(
            MinimumFriction, sample.RoadFrictionEstimate);
        MaximumWaterDepth = std::max(
            MaximumWaterDepth, sample.SurfaceWaterDepthMm);
        if (HistoryCount >= static_cast<int>(History.size()))
        {
            HistoryHead = (HistoryHead + 1) %
                static_cast<int>(History.size());
        }
        else
        {
            ++HistoryCount;
        }
        History[static_cast<std::size_t>(
            (HistoryHead + HistoryCount - 1) %
            static_cast<int>(History.size()))] =
            sample;
        UpdateReview(wetRate, temperatureRate);
        return true;
    }

    WeatherTrendReview Snapshot() const noexcept
    {
        return Last;
    }

    TrackEnvironmentSample OldestStored(int index) const noexcept
    {
        return index >= 0 && index < HistoryCount ?
            History[static_cast<std::size_t>(
                (HistoryHead + index) %
                static_cast<int>(History.size()))] :
            TrackEnvironmentSample{};
    }

    TrackEnvironmentSample LatestSample() const noexcept
    {
        return HasPrevious ? Previous :
            TrackEnvironmentSample{};
    }

    int StoredSamples() const noexcept
    {
        return HistoryCount;
    }

private:
    void UpdateReview(double wetRate,
                      double temperatureRate) noexcept
    {
        WeatherTrendReview out;
        out.CurrentWetness = Previous.WetnessFraction;
        out.MeanWetness = WetnessStats.Mean();
        out.MeanSurfaceTemperatureC =
            TrackTemperatureStats.Mean();
        out.MeanAmbientTemperatureC =
            AirTemperatureStats.Mean();
        out.MeanRoadFriction = FrictionStats.Mean();
        out.MeanGripEstimate = GripStats.Mean();
        out.MeanVisibilityFraction =
            VisibilityStats.Mean();
        out.MeanWindSpeedCmS = WindStats.Mean();
        out.MeanHumidityFraction =
            HumidityStats.Mean();
        out.CurrentWetnessRatePerSecond = wetRate;
        out.SmoothedWetnessRatePerSecond =
            WetRateFilter.Value();
        out.CurrentSurfaceTemperatureRateCPerSecond =
            temperatureRate;
        out.WetnessStandardDeviation =
            WetnessStats.Deviation();
        out.TemperatureStandardDeviationC =
            TrackTemperatureStats.Deviation();
        out.MaximumObservedWetness =
            MaximumWetness;
        out.MinimumObservedFriction =
            Finite(MinimumFriction) ?
            MinimumFriction : 0.0;
        out.MaximumObservedWaterDepthMm =
            MaximumWaterDepth;
        out.AccumulatedRainfallMm =
            RainfallMm;
        out.ObservationSpanSeconds =
            TotalSpan;
        out.WetConditionsSeconds = WetSeconds;
        out.DryConditionsSeconds = DrySeconds;
        out.LowVisibilitySeconds =
            LowVisibilitySeconds;
        out.Observations =
            static_cast<int>(WetnessStats.Size());
        out.Rejected = Rejections;
        out.Discontinuities = Discontinuities;
        out.OriginalWetnessMeasured =
            Previous.PhysicallyMeasuredWetness;
        out.OriginalFrictionMeasured =
            Previous.PhysicallyMeasuredFriction;
        out.EnoughHistory = TotalSpan >=
            Settings.MinimumTrendWindowSeconds;
        if (!out.EnoughHistory)
        {
            out.Trend = TrackConditionTrend::Unknown;
        }
        else if (std::abs(out.SmoothedWetnessRatePerSecond) >
                 Settings.TrendRateThresholdPerSecond)
        {
            out.Trend =
                out.SmoothedWetnessRatePerSecond > 0.0 ?
                TrackConditionTrend::Wetting :
                TrackConditionTrend::Drying;
        }
        else if (out.WetnessStandardDeviation > 0.18)
        {
            out.Trend = TrackConditionTrend::Unstable;
        }
        else if (out.CurrentWetness >=
                 Settings.HeavyWetThreshold)
        {
            out.Trend = TrackConditionTrend::StableWet;
        }
        else
        {
            out.Trend = TrackConditionTrend::StableDry;
        }
        if (out.SmoothedWetnessRatePerSecond <
            -Settings.TrendRateThresholdPerSecond)
        {
            out.DryingTimeEstimateSeconds = SafeRatio(
                out.CurrentWetness -
                    Settings.WetThreshold,
                -out.SmoothedWetnessRatePerSecond);
        }
        if (out.SmoothedWetnessRatePerSecond >
            Settings.TrendRateThresholdPerSecond)
        {
            out.TimeToHeavyWetSeconds = SafeRatio(
                Settings.HeavyWetThreshold -
                    out.CurrentWetness,
                out.SmoothedWetnessRatePerSecond);
        }
        out.SafeToRecommendTireChange = false;
        out.AdvisoryOnly = true;
        out.Valid = true;
        Last = out;
    }

    WeatherTrendSettings Settings{};
    TrackEnvironmentSample Previous{};
    WeatherTrendReview Last{};
    RunningStatistics WetnessStats;
    RunningStatistics TrackTemperatureStats;
    RunningStatistics AirTemperatureStats;
    RunningStatistics FrictionStats;
    RunningStatistics GripStats;
    RunningStatistics VisibilityStats;
    RunningStatistics WindStats;
    RunningStatistics HumidityStats;
    ExponentialFilter WetRateFilter;
    std::array<TrackEnvironmentSample, 64> History{};
    int HistoryHead = 0;
    int HistoryCount = 0;
    double TotalSpan = 0.0;
    double WetSeconds = 0.0;
    double DrySeconds = 0.0;
    double LowVisibilitySeconds = 0.0;
    double MaximumWetness = 0.0;
    double MinimumFriction =
        std::numeric_limits<double>::infinity();
    double MaximumWaterDepth = 0.0;
    double RainfallMm = 0.0;
    int Rejections = 0;
    int Discontinuities = 0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
