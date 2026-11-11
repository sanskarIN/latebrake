#pragma once

#include "Telemetry/TelemetryOpponents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class FleetGapQuality : std::uint8_t
{
    Missing,
    Unverified,
    IncompleteRoster,
    Inconsistent,
    Qualified
};

struct FleetProgressSample final
{
    std::uint64_t ActorToken = 0;
    std::uint64_t SessionToken = 0;
    double MonotonicSeconds = 0.0;
    double UnwrappedDistanceCm = 0.0;
    double TrackDistanceCm = 0.0;
    double SpeedCmS = 0.0;
    double LastLapSeconds = 0.0;
    double LastSectorSeconds = 0.0;
    int LapIndex = 0;
    int SectorIndex = 0;
    int GridSlot = -1;
    int ReportedRacePosition = 0;
    bool SourceEngineQualified = false;
    bool PhysicalRosterMember = false;
    bool VerifiedForwardProgress = false;
};

struct FleetGapSettings final
{
    double MaximumTimestampDifferenceSeconds = 0.10;
    double MinimumGapSpeedCmS = 125.0;
    double MaximumGapSpeedCmS = MaximumPlanarSpeedCmS;
    double MaximumPhysicalSeparationCm = 250000.0;
    double MaximumTimeGapSeconds = 300.0;
    double MaximumProgressMismatchCm = 1500.0;
    double MaximumLapTimeSeconds = 7200.0;
    double MaximumTrackLengthCm = 500000.0;
    double MinimumTrackLengthCm = 2000.0;
    bool RequireExactSixMemberRoster = true;
    bool IgnoreOfficialRacePosition = true;
    bool RequireForwardProgress = true;
};

inline bool ValidFleetGapSettings(
    FleetGapSettings s) noexcept
{
    return Finite(s.MaximumTimestampDifferenceSeconds) &&
        s.MaximumTimestampDifferenceSeconds >= 0.0 &&
        s.MaximumTimestampDifferenceSeconds <= 2.0 &&
        Finite(s.MinimumGapSpeedCmS) &&
        s.MinimumGapSpeedCmS > 0.0 &&
        Finite(s.MaximumGapSpeedCmS) &&
        s.MaximumGapSpeedCmS > s.MinimumGapSpeedCmS &&
        Finite(s.MaximumPhysicalSeparationCm) &&
        s.MaximumPhysicalSeparationCm > 0.0 &&
        Finite(s.MaximumTimeGapSeconds) &&
        s.MaximumTimeGapSeconds > 0.0 &&
        Finite(s.MaximumProgressMismatchCm) &&
        s.MaximumProgressMismatchCm >= 0.0 &&
        Finite(s.MaximumLapTimeSeconds) &&
        s.MaximumLapTimeSeconds > 0.0 &&
        Finite(s.MaximumTrackLengthCm) &&
        s.MaximumTrackLengthCm > s.MinimumTrackLengthCm &&
        Finite(s.MinimumTrackLengthCm) &&
        s.MinimumTrackLengthCm > 0.0;
}

struct FleetGapDriverReport final
{
    std::uint64_t ActorToken = 0;
    int Slot = -1;
    int AnalyticalRank = 0;
    int OfficialReportedRank = 0;
    int LapIndex = 0;
    int SectorIndex = 0;
    double UnwrappedDistanceCm = 0.0;
    double TrackDistanceCm = 0.0;
    double SpeedCmS = 0.0;
    double GapToLeaderCm = 0.0;
    double GapToLeaderSeconds = 0.0;
    double GapToNextAheadCm = 0.0;
    double GapToNextAheadSeconds = 0.0;
    double GapToNextBehindCm = 0.0;
    double GapToNextBehindSeconds = 0.0;
    double DifferenceFromReportedPosition = 0.0;
    double ClosingSpeedOnNextAheadCmS = 0.0;
    double PredictedSecondsToReachAhead = 0.0;
    bool HasAhead = false;
    bool HasBehind = false;
    bool AnalyticallyOrdered = false;
    bool OfficialRankChanged = false;
    bool Valid = false;
};

