#pragma once

#include "Track/CircuitLayout.h"
#include "Track/CircuitCollisionPlan.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Cosmetic reference line for the ONE original counter-clockwise starter circuit.
// Not a vehicle path follower, racing instruction, collision object, speed
// limit, checkpoint, official racing line, or proof of physical playability.
// Centimetre coordinates are shared with the actual road and gates.
enum class RacingLineSection { Straight, Corner };

struct RacingLineMark final
{
    CircuitPoint Center{};
    double HeadingDegrees = 0.0;
    double LengthCm = 0.0;
    RacingLineSection Section = RacingLineSection::Straight;
    bool Valid = false;
};

inline constexpr int RacingLineSectionCount = 4;
inline constexpr int RacingLineStraightMarks = 12;
inline constexpr int RacingLineCornerMarks = 10;
inline constexpr int RacingLineMarksPerSection =
    RacingLineStraightMarks + RacingLineCornerMarks;
inline constexpr int RacingLineMarkCount =
    RacingLineSectionCount * RacingLineMarksPerSection;
inline constexpr int RacingLineStraightPieceCount =
    RacingLineSectionCount * RacingLineStraightMarks;
inline constexpr int RacingLineCornerPieceCount =
    RacingLineSectionCount * RacingLineCornerMarks;
static_assert(RacingLineStraightPieceCount + RacingLineCornerPieceCount ==
              RacingLineMarkCount);
inline constexpr double RacingLineInsetCm = 250.0;
inline constexpr double RacingLineTurnRadiusCm = 650.0;
inline constexpr double RacingLinePi = 3.14159265358979323846;

// The actual four physical asphalt blocks, not the decorative marker height,
// are the single source of truth for the guide's vertical placement. Engine
// Cube geometry is 100 cm on each side. At 0.8 cm thickness and 0.25 cm
// separation, the bottom face is very close to but ABOVE the road surface.
inline constexpr double RacingLineCubeSizeCm = 100.0;
inline constexpr double RacingLinePaintWidthCm = 20.0;
inline constexpr double RacingLinePaintThicknessCm = 0.8;
inline constexpr double RacingLinePaintSurfaceGapCm = 0.25;
inline constexpr double RacingLineAsphaltTopCm =
    PlannedCollisionBlock(1).Height +
    0.5 * RacingLineCubeSizeCm * PlannedCollisionBlock(1).ScaleZ;
inline constexpr double RacingLinePaintCenterZCm =
    RacingLineAsphaltTopCm + RacingLinePaintSurfaceGapCm +
    0.5 * RacingLinePaintThicknessCm;

static_assert(PlannedCollisionBlock(1).Style == CollisionBlockStyle::Asphalt);
static_assert(PlannedCollisionBlock(2).Style == CollisionBlockStyle::Asphalt);
static_assert(PlannedCollisionBlock(3).Style == CollisionBlockStyle::Asphalt);
static_assert(PlannedCollisionBlock(4).Style == CollisionBlockStyle::Asphalt);
static_assert(
    RacingLineAsphaltTopCm ==
    PlannedCollisionBlock(2).Height +
        0.5 * RacingLineCubeSizeCm * PlannedCollisionBlock(2).ScaleZ &&
    RacingLineAsphaltTopCm ==
    PlannedCollisionBlock(3).Height +
        0.5 * RacingLineCubeSizeCm * PlannedCollisionBlock(3).ScaleZ &&
    RacingLineAsphaltTopCm ==
    PlannedCollisionBlock(4).Height +
        0.5 * RacingLineCubeSizeCm * PlannedCollisionBlock(4).ScaleZ,
    "The visual guide assumes four asphalt surfaces share one top height");
static_assert(RacingLinePaintWidthCm > 0.0 &&
              RacingLinePaintThicknessCm > 0.0 &&
              RacingLinePaintSurfaceGapCm > 0.0 &&
              RacingLinePaintSurfaceGapCm < 1.0,
              "Paint must fit the road without z-fighting or large floating gaps");

