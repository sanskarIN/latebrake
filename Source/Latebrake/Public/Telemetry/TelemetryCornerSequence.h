#pragma once

#include "Telemetry/TelemetryCornering.h"
#include "Telemetry/TelemetryRaceLineProfile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
struct CornerSequenceSettings final
{
    double MaximumGapSeconds = 6.0;
    double MaximumDistanceCm = 6000.0;
    double MinimumApproachSpeedCmS = 250.0;
    double MinimumExitSpeedCmS = 200.0;
    double RelativeSpeedWeight = 0.36;
    double GripStabilityWeight = 0.24;
    double LineAccuracyWeight = 0.23;
    double BrakePlacementWeight = 0.17;
    double MinimumFullSequenceConfidence = 0.65;
    int MinimumQualifiedCorners = 2;
    bool RequireOnRoad = true;
    bool RequireSameLap = true;
};

inline bool ValidCornerSequenceSettings(
    CornerSequenceSettings s) noexcept
{
    return Finite(s.MaximumGapSeconds) &&
           s.MaximumGapSeconds > 0.0 &&
           s.MaximumGapSeconds <= 60.0 &&
           Finite(s.MaximumDistanceCm) &&
           s.MaximumDistanceCm > 0.0 &&
           Finite(s.MinimumApproachSpeedCmS) &&
           s.MinimumApproachSpeedCmS >= 0.0 &&
           Finite(s.MinimumExitSpeedCmS) &&
           s.MinimumExitSpeedCmS >= 0.0 &&
           Finite(s.RelativeSpeedWeight) &&
           Finite(s.GripStabilityWeight) &&
           Finite(s.LineAccuracyWeight) &&
           Finite(s.BrakePlacementWeight) &&
           s.RelativeSpeedWeight >= 0.0 &&
           s.GripStabilityWeight >= 0.0 &&
           s.LineAccuracyWeight >= 0.0 &&
           s.BrakePlacementWeight >= 0.0 &&
           std::abs(s.RelativeSpeedWeight +
                    s.GripStabilityWeight +
                    s.LineAccuracyWeight +
                    s.BrakePlacementWeight - 1.0) < 0.0001 &&
           Finite(s.MinimumFullSequenceConfidence) &&
           s.MinimumFullSequenceConfidence >= 0.0 &&
           s.MinimumFullSequenceConfidence <= 1.0 &&
           s.MinimumQualifiedCorners >= 2 &&
           s.MinimumQualifiedCorners <= 32;
}

struct CornerSequenceEntry final
{
    CornerEventRecord Corner{};
    BrakeEventRecord ClosestBrake{};
    double ApproachSpeedCmS = 0.0;
    double BrakingDistanceCm = 0.0;
    double InitialLineErrorCm = 0.0;
    double ExitLineErrorCm = 0.0;
    double ExitTimePenaltySeconds = 0.0;
    double TimeInGripLimitSeconds = 0.0;
    double BrakeToApexDistanceCm = 0.0;
    double ApexToExitDistanceCm = 0.0;
    double LineConsistencyFraction = 0.0;
    double BrakingPlacementFraction = 0.0;
    double GripStabilityFraction = 0.0;
    double EntryExitRetentionFraction = 0.0;
    double Confidence = 0.0;
    double ReviewScore = 0.0;
    int LapIndex = -1;
    int Sector = -1;
    bool BrakeLinked = false;
    bool OnRoad = false;
    bool Qualified = false;
};

struct CornerSequenceComparison final
{
    double FirstCornerScore = 0.0;
    double LastCornerScore = 0.0;
    double MeanCornerScore = 0.0;
    double LowestCornerScore = 0.0;
    double HighestCornerScore = 0.0;
    double CumulativeCornerSeconds = 0.0;
    double CumulativeBrakeSeconds = 0.0;
    double CumulativeOffRoadSeconds = 0.0;
    double MeanExitRetentionFraction = 0.0;
    double MeanGripStabilityFraction = 0.0;
    double MeanBrakingPlacementFraction = 0.0;
    double MeanLineConsistencyFraction = 0.0;
    double MeanConfidence = 0.0;
    double ScoreTrendPerCorner = 0.0;
    double WorstCornerTimeSeconds = 0.0;
    int LowestScoringCorner = -1;
    int HighestScoringCorner = -1;
    int QualifiedCorners = 0;
    int RejectedCorners = 0;
    bool Complete = false;
    bool AdvisoryOnly = true;
};

