#pragma once

#include "Telemetry/TelemetrySectorMatrix.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class LapDeltaQuality : std::uint8_t
{
    Missing,
    Incompatible,
    MixedConditions,
    Qualified
};

struct LapComparisonSettings final
{
    double MaximumConditionTemperatureDifferenceC = 15.0;
    double MaximumWetnessDifference = 0.15;
    double MinimumComparableLapSeconds = 4.0;
    double MaximumComparableLapSeconds = 7200.0;
    double MinimumMeaningfulDeltaSeconds = 0.015;
    bool RequireSameSession = true;
    bool RequireSameDriver = false;
    bool RequireSameVehicleClass = true;
    bool RequireSameSectorCount = true;
    bool AllowOffTrackComparison = false;
};

struct LapComparisonInput final
{
    LapJournalEntry Lap{};
    TelemetryVehicleClass Vehicle =
        TelemetryVehicleClass::Unknown;
    int DriverSlot = -1;
    double AirTemperatureC = 20.0;
    double TrackTemperatureC = 25.0;
    double Wetness = 0.0;
    double FuelAtStart = 1.0;
    double AverageTireWear = 0.0;
    bool EngineQualified = false;
};

struct LapSectorDelta final
{
    int Sector = -1;
    double ReferenceSeconds = 0.0;
    double CandidateSeconds = 0.0;
    double DeltaSeconds = 0.0;
    double CumulativeDeltaSeconds = 0.0;
    double RelativePercent = 0.0;
    double ContributionFraction = 0.0;
    bool CandidateFaster = false;
    bool SignificantDifference = false;
    bool Valid = false;
};

struct LapComparisonReport final
{
    std::array<LapSectorDelta, MaximumTelemetrySectors> Sectors{};
    double ReferenceLapSeconds = 0.0;
    double CandidateLapSeconds = 0.0;
    double OverallDeltaSeconds = 0.0;
    double OverallDeltaPercent = 0.0;
    double ReferenceTheoreticalSeconds = 0.0;
    double CandidateTheoreticalSeconds = 0.0;
    double TotalSectorDeltaSeconds = 0.0;
    double UnexplainedDeltaSeconds = 0.0;
    double PositiveSectorLossSeconds = 0.0;
    double RecoveredSectorTimeSeconds = 0.0;
    double MaxSectorLossSeconds = 0.0;
    double MaxSectorGainSeconds = 0.0;
    double SectorDeltaStandardDeviation = 0.0;
    double FuelConditionDifference = 0.0;
    double TireConditionDifference = 0.0;
    double WetnessDifference = 0.0;
    double Confidence = 0.0;
    int SectorCount = 0;
    int CandidateFasterSectors = 0;
    int CandidateSlowerSectors = 0;
    int MostLostSector = -1;
    int MostGainedSector = -1;
    LapDeltaQuality Quality = LapDeltaQuality::Missing;
    bool Valid = false;
    bool AuthoritativeTiming = false;
};

inline bool ValidLapComparisonSettings(
    LapComparisonSettings settings) noexcept
{
    return Finite(settings.MaximumConditionTemperatureDifferenceC) &&
           settings.MaximumConditionTemperatureDifferenceC >= 0.0 &&
           Finite(settings.MaximumWetnessDifference) &&
           settings.MaximumWetnessDifference >= 0.0 &&
           settings.MaximumWetnessDifference <= 1.0 &&
           Finite(settings.MinimumComparableLapSeconds) &&
           settings.MinimumComparableLapSeconds > 0.0 &&
           Finite(settings.MaximumComparableLapSeconds) &&
           settings.MaximumComparableLapSeconds >
               settings.MinimumComparableLapSeconds &&
           Finite(settings.MinimumMeaningfulDeltaSeconds) &&
           settings.MinimumMeaningfulDeltaSeconds >= 0.0;
}

inline bool ValidLapComparisonInput(
    const LapComparisonInput& input,
    const LapComparisonSettings& settings) noexcept
{
    const TelemetryLap& lap = input.Lap.Lap;
    if (!input.Lap.EligibleForPerformanceReview ||
        !input.EngineQualified ||
        !lap.Completed ||
        !Finite(lap.DurationSeconds) ||
        lap.DurationSeconds < settings.MinimumComparableLapSeconds ||
        lap.DurationSeconds > settings.MaximumComparableLapSeconds ||
        lap.SectorCount < 2 ||
        lap.SectorCount > MaximumTelemetrySectors ||
        !Finite(input.AirTemperatureC) ||
        !Finite(input.TrackTemperatureC) ||
        !Finite(input.Wetness) ||
        !Finite(input.FuelAtStart) ||
        !Finite(input.AverageTireWear) ||
        input.Wetness < 0.0 || input.Wetness > 1.0 ||
        input.FuelAtStart < 0.0 || input.FuelAtStart > 1.0 ||
        input.AverageTireWear < 0.0 ||
        input.AverageTireWear > 1.0)
    {
        return false;
    }
    if (!settings.AllowOffTrackComparison &&
        lap.TotalOffTrackSeconds > 0.0)
    {
        return false;
    }
    double total = 0.0;
    for (int i = 0; i < lap.SectorCount; ++i)
    {
        const double seconds =
            lap.SectorTimes[static_cast<std::size_t>(i)];
        if (!Finite(seconds) || seconds <= 0.0)
        {
            return false;
        }
        total += seconds;
    }
    return std::abs(total - lap.DurationSeconds) <= 0.5;
}

