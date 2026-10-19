#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Fixed-rate, deterministic steering slew limiter. Inputs are normalized
// steering values in [-1,+1], rate is steering-units per second.
inline double SmoothSteering(double current, double requested,
                             double deltaSeconds, double ratePerSecond) noexcept
{
    if (!std::isfinite(current) || !std::isfinite(requested) ||
        !std::isfinite(deltaSeconds) || !std::isfinite(ratePerSecond) ||
        deltaSeconds <= 0.0 || ratePerSecond <= 0.0)
    {
        return std::isfinite(current) ? std::clamp(current, -1.0, 1.0) : 0.0;
    }
    const double from = std::clamp(current, -1.0, 1.0);
    if (deltaSeconds > std::numeric_limits<double>::max() / ratePerSecond)
    {
        return from;
    }
    const double target = std::clamp(requested, -1.0, 1.0);
    const double difference = target - from;
    const double maxStep = std::min(2.0, deltaSeconds * ratePerSecond);
    if (!std::isfinite(maxStep))
    {
        return from;
    }
    return std::clamp(from + std::clamp(difference, -maxStep, maxStep), -1.0, 1.0);
}
} // namespace latebrake
