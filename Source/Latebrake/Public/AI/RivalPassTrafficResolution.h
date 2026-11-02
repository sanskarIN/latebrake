#pragma once

#include "AI/RivalStraightPassAssist.h"

#include <cmath>

namespace latebrake
{
// Compose the SAME real-world traffic brake hold and optional passing steer.
// A valid emergency brake may never keep a speculative lateral passing
// command active. The player's controls, route and physics stay untouched.
struct RivalPassTrafficResolution final
{
    PilotCommand Command{};
    bool AbortPassing = false;
};

inline RivalPassTrafficResolution ResolveRivalPassTraffic(
    PilotCommand plannedRoute, const RivalPassOutput& pass,
    PilotCommand trafficAndHold) noexcept
{
    const auto valid = [](const PilotCommand& p) noexcept
    {
        return std::isfinite(p.Throttle) &&
            p.Throttle >= 0.0 && p.Throttle <= 1.0 &&
            std::isfinite(p.Steering) &&
            std::abs(p.Steering) <= 1.0;
    };
    if (!valid(plannedRoute) || !valid(pass.Command) ||
        !valid(trafficAndHold) ||
        !std::isfinite(pass.TargetLaneOffsetCm) ||
        std::abs(pass.TargetLaneOffsetCm) > RivalStraightPassAssist::LaneOffsetCm)
    {
        return {{}, true};  // default safe brake and no future passing state
    }
    // If the actual following or anti-collision policy activated braking
    // in this tick, restore the original route steering *while* braking.
    // It would be unsafe to retain a passing steer and scrape alongside
    // the slowing/stopped player's real chassis under emergency braking.
    if (pass.Passing && (trafficAndHold.Brake ||
                         trafficAndHold.Arrived))
    {
        trafficAndHold.Throttle = 0.0;
        trafficAndHold.Steering = plannedRoute.Steering;
        return {trafficAndHold, true};
    }
    return {trafficAndHold, false};
}
} // namespace latebrake
