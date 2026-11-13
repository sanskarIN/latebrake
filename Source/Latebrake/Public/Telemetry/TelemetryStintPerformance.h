#pragma once

#include "Telemetry/TelemetryLapAnalysis.h"
#include "Telemetry/TelemetryFuelStrategy.h"
#include "Telemetry/TelemetryVehicleHealth.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class StintTrendDirection : std::uint8_t
{
    Unknown,
    Improving,
    Stable,
    Degrading,
    Inconsistent
};

enum class StintAssessmentQuality : std::uint8_t
{
    Missing,
    SingleLap,
    Provisional,
    Qualified,
    Confounded
};

struct StintAnalysisSettings final
{
    double MinimumLapSeconds = 3.0;
    double MaximumLapSeconds = 7200.0;
    double MaximumLapDeviationFraction = 0.50;
    double StablePaceSlopeSecondsPerLap = 0.10;
    double DegradingPaceSlopeSecondsPerLap = 0.25;
    double MinimumDegradationCorrelation = 0.45;
    double MinimumTireDegradationFractionPerLap = 0.004;
    double MaximumTireDegradationFractionPerLap = 0.20;
    double MaximumFuelIncreaseFraction = 0.001;
    double MinimumObservedDistanceCm = 1000.0;
    double MaximumCautionFraction = 0.20;
    double MaximumOffTrackSeconds = 1.0;
    double MaximumRecoverySeconds = 0.5;
    double MaximumRateChangePerLap = 100.0;
    double MinimumValidLapConfidence = 0.70;
    int MinimumTrendLaps = 3;
    int MinimumMeanLaps = 2;
    int MaximumStintLaps = MaximumTelemetryLaps;
    bool ExcludeCautionLaps = true;
    bool ExcludeOffTrackLaps = true;
    bool RequireEngineVerifiedLap = true;
};

inline bool ValidStintSettings(
    const StintAnalysisSettings& s) noexcept
{
    return Finite(s.MinimumLapSeconds) &&
           s.MinimumLapSeconds > 0.0 &&
           Finite(s.MaximumLapSeconds) &&
           s.MaximumLapSeconds > s.MinimumLapSeconds &&
           Finite(s.MaximumLapDeviationFraction) &&
           s.MaximumLapDeviationFraction > 0.0 &&
           Finite(s.StablePaceSlopeSecondsPerLap) &&
           s.StablePaceSlopeSecondsPerLap >= 0.0 &&
           Finite(s.DegradingPaceSlopeSecondsPerLap) &&
           s.DegradingPaceSlopeSecondsPerLap >
               s.StablePaceSlopeSecondsPerLap &&
           Finite(s.MinimumDegradationCorrelation) &&
           s.MinimumDegradationCorrelation >= 0.0 &&
           s.MinimumDegradationCorrelation <= 1.0 &&
           Finite(s.MinimumTireDegradationFractionPerLap) &&
           s.MinimumTireDegradationFractionPerLap >= 0.0 &&
           Finite(s.MaximumTireDegradationFractionPerLap) &&
           s.MaximumTireDegradationFractionPerLap >
               s.MinimumTireDegradationFractionPerLap &&
           Finite(s.MaximumFuelIncreaseFraction) &&
           s.MaximumFuelIncreaseFraction >= 0.0 &&
           Finite(s.MinimumObservedDistanceCm) &&
           s.MinimumObservedDistanceCm > 0.0 &&
           Finite(s.MaximumCautionFraction) &&
           s.MaximumCautionFraction >= 0.0 &&
           s.MaximumCautionFraction <= 1.0 &&
           Finite(s.MaximumOffTrackSeconds) &&
           s.MaximumOffTrackSeconds >= 0.0 &&
           Finite(s.MaximumRecoverySeconds) &&
           s.MaximumRecoverySeconds >= 0.0 &&
           Finite(s.MaximumRateChangePerLap) &&
           s.MaximumRateChangePerLap > 0.0 &&
           Finite(s.MinimumValidLapConfidence) &&
           s.MinimumValidLapConfidence >= 0.0 &&
           s.MinimumValidLapConfidence <= 1.0 &&
           s.MinimumTrendLaps > 1 &&
           s.MinimumTrendLaps <= MaximumTelemetryLaps &&
           s.MinimumMeanLaps > 0 &&
           s.MinimumMeanLaps <= s.MinimumTrendLaps &&
           s.MaximumStintLaps >= s.MinimumTrendLaps &&
           s.MaximumStintLaps <= MaximumTelemetryLaps;
}