inline LapComparisonReport CompareRecordedLaps(
    const LapComparisonInput& reference,
    const LapComparisonInput& candidate,
    LapComparisonSettings settings = {}) noexcept
{
    LapComparisonReport result;
    if (!ValidLapComparisonSettings(settings) ||
        !ValidLapComparisonInput(reference, settings) ||
        !ValidLapComparisonInput(candidate, settings))
    {
        return result;
    }
    if ((settings.RequireSameSession &&
         reference.Lap.SessionToken != candidate.Lap.SessionToken) ||
        (settings.RequireSameDriver &&
         reference.DriverSlot != candidate.DriverSlot) ||
        (settings.RequireSameVehicleClass &&
         reference.Vehicle != candidate.Vehicle) ||
        (settings.RequireSameSectorCount &&
         reference.Lap.Lap.SectorCount !=
             candidate.Lap.Lap.SectorCount))
    {
        result.Quality = LapDeltaQuality::Incompatible;
        return result;
    }
    const int sectors = std::min(
        reference.Lap.Lap.SectorCount,
        candidate.Lap.Lap.SectorCount);
    result.ReferenceLapSeconds =
        reference.Lap.Lap.DurationSeconds;
    result.CandidateLapSeconds =
        candidate.Lap.Lap.DurationSeconds;
    result.OverallDeltaSeconds =
        result.CandidateLapSeconds -
        result.ReferenceLapSeconds;
    result.OverallDeltaPercent = SafeRatio(
        result.OverallDeltaSeconds * 100.0,
        result.ReferenceLapSeconds);
    result.FuelConditionDifference =
        candidate.FuelAtStart -
        reference.FuelAtStart;
    result.TireConditionDifference =
        candidate.AverageTireWear -
        reference.AverageTireWear;
    result.WetnessDifference =
        candidate.Wetness - reference.Wetness;
    result.SectorCount = sectors;
    double cumulative = 0.0;
    double sumDeltaSq = 0.0;
    for (int i = 0; i < sectors; ++i)
    {
        const std::size_t index =
            static_cast<std::size_t>(i);
        LapSectorDelta sector;
        sector.Sector = i;
        sector.ReferenceSeconds =
            reference.Lap.Lap.SectorTimes[index];
        sector.CandidateSeconds =
            candidate.Lap.Lap.SectorTimes[index];
        sector.DeltaSeconds =
            sector.CandidateSeconds -
            sector.ReferenceSeconds;
        cumulative += sector.DeltaSeconds;
        sector.CumulativeDeltaSeconds = cumulative;
        sector.RelativePercent = 100.0 * SafeRatio(
            sector.DeltaSeconds, sector.ReferenceSeconds);
        sector.CandidateFaster =
            sector.DeltaSeconds < 0.0;
        sector.SignificantDifference =
            std::abs(sector.DeltaSeconds) >=
            settings.MinimumMeaningfulDeltaSeconds;
        sector.Valid = true;
        result.Sectors[index] = sector;
        sumDeltaSq += sector.DeltaSeconds * sector.DeltaSeconds;
        if (sector.SignificantDifference)
        {
            if (sector.CandidateFaster)
            {
                ++result.CandidateFasterSectors;
            }
            else
            {
                ++result.CandidateSlowerSectors;
            }
        }
        if (sector.DeltaSeconds > 0.0)
        {
            result.PositiveSectorLossSeconds +=
                sector.DeltaSeconds;
            if (result.MostLostSector < 0 ||
                sector.DeltaSeconds >
                    result.MaxSectorLossSeconds)
            {
                result.MostLostSector = i;
                result.MaxSectorLossSeconds =
                    sector.DeltaSeconds;
            }
        }
        else if (sector.DeltaSeconds < 0.0)
        {
            result.RecoveredSectorTimeSeconds -=
                sector.DeltaSeconds;
            if (result.MostGainedSector < 0 ||
                -sector.DeltaSeconds >
                    result.MaxSectorGainSeconds)
            {
                result.MostGainedSector = i;
                result.MaxSectorGainSeconds =
                    -sector.DeltaSeconds;
            }
        }
    }
    result.TotalSectorDeltaSeconds = cumulative;
    result.UnexplainedDeltaSeconds =
        result.OverallDeltaSeconds - cumulative;
    const double mean = cumulative /
        static_cast<double>(sectors);
    result.SectorDeltaStandardDeviation =
        std::sqrt(std::max(0.0,
            sumDeltaSq / static_cast<double>(sectors) -
            mean * mean));
    for (int i = 0; i < sectors; ++i)
    {
        const std::size_t index =
            static_cast<std::size_t>(i);
        LapSectorDelta& delta = result.Sectors[index];
        delta.ContributionFraction = SafeRatio(
            std::abs(delta.DeltaSeconds),
            result.PositiveSectorLossSeconds +
            result.RecoveredSectorTimeSeconds);
    }
    const double temperatureDelta = std::abs(
        reference.TrackTemperatureC -
        candidate.TrackTemperatureC);
    const bool comparableEnvironment =
        temperatureDelta <=
            settings.MaximumConditionTemperatureDifferenceC &&
        std::abs(result.WetnessDifference) <=
            settings.MaximumWetnessDifference;
    result.Quality = comparableEnvironment ?
        LapDeltaQuality::Qualified :
        LapDeltaQuality::MixedConditions;
    result.Confidence = comparableEnvironment ?
        1.0 : std::clamp(
            1.0 - 0.03 * temperatureDelta -
            1.5 * std::abs(result.WetnessDifference),
            0.0, 0.8);
    result.Valid = std::abs(
        result.UnexplainedDeltaSeconds) <= 0.5;
    result.AuthoritativeTiming = false;
    return result;
}

