#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Source-only, engine-neutral arcade handling assist. This is not a tire
// model: it tempers positive motor demand when lateral motion dominates.
// Chaos, collision, braking and driver input ownership remain unchanged.
inline double CornerThrottleFactor(
    double longitudinalCmPerSecond, double lateralCmPerSecond,
    bool arcadeHandling) noexcept
{
    if (!std::isfinite(longitudinalCmPerSecond) ||
        !std::isfinite(lateralCmPerSecond))
    {
        return 0.0;
    }
    const double longitudinal = std::abs(longitudinalCmPerSecond);
    const double lateral = std::abs(lateralCmPerSecond);
    // Retain full low-speed acceleration, including launch from rest.
    if (longitudinal < 900.0 || lateral < 250.0)
    {
        return 1.0;
    }
    // The finite offset prevents dramatic reductions from tiny forward
    // movement while still responding to fast cornering/sliding.
    const double slip = lateral / (longitudinal + 400.0);
    const double start = arcadeHandling ? 0.18 : 0.13;
    const double saturated = arcadeHandling ? 0.75 : 0.55;
    const double minimum = arcadeHandling ? 0.58 : 0.45;
    const double blend = std::clamp(
        (slip - start) / (saturated - start), 0.0, 1.0);
    return 1.0 - blend * (1.0 - minimum);
}

inline double StabilizeCornerThrottle(
    double demand, double longitudinalCmPerSecond,
    double lateralCmPerSecond, bool arcadeHandling) noexcept
{
    if (!std::isfinite(demand) ||
        !std::isfinite(longitudinalCmPerSecond) ||
        !std::isfinite(lateralCmPerSecond))
    {
        // A corrupt source sample cannot authorize extra motor force.
        return 0.0;
    }
    const double bounded = std::clamp(demand, -1.0, 1.0);
    // Never interfere with reverse gear or with engine-braking requests.
    if (bounded <= 0.0)
    {
        return bounded;
    }
    return bounded * CornerThrottleFactor(
        longitudinalCmPerSecond, lateralCmPerSecond, arcadeHandling);
}
} // namespace latebrake
