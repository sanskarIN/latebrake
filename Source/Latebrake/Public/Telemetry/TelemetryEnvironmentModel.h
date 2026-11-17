#pragma once

#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryTrack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class TrackConditionClass : std::uint8_t
{
    Unknown,
    Dry,
    Damp,
    Wet,
    Saturated,
    Unreliable
};

enum class ConditionTrend : std::uint8_t
{
    Unknown,
    Drying,
    Stable,
    Worsening
};

struct EnvironmentAnalysisSettings final
{
    double DampWetnessFraction = 0.12;
    double WetWetnessFraction = 0.40;
    double SaturatedWetnessFraction = 0.83;
    double MinimumSensibleGrip = 0.10;
    double MaximumSensibleGrip = 2.0;
    double WetnessFilterSeconds = 2.0;
    double GripFilterSeconds = 1.0;
    double TemperatureFilterSeconds = 3.0;
    double TrendFilterSeconds = 4.0;
    double ChangeAlertPerSecond = 0.01;
    double MinimumStableSeconds = 3.0;
    double MaximumAcceptedFrameSeconds = 0.1;
    double MaximumSuddenWetnessJump = 0.75;
    double MaximumSuddenGripJump = 1.0;
    double MaximumAmbientVariationC = 60.0;
    bool RequireNativeWeatherObservation = true;
    bool RequireTrackProjection = false;
};

inline bool ValidEnvironmentSettings(
    const EnvironmentAnalysisSettings& s) noexcept
{
    return Finite(s.DampWetnessFraction) &&
           s.DampWetnessFraction >= 0.0 &&
           Finite(s.WetWetnessFraction) &&
           s.WetWetnessFraction > s.DampWetnessFraction &&
           Finite(s.SaturatedWetnessFraction) &&
           s.SaturatedWetnessFraction > s.WetWetnessFraction &&
           s.SaturatedWetnessFraction <= 1.0 &&
           Finite(s.MinimumSensibleGrip) &&
           s.MinimumSensibleGrip > 0.0 &&
           Finite(s.MaximumSensibleGrip) &&
           s.MaximumSensibleGrip > s.MinimumSensibleGrip &&
           Finite(s.WetnessFilterSeconds) &&
           s.WetnessFilterSeconds > 0.0 &&
           Finite(s.GripFilterSeconds) &&
           s.GripFilterSeconds > 0.0 &&
           Finite(s.TemperatureFilterSeconds) &&
           s.TemperatureFilterSeconds > 0.0 &&
           Finite(s.TrendFilterSeconds) &&
           s.TrendFilterSeconds > 0.0 &&
           Finite(s.ChangeAlertPerSecond) &&
           s.ChangeAlertPerSecond > 0.0 &&
           Finite(s.MinimumStableSeconds) &&
           s.MinimumStableSeconds > 0.0 &&
           Finite(s.MaximumAcceptedFrameSeconds) &&
           s.MaximumAcceptedFrameSeconds > 0.0 &&
           s.MaximumAcceptedFrameSeconds <= 1.0 &&
           Finite(s.MaximumSuddenWetnessJump) &&
           s.MaximumSuddenWetnessJump > 0.0 &&
           Finite(s.MaximumSuddenGripJump) &&
           s.MaximumSuddenGripJump > 0.0 &&
           Finite(s.MaximumAmbientVariationC) &&
           s.MaximumAmbientVariationC > 0.0;
}

struct ConditionSample final
{
    double Seconds = 0.0;
    double Wetness = 0.0;
    double RoadFriction = 0.0;
    double TireGripEstimate = 0.0;
    double TrackTemperatureC = 0.0;
    double AirTemperatureC = 0.0;
    double TrackAirDifferenceC = 0.0;
    double WindSpeedCmS = 0.0;
    double WindAlongHeadingCmS = 0.0;
    double WindAcrossHeadingCmS = 0.0;
    double VisibilityFraction = 0.0;
    double ChangeFromPreviousWetness = 0.0;
    double ChangeFromPreviousFriction = 0.0;
    double WetnessChangePerSecond = 0.0;
    double FrictionChangePerSecond = 0.0;
    double EstimatedDryingSeconds =
        std::numeric_limits<double>::infinity();
    double EstimatedGripTimeMargin = 0.0;
    double Confidence = 0.0;
    int Sector = -1;
    TrackConditionClass Class = TrackConditionClass::Unknown;
    ConditionTrend Trend = ConditionTrend::Unknown;
    bool NativeObserved = false;
    bool TrackLocalized = false;
    bool Valid = false;
};

