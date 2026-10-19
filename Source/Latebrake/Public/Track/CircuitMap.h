#pragma once

#include "Track/CircuitLayout.h"
#include <algorithm>

namespace latebrake
{
// Normalized view-space positions for a simple north-up HUD course map.
// No Unreal dependency; clamp outside-world player locations.
struct CircuitMapPoint final
{
    double U = 0.5;
    double V = 0.5;
};

inline CircuitMapPoint NormalizeCircuitMap(CircuitPoint point) noexcept
{
    if (!std::isfinite(point.X) || !std::isfinite(point.Y))
        return {};
    constexpr double HalfMapWidth = 4200.0;
    constexpr double HalfMapHeight = 3000.0;
    return {std::clamp((point.X + HalfMapWidth) / (2.0 * HalfMapWidth), 0.0, 1.0),
            std::clamp((point.Y + HalfMapHeight) / (2.0 * HalfMapHeight), 0.0, 1.0)};
}
} // namespace latebrake