struct StintLapSample final
{
    int LapOrdinal = -1;
    double LapTimeSeconds = 0.0;
    double DistanceCm = 0.0;
    double MeanSpeedKmh = 0.0;
    double MaximumSpeedKmh = 0.0;
    double BrakingSeconds = 0.0;
    double HighSlipSeconds = 0.0;
    double OffTrackSeconds = 0.0;
    double CautionSeconds = 0.0;
    double FuelAtStart = 0.0;
    double FuelAtFinish = 0.0;
    double FuelConsumedFraction = 0.0;
    double TireWearAtStart = 0.0;
    double TireWearAtFinish = 0.0;
    double TireWearIncreaseFraction = 0.0;
    double EngineTemperaturePeakC = 0.0;
    double BrakeTemperaturePeakC = 0.0;
    double DamageFraction = 0.0;
    double ConsistencyConfidence = 0.0;
    double TrackLengthCm = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    bool VerifiedForwardFinish = false;
    bool NativeMeasured = false;
    bool PitAffected = false;
    bool Qualified = false;
};

struct StintPaceTrend final
{
    StintTrendDirection Trend = StintTrendDirection::Unknown;
    StintAssessmentQuality Quality =
        StintAssessmentQuality::Missing;
    double MeanLapSeconds = 0.0;
    double MedianLapSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double WorstLapSeconds = 0.0;
    double LapTimeDeviationSeconds = 0.0;
    double PaceSlopeSecondsPerLap = 0.0;
    double RegressionInterceptSeconds = 0.0;
    double PaceCorrelation = 0.0;
    double MeanTireWearIncreasePerLap = 0.0;
    double FuelBurnFractionPerLap = 0.0;
    double MeanBrakeSecondsPerLap = 0.0;
    double MeanHighSlipSecondsPerLap = 0.0;
    double InitialLapMeanSeconds = 0.0;
    double RecentLapMeanSeconds = 0.0;
    double RecentPaceDeltaSeconds = 0.0;
    double ProjectedNextLapSeconds = 0.0;
    double ProjectedThreeLapMeanSeconds = 0.0;
    double EstimatedWearAtNextLap = 0.0;
    double TotalDistanceCm = 0.0;
    double TotalDrivenSeconds = 0.0;
    double Confidence = 0.0;
    int QualifiedLapCount = 0;
    int ExcludedLapCount = 0;
    int FirstLapOrdinal = -1;
    int LastLapOrdinal = -1;
    bool TireDegradationSuspected = false;
    bool PaceDegradationSuspected = false;
    bool FuelEstimateSupported = false;
    bool Qualified = false;
};

