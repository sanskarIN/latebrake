#pragma once

#include "Track/CircuitLayout.h"

#include <cmath>

namespace latebrake
{
// Cosmetic reference line for the ONE original clockwise starter circuit.
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
inline constexpr double RacingLineInsetCm = 250.0;
inline constexpr double RacingLineTurnRadiusCm = 650.0;
inline constexpr double RacingLinePi = 3.14159265358979323846;

// Clockwise lap: south eastbound, east northbound, north westbound,
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
