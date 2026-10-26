#pragma once

#include <cmath>

namespace latebrake
{
// Race-local, read-only launch INPUT onset. This is not a neurological
// reflex time, measured vehicle acceleration, penalty, career reward,
// leaderboard result or permanent player statistic.
//
// A forward motor command must first reach the actual local player pawn
// AFTER the authoritative 3/2/1/GO gate. The caller supplies the official
// RaceProgress time and already physics-approved command. A forward
// command held before GO may legitimately report near zero on green.
class LaunchInputOnset final
{
public:
    static constexpr double MinimumForwardCommand = 0.2;
    static constexpr double MaxTrackedSeconds = 120.0;

    void Reset() noexcept
    {
        Captured = false;
        StartInputSeconds = 0.0;
    }

    bool Observe(double officialRaceSeconds, double effectiveForwardThrottle,
                 bool braking, bool exactLocalPlayer) noexcept
    {
        if (Captured || braking || !exactLocalPlayer ||
            !std::isfinite(officialRaceSeconds) ||
            officialRaceSeconds < 0.0 ||
            officialRaceSeconds > MaxTrackedSeconds ||
            !std::isfinite(effectiveForwardThrottle) ||
            effectiveForwardThrottle < MinimumForwardCommand ||
            effectiveForwardThrottle > 1.0)
        {
            return false;
        }
        StartInputSeconds = officialRaceSeconds;
        Captured = true;
        return true;
    }

    bool HasInputOnset() const noexcept { return Captured; }
    double FirstForwardInputSeconds() const noexcept
    {
        return Captured ? StartInputSeconds : 0.0;
    }

private:
    double StartInputSeconds = 0.0;
    bool Captured = false;
};
} // namespace latebrake
