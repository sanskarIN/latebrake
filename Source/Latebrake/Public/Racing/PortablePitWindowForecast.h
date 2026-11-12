#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// A read-only pit decision forecast for independently measured lap-time
// inputs. No actual tire wear, pit entry detection, fuel, safety car,
// official race timing or garage reward is sourced by this component.
struct PortablePitWindowInput final
{
    int RemainingLaps = 10;
    int EarliestStopAfterLap = 1;
    int LatestStopAfterLap = 9;
    double BaselineLapSeconds = 90.0;
    double CurrentTireLossSecondsPerLap = 2.0;
    double CurrentTireLossGrowthSecondsPerLap = 0.5;
    double NewTireLossSecondsPerLap = 0.0;
    double NewTireLossGrowthSecondsPerLap = 0.1;
    double PitLaneAndServiceSeconds = 25.0;
    bool PitStopMandatory = false;
};

struct PortablePitWindowForecast final
{
    bool Valid = false;
    bool PitRecommended = false;
    int StopAfterUpcomingLap = -1;
    int EvaluatedPitWindows = 0;
    double NoStopProjectedSeconds = 0.0;
    double SelectedProjectedSeconds = 0.0;
    // Can be negative if an externally mandatory pit costs time.
    double BenefitSeconds = 0.0;
};

inline bool ValidPortablePitWindowInput(
    PortablePitWindowInput input) noexcept
{
    const auto finite = [](double value) noexcept { return std::isfinite(value); };
    return input.RemainingLaps >= 2 && input.RemainingLaps <= 150 &&
        input.EarliestStopAfterLap >= 1 &&
        input.LatestStopAfterLap < input.RemainingLaps &&
        input.EarliestStopAfterLap <= input.LatestStopAfterLap &&
        finite(input.BaselineLapSeconds) &&
        input.BaselineLapSeconds >= 30.0 &&
        input.BaselineLapSeconds <= 1200.0 &&
        finite(input.CurrentTireLossSecondsPerLap) &&
        input.CurrentTireLossSecondsPerLap >= 0.0 &&
        input.CurrentTireLossSecondsPerLap <= 120.0 &&
        finite(input.CurrentTireLossGrowthSecondsPerLap) &&
        input.CurrentTireLossGrowthSecondsPerLap >= 0.0 &&
        input.CurrentTireLossGrowthSecondsPerLap <= 10.0 &&
        finite(input.NewTireLossSecondsPerLap) &&
        input.NewTireLossSecondsPerLap >= 0.0 &&
        input.NewTireLossSecondsPerLap <= 120.0 &&
        finite(input.NewTireLossGrowthSecondsPerLap) &&
        input.NewTireLossGrowthSecondsPerLap >= 0.0 &&
        input.NewTireLossGrowthSecondsPerLap <= 10.0 &&
        finite(input.PitLaneAndServiceSeconds) &&
        input.PitLaneAndServiceSeconds >= 0.0 &&
        input.PitLaneAndServiceSeconds <= 600.0;
}

// Closed-form arithmetic-series sum is stable and deterministic; it does
// not require a simulated per-lap engine state or heap allocation.
inline double PortableTotalTireLoss(
    double firstLoss, double growth, int laps) noexcept
{
    const double n = static_cast<double>(laps);
    return firstLoss * n + growth * n * (n - 1.0) * 0.5;
}

inline PortablePitWindowForecast ForecastPortablePitWindow(
    PortablePitWindowInput input) noexcept
{
    if (!ValidPortablePitWindowInput(input))
    {
        return {};
    }
    PortablePitWindowForecast result;
    result.Valid = true;
    const double n = static_cast<double>(input.RemainingLaps);
    const double baseline = n * input.BaselineLapSeconds;
    result.NoStopProjectedSeconds = baseline + PortableTotalTireLoss(
        input.CurrentTireLossSecondsPerLap,
        input.CurrentTireLossGrowthSecondsPerLap, input.RemainingLaps);
    result.SelectedProjectedSeconds = result.NoStopProjectedSeconds;

    double bestPitSeconds = 604801.0;
    int bestLap = -1;
    for (int before = input.EarliestStopAfterLap;
         before <= input.LatestStopAfterLap; ++before)
    {
        const int after = input.RemainingLaps - before;
        const double candidate = baseline +
            PortableTotalTireLoss(input.CurrentTireLossSecondsPerLap,
                input.CurrentTireLossGrowthSecondsPerLap, before) +
            PortableTotalTireLoss(input.NewTireLossSecondsPerLap,
                input.NewTireLossGrowthSecondsPerLap, after) +
            input.PitLaneAndServiceSeconds;
        ++result.EvaluatedPitWindows;
        // Exact tie stays at the earlier stop; no numeric jitter can
        // silently reorder two equal-timing candidates.
        if (candidate < bestPitSeconds)
        {
            bestPitSeconds = candidate;
            bestLap = before;
        }
    }
    if (bestLap < 1 || !std::isfinite(bestPitSeconds) ||
        !std::isfinite(result.NoStopProjectedSeconds) ||
        result.NoStopProjectedSeconds > 604800.0 ||
        bestPitSeconds > 604800.0)
    {
        return {};
    }
    if (input.PitStopMandatory || bestPitSeconds < result.NoStopProjectedSeconds)
    {
        result.PitRecommended = true;
        result.StopAfterUpcomingLap = bestLap;
        result.SelectedProjectedSeconds = bestPitSeconds;
    }
    result.BenefitSeconds =
        result.NoStopProjectedSeconds - result.SelectedProjectedSeconds;
    return result;
}
} // namespace latebrake
