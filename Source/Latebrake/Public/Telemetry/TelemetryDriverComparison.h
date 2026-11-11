#pragma once

#include "Telemetry/TelemetryDeltaComparison.h"
#include "Telemetry/TelemetryLapAnalysis.h"
#include "Telemetry/TelemetryCornering.h"
#include "Telemetry/TelemetryBraking.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class DriverComparisonStatus : std::uint8_t
{
    Missing,
    DifferentTrack,
    DifferentRules,
    InsufficientLaps,
    UnqualifiedSource,
    InvalidMeasurements,
    Comparable,
    Provisional
};

enum class ComparisonAdvantage : std::uint8_t
{
    Unavailable,
    DriverA,
    DriverB,
    ApproximatelyEqual
};

struct DriverComparisonSettings final
{
    double MinimumLapSeconds = 3.0;
    double MaximumLapSeconds = 3600.0;
    double MaximumTrackLengthDifferenceCm = 5.0;
    double MaximumFinishClockDifferenceSeconds = 3600.0;
    double MinimumMeaningfulLapDeltaSeconds = 0.05;
    double MinimumMeaningfulSpeedDeltaCmS = 40.0;
    double MinimumMeaningfulCornerScoreDelta = 2.0;
    double MaximumMeanOffTrackSeconds = 10.0;
    double MinimumDrivingSeconds = 5.0;
    double MaximumSectorVarianceSeconds = 60.0;
    double MinimumCompletenessFraction = 0.70;
    int MinimumSampledLaps = 1;
    int MinimumSharedSectors = 2;
    bool RequireSameTrackFingerprint = true;
    bool RequireNativeMeasuredLaps = true;
    bool IncludeCautionLapComparison = false;
    bool AllowDifferentCarClasses = false;
};

inline bool ValidDriverComparisonSettings(
    const DriverComparisonSettings& s) noexcept
{
    return Finite(s.MinimumLapSeconds) &&
           s.MinimumLapSeconds > 0.0 &&
           Finite(s.MaximumLapSeconds) &&
           s.MaximumLapSeconds > s.MinimumLapSeconds &&
           Finite(s.MaximumTrackLengthDifferenceCm) &&
           s.MaximumTrackLengthDifferenceCm >= 0.0 &&
           Finite(s.MaximumFinishClockDifferenceSeconds) &&
           s.MaximumFinishClockDifferenceSeconds > 0.0 &&
           Finite(s.MinimumMeaningfulLapDeltaSeconds) &&
           s.MinimumMeaningfulLapDeltaSeconds > 0.0 &&
           Finite(s.MinimumMeaningfulSpeedDeltaCmS) &&
           s.MinimumMeaningfulSpeedDeltaCmS > 0.0 &&
           Finite(s.MinimumMeaningfulCornerScoreDelta) &&
           s.MinimumMeaningfulCornerScoreDelta > 0.0 &&
           Finite(s.MaximumMeanOffTrackSeconds) &&
           s.MaximumMeanOffTrackSeconds >= 0.0 &&
           Finite(s.MinimumDrivingSeconds) &&
           s.MinimumDrivingSeconds > 0.0 &&
           Finite(s.MaximumSectorVarianceSeconds) &&
           s.MaximumSectorVarianceSeconds > 0.0 &&
           Finite(s.MinimumCompletenessFraction) &&
           s.MinimumCompletenessFraction > 0.0 &&
           s.MinimumCompletenessFraction <= 1.0 &&
           s.MinimumSampledLaps > 0 &&
           s.MinimumSampledLaps <= MaximumTelemetryLaps &&
           s.MinimumSharedSectors >= 1 &&
           s.MinimumSharedSectors <= MaximumTelemetrySectors;
}

