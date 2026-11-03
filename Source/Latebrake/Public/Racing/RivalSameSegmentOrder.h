#pragma once

#include "Racing/RivalGateGap.h"
#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
// Optional two-car course-position hint, not an official race standing.
// Only compares physical along-road progress when both drivers have the
// SAME independently accepted gate ordinal and occupy the SAME straight.
enum class RivalSegmentLeader
{
    Unavailable,
    Player,
    Rival,
    TooClose
};

struct RivalSameSegmentSample final
{
    RivalGateGap CreditedGates;
    int NextGate = -1;
    CircuitPoint PlayerPosition;
    CircuitPoint RivalPosition;
    bool RaceActive = false;
};

struct RivalSameSegmentOrder final
{
    bool Valid = false;
    RivalSegmentLeader Leader = RivalSegmentLeader::Unavailable;
    double AlongRoadSeparationMeters = 0.0;
};

inline RivalSameSegmentOrder CompareRivalSameSegment(
    const RivalSameSegmentSample& sample) noexcept
{
    // Credited gate ordinals remain the ONLY proof of progress. Coordinates
    // merely resolve an otherwise ambiguous same-gate comparison, never
    // replace the race clock, gates, or scoring authority.
    if (!sample.RaceActive || !sample.CreditedGates.Valid ||
        sample.CreditedGates.Kind != RivalGateGapKind::SameCreditedGateCount ||
        sample.CreditedGates.CreditedGateDelta != 0 ||
        sample.CreditedGates.PlayerCreditedGates < 0 ||
        sample.CreditedGates.PlayerCreditedGates !=
            sample.CreditedGates.RivalCreditedGates ||
        !IsCircuitGate(sample.NextGate) ||
        sample.CreditedGates.PlayerCreditedGates % CircuitGateCount !=
            sample.NextGate ||
        !IsOnPavedSurface(sample.PlayerPosition) ||
        !IsOnPavedSurface(sample.RivalPosition))
    {
        return {};
    }

    const CircuitPoint from = CircuitGate(
        (sample.NextGate + CircuitGateCount - 1) % CircuitGateCount);
    const CircuitPoint to = CircuitGate(sample.NextGate);
    const double dx = to.X - from.X;
    const double dy = to.Y - from.Y;
    const double lengthSq = dx * dx + dy * dy;
    if (!std::isfinite(lengthSq) || lengthSq <= 0.0)
    {
        return {};
    }
    const double length = std::sqrt(lengthSq);
    const auto progress = [=](CircuitPoint p) noexcept {
        const double x = p.X - from.X;
        const double y = p.Y - from.Y;
        const double along = (x * dx + y * dy) / length;
        const double lateral = std::abs(x * dy - y * dx) / length;
        // Near turns or outside the central lane, the straight projection
        // no longer has an unambiguous physical race-order meaning.
        if (!std::isfinite(along) || !std::isfinite(lateral) ||
            along < length * 0.035 || along > length * 0.965 ||
            lateral > 500.0)
        {
            return -1.0;
        }
        return along;
    };
    const double player = progress(sample.PlayerPosition);
    const double rival = progress(sample.RivalPosition);
    if (player < 0.0 || rival < 0.0)
    {
        return {};
    }
    const double deltaMeters = (player - rival) / 100.0;
    if (!std::isfinite(deltaMeters))
    {
        return {};
    }
    if (std::abs(deltaMeters) < 3.0)
    {
        return {true, RivalSegmentLeader::TooClose, 0.0};
    }
    return {true,
        deltaMeters > 0.0 ? RivalSegmentLeader::Player
                          : RivalSegmentLeader::Rival,
        std::abs(deltaMeters)};
}
} // namespace latebrake
