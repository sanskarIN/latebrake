#pragma once

#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Experimental portable geometry search, not wired to Unreal Tick, lap
// credit, AI controls or live physics. This has no authority over races.
// Each bound encloses 16 complete source segments, including the closing
// segment that joins node 511 back to node 0.
inline constexpr unsigned WorldScaleProjectionBlockWidth = 16;
static_assert(WorldScalePaceStations %
              WorldScaleProjectionBlockWidth == 0);
inline constexpr unsigned WorldScaleProjectionBlockCount =
    WorldScalePaceStations / WorldScaleProjectionBlockWidth;

struct WorldScaleProjectionBlockBounds final
{
    double MinX = 0.0;
    double MinY = 0.0;
    double MaxX = 0.0;
    double MaxY = 0.0;
};

struct WorldScaleProjectionBlockIndex final
{
    bool Valid = false;
    // The built index is tied to ONE stable pace-plan object. Never mutate,
    // replace, free or move that plan after building an index for it.
    const WorldScalePacePlan* Source = nullptr;
    std::array<WorldScaleProjectionBlockBounds,
               WorldScaleProjectionBlockCount> Blocks{};
};

struct WorldScaleProjectionBlockStats final
{
    unsigned TestedSegments = 0;
    unsigned SkippedBlocks = 0;
};

// Prevalidate every original segment at construction, so skipping a distant
// block cannot conceal a malformed source segment that the full scan rejects.
// This is a source-only immutable geometry index; not a runtime cache manager.
inline WorldScaleProjectionBlockIndex BuildWorldScaleProjectionBlockIndex(
    const WorldScalePacePlan& plan) noexcept
{
    if (!plan.Valid) return {};
    WorldScaleProjectionBlockIndex index{};
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const auto a = plan.Nodes[i].Position;
        const auto b = plan.Nodes[(i + 1) % WorldScalePaceStations].Position;
        const double dx = b.X - a.X;
        const double dy = b.Y - a.Y;
        const double norm2 = dx * dx + dy * dy;
        if (!std::isfinite(a.X) || !std::isfinite(a.Y) ||
            !std::isfinite(b.X) || !std::isfinite(b.Y) ||
            !std::isfinite(norm2) || norm2 <= 0.0)
        {
            return {};
        }
        auto& bounds = index.Blocks[i / WorldScaleProjectionBlockWidth];
        if (i % WorldScaleProjectionBlockWidth == 0)
        {
            bounds = {std::min(a.X, b.X), std::min(a.Y, b.Y),
                      std::max(a.X, b.X), std::max(a.Y, b.Y)};
        }
        else
        {
            bounds.MinX = std::min({bounds.MinX, a.X, b.X});
            bounds.MinY = std::min({bounds.MinY, a.Y, b.Y});
            bounds.MaxX = std::max({bounds.MaxX, a.X, b.X});
            bounds.MaxY = std::max({bounds.MaxY, a.Y, b.Y});
        }
    }
    index.Source = &plan;
    index.Valid = true;
    return index;
}

inline double SquaredWorldScaleBlockLowerBound(
    const WorldScaleProjectionBlockBounds& bounds,
    CircuitPoint car) noexcept
{
    const double dx =
        std::max({bounds.MinX - car.X, 0.0, car.X - bounds.MaxX});
    const double dy =
        std::max({bounds.MinY - car.Y, 0.0, car.Y - bounds.MaxY});
    return dx * dx + dy * dy;
}

// Same ordered segment evaluation and same projection post-processing as
// the trusted O(512) reference. Only skip a 16-segment block if a
// conservative squared-distance lower bound is STRICTLY worse than
// the current measured best, with a small floating-point safety margin.
// Iterating blocks in increasing order retains the reference's first
// lowest-index winner when two physical segments are equally close.
inline WorldScaleRouteProjection ProjectWorldScaleRouteBlocks(
    const WorldScalePacePlan& plan,
    const WorldScaleProjectionBlockIndex& index,
    CircuitPoint car,
    WorldScaleProjectionBlockStats* stats = nullptr) noexcept
{
    if (stats != nullptr) *stats = {};
    if (!index.Valid || index.Source != &plan || !plan.Valid ||
        !std::isfinite(car.X) || !std::isfinite(car.Y) ||
        std::abs(car.X) > 1000000.0 || std::abs(car.Y) > 1000000.0)
    {
        return {};
    }
    double least = std::numeric_limits<double>::infinity();
    unsigned bestIndex = 0;
    double fraction = 0.0;
    double signedLateral = 0.0;
    for (unsigned block = 0; block < WorldScaleProjectionBlockCount; ++block)
    {
        const double lower = SquaredWorldScaleBlockLowerBound(
            index.Blocks[block], car);
        if (!std::isfinite(lower)) return {};
        // Lower bound is nonnegative and all source endpoints are finite.
        // The slack helps guard against rounding at exact equidistance.
        const double slack = std::max(1.0e-6, least * 1.0e-12);
        if (std::isfinite(least) && lower > least + slack)
        {
            if (stats != nullptr) ++stats->SkippedBlocks;
            continue;
        }
        const unsigned start = block * WorldScaleProjectionBlockWidth;
        for (unsigned i = start;
             i < start + WorldScaleProjectionBlockWidth; ++i)
        {
            if (stats != nullptr) ++stats->TestedSegments;
            const auto a = plan.Nodes[i].Position;
            const auto b = plan.Nodes[(i + 1) % WorldScalePaceStations].Position;
            const double dx = b.X - a.X;
            const double dy = b.Y - a.Y;
            const double norm2 = dx * dx + dy * dy;
            // Prevalidated during index construction; retain the same
            // fail-closed behavior if an illegal mutation is discovered.
            if (!std::isfinite(norm2) || norm2 <= 0.0) return {};
            const double u = std::clamp(
                ((car.X - a.X) * dx + (car.Y - a.Y) * dy) / norm2,
                0.0, 1.0);
            const double ex = car.X - (a.X + u * dx);
            const double ey = car.Y - (a.Y + u * dy);
            const double d2 = ex * ex + ey * ey;
            if (!std::isfinite(d2)) return {};
            if (d2 < least)
            {
                least = d2;
                bestIndex = i;
                fraction = u;
                signedLateral =
                    (dx * (car.Y - a.Y) - dy * (car.X - a.X)) /
                    std::sqrt(norm2);
            }
        }
    }
    if (!std::isfinite(least) ||
        least > WorldScaleProjectionMaxDistanceCm *
                WorldScaleProjectionMaxDistanceCm)
    {
        return {};
    }
    double along = (bestIndex + fraction) * WorldScalePaceStepCm;
    if (along >= WorldScaleCircuitLapCm) along -= WorldScaleCircuitLapCm;
    const auto reference = SampleWorldScalePace(plan, along);
    if (!reference.Valid || !std::isfinite(signedLateral)) return {};
    return {true, along, signedLateral, std::sqrt(least),
            IsOnWorldScaleRoad(car), reference.TargetSpeedCmps};
}
} // namespace latebrake