// Actual game-time geometry gate, separate from passive C++ tests.
// Every stripe is an oriented rectangle on one of the four genuine asphalt
// slab unions; checking only the centre misses future track edits that could
// put most of a piece on grass. The 5x5 grid also samples mid-edges and
// interior, including quarter-turn seams. The independent exact polygon
// union-area check below also catches narrow unsampled overhangs. Neither
// check can establish physical GPU contact or real Unreal gameplay.
inline bool IsOnPhysicalRacingLineAsphalt(CircuitPoint point) noexcept
{
    if (!std::isfinite(point.X) || !std::isfinite(point.Y)) return false;
    for (int index = 1; index <= 4; ++index)
    {
        const CircuitCollisionBlock road = PlannedCollisionBlock(index);
        if (road.Style == CollisionBlockStyle::Asphalt &&
            std::abs(point.X - road.Center.X) <=
                0.5 * RacingLineCubeSizeCm * road.ScaleX &&
            std::abs(point.Y - road.Center.Y) <=
                0.5 * RacingLineCubeSizeCm * road.ScaleY)
        {
            return true;
        }
    }
    return false;
}

// Analytic coverage of the ENTIRE rotated paint rectangle by the union
// of the four collision-bearing asphalt AABBs. A 5x5 grid by itself can
// incorrectly accept a narrow sliver crossing the inside corner of a road
// intersection. Here Sutherland-Hodgman clips the convex paint quadrilateral
// against each overlapping asphalt intersection, then inclusion-exclusion
// computes the union area. Four road slabs require only 15 small polygons.
// Fixed-size buffers: no allocation, Unreal dependency or race authority.
struct RacingLineClipPolygon final
{
    std::array<CircuitPoint, 8> Points{};
    int Count = 0;
};

inline RacingLineClipPolygon ClipRacingLineHalfPlane(
    const RacingLineClipPolygon& polygon, int axis,
    double boundary, bool keepAbove) noexcept
{
    RacingLineClipPolygon clipped;
    if (polygon.Count < 1 || polygon.Count > 8) return clipped;
    const auto coordinate = [axis](CircuitPoint p) noexcept
    {
        return axis == 0 ? p.X : p.Y;
    };
    CircuitPoint prior = polygon.Points[static_cast<unsigned>(polygon.Count - 1)];
    double priorCoordinate = coordinate(prior);
    bool priorInside = keepAbove ? priorCoordinate >= boundary :
                                  priorCoordinate <= boundary;
    for (int i = 0; i < polygon.Count; ++i)
    {
        const CircuitPoint current = polygon.Points[static_cast<unsigned>(i)];
        const double currentCoordinate = coordinate(current);
        const bool currentInside = keepAbove
            ? currentCoordinate >= boundary
            : currentCoordinate <= boundary;
        if (priorInside != currentInside)
        {
            const double difference = currentCoordinate - priorCoordinate;
            if (difference == 0.0 || clipped.Count >= 8) return {};
            const double alpha = std::clamp(
                (boundary - priorCoordinate) / difference, 0.0, 1.0);
            const CircuitPoint crossing{
                prior.X + alpha * (current.X - prior.X),
                prior.Y + alpha * (current.Y - prior.Y)
            };
            if (!std::isfinite(crossing.X) ||
                !std::isfinite(crossing.Y)) return {};
            clipped.Points[static_cast<unsigned>(clipped.Count++)] = crossing;
        }
        if (currentInside)
        {
            if (clipped.Count >= 8) return {};
            clipped.Points[static_cast<unsigned>(clipped.Count++)] = current;
        }
        prior = current;
        priorCoordinate = currentCoordinate;
        priorInside = currentInside;
    }
    return clipped;
}

inline double RacingLinePolygonAreaCm2(
    const RacingLineClipPolygon& polygon) noexcept
{
    if (polygon.Count < 3 || polygon.Count > 8) return 0.0;
    // Origin-relative triangles avoid subtracting 1e7 cm² world-coordinate
    // cross products merely to compute a narrow 20 cm stripe's area.
    const CircuitPoint origin = polygon.Points[0];
    double twiceArea = 0.0;
    for (int i = 1; i + 1 < polygon.Count; ++i)
    {
        const CircuitPoint a = polygon.Points[static_cast<unsigned>(i)];
        const CircuitPoint b = polygon.Points[static_cast<unsigned>(i + 1)];
        twiceArea += (a.X - origin.X) * (b.Y - origin.Y) -
                     (a.Y - origin.Y) * (b.X - origin.X);
    }
    return 0.5 * std::abs(twiceArea);
}