inline bool ValidStintSample(
    const StintLapSample& sample,
    const StintAnalysisSettings& s) noexcept
{
    return sample.Qualified &&
           sample.LapOrdinal >= 0 &&
           sample.LapOrdinal < MaximumTelemetryLaps &&
           sample.SessionToken != 0 &&
           sample.ActorToken != 0 &&
           Finite(sample.LapTimeSeconds) &&
           sample.LapTimeSeconds >= s.MinimumLapSeconds &&
           sample.LapTimeSeconds <= s.MaximumLapSeconds &&
           Finite(sample.DistanceCm) &&
           sample.DistanceCm >= s.MinimumObservedDistanceCm &&
           Finite(sample.MeanSpeedKmh) &&
           sample.MeanSpeedKmh >= 0.0 &&
           Finite(sample.MaximumSpeedKmh) &&
           sample.MaximumSpeedKmh >= 0.0 &&
           Finite(sample.BrakingSeconds) &&
           sample.BrakingSeconds >= 0.0 &&
           Finite(sample.HighSlipSeconds) &&
           sample.HighSlipSeconds >= 0.0 &&
           Finite(sample.OffTrackSeconds) &&
           sample.OffTrackSeconds >= 0.0 &&
           Finite(sample.CautionSeconds) &&
           sample.CautionSeconds >= 0.0 &&
           Finite(sample.FuelAtStart) &&
           sample.FuelAtStart >= 0.0 &&
           sample.FuelAtStart <= 1.0 &&
           Finite(sample.FuelAtFinish) &&
           sample.FuelAtFinish >= 0.0 &&
           sample.FuelAtFinish <= 1.0 &&
           Finite(sample.TireWearAtStart) &&
           sample.TireWearAtStart >= 0.0 &&
           sample.TireWearAtStart <= 1.0 &&
           Finite(sample.TireWearAtFinish) &&
           sample.TireWearAtFinish >= 0.0 &&
           sample.TireWearAtFinish <= 1.0 &&
           Finite(sample.DamageFraction) &&
           sample.DamageFraction >= 0.0 &&
           sample.DamageFraction <= 1.0 &&
           Finite(sample.ConsistencyConfidence) &&
           sample.ConsistencyConfidence >= 0.0 &&
           sample.ConsistencyConfidence <= 1.0 &&
           (!s.RequireEngineVerifiedLap ||
            (sample.NativeMeasured &&
             sample.VerifiedForwardFinish));
}

