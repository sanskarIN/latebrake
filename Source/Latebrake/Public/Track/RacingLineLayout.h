#pragma once

#include "Track/CircuitLayout.h"
#include "Track/CircuitCollisionPlan.h"

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
// interior, including quarter-turn seams. This is a conservative sampled
// policy, NOT a proof of continuous polygon coverage or GPU contact.
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
    return true;
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
