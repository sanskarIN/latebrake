#pragma once

#include "Telemetry/TelemetryLapJournal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SectorTrend : std::uint8_t
{
    Unknown,
    Improving,
    Stable,
    Worsening,
    Erratic
};

struct SectorPerformance final
{
    int Sector = -1;
    double BestSeconds = 0.0;
    double WorstSeconds = 0.0;
    double MeanSeconds = 0.0;
    double MedianSeconds = 0.0;
    double RecentMeanSeconds = 0.0;
    double StandardDeviationSeconds = 0.0;
    double LastSeconds = 0.0;
    double LastDeltaToBestSeconds = 0.0;
    double ImprovementSinceFirstSeconds = 0.0;
    double RecentSlopeSecondsPerLap = 0.0;
    double SectorContributionToLapLoss = 0.0;
    double CoefficientOfVariation = 0.0;
    double TrendConfidence = 0.0;
    int QualifiedSamples = 0;
    int RejectedSamples = 0;
    int ConsistentSamples = 0;
    SectorTrend Trend = SectorTrend::Unknown;
    bool Valid = false;
};

struct SectorMatrixSettings final
{
    int SectorCount = 4;
    double MinimumSplitSeconds = 0.10;
    double MaximumSplitSeconds = 900.0;
    double StableSlopeSecondsPerLap = 0.03;
    double ErraticVariationFraction = 0.17;
    double MinimumTrendConfidence = 0.45;
    int MinimumSamplesForTrend = 4;
    int RollingWindowSamples = 12;
};

inline bool ValidSectorMatrixSettings(
    const SectorMatrixSettings& s) noexcept
{
    return s.SectorCount >= 2 &&
           s.SectorCount <= MaximumTelemetrySectors &&
           s.MinimumSamplesForTrend >= 2 &&
           s.MinimumSamplesForTrend <= 32 &&
           s.RollingWindowSamples >= s.MinimumSamplesForTrend &&
           s.RollingWindowSamples <= 32 &&
           Finite(s.MinimumSplitSeconds) &&
           s.MinimumSplitSeconds > 0.0 &&
           Finite(s.MaximumSplitSeconds) &&
           s.MaximumSplitSeconds > s.MinimumSplitSeconds &&
           Finite(s.StableSlopeSecondsPerLap) &&
           s.StableSlopeSecondsPerLap >= 0.0 &&
           Finite(s.ErraticVariationFraction) &&
           s.ErraticVariationFraction >= 0.0 &&
           Finite(s.MinimumTrendConfidence) &&
           s.MinimumTrendConfidence >= 0.0 &&
           s.MinimumTrendConfidence <= 1.0;
}

struct SectorSample final
{
    double Seconds = 0.0;
    int LapIndex = -1;
    double TimeRecorded = 0.0;
    bool Qualified = false;
    bool Caution = false;
    bool OffTrack = false;
};

class TelemetrySectorMatrix final
{
public:
    void Reset() noexcept
    {
        Histories.fill({});
        Head.fill(0);
        Count.fill(0);
        TotalCount.fill(0);
        Rejected.fill(0);
        Sum.fill(0.0);
        SumSquares.fill(0.0);
        Best.fill(0.0);
        Worst.fill(0.0);
        Last.fill(0.0);
        QualifiedLaps = 0;
        RejectedLaps = 0;
    }