struct DriverPerformanceRecord final
{
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t TrackFingerprint = 0;
    TelemetryVehicleClass VehicleClass = TelemetryVehicleClass::Unknown;
    int DriverSlot = -1;
    int CompletedLaps = 0;
    int ValidTimedLaps = 0;
    int SectorCount = 0;
    double TrackLengthCm = 0.0;
    double RaceClockSeconds = 0.0;
    double BestLapSeconds = 0.0;
    double MeanLapSeconds = 0.0;
    double MedianLapSeconds = 0.0;
    double LapTimeDeviationSeconds = 0.0;
    double TheoreticalBestLapSeconds = 0.0;
    double MeanPeakSpeedKmh = 0.0;
    double MeanBrakingSecondsPerLap = 0.0;
    double MeanOffTrackSecondsPerLap = 0.0;
    double MeanCornerScore = 0.0;
    double MeanApexSpeedCmS = 0.0;
    double MeanCornerExitSpeedCmS = 0.0;
    double MeanPeakSlipRadians = 0.0;
    double MeanBrakeEventDecelerationG = 0.0;
    double GripLimitTimeFraction = 0.0;
    double FuelConsumedFraction = 0.0;
    double ObservedDistanceCm = 0.0;
    double Confidence = 0.0;
    std::array<double, MaximumTelemetrySectors> BestSectorSeconds{};
    std::array<double, MaximumTelemetrySectors> MeanSectorSeconds{};
    std::array<int, MaximumTelemetrySectors> SectorSamples{};
    bool EngineObserved = false;
    bool WasCautionAffected = false;
    bool Qualified = false;
};

struct DriverComparisonResult final
{
    DriverComparisonStatus Status = DriverComparisonStatus::Missing;
    ComparisonAdvantage BestLapAdvantage =
        ComparisonAdvantage::Unavailable;
    ComparisonAdvantage AverageLapAdvantage =
        ComparisonAdvantage::Unavailable;
    ComparisonAdvantage ConsistencyAdvantage =
        ComparisonAdvantage::Unavailable;
    ComparisonAdvantage MaximumSpeedAdvantage =
        ComparisonAdvantage::Unavailable;
    ComparisonAdvantage CornerAdvantage =
        ComparisonAdvantage::Unavailable;
    ComparisonAdvantage BrakeAdvantage =
        ComparisonAdvantage::Unavailable;
    double BestLapDeltaSeconds = 0.0;
    double MeanLapDeltaSeconds = 0.0;
    double TheoreticalLapDeltaSeconds = 0.0;
    double LapConsistencyDeltaSeconds = 0.0;
    double MeanPeakSpeedDeltaKmh = 0.0;
    double MeanBrakingTimeDeltaSeconds = 0.0;
    double MeanOffTrackTimeDeltaSeconds = 0.0;
    double MeanCornerScoreDelta = 0.0;
    double MeanCornerApexSpeedDeltaCmS = 0.0;
    double MeanCornerExitSpeedDeltaCmS = 0.0;
    double MeanPeakSlipDeltaRadians = 0.0;
    double BrakeDecelerationDeltaG = 0.0;
    double GripSaturationFractionDelta = 0.0;
    double FuelEfficiencyDelta = 0.0;
    double Confidence = 0.0;
    double SharedSectorFraction = 0.0;
    int SharedSectors = 0;
    int DriverASlot = -1;
    int DriverBSlot = -1;
    std::array<double, MaximumTelemetrySectors> BestSectorDeltas{};
    std::array<double, MaximumTelemetrySectors> MeanSectorDeltas{};
    std::array<ComparisonAdvantage,
               MaximumTelemetrySectors> SectorAdvantages{};
    bool Qualified = false;
    bool SameVehicleClass = false;
    bool SameTrack = false;
};

inline ComparisonAdvantage CompareSmaller(
    double a, double b, double threshold) noexcept
{
    if (!Finite(a) || !Finite(b) ||
        !Finite(threshold) || threshold < 0.0)
    {
        return ComparisonAdvantage::Unavailable;
    }
    const double difference = a - b;
    if (difference < -threshold)
    {
        return ComparisonAdvantage::DriverA;
    }
    if (difference > threshold)
    {
        return ComparisonAdvantage::DriverB;
    }
    return ComparisonAdvantage::ApproximatelyEqual;
}

inline ComparisonAdvantage CompareLarger(
    double a, double b, double threshold) noexcept
{
    return CompareSmaller(b, a, threshold) ==
            ComparisonAdvantage::DriverA ?
            ComparisonAdvantage::DriverB :
        CompareSmaller(b, a, threshold) ==
            ComparisonAdvantage::DriverB ?
            ComparisonAdvantage::DriverA :
        CompareSmaller(b, a, threshold);
}