struct FleetGapSnapshot final
{
    std::array<FleetGapDriverReport, MaximumTelemetryDrivers> BySlot{};
    std::array<int, MaximumTelemetryDrivers> SlotByAnalyticalRank{};
    std::array<std::uint64_t, MaximumTelemetryDrivers> ActorByRank{};
    double LeaderDistanceCm = 0.0;
    double LastDistanceCm = 0.0;
    double TotalFleetSpanCm = 0.0;
    double MeanInterCarGapCm = 0.0;
    double MeanInterCarGapSeconds = 0.0;
    double MinimumInterCarGapCm = 0.0;
    double MaximumInterCarGapCm = 0.0;
    double MaximumObservedSpeedCmS = 0.0;
    double MinimumObservedSpeedCmS = 0.0;
    double TimeSeconds = 0.0;
    std::uint64_t SessionToken = 0;
    int QualifiedDrivers = 0;
    int PositionDisagreements = 0;
    int SameLapPairs = 0;
    int LappedPairs = 0;
    FleetGapQuality Quality = FleetGapQuality::Missing;
    bool OfficialClassificationChanged = false;
    bool Valid = false;
};

inline bool ValidFleetProgressSample(
    FleetProgressSample s,
    const FleetGapSettings& settings) noexcept
{
    return s.ActorToken != 0 &&
        s.SessionToken != 0 &&
        s.SourceEngineQualified &&
        s.PhysicalRosterMember &&
        (!settings.RequireForwardProgress ||
         s.VerifiedForwardProgress) &&
        Finite(s.MonotonicSeconds) &&
        Finite(s.UnwrappedDistanceCm) &&
        Finite(s.TrackDistanceCm) &&
        Finite(s.SpeedCmS) &&
        Finite(s.LastLapSeconds) &&
        Finite(s.LastSectorSeconds) &&
        s.MonotonicSeconds >= 0.0 &&
        s.TrackDistanceCm >= 0.0 &&
        s.TrackDistanceCm <=
            settings.MaximumTrackLengthCm &&
        s.SpeedCmS >= 0.0 &&
        s.SpeedCmS <=
            settings.MaximumGapSpeedCmS &&
        s.LastLapSeconds >= 0.0 &&
        s.LastLapSeconds <=
            settings.MaximumLapTimeSeconds &&
        s.LastSectorSeconds >= 0.0 &&
        s.LastSectorSeconds <=
            settings.MaximumLapTimeSeconds &&
        s.LapIndex >= 0 &&
        s.LapIndex <= MaximumTelemetryLaps &&
        s.SectorIndex >= 0 &&
        s.SectorIndex < MaximumTelemetrySectors &&
        s.GridSlot >= 0 &&
        s.GridSlot < MaximumTelemetryDrivers &&
        s.ReportedRacePosition >= 0 &&
        s.ReportedRacePosition <= MaximumTelemetryDrivers;
}