inline CornerSequenceEntry ReviewCornerEntry(
    CornerEventRecord corner,
    BrakeEventRecord brake,
    int lap, int sector,
    bool onRoad,
    const CornerSequenceSettings& settings) noexcept
{
    CornerSequenceEntry result;
    result.Corner = corner;
    result.ClosestBrake = brake;
    result.LapIndex = lap;
    result.Sector = sector;
    result.OnRoad = onRoad;
    if (!corner.Valid ||
        !Finite(corner.EntrySpeedCmS) ||
        !Finite(corner.ExitSpeedCmS) ||
        corner.EntrySpeedCmS <
            settings.MinimumApproachSpeedCmS ||
        corner.ExitSpeedCmS <
            settings.MinimumExitSpeedCmS ||
        (settings.RequireOnRoad && !onRoad))
    {
        return result;
    }
    result.ApproachSpeedCmS =
        corner.EntrySpeedCmS;
    result.EntryExitRetentionFraction =
        Clamp01(SafeRatio(corner.ExitSpeedCmS,
            std::max(1.0, corner.EntrySpeedCmS)));
    result.LineConsistencyFraction = 1.0 - Clamp01(
        SafeRatio(corner.IntegratedLateralErrorCmSeconds,
                  std::max(Tiny, corner.DurationSeconds * 1000.0)));
    result.GripStabilityFraction = 1.0 - Clamp01(
        SafeRatio(corner.OversteerProxySeconds +
                  corner.UndersteerProxySeconds +
                  corner.LargeSlipSeconds,
                  std::max(Tiny, corner.DurationSeconds * 1.5)));
    result.ExitLineErrorCm =
        corner.PeakLateralErrorCm;
    result.TimeInGripLimitSeconds =
        corner.OversteerProxySeconds +
        corner.UndersteerProxySeconds;
    result.ApexToExitDistanceCm =
        std::max(0.0, corner.ExitAtTrackCm -
                 corner.ApexAtTrackCm);
    result.BrakeLinked = brake.Valid &&
        brake.EndedSeconds <=
            corner.ApexTimeSeconds &&
        corner.EntryTimeSeconds -
            brake.EndedSeconds <=
            settings.MaximumGapSeconds &&
        std::abs(corner.EntryAtTrackCm -
                 brake.EndedAtTrackCm) <=
            settings.MaximumDistanceCm;
    if (result.BrakeLinked)
    {
        result.BrakingDistanceCm = brake.DistanceCm;
        result.BrakeToApexDistanceCm = std::max(
            0.0, corner.ApexAtTrackCm -
                 brake.StartedAtTrackCm);
        result.BrakingPlacementFraction = Clamp01(
            SafeRatio(result.BrakeToApexDistanceCm -
                      result.BrakingDistanceCm,
                      std::max(1.0, result.BrakeToApexDistanceCm)));
    }
    else
    {
        result.BrakingPlacementFraction = 0.5;
    }
    result.Confidence = Clamp01(
        (corner.DirectionReliable ? 0.25 : 0.0) +
        (corner.ApexObserved ? 0.25 : 0.0) +
        (result.BrakeLinked ? 0.20 : 0.0) +
        (onRoad ? 0.15 : 0.0) +
        (corner.DurationSeconds >= 0.35 ? 0.15 : 0.0));
    result.ReviewScore = 100.0 * (
        settings.RelativeSpeedWeight *
            result.EntryExitRetentionFraction +
        settings.GripStabilityWeight *
            result.GripStabilityFraction +
        settings.LineAccuracyWeight *
            result.LineConsistencyFraction +
        settings.BrakePlacementWeight *
            result.BrakingPlacementFraction);
    result.Qualified = result.Confidence >=
        settings.MinimumFullSequenceConfidence;
    return result;
}

class CornerSequenceJournal final
{
public:
    static constexpr int MaximumEvents = 128;

    void Reset() noexcept
    {
        Settings = {};
        Samples.fill({});
        Count = 0;
        Head = 0;
        Rejected = 0;
        SumScores = 0.0;
        SumSquares = 0.0;
        SumDurations = 0.0;
        SumBrake = 0.0;
        SumGrip = 0.0;
        SumLine = 0.0;
        SumExit = 0.0;
        SumConfidence = 0.0;
        LowestScore = 100.0;
        HighestScore = 0.0;
        HasLast = false;
        Session = 0;
        Actor = 0;
        Lap = -1;
    }

