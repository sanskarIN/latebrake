#pragma once

#include "Track/CircuitCollisionPlan.h"

#include <array>
#include <cmath>

namespace latebrake
{
// Non-colliding details for the ACTUAL nine-block starter circuit, not a
// fabricated alternative road. A 100 cm Unreal Cube uses these world scales.
// White edge paint covers unambiguous straight portions only; no fictitious
// curved lane is drawn across the rectangular intersection joins.
enum class CircuitSurfaceStyle
{
    EdgePaint = 0,
    Rubber,
    Count
};

struct CircuitSurfaceDetail final
{
    CircuitPoint Center{};
    double Height = 0.15; // 2.2 cm slab bottoms at -0.95, asphalt top=-1
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.022;
    CircuitSurfaceStyle Style = CircuitSurfaceStyle::EdgePaint;
    bool Valid = false;
};

inline constexpr int CircuitSurfaceEdgeCount = 8;
inline constexpr int CircuitRubberCornerCount = CircuitGateCount;
inline constexpr int CircuitRubberTracksPerCorner = 2;
inline constexpr int CircuitRubberPiecesPerTrack = 6;
inline constexpr int CircuitRubberCount =
    CircuitRubberCornerCount * CircuitRubberTracksPerCorner *
    CircuitRubberPiecesPerTrack;
inline constexpr int CircuitSurfaceDetailCount =
    CircuitSurfaceEdgeCount + CircuitRubberCount;
static_assert(CircuitSurfaceDetailCount == 56);
inline constexpr int CircuitSurfaceStyleCount =
    static_cast<int>(CircuitSurfaceStyle::Count);

constexpr int PlannedSurfaceStyleCount(CircuitSurfaceStyle style) noexcept
{
    return style == CircuitSurfaceStyle::EdgePaint ? CircuitSurfaceEdgeCount
        : style == CircuitSurfaceStyle::Rubber ? CircuitRubberCount : 0;
}

constexpr CircuitSurfaceDetail PlannedCircuitSurfaceDetail(int index) noexcept
{
    if (index < 0 || index >= CircuitSurfaceDetailCount) return {};
    if (index < CircuitSurfaceEdgeCount)
    {
        const int road = index / 2; // south, north, east, west
        const double side = index % 2 ? 1.0 : -1.0;
        constexpr double offset = RoadHalfWidth - 60.0; // 60 cm inset
        if (road < 2)
        {
            return {{0.0, (road == 0 ? SouthY : NorthY) + side * offset},
                    0.15, 35.0, 0.12, 0.022,
                    CircuitSurfaceStyle::EdgePaint, true};
        }
        return {{(road == 2 ? EastX : WestX) + side * offset, 0.0},
                0.15, 0.12, 16.0, 0.022,
                CircuitSurfaceStyle::EdgePaint, true};
    }

    const int rubber = index - CircuitSurfaceEdgeCount;
    const int corner = rubber / (CircuitRubberTracksPerCorner *
                                CircuitRubberPiecesPerTrack);
    const int track = (rubber / CircuitRubberPiecesPerTrack) % 2;
    const int section = rubber % CircuitRubberPiecesPerTrack;
    // Two narrow and finite tire paths before each of the four actual
    // directional gates, never at a gate or across the grass interior.
    const double beforeGate = 600.0 + section * 190.0;
    const double trackOffset = track == 0 ? -105.0 : 105.0;
    const CircuitPoint gate = CircuitGate(corner);
    if (corner == 0 || corner == 2)
    {
        const double forward = corner == 0 ? 1.0 : -1.0;
        return {{gate.X - forward * beforeGate, gate.Y + trackOffset},
                0.15, 1.35, 0.13, 0.022,
                CircuitSurfaceStyle::Rubber, true};
    }
    const double forward = corner == 1 ? 1.0 : -1.0;
    return {{gate.X + trackOffset, gate.Y - forward * beforeGate},
            0.15, 0.13, 1.35, 0.022,
            CircuitSurfaceStyle::Rubber, true};
}

// The entire rectangle must fit ONE of the actual four physical asphalt
// collision slabs. Merely checking its centre (or a finite set of samples)
// is insufficient: even a thin sliver of decorative paint over grass could
// imply a road that the car cannot physically drive on.
inline bool IsSurfaceDetailOnPhysicalAsphalt(
    CircuitSurfaceDetail detail) noexcept
{
    if (!detail.Valid ||
        !std::isfinite(detail.Center.X) ||
        !std::isfinite(detail.Center.Y) ||
        !std::isfinite(detail.Height) ||
        !std::isfinite(detail.ScaleX) ||
        !std::isfinite(detail.ScaleY) ||
        !std::isfinite(detail.ScaleZ) ||
        detail.ScaleX <= 0.0 || detail.ScaleY <= 0.0 ||
        detail.ScaleZ <= 0.0 ||
        (detail.Style != CircuitSurfaceStyle::EdgePaint &&
         detail.Style != CircuitSurfaceStyle::Rubber))
    {
        return false;
    }
    // The shared stock cube is 100 cm wide. Paint must never intersect
    // actual rigid-body asphalt (top = -1 cm) or hover above it. Keep a
    // 2 cm high thin strip with a 0.05 cm contact tolerance.
    const double bottom = detail.Height - 50.0 * detail.ScaleZ;
    if (bottom < -1.05 || bottom > -0.90) return false;
    const double dx = 50.0 * detail.ScaleX;
    const double dy = 50.0 * detail.ScaleY;
    for (int i = 1; i <= 4; ++i)
    {
        const auto slab = PlannedCollisionBlock(i);
        if (slab.Style != CollisionBlockStyle::Asphalt) return false;
        if (std::abs(detail.Center.X - slab.Center.X) + dx <=
                50.0 * slab.ScaleX &&
            std::abs(detail.Center.Y - slab.Center.Y) + dy <=
                50.0 * slab.ScaleY)
        {
            return true;
        }
    }
    return false;
}
} // namespace latebrake
