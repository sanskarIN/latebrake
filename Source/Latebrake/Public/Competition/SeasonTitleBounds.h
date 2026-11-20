#pragma once

#include "SeasonArchive.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake::competition {

// Optimistic title-race and conservative clinch bounds from an already
// validated read-only season ledger. The calculations neither predict who
// actually wins upcoming races nor modify official Unreal classification.
struct SeasonTitleOptions final {
    bool fastestLapBonusEnabled = true;
};

struct DriverTitleBounds final {
    std::uint64_t actor = 0;
    int currentPoints = 0;
    int minimumFinalPoints = 0;
    int maximumFinalPoints = 0;
    int pointsBehindCurrentLeader = 0;
    int pointsNeededToExceedCurrentLeader = 0;
    int additionalPointsToGuaranteeTitleOnPoints = -1;
    int remainingPointCapacity = 0;
    bool canReachCurrentLeaderOnPoints = false;
    bool canOutscoreCurrentLeaderOnPoints = false;
    bool alreadyClinchedTitleOnPoints = false;
    bool definitivelyEliminatedOnPoints = false;
};

struct SeasonTitleBounds final {
    std::array<DriverTitleBounds, MaximumSeasonDrivers> drivers{};
    std::size_t count = 0;
    int completedRounds = 0;
    int remainingRounds = 0;
    int currentLeaderPoints = 0;
    int maximumPointsPerRemainingRound = 0;
    std::uint64_t guaranteedChampionOnPoints = 0;
    bool valid = false;
};

inline SeasonTitleBounds ProjectSeasonTitleOnPoints(
    const SeasonRanking& ranking,
    SeasonTitleOptions options = {}) noexcept {
    SeasonTitleBounds output{};
    // Reject inconsistent standings, duplicated actors and impossible
    // position/bonus receipts before showing title predictions.
    if (!ValidArchiveRanking(ranking)) return output;
    output.count = ranking.count;
    output.completedRounds = ranking.completedRounds;
    output.remainingRounds = ranking.remainingRounds;
    output.maximumPointsPerRemainingRound =
        SeasonPositionPoints[0] +
        (options.fastestLapBonusEnabled ? 1 : 0);
    output.currentLeaderPoints = ranking.sorted[0].points;
    const int capacity = ranking.remainingRounds *
        output.maximumPointsPerRemainingRound;

    for (std::size_t i = 0; i < ranking.count; ++i) {
        const auto& driver = ranking.sorted[i];
        auto& destination = output.drivers[i];
        destination.actor = driver.actor;
        destination.currentPoints = driver.points;
        destination.minimumFinalPoints = driver.points;
        destination.maximumFinalPoints = driver.points + capacity;
        destination.remainingPointCapacity = capacity;
        destination.pointsBehindCurrentLeader =
            output.currentLeaderPoints - driver.points;
        // Only higher points make an unconditional points-only overtake;
        // a points tie may depend on wins and finishing-place tie-breakers.
        destination.pointsNeededToExceedCurrentLeader =
            i == 0 ? 0 : destination.pointsBehindCurrentLeader + 1;
        destination.canReachCurrentLeaderOnPoints =
            destination.maximumFinalPoints >= output.currentLeaderPoints;
        destination.canOutscoreCurrentLeaderOnPoints =
            i == 0 || destination.maximumFinalPoints >
                output.currentLeaderPoints;
        destination.definitivelyEliminatedOnPoints =
            destination.maximumFinalPoints < output.currentLeaderPoints;
    }
    for (std::size_t i = 0; i < ranking.count; ++i) {
        auto& candidate = output.drivers[i];
        int highestOtherMaximum = 0;
        for (std::size_t j = 0; j < ranking.count; ++j) {
            if (i == j) continue;
            highestOtherMaximum = std::max(
                highestOtherMaximum, output.drivers[j].maximumFinalPoints);
        }
        candidate.alreadyClinchedTitleOnPoints =
            candidate.currentPoints > highestOtherMaximum;
        if (candidate.alreadyClinchedTitleOnPoints) {
            output.guaranteedChampionOnPoints = candidate.actor;
            candidate.additionalPointsToGuaranteeTitleOnPoints = 0;
            continue;
        }
        // This deliberately holds other drivers' *independent* maximum
        // future scores fixed. Therefore the threshold is conservative:
        // opponents cannot all physically win the same race simultaneously,
        // but exceeding these bounds is always sufficient.
        const int needed = std::max(
            0, highestOtherMaximum - candidate.currentPoints + 1);
        if (needed <= capacity) {
            candidate.additionalPointsToGuaranteeTitleOnPoints = needed;
        }
    }
    output.valid = true;
    return output;
}

} // namespace latebrake::competition
