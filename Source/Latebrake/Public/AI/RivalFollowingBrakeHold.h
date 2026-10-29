#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Small post-decision brake hysteresis for a prototype autonomous rival.
// This does not steer around cars or claim real collision avoidance. It
// cannot change a player car, scoring, route progression or save data.
class RivalFollowingBrakeHold final
{
public:
    static constexpr double HoldSeconds = 0.18;
    static constexpr double MaxFrameSeconds = 0.25;

    void Reset() noexcept { RemainingSeconds = 0.0; }
    double GetRemainingSeconds() const noexcept { return RemainingSeconds; }

    PilotCommand Apply(PilotCommand planned, double deltaSeconds,
                       bool qualified, bool newTrafficBrake) noexcept
    {
        // A race/session interruption or malformed time must not carry
        // stale traffic state into a resumed attempt or teleported rival.
        if (!qualified || !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 || deltaSeconds > MaxFrameSeconds ||
            !std::isfinite(planned.Throttle) ||
            !std::isfinite(planned.Steering) ||
            planned.Throttle < -1.0 || planned.Throttle > 1.0 ||
            planned.Steering < -1.0 || planned.Steering > 1.0)
        {
            Reset();
            return {};  // zero throttle, neutral steering and braking
        }
        if (planned.Arrived)
        {
            Reset();
            return planned;  // waypoint logic remains authoritative
        }

        if (newTrafficBrake)
        {
            RemainingSeconds = HoldSeconds;
            planned.Throttle = 0.0;
            planned.Brake = true;
        }
        else if (RemainingSeconds > 0.0)
        {
            // A few stable frames prevent oscillation at the traffic
            // envelope boundary. A real already-braking corner is never
            // weakened and steering is never altered by this latch.
            RemainingSeconds = std::max(0.0, RemainingSeconds - deltaSeconds);
            if (RemainingSeconds > 0.0)
            {
                planned.Throttle = 0.0;
                planned.Brake = true;
            }
        }
        return planned;
    }

private:
    double RemainingSeconds = 0.0;
};
} // namespace latebrake
