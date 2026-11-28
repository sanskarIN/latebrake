#pragma once

#include "Racing/RaceProgress.h"
#include "Progression/RecordValidation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Projection of the remaining reference sectors, not physical position,
// live speed extrapolation, an official lap record or a guarantee. The
// clock already includes charged fees. When an unfinished sector has
// exhausted its reference time, do not pretend it needs zero more time.
enum class GatePaceStatus
{
    Unavailable,
    Estimated,
    CurrentSectorOverdue
};

struct GatePaceProjection final
{
    GatePaceStatus Status = GatePaceStatus::Unavailable;
    int ReferenceLapNumber = 0;
    int CurrentLapNumber = 0;
    int NextGateIndex = -1;
    int LapsRemaining = 0;
    double ReferenceLapSeconds = 0.0;
    double ReferenceSectorSeconds = 0.0;
    double CurrentSectorClockSeconds = 0.0;
    double CurrentSectorFeeSeconds = 0.0;
    double CurrentSectorDrivingClockSeconds = 0.0;
    double CurrentSectorOverdueSeconds = 0.0;
    double RemainingCurrentSectorSeconds = 0.0;
    double RemainingLaterSectorsSeconds = 0.0;
    double RemainingLaterLapsSeconds = 0.0;
    double EstimatedRemainingSeconds = 0.0;
    double EstimatedFinishSeconds = 0.0;
};

namespace gate_pace_detail
{
constexpr double ClockTolerance = 1e-7;

inline bool NonnegativeClock(double value) noexcept
{
    return std::isfinite(value) && value >= 0.0 &&
        value <= MeasuredLapClockLimit;
}

inline bool EqualClock(double a, double b) noexcept
{
    return NonnegativeClock(a) && NonnegativeClock(b) &&
        std::abs(a-b) <= ClockTolerance;
}

// Prefix count and lap number are checked before indexing. Every accepted
// gate must have a valid ordered cumulative clock; the active-evidence bit
// is maintained by the authoritative RaceProgress observer.
inline bool ValidPrefix(const MeasuredLapTrace& prefix, int nextGate,
    int lapNumber, int gates) noexcept
{
    if (prefix.Complete || !prefix.AllGatesActivelyTimed ||
        prefix.LapNumber != lapNumber || prefix.GateCount != nextGate ||
        gates < 1 || gates > MeasuredLapMaxGates ||
        nextGate < 0 || nextGate >= gates)
        return false;
    MeasuredLapGate previous{};
    for (int gate=0; gate<nextGate; ++gate)
    {
        const auto observation=prefix.Gates[static_cast<std::size_t>(gate)];
        if (!ValidMeasuredLapGate(observation,previous))
            return false;
        previous=observation;
    }
    return true;
}

inline bool AddWithinBound(double value, double& total) noexcept
{
    if (!NonnegativeClock(total) || !NonnegativeClock(value) ||
        value > MeasuredLapClockLimit-total)
        return false;
    total += value;
    return NonnegativeClock(total);
}
} // namespace gate_pace_detail

inline GatePaceProjection ProjectGateAwareRacePace(
    const RaceProgress& race) noexcept
{
    using namespace gate_pace_detail;
    GatePaceProjection result;
    const int completed=race.GetCompletedLaps();
    const int required=race.GetRequiredLaps();
    const int gates=race.GetCheckpointCount();
    const int next=race.GetNextCheckpoint();
    if (race.IsFinished() || completed < 1 || completed >= required ||
        gates < 1 || gates > MeasuredLapMaxGates || next < 0 || next >= gates)
        return result;

    const auto& measured=race.GetMeasuredLapSplits();
    const auto& reference=measured.BestReference();
    const auto& prefix=measured.CurrentTrace();
    if (!ValidMeasuredLapTrace(reference) || !reference.FeeFree ||
        reference.GateCount != gates || reference.LapNumber > completed ||
        !ValidPrefix(prefix,next,completed+1,gates))
        return result;

    const double elapsed=race.GetElapsedSeconds();
    const double current=race.GetCurrentLapSeconds();
    const double sector=race.GetCurrentSectorSeconds();
    const double currentFees=race.GetCurrentLapPenaltySeconds();
    const double totalFees=race.GetTotalPenaltySeconds();
    if (!IsValidFinishedRaceTime(elapsed) || !NonnegativeClock(current) ||
        !NonnegativeClock(sector) || !NonnegativeClock(currentFees) ||
        !NonnegativeClock(totalFees) || current > elapsed ||
        currentFees > current || currentFees > totalFees ||
        totalFees > elapsed || sector > current)
        return result;

    const auto index=static_cast<std::size_t>(next);
    const MeasuredLapGate prev=next>0?prefix.Gates[index-1]:MeasuredLapGate{};
    if (prev.CumulativeSeconds > current ||
        prev.CumulativeFeeSeconds > currentFees ||
        !EqualClock(current-prev.CumulativeSeconds,sector))
        return result;

    const double sectorFees=currentFees-prev.CumulativeFeeSeconds;
    if (!NonnegativeClock(sectorFees) || sectorFees > sector+ClockTolerance)
        return result;
    const double driving=std::max(0.0,sector-sectorFees);
    const double referencePrevious=next>0
        ? reference.Gates[index-1].CumulativeSeconds:0.0;
    const double referenceGate=reference.Gates[index].CumulativeSeconds;
    const double referenceSector=referenceGate-referencePrevious;
    const double later=reference.LapSeconds-referenceGate;
    if (!IsValidFinishedRaceTime(referenceSector) ||
        !NonnegativeClock(later) || !NonnegativeClock(driving))
        return result;

    result.ReferenceLapNumber=reference.LapNumber;
    result.CurrentLapNumber=completed+1;
    result.NextGateIndex=next;
    result.LapsRemaining=required-completed;
    result.ReferenceLapSeconds=reference.LapSeconds;
    result.ReferenceSectorSeconds=referenceSector;
    result.CurrentSectorClockSeconds=sector;
    result.CurrentSectorFeeSeconds=std::min(sector,sectorFees);
    result.CurrentSectorDrivingClockSeconds=driving;
    if (driving >= referenceSector)
    {
        result.Status=GatePaceStatus::CurrentSectorOverdue;
        result.CurrentSectorOverdueSeconds=driving-referenceSector;
        return result;
    }

    const double remainder=referenceSector-driving;
    const int laterLaps=result.LapsRemaining-1;
    const double future=static_cast<double>(laterLaps)*reference.LapSeconds;
    double remaining=0.0;
    double predicted=elapsed;
    if (!(remainder>0.0) || !AddWithinBound(remainder,remaining) ||
        !AddWithinBound(later,remaining) || !AddWithinBound(future,remaining) ||
        !AddWithinBound(remaining,predicted) ||
        predicted <= elapsed || !IsValidFinishedRaceTime(predicted))
        return {};

    result.Status=GatePaceStatus::Estimated;
    result.RemainingCurrentSectorSeconds=remainder;
    result.RemainingLaterSectorsSeconds=later;
    result.RemainingLaterLapsSeconds=future;
    result.EstimatedRemainingSeconds=remaining;
    result.EstimatedFinishSeconds=predicted;
    return result;
}
} // namespace latebrake
