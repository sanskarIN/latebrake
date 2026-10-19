#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Shared, engine-independent numeric guards for keyboard, gamepad,
// experimental touch and AI commands.
inline double SanitizeDriveAxis(double requested) noexcept
{
    return std::isfinite(requested) ? std::clamp(requested, -1.0, 1.0) : 0.0;
}

inline double SanitizeTouchSteering(double x, double viewportWidth) noexcept
{
    if (!std::isfinite(x) || !std::isfinite(viewportWidth) || viewportWidth <= 0.0)
    {
        return 0.0;
    }
    const double fraction = std::clamp(x / viewportWidth, 0.0, 1.0);
    return SanitizeDriveAxis((fraction - 0.25) * 4.0);
}

// Throttle smoothing only (braking is immediate for safety).
// Invalid frame steps preserve the last valid command, preventing numerical
// explosions after a pause, resize or a corrupted controller reading.
inline double SmoothThrottle(double previous, double requested,
                             double deltaSeconds, double riseRate = 4.0) noexcept
{
    const double prior = SanitizeDriveAxis(previous);
    const double target = SanitizeDriveAxis(requested);
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0 ||
        !std::isfinite(riseRate) || riseRate <= 0.0)
    {
        return prior;
    }
    if (deltaSeconds > std::numeric_limits<double>::max() / riseRate)
    {
        return prior;
    }
    const double step = std::min(2.0, deltaSeconds * riseRate);
    return std::clamp(prior + std::clamp(target - prior, -step, step), -1.0, 1.0);
}
} // namespace latebrake