inline bool ValidDriverRecord(
    const DriverPerformanceRecord& r,
    const DriverComparisonSettings& s) noexcept
{
    return r.Qualified &&
           r.SessionToken != 0 && r.ActorToken != 0 &&
           r.DriverSlot >= 0 &&
           r.DriverSlot < MaximumTelemetryDrivers &&
           r.CompletedLaps >= s.MinimumSampledLaps &&
           r.ValidTimedLaps >= s.MinimumSampledLaps &&
           r.SectorCount >= s.MinimumSharedSectors &&
           r.SectorCount <= MaximumTelemetrySectors &&
           Finite(r.TrackLengthCm) && r.TrackLengthCm > 0.0 &&
           Finite(r.RaceClockSeconds) &&
           r.RaceClockSeconds >= 0.0 &&
           Finite(r.BestLapSeconds) &&
           r.BestLapSeconds >= s.MinimumLapSeconds &&
           r.BestLapSeconds <= s.MaximumLapSeconds &&
           Finite(r.MeanLapSeconds) &&
           r.MeanLapSeconds >= s.MinimumLapSeconds &&
           r.MeanLapSeconds <= s.MaximumLapSeconds &&
           Finite(r.LapTimeDeviationSeconds) &&
           r.LapTimeDeviationSeconds >= 0.0 &&
           Finite(r.TheoreticalBestLapSeconds) &&
           r.TheoreticalBestLapSeconds >= 0.0 &&
           Finite(r.MeanPeakSpeedKmh) &&
           r.MeanPeakSpeedKmh >= 0.0 &&
           Finite(r.MeanBrakingSecondsPerLap) &&
           r.MeanBrakingSecondsPerLap >= 0.0 &&
           Finite(r.MeanOffTrackSecondsPerLap) &&
           r.MeanOffTrackSecondsPerLap >= 0.0 &&
           Finite(r.MeanCornerScore) &&
           r.MeanCornerScore >= 0.0 &&
           r.MeanCornerScore <= 100.0 &&
           Finite(r.MeanCornerExitSpeedCmS) &&
           r.MeanCornerExitSpeedCmS >= 0.0 &&
           Finite(r.MeanApexSpeedCmS) &&
           r.MeanApexSpeedCmS >= 0.0 &&
           Finite(r.MeanPeakSlipRadians) &&
           Finite(r.MeanBrakeEventDecelerationG) &&
           Finite(r.GripLimitTimeFraction) &&
           r.GripLimitTimeFraction >= 0.0 &&
           r.GripLimitTimeFraction <= 1.0 &&
           Finite(r.FuelConsumedFraction) &&
           r.FuelConsumedFraction >= 0.0 &&
           r.FuelConsumedFraction <= 1.0 &&
           Finite(r.ObservedDistanceCm) &&
           r.ObservedDistanceCm >= 0.0 &&
           Finite(r.Confidence) &&
           r.Confidence >= 0.0 && r.Confidence <= 1.0 &&
           (!s.RequireNativeMeasuredLaps || r.EngineObserved) &&
           (s.IncludeCautionLapComparison ||
            !r.WasCautionAffected);
}

