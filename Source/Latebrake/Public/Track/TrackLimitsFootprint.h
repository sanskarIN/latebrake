#pragma once

#include "Track/CircuitLayout.h"
#include "Vehicles/VehicleWheelLayout.h"

#include <cmath>

namespace latebrake
{
// Conservative source-only *visual* wheel overlap with the original four
// paved starter-circuit rectangles. Inflated by one cosmetic tire radius.
// This is NOT a Chaos wheel-ground trace or a drivable road-mesh proof.
inline bool IsNearStarterRoadTireRadius(CircuitPoint position) noexcept
{
    if (!std::isfinite(position.X) || !std::isfinite(position.Y))
        return false;
    const double margin = VehicleVisualWheelRadiusCm;
    const bool horizontal =
        std::abs(position.X) <= 4000.0 + margin &&
        (std::abs(position.Y - SouthY) <= RoadHalfWidth + margin ||
         std::abs(position.Y - NorthY) <= RoadHalfWidth + margin);
    const bool vertical =
        std::abs(position.Y) <= 2600.0 + margin &&
        (std::abs(position.X - EastX) <= RoadHalfWidth + margin ||
         std::abs(position.X - WestX) <= RoadHalfWidth + margin);
    return horizontal || vertical;
}

struct TrackLimitsFootprint final
{
    bool Valid = false;
    int WheelsNearPavement = 0;
    bool FullyOffPavement = false;
};

inline TrackLimitsFootprint ObserveStarterWheelFootprint(
    CircuitPoint center, double headingDegrees) noexcept
{
    // The starter circuit fits inside a 100m-wide game region. Physics
    // positions beyond 1km are not valid evidence for a racing violation;
    // reject them instead of treating numeric extremes as grass.
    constexpr double MaximumAuditDistanceCm = 100000.0;
    if (!std::isfinite(center.X) || !std::isfinite(center.Y) ||
        !std::isfinite(headingDegrees) ||
        std::abs(center.X) > MaximumAuditDistanceCm ||
        std::abs(center.Y) > MaximumAuditDistanceCm) return {};
    constexpr double Pi = 3.14159265358979323846;
    const double yaw = std::remainder(headingDegrees, 360.0) * Pi / 180.0;
    const double cosine = std::cos(yaw);
    const double sine = std::sin(yaw);
    if (!std::isfinite(cosine) || !std::isfinite(sine)) return {};
    int near = 0;
    for (const CircuitPoint local : VehicleVisualWheelLocalXY)
    {
        const CircuitPoint projected{
            center.X + cosine * local.X - sine * local.Y,
            center.Y + sine * local.X + cosine * local.Y
        };
        if (!std::isfinite(projected.X) || !std::isfinite(projected.Y))
            return {}; // No penalty when projection overflows.
        near += IsNearStarterRoadTireRadius(projected) ? 1 : 0;
    }
    return {true, near, near == 0};
}
} // namespace latebrake