inline FleetGapSnapshot EvaluateFleetGaps(
    const FleetProgressSample* members,
    int count,
    double trackLengthCm,
    FleetGapSettings settings = {}) noexcept
{
    FleetGapSnapshot result;
    result.SlotByAnalyticalRank.fill(-1);
    if (!members ||
        !ValidFleetGapSettings(settings) ||
        !Finite(trackLengthCm) ||
        trackLengthCm < settings.MinimumTrackLengthCm ||
        trackLengthCm > settings.MaximumTrackLengthCm ||
        count <= 0 ||
        count > MaximumTelemetryDrivers ||
        (settings.RequireExactSixMemberRoster &&
         count != MaximumTelemetryDrivers))
    {
        result.Quality = FleetGapQuality::IncompleteRoster;
        return result;
    }
    const std::uint64_t session =
        members[0].SessionToken;
    const double timestamp =
        members[0].MonotonicSeconds;
    std::array<bool, MaximumTelemetryDrivers> slotUsed{};
    std::array<int, MaximumTelemetryDrivers> order{};
    for (int i = 0; i < count; ++i)
    {
        const FleetProgressSample& current =
            members[static_cast<std::size_t>(i)];
        if (!ValidFleetProgressSample(current, settings) ||
            current.SessionToken != session ||
            std::abs(current.MonotonicSeconds - timestamp) >
                settings.MaximumTimestampDifferenceSeconds ||
            slotUsed[static_cast<std::size_t>(current.GridSlot)])
        {
            result.Quality = FleetGapQuality::Inconsistent;
            return result;
        }
        slotUsed[static_cast<std::size_t>(current.GridSlot)] = true;
        for (int earlier = 0; earlier < i; ++earlier)
        {
            if (current.ActorToken ==
                members[static_cast<std::size_t>(earlier)].ActorToken)
            {
                result.Quality = FleetGapQuality::Inconsistent;
                return result;
            }
        }
        order[static_cast<std::size_t>(i)] = i;
    }
    for (int i = 1; i < count; ++i)
    {
        const int selected =
            order[static_cast<std::size_t>(i)];
        const FleetProgressSample& candidate =
            members[static_cast<std::size_t>(selected)];
        int j = i;
        while (j > 0)
        {
            const int prior =
                order[static_cast<std::size_t>(j - 1)];
            const FleetProgressSample& before =
                members[static_cast<std::size_t>(prior)];
            const bool better =
                candidate.UnwrappedDistanceCm >
                    before.UnwrappedDistanceCm ||
                (candidate.UnwrappedDistanceCm ==
                     before.UnwrappedDistanceCm &&
                 candidate.GridSlot < before.GridSlot);
            if (!better)
            {
                break;
            }
            order[static_cast<std::size_t>(j)] = prior;
            --j;
        }
        order[static_cast<std::size_t>(j)] = selected;
    }
    const FleetProgressSample& leader =
        members[static_cast<std::size_t>(order[0])];
    const FleetProgressSample& last =
        members[static_cast<std::size_t>(
            order[static_cast<std::size_t>(count - 1)])];
    result.LeaderDistanceCm =
        leader.UnwrappedDistanceCm;
    result.LastDistanceCm =
        last.UnwrappedDistanceCm;
    result.TotalFleetSpanCm =
        result.LeaderDistanceCm - result.LastDistanceCm;
    result.MinimumInterCarGapCm =
        std::numeric_limits<double>::infinity();
    result.MaximumInterCarGapCm = 0.0;
    result.MaximumObservedSpeedCmS = 0.0;
    result.MinimumObservedSpeedCmS =
        std::numeric_limits<double>::infinity();
    double totalGapSeconds = 0.0;
    double totalGapCm = 0.0;
    for (int rank = 0; rank < count; ++rank)
    {
        const FleetProgressSample& current =
            members[static_cast<std::size_t>(
                order[static_cast<std::size_t>(rank)])];
        FleetGapDriverReport report;
        report.ActorToken = current.ActorToken;
        report.Slot = current.GridSlot;
        report.AnalyticalRank = rank + 1;
        report.OfficialReportedRank =
            current.ReportedRacePosition;
        report.LapIndex = current.LapIndex;
        report.SectorIndex = current.SectorIndex;
        report.UnwrappedDistanceCm =
            current.UnwrappedDistanceCm;
        report.TrackDistanceCm =
            current.TrackDistanceCm;
        report.SpeedCmS = current.SpeedCmS;
        report.GapToLeaderCm = std::max(
            0.0, leader.UnwrappedDistanceCm -
                 current.UnwrappedDistanceCm);
        report.GapToLeaderSeconds = SafeRatio(
            report.GapToLeaderCm,
            std::max(settings.MinimumGapSpeedCmS,
                     leader.SpeedCmS));
        report.DifferenceFromReportedPosition =
            static_cast<double>(
                report.AnalyticalRank -
                report.OfficialReportedRank);
        report.OfficialRankChanged =
            current.ReportedRacePosition > 0 &&
            current.ReportedRacePosition !=
                report.AnalyticalRank;
        result.PositionDisagreements +=
            report.OfficialRankChanged ? 1 : 0;
        if (rank > 0)
        {
            const FleetProgressSample& ahead =
                members[static_cast<std::size_t>(
                    order[static_cast<std::size_t>(rank - 1)])];
            report.HasAhead = true;
            report.GapToNextAheadCm = std::max(
                0.0, ahead.UnwrappedDistanceCm -
                     current.UnwrappedDistanceCm);
            report.GapToNextAheadSeconds = SafeRatio(
                report.GapToNextAheadCm,
                std::max(settings.MinimumGapSpeedCmS,
                         ahead.SpeedCmS));
            report.ClosingSpeedOnNextAheadCmS =
                current.SpeedCmS - ahead.SpeedCmS;
            if (report.ClosingSpeedOnNextAheadCmS >
                settings.MinimumGapSpeedCmS)
            {
                report.PredictedSecondsToReachAhead =
                    SafeRatio(
                        report.GapToNextAheadCm,
                        report.ClosingSpeedOnNextAheadCmS);
            }
            totalGapCm += report.GapToNextAheadCm;
            totalGapSeconds +=
                report.GapToNextAheadSeconds;
            result.MinimumInterCarGapCm = std::min(
                result.MinimumInterCarGapCm,
                report.GapToNextAheadCm);
            result.MaximumInterCarGapCm = std::max(
                result.MaximumInterCarGapCm,
                report.GapToNextAheadCm);
            if (current.LapIndex == ahead.LapIndex)
            {
                ++result.SameLapPairs;
            }
            else
            {
                ++result.LappedPairs;
            }
        }
        if (rank + 1 < count)
        {
            const FleetProgressSample& behind =
                members[static_cast<std::size_t>(
                    order[static_cast<std::size_t>(rank + 1)])];
            report.HasBehind = true;
            report.GapToNextBehindCm = std::max(
                0.0, current.UnwrappedDistanceCm -
                     behind.UnwrappedDistanceCm);
            report.GapToNextBehindSeconds = SafeRatio(
                report.GapToNextBehindCm,
                std::max(settings.MinimumGapSpeedCmS,
                         current.SpeedCmS));
        }
        report.AnalyticallyOrdered = true;
        report.Valid = true;
        result.BySlot[static_cast<std::size_t>(
            current.GridSlot)] = report;
        result.SlotByAnalyticalRank[static_cast<std::size_t>(rank)] =
            current.GridSlot;
        result.ActorByRank[static_cast<std::size_t>(rank)] =
            current.ActorToken;
        ++result.QualifiedDrivers;
        result.MaximumObservedSpeedCmS = std::max(
            result.MaximumObservedSpeedCmS,
            current.SpeedCmS);
        result.MinimumObservedSpeedCmS = std::min(
            result.MinimumObservedSpeedCmS,
            current.SpeedCmS);
    }
    result.MeanInterCarGapCm = count > 1 ?
        totalGapCm / static_cast<double>(count - 1) : 0.0;
    result.MeanInterCarGapSeconds = count > 1 ?
        totalGapSeconds / static_cast<double>(count - 1) : 0.0;
    if (!Finite(result.MinimumInterCarGapCm))
    {
        result.MinimumInterCarGapCm = 0.0;
    }
    if (!Finite(result.MinimumObservedSpeedCmS))
    {
        result.MinimumObservedSpeedCmS = 0.0;
    }
    result.SessionToken = session;
    result.TimeSeconds = timestamp;
    result.Quality = FleetGapQuality::Qualified;
    result.Valid = true;
    result.OfficialClassificationChanged = false;
    return result;
}

