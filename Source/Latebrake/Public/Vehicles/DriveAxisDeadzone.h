#pragma once

#include "Vehicles/DriverInputSafety.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Pure axis noise filter for human keyboard/gamepad input only. Touch uses
// physical steering geometry; AI steering already goes through separate
// sanitized autonomous controls, not these functions.
//
// Removes a symmetric center deadzone and re-scales the remaining travel.
// Endpoints (+/-1) remain exactly full input authority; no frame delta,
// hidden smoothing state, input capture or engine object is involved.
inline double RescaleDriveAxisDeadzone(
    double rawAxis, double threshold) noexcept
{
    if (!std::isfinite(rawAxis) || !std::isfinite(threshold) ||
        threshold < 0.0 || threshold >= 1.0)
    {
        return 0.0;
    }
    const double axis = SanitizeDriveAxis(rawAxis);
    const double magnitude = std::abs(axis);
    if (magnitude <= threshold)
    {
        return 0.0;
    }
    const double output = (magnitude - threshold) / (1.0 - threshold);
    return std::copysign(std::clamp(output, 0.0, 1.0), axis);
}

// InputSettings maps W/S plus both signed gamepad trigger axes into
// Throttle; A/D and left-X into Steer. Explicitly filter *only* the
// player callbacks so rivals/experimental touch are not double-filtered.
inline double FilterHumanSteeringAxis(double rawAxis) noexcept
{
    return RescaleDriveAxisDeadzone(rawAxis, 0.12);
}

inline double FilterHumanThrottleAxis(double rawAxis) noexcept
{
    return RescaleDriveAxisDeadzone(rawAxis, 0.055);
}
} // namespace latebrake