inline double RacingLineAsphaltCoveredAreaCm2(
    RacingLineMark mark) noexcept
{
    const double radians = mark.HeadingDegrees * RacingLinePi / 180.0;
    const double fx = std::cos(radians);
    const double fy = std::sin(radians);
    const double halfLength = mark.LengthCm * 0.5;
    const double halfWidth = RacingLinePaintWidthCm * 0.5;
    RacingLineClipPolygon footprint;
    footprint.Count = 4;
    const std::array<CircuitPoint, 4> offsets{{
        {-halfLength, -halfWidth}, {halfLength, -halfWidth},
        {halfLength, halfWidth}, {-halfLength, halfWidth}
    }};
    for (int i = 0; i < 4; ++i)
    {
        const CircuitPoint offset = offsets[static_cast<unsigned>(i)];
        footprint.Points[static_cast<unsigned>(i)] = {
            mark.Center.X + offset.X * fx - offset.Y * fy,
            mark.Center.Y + offset.X * fy + offset.Y * fx
        };
    }
    double covered = 0.0;
    // Intersection area of a finite union, not the sum of all four slab
    // areas: overlapping intersection corners must not count twice.
    for (int mask = 1; mask < (1 << 4); ++mask)
    {
        double left = 0.0;
        double right = 0.0;
        double bottom = 0.0;
        double top = 0.0;
        int members = 0;
        for (int index = 0; index < 4; ++index)
        {
            if ((mask & (1 << index)) == 0) continue;
            const CircuitCollisionBlock slab = PlannedCollisionBlock(index + 1);
            if (slab.Style != CollisionBlockStyle::Asphalt ||
                !IsValidCollisionBlock(slab)) return 0.0;
            const double minX =
                slab.Center.X - slab.ScaleX * RacingLineCubeSizeCm * 0.5;
            const double maxX =
                slab.Center.X + slab.ScaleX * RacingLineCubeSizeCm * 0.5;
            const double minY =
                slab.Center.Y - slab.ScaleY * RacingLineCubeSizeCm * 0.5;
            const double maxY =
                slab.Center.Y + slab.ScaleY * RacingLineCubeSizeCm * 0.5;
            if (members++ == 0)
            {
                left = minX; right = maxX;
                bottom = minY; top = maxY;
            }
            else
            {
                left = std::max(left, minX);
                right = std::min(right, maxX);
                bottom = std::max(bottom, minY);
                top = std::min(top, maxY);
            }
        }
        if (left >= right || bottom >= top) continue;
        RacingLineClipPolygon clipped =
            ClipRacingLineHalfPlane(footprint, 0, left, true);
        clipped = ClipRacingLineHalfPlane(clipped, 0, right, false);
        clipped = ClipRacingLineHalfPlane(clipped, 1, bottom, true);
        clipped = ClipRacingLineHalfPlane(clipped, 1, top, false);
        const double intersection = RacingLinePolygonAreaCm2(clipped);
        covered += (members % 2 == 1 ? intersection : -intersection);
    }
    return covered;
}