struct SectorEnvironmentSummary final
{
    int SectorIndex = -1;
    std::uint64_t QualifiedFrames = 0;
    double ObservedSeconds = 0.0;
    double MeanWetness = 0.0;
    double MinimumWetness = 0.0;
    double MaximumWetness = 0.0;
    double MeanFriction = 0.0;
    double MinimumFriction = 0.0;
    double MeanTemperatureC = 0.0;
    double MaximumGripLoss = 0.0;
    double WetSeconds = 0.0;
    double DampSeconds = 0.0;
    double DrySeconds = 0.0;
    double MaximumWindCmS = 0.0;
    std::uint64_t AbruptChanges = 0;
    TrackConditionClass WorstCondition = TrackConditionClass::Unknown;
    bool Qualified = false;
};

inline TrackConditionClass ClassifyTrackWetness(
    double wetness,
    const EnvironmentAnalysisSettings& s) noexcept
{
    if (!Finite(wetness) ||
        wetness < 0.0 || wetness > 1.0)
    {
        return TrackConditionClass::Unknown;
    }
    if (wetness >= s.SaturatedWetnessFraction)
    {
        return TrackConditionClass::Saturated;
    }
    if (wetness >= s.WetWetnessFraction)
    {
        return TrackConditionClass::Wet;
    }
    if (wetness >= s.DampWetnessFraction)
    {
        return TrackConditionClass::Damp;
    }
    return TrackConditionClass::Dry;
}

inline double EstimatedSurfaceGripFactor(
    double wetness,
    double friction,
    double temperatureC) noexcept
{
    if (!Finite(wetness) || !Finite(friction) ||
        !Finite(temperatureC) ||
        wetness < 0.0 || wetness > 1.0 ||
        friction <= 0.0 || friction > 4.0)
    {
        return 0.0;
    }
    const double waterFactor = std::clamp(
        1.0 - 0.30 * wetness - 0.34 * wetness * wetness,
        0.20, 1.0);
    const double temperatureFactor =
        temperatureC < 5.0 ? 0.85 :
        temperatureC > 65.0 ? 0.90 : 1.0;
    return std::clamp(
        friction * waterFactor * temperatureFactor,
        0.05, 2.5);
}

