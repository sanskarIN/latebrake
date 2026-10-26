#pragma once

#include "Racing/FinishTimingEligibility.h"

#include <array>
#include <cmath>

namespace latebrake
{
// Per-gate BEST positive splits measured within the current completed race.
// The sum of best splits can combine different laps: it is explicitly NOT
// an achieved lap, permanent record or authoritative reward threshold.
struct BestSectorReview final
{
    bool Valid = false;
    int Count = 0;
    double MixedBestSectorsSeconds = 0.0;
    double SecondsBehindAchievedBest = 0.0;
    std::array<double, RaceProgress::MaxCheckpoints> SectorSeconds{};
};

inline BestSectorReview BuildBestSectorReview(
    const RaceProgress& race) noexcept
{
    BestSectorReview result;
    if (!CanAwardFinishedRace(race))
    {
        return result;
    }
    const int count = race.GetCheckpointCount();
    const double bestAchieved = race.GetBestLapSeconds();
    const double theoretical = race.GetTheoreticalBestLapSeconds();
    if (count < 1 || count > RaceProgress::MaxCheckpoints ||
        !IsValidFinishedRaceTime(bestAchieved) ||
        !IsValidFinishedRaceTime(theoretical) ||
        theoretical > bestAchieved)
    {
        return result;
    }
    double sum = 0.0;
    for (int index = 0; index < count; ++index)
    {
        const double split = race.GetBestSectorSeconds(index);
        if (!IsValidFinishedRaceTime(split) ||
            split > 604800.0 - sum)
        {
            return {};
        }
        sum += split;
        result.SectorSeconds[static_cast<std::size_t>(index)] = split;
    }
    // The production theoretical sum must match the same actual splits;
    // do not independently invent a faster timing reference.
    if (!std::isfinite(sum) || std::abs(sum - theoretical) > 1e-7)
    {
        return {};
    }
    result.Valid = true;
    result.Count = count;
    result.MixedBestSectorsSeconds = theoretical;
    result.SecondsBehindAchievedBest = bestAchieved - theoretical;
    return result;
}
} // namespace latebrake
