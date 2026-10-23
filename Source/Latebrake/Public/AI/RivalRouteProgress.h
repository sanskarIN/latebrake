#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Pure C++20 route bookkeeping for the experimental autonomous car.
// Waypoint progress is approximate; it is not an authoritative race result.
struct RouteObservation final
{
    bool Advanced = false;
    bool NeedsRecovery = false;
    bool Completed = false;
};

class RivalRouteProgress final
{
public:
    // Keep the number of autonomous teleports within an observable, bounded
    // session budget. Never request a recovery without recording it.
    static constexpr int MaxRecoveries = 100000;

    explicit RivalRouteProgress(int waypointCount = 4, int requiredLaps = 3) noexcept
        : WaypointCount(std::clamp(waypointCount, 1, 256)),
          RequiredLaps(std::clamp(requiredLaps, 1, 100))
    {
    }

    // Reconfigure when switching quick races/career rounds. All route,
    // recovery and stuck-timer state belongs to the previous attempt and
    // must not leak into the new event's lap count.
    void Configure(int waypointCount, int requiredLaps) noexcept
    {
        WaypointCount = std::clamp(waypointCount, 1, 256);
        RequiredLaps = std::clamp(requiredLaps, 1, 100);
        Reset();
    }

    void Reset() noexcept
    {
        NextWaypoint = 0;
        CompletedLaps = 0;
        StationarySeconds = 0.0;
        Recoveries = 0;
    }

    RouteObservation Observe(double distanceCm, double speedCmPerSecond,
                             double deltaSeconds) noexcept
    {
        if (IsFinished() || !std::isfinite(distanceCm) || distanceCm < 0.0 ||
            !std::isfinite(speedCmPerSecond) || speedCmPerSecond < 0.0 ||
            !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
        {
            return {};
        }

        if (distanceCm <= 440.0)
        {
            StationarySeconds = 0.0;
            ++NextWaypoint;
            if (NextWaypoint == WaypointCount)
            {
                NextWaypoint = 0;
                ++CompletedLaps;
            }
            return {true, false, IsFinished()};
        }

        if (speedCmPerSecond >= 80.0)
        {
            StationarySeconds = 0.0;
            return {};
        }

        // A genuinely stuck car should not remain pressed against a wall
        // forever. One recovery request per 8s of low-speed observation.
        StationarySeconds = std::min(8.0, StationarySeconds + deltaSeconds);
        if (StationarySeconds >= 8.0)
        {
            StationarySeconds = 0.0;
            // The telemetry counter and the actual teleport request must
            // never diverge. Once the finite budget is exhausted the rival
            // remains stopped instead of receiving free untracked recoveries.
            if (Recoveries >= MaxRecoveries)
            {
                return {};
            }
            ++Recoveries;
            return {false, true, false};
        }
        return {};
    }

    int GetNextWaypoint() const noexcept { return NextWaypoint; }
    int GetWaypointCount() const noexcept { return WaypointCount; }
    int GetRequiredLaps() const noexcept { return RequiredLaps; }
    int GetCompletedLaps() const noexcept { return CompletedLaps; }
    int GetRecoveryCount() const noexcept { return Recoveries; }
    bool IsFinished() const noexcept { return CompletedLaps >= RequiredLaps; }

private:
    int WaypointCount;
    int RequiredLaps;
    int NextWaypoint = 0;
    int CompletedLaps = 0;
    int Recoveries = 0;
    double StationarySeconds = 0.0;
};
} // namespace latebrake