    bool Configure(SectorMatrixSettings settings) noexcept
    {
        if (!ValidSectorMatrixSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    bool ObserveSector(int sector, SectorSample sample) noexcept
    {
        if (sector < 0 || sector >= Settings.SectorCount ||
            !sample.Qualified ||
            !Finite(sample.Seconds) ||
            !Finite(sample.TimeRecorded) ||
            sample.Seconds < Settings.MinimumSplitSeconds ||
            sample.Seconds > Settings.MaximumSplitSeconds ||
            sample.TimeRecorded < 0.0 ||
            sample.LapIndex < 0 ||
            sample.LapIndex > MaximumTelemetryLaps)
        {
            if (sector >= 0 &&
                sector < Settings.SectorCount)
            {
                ++Rejected[static_cast<std::size_t>(sector)];
            }
            return false;
        }
        const std::size_t slot =
            static_cast<std::size_t>(sector);
        const int head = Head[slot];
        const int count = Count[slot];
        const int capacity = Settings.RollingWindowSamples;
        if (count >= capacity)
        {
            const SectorSample oldest =
                Histories[slot][static_cast<std::size_t>(head)];
            Sum[slot] -= oldest.Seconds;
            SumSquares[slot] -= oldest.Seconds * oldest.Seconds;
            Head[slot] = (head + 1) % capacity;
        }
        else
        {
            ++Count[slot];
        }
        const int destination =
            (Head[slot] + Count[slot] - 1) % capacity;
        Histories[slot][static_cast<std::size_t>(destination)] =
            sample;
        Sum[slot] += sample.Seconds;
        SumSquares[slot] += sample.Seconds * sample.Seconds;
        ++TotalCount[slot];
        Last[slot] = sample.Seconds;
        if (Best[slot] <= 0.0 ||
            sample.Seconds < Best[slot])
        {
            Best[slot] = sample.Seconds;
        }
        Worst[slot] = std::max(
            Worst[slot], sample.Seconds);
        return true;
    }

    bool ObserveLap(
        const LapJournalEntry& entry) noexcept
    {
        if (!entry.EligibleForPerformanceReview ||
            entry.Lap.SectorCount != Settings.SectorCount)
        {
            ++RejectedLaps;
            return false;
        }
        for (int i = 0; i < Settings.SectorCount; ++i)
        {
            const double seconds =
                entry.Lap.SectorTimes[static_cast<std::size_t>(i)];
            if (!Finite(seconds) ||
                seconds < Settings.MinimumSplitSeconds ||
                seconds > Settings.MaximumSplitSeconds)
            {
                ++RejectedLaps;
                return false;
            }
        }
        for (int i = 0; i < Settings.SectorCount; ++i)
        {
            ObserveSector(i, {
                entry.Lap.SectorTimes[static_cast<std::size_t>(i)],
                entry.Lap.LapIndex,
                entry.Lap.FinishedAtSeconds,
                true,
                false,
                entry.Lap.TotalOffTrackSeconds > 0.0
            });
        }
        ++QualifiedLaps;
        return true;
    }

    SectorSample AtOldest(
        int sector, int index) const noexcept
    {
        if (sector < 0 || sector >= Settings.SectorCount ||
            index < 0 ||
            index >= Count[static_cast<std::size_t>(sector)])
        {
            return {};
        }
        const std::size_t slot =
            static_cast<std::size_t>(sector);
        const int destination =
            (Head[slot] + index) %
            Settings.RollingWindowSamples;
        return Histories[slot][static_cast<std::size_t>(
            destination)];
    }

    SectorPerformance Evaluate(int sector) const noexcept
    {
        SectorPerformance out;
        out.Sector = sector;
        if (sector < 0 || sector >= Settings.SectorCount)
        {
            return out;
        }
        const std::size_t slot =
            static_cast<std::size_t>(sector);
        const int count = Count[slot];
        out.QualifiedSamples = TotalCount[slot];
        out.RejectedSamples = Rejected[slot];
        if (!count)
        {
            return out;
        }
        out.BestSeconds = Best[slot];
        out.WorstSeconds = Worst[slot];
        out.LastSeconds = Last[slot];
        out.LastDeltaToBestSeconds =
            out.LastSeconds - out.BestSeconds;
        out.MeanSeconds = Sum[slot] / static_cast<double>(count);
        out.RecentMeanSeconds = out.MeanSeconds;
        const double rawVariance = SumSquares[slot] /
            static_cast<double>(count) -
            out.MeanSeconds * out.MeanSeconds;
        out.StandardDeviationSeconds =
            std::sqrt(std::max(0.0, rawVariance));
        out.CoefficientOfVariation = SafeRatio(
            out.StandardDeviationSeconds,
            out.MeanSeconds);
        std::array<double, 32> sorted{};
        for (int i = 0; i < count; ++i)
        {
            const double value =
                AtOldest(sector, i).Seconds;
            int j = i;
            while (j > 0 &&
                   sorted[static_cast<std::size_t>(j - 1)] > value)
            {
                sorted[static_cast<std::size_t>(j)] =
                    sorted[static_cast<std::size_t>(j - 1)];
                --j;
            }
            sorted[static_cast<std::size_t>(j)] = value;
        }
        const int middle = count / 2;
        out.MedianSeconds = count % 2 ?
            sorted[static_cast<std::size_t>(middle)] :
            0.5 * (sorted[static_cast<std::size_t>(middle - 1)] +
                   sorted[static_cast<std::size_t>(middle)]);
        out.ImprovementSinceFirstSeconds =
            AtOldest(sector, 0).Seconds - out.LastSeconds;
        out.SectorContributionToLapLoss =
            std::max(0.0, out.LastDeltaToBestSeconds);
        if (count >= Settings.MinimumSamplesForTrend)
        {
            double sumTime = 0.0;
            double sumY = 0.0;
            double sumTimeY = 0.0;
            double sumTimeSq = 0.0;
            for (int i = 0; i < count; ++i)
            {
                const double x = static_cast<double>(i);
                const double y =
                    AtOldest(sector, i).Seconds;
                sumTime += x;
                sumY += y;
                sumTimeY += x * y;
                sumTimeSq += x * x;
            }
            const double n = static_cast<double>(count);
            const double denominator =
                n * sumTimeSq - sumTime * sumTime;
            const double slope = SafeRatio(
                n * sumTimeY - sumTime * sumY,
                denominator);
            out.RecentSlopeSecondsPerLap = slope;
            out.TrendConfidence = Clamp01(
                n / static_cast<double>(
                    Settings.MinimumSamplesForTrend * 2));
            if (out.CoefficientOfVariation >
                Settings.ErraticVariationFraction)
            {
                out.Trend = SectorTrend::Erratic;
            }
            else if (out.TrendConfidence <
                     Settings.MinimumTrendConfidence)
            {
                out.Trend = SectorTrend::Unknown;
            }
            else if (slope <
                     -Settings.StableSlopeSecondsPerLap)
            {
                out.Trend = SectorTrend::Improving;
            }
            else if (slope >
                     Settings.StableSlopeSecondsPerLap)
            {
                out.Trend = SectorTrend::Worsening;
            }
            else
            {
                out.Trend = SectorTrend::Stable;
            }
        }
        out.Valid = true;
        return out;
    }

    int SectorCount() const noexcept
    {
        return Settings.SectorCount;
    }

    double BestTheoreticalLapSeconds() const noexcept
    {
        double total = 0.0;
        for (int i = 0; i < Settings.SectorCount; ++i)
        {
            const double seconds =
                Best[static_cast<std::size_t>(i)];
            if (seconds <= 0.0)
            {
                return 0.0;
            }
            total += seconds;
        }
        return total;
    }

    int MostInconsistentSector() const noexcept
    {
        int index = -1;
        double worstVariation = -1.0;
        for (int i = 0; i < Settings.SectorCount; ++i)
        {
            const SectorPerformance p = Evaluate(i);
            if (p.Valid &&
                p.CoefficientOfVariation > worstVariation)
            {
                worstVariation = p.CoefficientOfVariation;
                index = i;
            }
        }
        return index;
    }

    int BiggestLastLapLossSector() const noexcept
    {
        int index = -1;
        double biggest = -1.0;
        for (int i = 0; i < Settings.SectorCount; ++i)
        {
            const SectorPerformance p = Evaluate(i);
            if (p.Valid &&
                p.SectorContributionToLapLoss > biggest)
            {
                biggest = p.SectorContributionToLapLoss;
                index = i;
            }
        }
        return index;
    }

    std::uint64_t AcceptedLaps() const noexcept
    {
        return QualifiedLaps;
    }

    std::uint64_t FailedLaps() const noexcept
    {
        return RejectedLaps;
    }

private:
    SectorMatrixSettings Settings{};
    std::array<std::array<SectorSample, 32>,
               MaximumTelemetrySectors> Histories{};
    std::array<int, MaximumTelemetrySectors> Head{};
    std::array<int, MaximumTelemetrySectors> Count{};
    std::array<int, MaximumTelemetrySectors> TotalCount{};
    std::array<int, MaximumTelemetrySectors> Rejected{};
    std::array<double, MaximumTelemetrySectors> Sum{};
    std::array<double, MaximumTelemetrySectors> SumSquares{};
    std::array<double, MaximumTelemetrySectors> Best{};
    std::array<double, MaximumTelemetrySectors> Worst{};
    std::array<double, MaximumTelemetrySectors> Last{};
    std::uint64_t QualifiedLaps = 0;
    std::uint64_t RejectedLaps = 0;
};
} // namespace latebrake::engineering
