#pragma once

#include "Track/CircuitLayout.h"

#include <array>

namespace latebrake
{
// Exactly the noncolliding STARTER car's visual-wheel local XY offsets.
// Wheel order: front-left, front-right, rear-left, rear-right.
// These positions are not actual Chaos suspension/contact observations.
inline constexpr std::array<CircuitPoint, 4> VehicleVisualWheelLocalXY{{
    {122.0, -94.0}, {122.0, 94.0},
    {-122.0, -94.0}, {-122.0, 94.0}
}};
inline constexpr double VehicleVisualWheelLocalZCm = -22.0;
// Stock cylinder 100cm diameter, scaled 0.58 on the tire radial axes.
inline constexpr double VehicleVisualWheelRadiusCm = 29.0;
static_assert(VehicleVisualWheelLocalXY.size() == 4);
} // namespace latebrake