inline DriverComparisonResult CompareDrivers(
    const DriverPerformanceRecord& a,
    const DriverPerformanceRecord& b,
    DriverComparisonSettings settings = {}) noexcept
{
    DriverComparisonResult result;
    if (!ValidDriverComparisonSettings(settings) ||
        !ValidDriverRecord(a, settings) ||
        !ValidDriverRecord(b, settings))
    {
        result.Status =
            DriverComparisonStatus::InvalidMeasurements;
        return result;
    }
    result.DriverASlot = a.DriverSlot;
    result.DriverBSlot = b.DriverSlot;
    if (a.ActorToken == b.ActorToken ||
        a.DriverSlot == b.DriverSlot)
    {
        result.Status =
            DriverComparisonStatus::InvalidMeasurements;
        return result;
    }
    result.SameTrack = a.TrackFingerprint == b.TrackFingerprint &&
        std::abs(a.TrackLengthCm - b.TrackLengthCm) <=
            settings.MaximumTrackLengthDifferenceCm;
    if (settings.RequireSameTrackFingerprint &&
        !result.SameTrack)
    {
        result.Status = DriverComparisonStatus::DifferentTrack;
        return result;
    }
    result.SameVehicleClass =
        a.VehicleClass == b.VehicleClass;
    if (!result.SameVehicleClass &&
        !settings.AllowDifferentCarClasses)
    {
        result.Status = DriverComparisonStatus::DifferentRules;
        return result;
    }
    result.Confidence = std::min(a.Confidence, b.Confidence);
    if (result.Confidence <
        settings.MinimumCompletenessFraction)
    {
        result.Status =
            DriverComparisonStatus::UnqualifiedSource;
        return result;
    }
    result.BestLapDeltaSeconds =
        a.BestLapSeconds - b.BestLapSeconds;
    result.MeanLapDeltaSeconds =
        a.MeanLapSeconds - b.MeanLapSeconds;
    result.TheoreticalLapDeltaSeconds =
        a.TheoreticalBestLapSeconds -
        b.TheoreticalBestLapSeconds;
    result.LapConsistencyDeltaSeconds =
        a.LapTimeDeviationSeconds -
        b.LapTimeDeviationSeconds;
    result.MeanPeakSpeedDeltaKmh =
        a.MeanPeakSpeedKmh - b.MeanPeakSpeedKmh;
    result.MeanBrakingTimeDeltaSeconds =
        a.MeanBrakingSecondsPerLap -
        b.MeanBrakingSecondsPerLap;
    result.MeanOffTrackTimeDeltaSeconds =
        a.MeanOffTrackSecondsPerLap -
        b.MeanOffTrackSecondsPerLap;
    result.MeanCornerScoreDelta =
        a.MeanCornerScore - b.MeanCornerScore;
    result.MeanCornerApexSpeedDeltaCmS =
        a.MeanApexSpeedCmS - b.MeanApexSpeedCmS;
    result.MeanCornerExitSpeedDeltaCmS =
        a.MeanCornerExitSpeedCmS -
        b.MeanCornerExitSpeedCmS;
    result.MeanPeakSlipDeltaRadians =
        a.MeanPeakSlipRadians - b.MeanPeakSlipRadians;
    result.BrakeDecelerationDeltaG =
        a.MeanBrakeEventDecelerationG -
        b.MeanBrakeEventDecelerationG;
    result.GripSaturationFractionDelta =
        a.GripLimitTimeFraction - b.GripLimitTimeFraction;
    result.FuelEfficiencyDelta =
        a.FuelConsumedFraction - b.FuelConsumedFraction;
    result.BestLapAdvantage = CompareSmaller(
        a.BestLapSeconds, b.BestLapSeconds,
        settings.MinimumMeaningfulLapDeltaSeconds);
    result.AverageLapAdvantage = CompareSmaller(
        a.MeanLapSeconds, b.MeanLapSeconds,
        settings.MinimumMeaningfulLapDeltaSeconds);
    result.ConsistencyAdvantage = CompareSmaller(
        a.LapTimeDeviationSeconds,
        b.LapTimeDeviationSeconds,
        settings.MinimumMeaningfulLapDeltaSeconds);
    result.MaximumSpeedAdvantage = CompareLarger(
        a.MeanPeakSpeedKmh, b.MeanPeakSpeedKmh,
        settings.MinimumMeaningfulSpeedDeltaCmS *
            CmPerSecondToKmh);
    result.CornerAdvantage = CompareLarger(
        a.MeanCornerScore, b.MeanCornerScore,
        settings.MinimumMeaningfulCornerScoreDelta);
    result.BrakeAdvantage = CompareLarger(
        a.MeanBrakeEventDecelerationG,
        b.MeanBrakeEventDecelerationG, 0.03);
    const int common = std::min(
        a.SectorCount, b.SectorCount);
    for (int i = 0; i < common; ++i)
    {
        const std::size_t j = static_cast<std::size_t>(i);
        if (a.SectorSamples[j] == 0 ||
            b.SectorSamples[j] == 0 ||
            !Finite(a.BestSectorSeconds[j]) ||
            !Finite(b.BestSectorSeconds[j]) ||
            a.BestSectorSeconds[j] <= 0.0 ||
            b.BestSectorSeconds[j] <= 0.0)
        {
            continue;
        }
        result.BestSectorDeltas[j] =
            a.BestSectorSeconds[j] - b.BestSectorSeconds[j];
        result.MeanSectorDeltas[j] =
            a.MeanSectorSeconds[j] - b.MeanSectorSeconds[j];
        result.SectorAdvantages[j] = CompareSmaller(
            a.BestSectorSeconds[j],
            b.BestSectorSeconds[j],
            settings.MinimumMeaningfulLapDeltaSeconds);
        ++result.SharedSectors;
    }
    result.SharedSectorFraction = SafeRatio(
        result.SharedSectors, common);
    result.Qualified =
        result.SharedSectors >=
        settings.MinimumSharedSectors &&
        result.SharedSectorFraction >=
        settings.MinimumCompletenessFraction;
    result.Status = result.Qualified ?
        DriverComparisonStatus::Comparable :
        DriverComparisonStatus::Provisional;
    return result;
}