struct FleetGapTrend final
{
    double MeanLeaderGapSeconds = 0.0;
    double BestLeaderGapSeconds = 0.0;
    double WorstLeaderGapSeconds = 0.0;
    double MeanNearbyGapSeconds = 0.0;
    double MeanFleetSpanCm = 0.0;
    double MaximumClosingSpeedCmS = 0.0;
    double GapSlopeSecondsPerSecond = 0.0;
    double MostRecentGapSeconds = 0.0;
    double TotalObservedSeconds = 0.0;
    int Samples = 0;
    int Rejected = 0;
    bool ConvergingWithLeader = false;
    bool DriftDetected = false;
    bool Valid = false;
};

class FleetGapHistory final
{
public:
    bool Configure(int observedSlot) noexcept
    {
        if (observedSlot < 0 ||
            observedSlot >= MaximumTelemetryDrivers)
        {
            return false;
        }
        Reset();
        Slot = observedSlot;
        return true;
    }

    void Reset() noexcept
    {
        Samples.fill({});
        Head = 0;
        Count = 0;
        Slot = -1;
        Session = 0;
        PreviousSeconds = 0.0;
        Last = {};
    }

    bool Observe(FleetGapSnapshot report) noexcept
    {
        if (!report.Valid || Slot < 0 ||
            !report.BySlot[static_cast<std::size_t>(Slot)].Valid)
        {
            ++Last.Rejected;
            return false;
        }
        if (Session != report.SessionToken)
        {
            const int chosenSlot = Slot;
            Reset();
            Slot = chosenSlot;
            Session = report.SessionToken;
        }
        if (Count > 0 &&
            report.TimeSeconds <= PreviousSeconds)
        {
            ++Last.Rejected;
            return false;
        }
        const double gap =
            report.BySlot[static_cast<std::size_t>(Slot)]
                .GapToLeaderSeconds;
        if (Count == static_cast<int>(Samples.size()))
        {
            Head = (Head + 1) %
                static_cast<int>(Samples.size());
        }
        else
        {
            ++Count;
        }
        Samples[static_cast<std::size_t>(
            (Head + Count - 1) %
            static_cast<int>(Samples.size()))] =
            {report.TimeSeconds, gap};
        PreviousSeconds = report.TimeSeconds;
        Recalculate(report);
        return true;
    }