inline bool IsValidRacingLinePaintFootprint(RacingLineMark mark) noexcept
{
    if (!mark.Valid || !std::isfinite(mark.Center.X) ||
        !std::isfinite(mark.Center.Y) ||
        !std::isfinite(mark.HeadingDegrees) ||
        !std::isfinite(mark.LengthCm) ||
        mark.LengthCm <= 0.0 || mark.LengthCm > 250.0 ||
        (mark.Section != RacingLineSection::Straight &&
         mark.Section != RacingLineSection::Corner))
    {
        return false;
    }
    const double radians = mark.HeadingDegrees * RacingLinePi / 180.0;
    const double forwardX = std::cos(radians);
    const double forwardY = std::sin(radians);
    // 5 samples across both dimensions, all computed in local stripe space.
    for (int longitudinal = -2; longitudinal <= 2; ++longitudinal)
    {
        const double along = 0.25 * longitudinal * mark.LengthCm;
        for (int lateral = -2; lateral <= 2; ++lateral)
        {
            const double across =
                0.25 * lateral * RacingLinePaintWidthCm;
            const CircuitPoint sample{
                mark.Center.X + along * forwardX - across * forwardY,
                mark.Center.Y + along * forwardY + across * forwardX
            };
            if (!IsOnPhysicalRacingLineAsphalt(sample))
            {
                return false;
            }
        }
    }
    // Analytic union-area proof (to floating-point tolerance) is stronger
    // than the sampled gate: no thin corner triangle can escape detection.
    const double fullArea = mark.LengthCm * RacingLinePaintWidthCm;
    const double covered = RacingLineAsphaltCoveredAreaCm2(mark);
    const double tolerance = std::max(1.0e-5, fullArea * 1.0e-8);
    return std::isfinite(covered) &&
           std::abs(fullArea - covered) <= tolerance;
}

// Counter-clockwise lap: south eastbound, east northbound, north westbound,
// west southbound. A turn follows each straight. The line sits 250 cm
// towards the infield so it does not cover the existing road-centre dashes.
inline RacingLineMark PlannedRacingLineMark(int index) noexcept
{
    if (index < 0 || index >= RacingLineMarkCount) return {};
    const int side = index / RacingLineMarksPerSection;
    const int within = index % RacingLineMarksPerSection;
    const double south = SouthY + RacingLineInsetCm;
    const double north = NorthY - RacingLineInsetCm;
    const double east = EastX - RacingLineInsetCm;
    const double west = WestX + RacingLineInsetCm;
    const double radius = RacingLineTurnRadiusCm;

    if (within < RacingLineStraightMarks)
    {
        const double fraction =
            (static_cast<double>(within) + 0.5) /
            static_cast<double>(RacingLineStraightMarks);
        const double horizontalStart = west + radius;
        const double horizontalEnd = east - radius;
        const double verticalStart = south + radius;
        const double verticalEnd = north - radius;
        const double horizontalSpan = horizontalEnd - horizontalStart;
        const double verticalSpan = verticalEnd - verticalStart;
        const double length =
            (side % 2 == 0 ? horizontalSpan : verticalSpan) /
            RacingLineStraightMarks * 0.62;
        switch (side)
        {
        case 0: return {{horizontalStart + horizontalSpan * fraction, south},
                        0.0, length, RacingLineSection::Straight, true};
        case 1: return {{east, verticalStart + verticalSpan * fraction},
                        90.0, length, RacingLineSection::Straight, true};
        case 2: return {{horizontalEnd - horizontalSpan * fraction, north},
                        180.0, length, RacingLineSection::Straight, true};
        default: return {{west, verticalEnd - verticalSpan * fraction},
                         270.0, length, RacingLineSection::Straight, true};
        }
    }

    // Rounded quarter turns are rendered by short tangential dashes. No
    // interpolation crosses a wrong-way gate or modifies physics surfaces.
    const int turnMark = within - RacingLineStraightMarks;
    const double sweepDegrees =
        -90.0 + side * 90.0 +
        (static_cast<double>(turnMark) + 0.5) *
        (90.0 / RacingLineCornerMarks);
    const double angle = sweepDegrees * RacingLinePi / 180.0;
    const CircuitPoint center =
        side == 0 ? CircuitPoint{east - radius, south + radius} :
        side == 1 ? CircuitPoint{east - radius, north - radius} :
        side == 2 ? CircuitPoint{west + radius, north - radius} :
                    CircuitPoint{west + radius, south + radius};
    const double chord = 2.0 * radius *
        std::sin((RacingLinePi / 2.0) / (2.0 * RacingLineCornerMarks));
    return {{center.X + radius * std::cos(angle),
             center.Y + radius * std::sin(angle)},
            sweepDegrees + 90.0, chord * 0.64,
            RacingLineSection::Corner, true};
}
} // namespace latebrake