    bool Configure(CornerSequenceSettings settings) noexcept
    {
        if (!ValidCornerSequenceSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool BeginSession(
        std::uint64_t session,
        std::uint64_t actor, int lap) noexcept
    {
        if (session == 0 || actor == 0 ||
            lap < 0 || lap > MaximumTelemetryLaps)
        {
            return false;
        }
        Reset();
        Session = session;
        Actor = actor;
        Lap = lap;
        return true;
    }

    bool Observe(
        CornerEventRecord corner,
        BrakeEventRecord closestBrake,
        std::uint64_t session,
        std::uint64_t actor,
        int lap, int sector, bool onRoad) noexcept
    {
        if (session == 0 || actor == 0 ||
            session != Session ||
            actor != Actor ||
            (Settings.RequireSameLap && lap != Lap))
        {
            ++Rejected;
            return false;
        }
        CornerSequenceEntry result = ReviewCornerEntry(
            corner, closestBrake, lap, sector, onRoad, Settings);
        if (!result.Qualified)
        {
            ++Rejected;
            return false;
        }
        if (HasLast && result.Corner.EntryTimeSeconds <=
            Latest().Corner.EntryTimeSeconds)
        {
            ++Rejected;
            return false;
        }
        if (Count == MaximumEvents)
        {
            Head = (Head + 1) % MaximumEvents;
        }
        else
        {
            ++Count;
        }
        Samples[static_cast<std::size_t>(
            (Head + Count - 1) % MaximumEvents)] =
            result;
        SumScores += result.ReviewScore;
        SumSquares +=
            result.ReviewScore * result.ReviewScore;
        SumDurations += result.Corner.DurationSeconds;
        SumBrake += result.ClosestBrake.Valid ?
            result.ClosestBrake.DurationSeconds : 0.0;
        SumGrip += result.GripStabilityFraction;
        SumLine += result.LineConsistencyFraction;
        SumExit += result.EntryExitRetentionFraction;
        SumConfidence += result.Confidence;
        LowestScore = std::min(
            LowestScore, result.ReviewScore);
        HighestScore = std::max(
            HighestScore, result.ReviewScore);
        HasLast = true;
        return true;
    }

    CornerSequenceEntry AtOldest(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Samples[static_cast<std::size_t>(
                (Head + index) % MaximumEvents)] :
            CornerSequenceEntry{};
    }

    CornerSequenceEntry Latest() const noexcept
    {
        return Count > 0 ?
            AtOldest(Count - 1) : CornerSequenceEntry{};
    }

    CornerSequenceComparison Compare() const noexcept
    {
        CornerSequenceComparison summary;
        summary.QualifiedCorners = Count;
        summary.RejectedCorners = Rejected;
        if (Count == 0)
        {
            return summary;
        }
        const double count = static_cast<double>(Count);
        summary.FirstCornerScore =
            AtOldest(0).ReviewScore;
        summary.LastCornerScore =
            Latest().ReviewScore;
        summary.MeanCornerScore = SumScores / count;
        summary.LowestCornerScore = LowestScore;
        summary.HighestCornerScore = HighestScore;
        summary.CumulativeCornerSeconds =
            SumDurations;
        summary.CumulativeBrakeSeconds = SumBrake;
        summary.MeanExitRetentionFraction =
            SumExit / count;
        summary.MeanGripStabilityFraction =
            SumGrip / count;
        summary.MeanLineConsistencyFraction =
            SumLine / count;
        summary.MeanConfidence =
            SumConfidence / count;
        double xMean = 0.0;
        double yMean = 0.0;
        double covariance = 0.0;
        double variance = 0.0;
        for (int i = 0; i < Count; ++i)
        {
            const double x = static_cast<double>(i);
            const double y =
                AtOldest(i).ReviewScore;
            const double deltaX = x - xMean;
            const double deltaY = y - yMean;
            const double n = static_cast<double>(i + 1);
            xMean += deltaX / n;
            yMean += deltaY / n;
            covariance += deltaX * (y - yMean);
            variance += deltaX * (x - xMean);
            if (summary.LowestScoringCorner < 0 ||
                y < AtOldest(summary.LowestScoringCorner)
                    .ReviewScore)
            {
                summary.LowestScoringCorner = i;
            }
            if (summary.HighestScoringCorner < 0 ||
                y > AtOldest(summary.HighestScoringCorner)
                    .ReviewScore)
            {
                summary.HighestScoringCorner = i;
            }
        }
        summary.ScoreTrendPerCorner =
            SafeRatio(covariance, variance);
        summary.Complete =
            Count >= Settings.MinimumQualifiedCorners;
        summary.AdvisoryOnly = true;
        return summary;
    }

    int StoredCorners() const noexcept { return Count; }

private:
    CornerSequenceSettings Settings{};
    std::array<CornerSequenceEntry, MaximumEvents> Samples{};
    int Count = 0;
    int Head = 0;
    int Rejected = 0;
    double SumScores = 0.0;
    double SumSquares = 0.0;
    double SumDurations = 0.0;
    double SumBrake = 0.0;
    double SumGrip = 0.0;
    double SumLine = 0.0;
    double SumExit = 0.0;
    double SumConfidence = 0.0;
    double LowestScore = 100.0;
    double HighestScore = 0.0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int Lap = -1;
    bool HasLast = false;
};
} // namespace latebrake::engineering