struct LapComparisonHistory final
{
    std::array<LapComparisonReport, 32> Recent{};
    int Count = 0;
    int Head = 0;
    int QualifiedCount = 0;
    int MixedConditionsCount = 0;
    double CumulativeGainSeconds = 0.0;
    double CumulativeLossSeconds = 0.0;
    double BestGainSeconds = 0.0;
    double WorstLossSeconds = 0.0;
    double MeanDeltaSeconds = 0.0;
    double SumSquareDeltaSeconds = 0.0;

    void Reset() noexcept
    {
        *this = {};
    }

    bool Push(LapComparisonReport report) noexcept
    {
        if (!report.Valid ||
            !Finite(report.OverallDeltaSeconds))
        {
            return false;
        }
        const int destination =
            (Head + Count) % static_cast<int>(Recent.size());
        if (Count == static_cast<int>(Recent.size()))
        {
            Recent[static_cast<std::size_t>(Head)] =
                report;
            Head = (Head + 1) %
                static_cast<int>(Recent.size());
        }
        else
        {
            Recent[static_cast<std::size_t>(destination)] =
                report;
            ++Count;
        }
        QualifiedCount += report.Quality ==
            LapDeltaQuality::Qualified ? 1 : 0;
        MixedConditionsCount += report.Quality ==
            LapDeltaQuality::MixedConditions ? 1 : 0;
        CumulativeGainSeconds +=
            std::max(0.0, -report.OverallDeltaSeconds);
        CumulativeLossSeconds +=
            std::max(0.0, report.OverallDeltaSeconds);
        BestGainSeconds = std::max(
            BestGainSeconds, -report.OverallDeltaSeconds);
        WorstLossSeconds = std::max(
            WorstLossSeconds, report.OverallDeltaSeconds);
        double sum = 0.0;
        double squares = 0.0;
        for (int i = 0; i < Count; ++i)
        {
            const double v = AtOldest(i)
                .OverallDeltaSeconds;
            sum += v;
            squares += v * v;
        }
        MeanDeltaSeconds = sum /
            static_cast<double>(Count);
        SumSquareDeltaSeconds = squares;
        return true;
    }

    LapComparisonReport AtOldest(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Recent[static_cast<std::size_t>(
                (Head + index) %
                static_cast<int>(Recent.size()))] :
            LapComparisonReport{};
    }

    LapComparisonReport Latest() const noexcept
    {
        return Count > 0 ? AtOldest(Count - 1) :
            LapComparisonReport{};
    }

    double RecentDeltaDeviation() const noexcept
    {
        return Count > 0 ? std::sqrt(std::max(0.0,
            SumSquareDeltaSeconds /
                static_cast<double>(Count) -
            MeanDeltaSeconds * MeanDeltaSeconds)) : 0.0;
    }
};
} // namespace latebrake::engineering