class TrackEnvironmentMonitor final
{
public:
    bool Configure(
        EnvironmentAnalysisSettings settings) noexcept
    {
        if (!ValidEnvironmentSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        SectorStats.fill({});
        Current = {};
        Previous = {};
        WetnessSmooth.Reset();
        FrictionSmooth.Reset();
        TemperatureSmooth.Reset();
        WetnessRateSmooth.Reset();
        GripStats.Reset();
        TemperatureStats.Reset();
        Accepted = 0;
        Rejected = 0;
        AbruptChanges = 0;
        ObservedSeconds = 0.0;
        StableSeconds = 0.0;
        PreviousSeconds = 0.0;
        HavePrevious = false;
        Session = 0;
        Actor = 0;
    }

    void Suspend() noexcept
    {
        WetnessSmooth.Reset();
        FrictionSmooth.Reset();
        TemperatureSmooth.Reset();
        WetnessRateSmooth.Reset();
        HavePrevious = false;
        Current = {};
        StableSeconds = 0.0;
    }

    bool Observe(
        const TelemetryFrame& frame,
        const GripTelemetry& grip,
        const TrackProjection& track,
        double dt) noexcept
    {
        if (!ValidEnvironmentSettings(Settings) ||
            !Finite(dt) || dt <= 0.0 ||
            dt > Settings.MaximumAcceptedFrameSeconds ||
            !ValidTelemetryEnvironment(frame.Environment) ||
            (Settings.RequireNativeWeatherObservation &&
             (!frame.Race.EnginePhysicsReady ||
              !IsEngineObservation(frame.Identity.Trust))) ||
            (Settings.RequireTrackProjection && !track.Valid) ||
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
        const TelemetryEnvironment& source = frame.Environment;
        const double seconds = frame.MonotonicSeconds;
        const double wetness =
            WetnessSmooth.Observe(
                source.AmbientWetness, dt,
                Settings.WetnessFilterSeconds);
        const double friction =
            FrictionSmooth.Observe(
                source.RoadFrictionEstimate, dt,
                Settings.GripFilterSeconds);
        const double temperature =
            TemperatureSmooth.Observe(
                source.TrackTemperatureCelsius, dt,
                Settings.TemperatureFilterSeconds);
        const double measuredGrip = grip.Valid ?
            grip.LoadWeightedGripFactor :
            EstimatedSurfaceGripFactor(
                wetness, friction, temperature);
        ConditionSample sample;
        sample.Seconds = seconds;
        sample.Wetness = wetness;
        sample.RoadFriction = friction;
        sample.TireGripEstimate = measuredGrip;
        sample.TrackTemperatureC = temperature;
        sample.AirTemperatureC = source.AirTemperatureCelsius;
        sample.TrackAirDifferenceC =
            temperature - source.AirTemperatureCelsius;
        sample.WindSpeedCmS = source.WindSpeedCmS;
        const Vector2 wind =
            HeadingUnit(source.WindDirectionRadians) *
            source.WindSpeedCmS;
        const Vector2 forward =
            HeadingUnit(frame.Dynamics.YawRadians);
        sample.WindAlongHeadingCmS = Dot(wind, forward);
        sample.WindAcrossHeadingCmS =
            Cross(forward, wind);
        sample.VisibilityFraction = source.VisibilityFraction;
        sample.Confidence = 0.6 + (grip.Valid ?
            std::clamp(grip.Confidence, 0.0, 1.0) * 0.4 : 0.0);
        sample.Sector = track.Valid ? track.SectorIndex : -1;
        sample.TrackLocalized = track.Valid;
        sample.NativeObserved =
            IsEngineObservation(frame.Identity.Trust);
        sample.Class = ClassifyTrackWetness(wetness, Settings);
        sample.Valid = true;
        if (HavePrevious)
        {
            sample.ChangeFromPreviousWetness =
                wetness - Previous.Wetness;
            sample.ChangeFromPreviousFriction =
                friction - Previous.RoadFriction;
            sample.WetnessChangePerSecond =
                SafeRatio(sample.ChangeFromPreviousWetness,
                          std::max(dt, 0.001));
            sample.FrictionChangePerSecond =
                SafeRatio(sample.ChangeFromPreviousFriction,
                          std::max(dt, 0.001));
        }
        const double smoothedRate = WetnessRateSmooth.Observe(
            sample.WetnessChangePerSecond,
            dt, Settings.TrendFilterSeconds);
        if (!HavePrevious ||
            std::abs(smoothedRate) < Settings.ChangeAlertPerSecond)
        {
            sample.Trend = ConditionTrend::Stable;
        }
        else if (smoothedRate > 0.0)
        {
            sample.Trend = ConditionTrend::Worsening;
        }
        else
        {
            sample.Trend = ConditionTrend::Drying;
        }
        sample.EstimatedDryingSeconds = smoothedRate < -Tiny ?
            (wetness - Settings.DampWetnessFraction) /
                -smoothedRate :
            std::numeric_limits<double>::infinity();
        if (sample.EstimatedDryingSeconds < 0.0)
        {
            sample.EstimatedDryingSeconds = 0.0;
        }
        const double nominalGrip = EstimatedSurfaceGripFactor(
            0.0, friction, temperature);
        sample.EstimatedGripTimeMargin =
            measuredGrip - nominalGrip;
        bool abrupt = false;
        if (HavePrevious)
        {
            abrupt =
                std::abs(source.AmbientWetness -
                         Previous.Wetness) >
                    Settings.MaximumSuddenWetnessJump ||
                std::abs(source.RoadFrictionEstimate -
                         Previous.RoadFriction) >
                    Settings.MaximumSuddenGripJump;
        }
        if (abrupt)
        {
            AbruptChanges = SaturatingIncrement(AbruptChanges);
            StableSeconds = 0.0;
        }
        else
        {
            StableSeconds += dt;
        }
        if (sample.Sector >= 0 &&
            sample.Sector < MaximumTelemetrySectors)
        {
            const std::size_t sector =
                static_cast<std::size_t>(sample.Sector);
            SectorEnvironmentSummary& stats =
                SectorStats[sector];
            stats.SectorIndex = sample.Sector;
            if (stats.QualifiedFrames == 0)
            {
                stats.MinimumWetness = wetness;
                stats.MaximumWetness = wetness;
                stats.MinimumFriction = friction;
                stats.MeanWetness = wetness;
                stats.MeanFriction = friction;
                stats.MeanTemperatureC = temperature;
            }
            stats.QualifiedFrames =
                SaturatingIncrement(stats.QualifiedFrames);
            const double count =
                static_cast<double>(stats.QualifiedFrames);
            stats.MeanWetness +=
                (wetness - stats.MeanWetness) / count;
            stats.MeanFriction +=
                (friction - stats.MeanFriction) / count;
            stats.MeanTemperatureC +=
                (temperature - stats.MeanTemperatureC) / count;
            stats.MinimumWetness =
                std::min(stats.MinimumWetness, wetness);
            stats.MaximumWetness =
                std::max(stats.MaximumWetness, wetness);
            stats.MinimumFriction =
                std::min(stats.MinimumFriction, friction);
            stats.MaximumGripLoss = std::max(
                stats.MaximumGripLoss,
                std::max(0.0, nominalGrip - measuredGrip));
            stats.MaximumWindCmS = std::max(
                stats.MaximumWindCmS, source.WindSpeedCmS);
            stats.WetSeconds +=
                sample.Class == TrackConditionClass::Wet ||
                sample.Class == TrackConditionClass::Saturated ?
                    dt : 0.0;
            stats.DampSeconds +=
                sample.Class == TrackConditionClass::Damp ?
                    dt : 0.0;
            stats.DrySeconds +=
                sample.Class == TrackConditionClass::Dry ?
                    dt : 0.0;
            stats.ObservedSeconds += dt;
            stats.AbruptChanges += abrupt ? 1U : 0U;
            stats.WorstCondition =
                static_cast<int>(sample.Class) >
                    static_cast<int>(stats.WorstCondition) ?
                        sample.Class : stats.WorstCondition;
            stats.Qualified = true;
        }
        Current = sample;
        Previous = sample;
        HavePrevious = true;
        PreviousSeconds = seconds;
        ObservedSeconds += dt;
        Accepted = SaturatingIncrement(Accepted);
        GripStats.Observe(measuredGrip);
        TemperatureStats.Observe(temperature);
        return true;
    }

    ConditionSample Snapshot() const noexcept { return Current; }
    SectorEnvironmentSummary SectorReview(int sector) const noexcept
    {
        return sector >= 0 && sector < MaximumTelemetrySectors ?
            SectorStats[static_cast<std::size_t>(sector)] :
            SectorEnvironmentSummary{};
    }
    double MeanEstimatedGrip() const noexcept
    {
        return GripStats.Mean();
    }
    double MeanTrackTemperatureC() const noexcept
    {
        return TemperatureStats.Mean();
    }
    double MonitoredSeconds() const noexcept { return ObservedSeconds; }
    double StableConditionSeconds() const noexcept { return StableSeconds; }
    std::uint64_t AbruptConditionChanges() const noexcept
    {
        return AbruptChanges;
    }
    std::uint64_t AcceptedSamples() const noexcept { return Accepted; }
    std::uint64_t RejectedSamples() const noexcept { return Rejected; }

private:
    EnvironmentAnalysisSettings Settings{};
    std::array<SectorEnvironmentSummary, MaximumTelemetrySectors>
        SectorStats{};
    ConditionSample Current{};
    ConditionSample Previous{};
    ExponentialFilter WetnessSmooth;
    ExponentialFilter FrictionSmooth;
    ExponentialFilter TemperatureSmooth;
    ExponentialFilter WetnessRateSmooth;
    RunningStatistics GripStats;
    RunningStatistics TemperatureStats;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t AbruptChanges = 0;
    double ObservedSeconds = 0.0;
    double StableSeconds = 0.0;
    double PreviousSeconds = 0.0;
    bool HavePrevious = false;
};
} // namespace latebrake::engineering
