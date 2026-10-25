#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace latebrake
{
// Pure advisory geometry. The nearest point is measured against the same
// rectangle union as IsOnPavedSurface, not an invented terrain or road mesh.
// The result cannot authorize checkpoints, medals, or paid recovery.
struct RoadReturnAdvice final
{
    bool Valid = false;
    bool OffPavement = false;
    double DistanceMeters = 0.0;
    // World-space direction toward the nearest paved rectangle. A zero
    // direction means already paved or no unambiguous displacement.
    CircuitPoint UnitDirection{};
};

inline RoadReturnAdvice AdviseReturnToCircuit(
    CircuitPoint position) noexcept
{
    if (!std::isfinite(position.X) ||
        !std::isfinite(position.Y))
    {
        return {};
    }

    struct Rect final
    {
        double MinX, MinY, MaxX, MaxY;
    };
    constexpr std::array<Rect, 4> roads{{
        {-4000.0, SouthY - RoadHalfWidth,
           4000.0, SouthY + RoadHalfWidth},
        {-4000.0, NorthY - RoadHalfWidth,
           4000.0, NorthY + RoadHalfWidth},
        {EastX - RoadHalfWidth, -2600.0,
         EastX + RoadHalfWidth, 2600.0},
        {WestX - RoadHalfWidth, -2600.0,
         WestX + RoadHalfWidth, 2600.0}
    }};
    double best = std::numeric_limits<double>::infinity();
    CircuitPoint target{};
    for (const Rect& road : roads)
    {
        const CircuitPoint candidate{
            std::clamp(position.X, road.MinX, road.MaxX),
            std::clamp(position.Y, road.MinY, road.MaxY)
        };
        const double dx = candidate.X - position.X;
        const double dy = candidate.Y - position.Y;
        const double distance = std::hypot(dx, dy);
        if (std::isfinite(distance) && distance < best)
        {
            best = distance;
            target = candidate;
        }
    }
    if (!std::isfinite(best))
    {
        return {};
    }
    if (best == 0.0)
    {
        return {true, false, 0.0, {}};
    }
    // Reject impossibly remote, damaged physics samples; no arrow is
    // preferable to an ungrounded visual hint.
    if (best > 50000.0)
    {
        return {};
    }
    return {true, true, best / 100.0,
            {(target.X - position.X) / best,
             (target.Y - position.Y) / best}};
}

// Debounced OFF TRACK warning: disallow a single frame hitch from
// introducing a false alarm. Restarts/teleports/pause explicitly clear it.
class OffTrackIndicator final
{
public:
    void Reset() noexcept { OffPavementSeconds = 0.0; }

    bool Update(CircuitPoint position, double deltaSeconds,
                bool raceEligible) noexcept
    {
        if (!raceEligible || !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 || deltaSeconds > 0.25)
        {
            Reset();
            return false;
        }
        const RoadReturnAdvice advice = AdviseReturnToCircuit(position);
        if (!advice.Valid || !advice.OffPavement)
        {
            Reset();
            return false;
        }
        OffPavementSeconds = std::min(
            2.0, OffPavementSeconds + deltaSeconds);
        return OffPavementSeconds >= 0.8;
    }

    double GetOffPavementSeconds() const noexcept
    {
        return OffPavementSeconds;
    }

private:
    double OffPavementSeconds = 0.0;
};
} // namespace latebrake