    FleetGapTrend Snapshot() const noexcept
    {
        return Last;
    }

private:
    struct Point final
    {
        double Time = 0.0;
        double Gap = 0.0;
    };

    void Recalculate(FleetGapSnapshot report) noexcept
    {
        FleetGapTrend trend = Last;
        double sum = 0.0;
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = 0.0;
        double meanX = 0.0;
        double meanY = 0.0;
        double covariance = 0.0;
        double variance = 0.0;
        for (int i = 0; i < Count; ++i)
        {
            const Point p =
                Samples[static_cast<std::size_t>(
                    (Head + i) %
                    static_cast<int>(Samples.size()))];
            sum += p.Gap;
            minimum = std::min(minimum, p.Gap);
            maximum = std::max(maximum, p.Gap);
            const double n =
                static_cast<double>(i + 1);
            const double dx = p.Time - meanX;
            meanX += dx / n;
            meanY += (p.Gap - meanY) / n;
            covariance += dx * (p.Gap - meanY);
            variance += dx * (p.Time - meanX);
        }
        trend.Samples = Count;
        trend.MeanLeaderGapSeconds =
            sum / static_cast<double>(Count);
        trend.BestLeaderGapSeconds = minimum;
        trend.WorstLeaderGapSeconds = maximum;
        trend.MostRecentGapSeconds =
            Samples[static_cast<std::size_t>(
                (Head + Count - 1) %
                static_cast<int>(Samples.size()))].Gap;
        trend.GapSlopeSecondsPerSecond =
            SafeRatio(covariance, variance);
        trend.ConvergingWithLeader =
            trend.GapSlopeSecondsPerSecond < -0.01;
        trend.MeanNearbyGapSeconds =
            report.MeanInterCarGapSeconds;
        trend.MeanFleetSpanCm =
            report.TotalFleetSpanCm;
        trend.MaximumClosingSpeedCmS =
            report.MaximumObservedSpeedCmS -
            report.MinimumObservedSpeedCmS;
        trend.TotalObservedSeconds = Count > 1 ?
            report.TimeSeconds -
            Samples[static_cast<std::size_t>(Head)].Time : 0.0;
        trend.Valid = true;
        Last = trend;
    }

    std::array<Point, 64> Samples{};
    int Head = 0;
    int Count = 0;
    int Slot = -1;
    std::uint64_t Session = 0;
    double PreviousSeconds = 0.0;
    FleetGapTrend Last{};
};
} // namespace latebrake::engineering
