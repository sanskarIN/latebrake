#pragma once

#include <limits>

namespace latebrake
{
// A read-only display of accepted ordered gates, NEVER physical race position,
// guessed distance, lap time, a checkpoint grant or reward authority.
struct RaceGateProgressGauge final
{
    bool Valid = false;
    int CreditedGates = 0;
    int TotalGates = 0;
    double Fraction = 0.0;
};

inline RaceGateProgressGauge CalculateRaceGateProgressGauge(
    int completedLaps, int nextCheckpoint, int requiredLaps,
    int checkpointCount, bool raceFinished) noexcept
{
    // Reject corrupt/mixed-session inputs, and multiplication overflow BEFORE
    // computing totals. This can only describe authoritative race snapshots.
    if (checkpointCount < 1 || checkpointCount > 256 ||
        requiredLaps < 1 ||
        requiredLaps > std::numeric_limits<int>::max() / checkpointCount ||
        completedLaps < 0 || completedLaps > requiredLaps ||
        nextCheckpoint < 0 || nextCheckpoint >= checkpointCount ||
        (raceFinished != (completedLaps == requiredLaps)) ||
        (raceFinished && nextCheckpoint != 0))
    {
        return {};
    }
    const int total = requiredLaps * checkpointCount;
    const int credited = completedLaps * checkpointCount + nextCheckpoint;
    // The final gate wraps the next index to zero on race completion.
    if (credited < 0 || credited > total ||
        (!raceFinished && credited == total))
    {
        return {};
    }
    return {true, credited, total,
            static_cast<double>(credited) / static_cast<double>(total)};
}
} // namespace latebrake