class StintPaceMonitor final
{
public:
    bool Configure(StintAnalysisSettings settings) noexcept
    {
        if (!ValidStintSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Samples.fill({});
        Count = 0;
        Excluded = 0;
        Session = 0;
        Actor = 0;
        NextLap = -1;
        PreviousFuel = 0.0;
        PreviousTireWear = 0.0;
        Last = {};
    }

    bool Observe(StintLapSample sample) noexcept
    {
        if (!ValidStintSample(sample, Settings) ||
            Count >= Settings.MaximumStintLaps)
        {
            ++Excluded;
            Recalculate();
            return false;
        }
        if (Session != 0 &&
            (Session != sample.SessionToken ||
             Actor != sample.ActorToken))
        {
            Reset();
        }
        Session = sample.SessionToken;
        Actor = sample.ActorToken;
        if (NextLap >= 0 && sample.LapOrdinal != NextLap)
        {
            ++Excluded;
            Recalculate();
            return false;
        }
        if ((Settings.ExcludeCautionLaps &&
             sample.CautionSeconds > sample.LapTimeSeconds *
                Settings.MaximumCautionFraction) ||
            (Settings.ExcludeOffTrackLaps &&
             sample.OffTrackSeconds >
                Settings.MaximumOffTrackSeconds) ||
            sample.PitAffected ||
            sample.ConsistencyConfidence <
                Settings.MinimumValidLapConfidence)
        {
            ++Excluded;
            NextLap = sample.LapOrdinal + 1;
            Recalculate();
            return false;
        }
        const double consumed =
            sample.FuelAtStart - sample.FuelAtFinish;
        const double wear =
            sample.TireWearAtFinish -
            sample.TireWearAtStart;
        if (consumed < -Settings.MaximumFuelIncreaseFraction ||
            wear < -Settings.MaximumFuelIncreaseFraction ||
            wear > Settings.MaximumTireDegradationFractionPerLap)
        {
            ++Excluded;
            NextLap = sample.LapOrdinal + 1;
            Recalculate();
            return false;
        }
        sample.FuelConsumedFraction = std::max(0.0, consumed);
        sample.TireWearIncreaseFraction = std::max(0.0, wear);
        Samples[static_cast<std::size_t>(Count++)] = sample;
        NextLap = sample.LapOrdinal + 1;
        PreviousFuel = sample.FuelAtFinish;
        PreviousTireWear = sample.TireWearAtFinish;
        Recalculate();
        return true;
    }

    StintPaceTrend Snapshot() const noexcept { return Last; }
    int QualifiedLaps() const noexcept { return Count; }
    int ExcludedLaps() const noexcept { return Excluded; }
    StintLapSample Sample(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Samples[static_cast<std::size_t>(index)] :
            StintLapSample{};
    }

    double ProjectedLapAt(int lapIndex) const noexcept
    {
        if (!Last.Qualified || lapIndex < 0 ||
            lapIndex >= MaximumTelemetryLaps)
        {
            return 0.0;
        }
        return std::clamp(
            Last.RegressionInterceptSeconds +
            Last.PaceSlopeSecondsPerLap *
                static_cast<double>(lapIndex),
            Settings.MinimumLapSeconds,
            Settings.MaximumLapSeconds);
    }

    bool WearLimited() const noexcept
    {
        return Last.TireDegradationSuspected &&
            Last.PaceDegradationSuspected;
    }

private:
    void Recalculate() noexcept
    {
        StintPaceTrend report;
        report.QualifiedLapCount = Count;
        report.ExcludedLapCount = Excluded;
        if (Count == 0)
        {
            Last = report;
            return;
        }
        RunningStatistics lapTimes;
        RunningStatistics burn;
        RunningStatistics wear;
        RunningStatistics brakes;
        RunningStatistics slip;
        RollingWindow<128> sampleTimes;
        double sumX = 0.0;
        double sumY = 0.0;
        double sumXX = 0.0;
        double sumXY = 0.0;
        double sumYY = 0.0;
        double initialSum = 0.0;
        double recentSum = 0.0;
        int initialCount = 0;
        int recentCount = 0;
        for (int i = 0; i < Count; ++i)
        {
            const StintLapSample& s =
                Samples[static_cast<std::size_t>(i)];
            const double x = static_cast<double>(s.LapOrdinal);
            const double y = s.LapTimeSeconds;
            sumX += x;
            sumY += y;
            sumXX += x * x;
            sumXY += x * y;
            sumYY += y * y;
            lapTimes.Observe(y);
            burn.Observe(s.FuelConsumedFraction);
            wear.Observe(s.TireWearIncreaseFraction);
            brakes.Observe(s.BrakingSeconds);
            slip.Observe(s.HighSlipSeconds);
            sampleTimes.Push(y);
            report.TotalDistanceCm += s.DistanceCm;
            report.TotalDrivenSeconds += y;
            if (i < 3)
            {
                initialSum += y;
                ++initialCount;
            }
            if (i >= Count - 3)
            {
                recentSum += y;
                ++recentCount;
            }
        }
        const double n = static_cast<double>(Count);
        const double denominator =
            n * sumXX - sumX * sumX;
        const double slope = std::abs(denominator) > Tiny ?
            (n * sumXY - sumX * sumY) / denominator : 0.0;
        const double intercept =
            (sumY - slope * sumX) / n;
        const double correlationNumerator =
            n * sumXY - sumX * sumY;
        const double correlationDenominator =
            SafeSquareRoot(
                (n * sumXX - sumX * sumX) *
                (n * sumYY - sumY * sumY));
        report.MeanLapSeconds = lapTimes.Mean();
        report.MedianLapSeconds = sampleTimes.Percentile(0.5);
        report.BestLapSeconds = lapTimes.Minimum();
        report.WorstLapSeconds = lapTimes.Maximum();
        report.LapTimeDeviationSeconds =
            lapTimes.Deviation();
        report.PaceSlopeSecondsPerLap = slope;
        report.RegressionInterceptSeconds = intercept;
        report.PaceCorrelation =
            SafeRatio(correlationNumerator,
                      correlationDenominator);
        report.MeanTireWearIncreasePerLap = wear.Mean();
        report.FuelBurnFractionPerLap = burn.Mean();
        report.MeanBrakeSecondsPerLap = brakes.Mean();
        report.MeanHighSlipSecondsPerLap = slip.Mean();
        report.InitialLapMeanSeconds =
            SafeRatio(initialSum, initialCount);
        report.RecentLapMeanSeconds =
            SafeRatio(recentSum, recentCount);
        report.RecentPaceDeltaSeconds =
            report.RecentLapMeanSeconds -
            report.InitialLapMeanSeconds;
        report.FirstLapOrdinal = Samples[0].LapOrdinal;
        report.LastLapOrdinal =
            Samples[static_cast<std::size_t>(Count - 1)].LapOrdinal;
        report.ProjectedNextLapSeconds = std::clamp(
            intercept + slope *
                static_cast<double>(report.LastLapOrdinal + 1),
            Settings.MinimumLapSeconds,
            Settings.MaximumLapSeconds);
        report.ProjectedThreeLapMeanSeconds = std::clamp(
            intercept + slope *
                static_cast<double>(report.LastLapOrdinal + 2),
            Settings.MinimumLapSeconds,
            Settings.MaximumLapSeconds);
        report.EstimatedWearAtNextLap = std::clamp(
            Samples[static_cast<std::size_t>(Count - 1)]
                .TireWearAtFinish +
            wear.Mean(), 0.0, 1.0);
        report.TireDegradationSuspected =
            wear.Mean() >=
                Settings.MinimumTireDegradationFractionPerLap;
        report.PaceDegradationSuspected =
            slope >= Settings.DegradingPaceSlopeSecondsPerLap &&
            report.PaceCorrelation >=
                Settings.MinimumDegradationCorrelation;
        report.FuelEstimateSupported =
            burn.Mean() > 0.0 &&
            Count >= Settings.MinimumMeanLaps;
        report.Confidence = std::clamp(
            static_cast<double>(Count) /
            static_cast<double>(Settings.MinimumTrendLaps) *
            (1.0 - std::min(1.0,
                SafeRatio(report.LapTimeDeviationSeconds,
                          report.MeanLapSeconds))),
            0.0, 1.0);
        if (Count < Settings.MinimumTrendLaps)
        {
            report.Trend = StintTrendDirection::Unknown;
            report.Quality = Count == 1 ?
                StintAssessmentQuality::SingleLap :
                StintAssessmentQuality::Provisional;
        }
        else if (std::abs(slope) <=
                 Settings.StablePaceSlopeSecondsPerLap)
        {
            report.Trend = StintTrendDirection::Stable;
        }
        else if (slope < 0.0)
        {
            report.Trend = StintTrendDirection::Improving;
        }
        else if (slope >= Settings.DegradingPaceSlopeSecondsPerLap)
        {
            report.Trend = StintTrendDirection::Degrading;
        }
        else
        {
            report.Trend = StintTrendDirection::Inconsistent;
        }
        if (Count >= Settings.MinimumTrendLaps)
        {
            report.Quality = StintAssessmentQuality::Qualified;
        }
        report.Qualified = Count >= Settings.MinimumTrendLaps &&
            report.Confidence >=
                Settings.MinimumValidLapConfidence;
        Last = report;
    }

    StintAnalysisSettings Settings{};
    std::array<StintLapSample, MaximumTelemetryLaps> Samples{};
    StintPaceTrend Last{};
    int Count = 0;
    int Excluded = 0;
    int NextLap = -1;
    double PreviousFuel = 0.0;
    double PreviousTireWear = 0.0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
};
} // namespace latebrake::engineering
