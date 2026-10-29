#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Bound the first-order velocity resistance requested by the prototype's
// force-based chassis. A single finite frame cannot request more than the
// existing signed axis momentum through its damping term alone.
//
// This is a source-level safety bound, not a replacement for Chaos substep
// integration, road friction, tire slip, or actual installed-game playtests.
inline double FrameStableDampingRate(double perSecondRate,
                                     double deltaSeconds) noexcept
{
    if (!std::isfinite(perSecondRate) || perSecondRate < 0.0 ||
        !std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
    {
        return 0.0;
    }
    return std::min(perSecondRate, 1.0 / deltaSeconds);
}
} // namespace latebrake
