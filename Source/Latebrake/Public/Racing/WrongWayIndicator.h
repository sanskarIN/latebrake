#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Low-cost, advisory UI ONLY. Never authorize/reject checkpoints,
// timing, career rewards or player movement from this approximate detector.
enum class CircuitTravelHeading
{
    Unavailable, East, North, West, South
};

enum class CircuitTravelAssessment
{
    Unknown, WithRoute, AgainstRoute
};

// The four central straights are unambiguous. Suppress hints at corners,
// intersections and away from the lane rather than guessing a heading.
inline CircuitTravelHeading ExpectedCircuitHeading(
    CircuitPoint position) noexcept
{
    if (!std::isfinite(position.X) || !std::isfinite(position.Y))
    {
        return CircuitTravelHeading::Unavailable;
    }
    constexpr double midlineWidth = 450.0;
    constexpr double cornerClearance = 1000.0;
    if (std::abs(position.Y - SouthY) <= midlineWidth &&
        position.X >= WestX + cornerClearance &&
        position.X <= EastX - cornerClearance)
    {
        return CircuitTravelHeading::East;
    }
    if (std::abs(position.X - EastX) <= midlineWidth &&
        position.Y >= SouthY + cornerClearance &&
        position.Y <= NorthY - cornerClearance)
    {
        return CircuitTravelHeading::North;
    }
    if (std::abs(position.Y - NorthY) <= midlineWidth &&
        position.X >= WestX + cornerClearance &&
        position.X <= EastX - cornerClearance)
    {
        return CircuitTravelHeading::West;
    }
    if (std::abs(position.X - WestX) <= midlineWidth &&
        position.Y >= SouthY + cornerClearance &&
        position.Y <= NorthY - cornerClearance)
    {
        return CircuitTravelHeading::South;
    }
    return CircuitTravelHeading::Unavailable;
}

inline CircuitTravelAssessment AssessCircuitTravel(
    CircuitPoint position, CircuitPoint velocityCmPerSecond) noexcept
{
    const CircuitTravelHeading heading = ExpectedCircuitHeading(position);
    if (heading == CircuitTravelHeading::Unavailable ||
        !std::isfinite(velocityCmPerSecond.X) ||
        !std::isfinite(velocityCmPerSecond.Y))
    {
        return CircuitTravelAssessment::Unknown;
    }
    const double speed = std::hypot(
        velocityCmPerSecond.X, velocityCmPerSecond.Y);
    // ~22 km/h: low-speed manoeuvres and near-stationary steering should
    // never show a WRONG WAY warning. Reject nonfinite derived speed.
    if (!std::isfinite(speed) || speed < 600.0)
    {
        return CircuitTravelAssessment::Unknown;
    }
    double along = 0.0;
    switch (heading)
    {
    case CircuitTravelHeading::East:  along = velocityCmPerSecond.X; break;
    case CircuitTravelHeading::North: along = velocityCmPerSecond.Y; break;
    case CircuitTravelHeading::West:  along = -velocityCmPerSecond.X; break;
    case CircuitTravelHeading::South: along = -velocityCmPerSecond.Y; break;
    case CircuitTravelHeading::Unavailable: return CircuitTravelAssessment::Unknown;
    }
    const double cosine = along / speed;
    // Avoid false alerts from pure sideways drifting.
    if (cosine <= -0.65)
    {
        return CircuitTravelAssessment::AgainstRoute;
    }
    if (cosine >= 0.65)
    {
        return CircuitTravelAssessment::WithRoute;
    }
    return CircuitTravelAssessment::Unknown;
}

class WrongWayIndicator final
{
public:
    void Reset() noexcept
    {
        OppositeSeconds = 0.0;
    }

    bool Update(CircuitPoint position, CircuitPoint velocityCmPerSecond,
                double deltaSeconds, bool raceEligible) noexcept
    {
        // A long hitch or pause cannot fabricate a sustained wrong-way
        // interval. Other interruptions explicitly reset from Unreal HUD.
        if (!raceEligible || !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 || deltaSeconds > 0.25 ||
            AssessCircuitTravel(position, velocityCmPerSecond) !=
                CircuitTravelAssessment::AgainstRoute)
        {
            Reset();
            return false;
        }
        OppositeSeconds = std::min(2.0, OppositeSeconds + deltaSeconds);
        return OppositeSeconds >= 0.8;
    }

    double GetOppositeSeconds() const noexcept { return OppositeSeconds; }

private:
    double OppositeSeconds = 0.0;
};
} // namespace latebrake