struct ComparisonHistory final
{
    std::uint64_t Compared = 0;
    std::uint64_t Qualified = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t DriverAWins = 0;
    std::uint64_t DriverBWins = 0;
    std::uint64_t Ties = 0;
    double MeanBestLapGapSeconds = 0.0;
    double MeanCornerAdvantage = 0.0;
    double LargestAbsoluteLapDifference = 0.0;
    double MeanConfidence = 0.0;
    double CombinedQualifiedDistanceCm = 0.0;
    int LastDriverASlot = -1;
    int LastDriverBSlot = -1;
    bool HasObservation = false;
};

class DriverComparisonJournal final
{
public:
    void Reset() noexcept
    {
        History = {};
        Recent.fill({});
        Next = 0;
        Count = 0;
    }

    bool Observe(
        DriverComparisonResult comparison,
        double jointlyObservedDistanceCm) noexcept
    {
        History.Compared =
            SaturatingIncrement(History.Compared);
        if (!comparison.Qualified ||
            !Finite(jointlyObservedDistanceCm) ||
            jointlyObservedDistanceCm < 0.0)
        {
            History.Rejected =
                SaturatingIncrement(History.Rejected);
            return false;
        }
        History.Qualified =
            SaturatingIncrement(History.Qualified);
        const double n =
            static_cast<double>(History.Qualified);
        History.MeanBestLapGapSeconds +=
            (comparison.BestLapDeltaSeconds -
             History.MeanBestLapGapSeconds) / n;
        History.MeanCornerAdvantage +=
            (comparison.MeanCornerScoreDelta -
             History.MeanCornerAdvantage) / n;
        History.MeanConfidence +=
            (comparison.Confidence -
             History.MeanConfidence) / n;
        History.LargestAbsoluteLapDifference = std::max(
            History.LargestAbsoluteLapDifference,
            std::abs(comparison.BestLapDeltaSeconds));
        History.CombinedQualifiedDistanceCm +=
            jointlyObservedDistanceCm;
        History.LastDriverASlot = comparison.DriverASlot;
        History.LastDriverBSlot = comparison.DriverBSlot;
        History.HasObservation = true;
        switch (comparison.BestLapAdvantage)
        {
        case ComparisonAdvantage::DriverA:
            History.DriverAWins =
                SaturatingIncrement(History.DriverAWins);
            break;
        case ComparisonAdvantage::DriverB:
            History.DriverBWins =
                SaturatingIncrement(History.DriverBWins);
            break;
        case ComparisonAdvantage::ApproximatelyEqual:
            History.Ties = SaturatingIncrement(History.Ties);
            break;
        case ComparisonAdvantage::Unavailable:
            break;
        }
        Recent[static_cast<std::size_t>(Next)] = comparison;
        Next = (Next + 1) %
            static_cast<int>(Recent.size());
        Count = std::min(
            Count + 1, static_cast<int>(Recent.size()));
        return true;
    }

    ComparisonHistory Snapshot() const noexcept
    {
        return History;
    }
    DriverComparisonResult RecentAt(int fromNewest) const noexcept
    {
        if (fromNewest < 0 || fromNewest >= Count)
        {
            return {};
        }
        const int offset = (
            Next + static_cast<int>(Recent.size()) -
            1 - fromNewest) %
            static_cast<int>(Recent.size());
        return Recent[static_cast<std::size_t>(offset)];
    }
    int StoredComparisons() const noexcept { return Count; }

private:
    ComparisonHistory History{};
    std::array<DriverComparisonResult, 32> Recent{};
    int Next = 0;
    int Count = 0;
};
} // namespace latebrake::engineering
