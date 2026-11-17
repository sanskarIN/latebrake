#pragma once

#include "Track/CircuitLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace latebrake
{
// EXPERIMENTAL SECOND CIRCUIT; source model only. It does not replace the
// nine collision-bearing actors in the current game and is not road art.
// Every placement describes one genuine (future) WorldStatic collision cube,
// not a non-colliding cosmetic stripe. Length is LOCAL X; width is LOCAL Y.
// Unreal Cube has 100-cm axes. All pieces share the existing road top z=-1.
inline constexpr double RoundedArcRadiusCm = 800.0;
inline constexpr double RoundedHalfRoadWidthCm = 450.0;
inline constexpr double RoundedHorizontalTangentCm = EastX - RoundedArcRadiusCm;
inline constexpr double RoundedVerticalTangentCm = NorthY - RoundedArcRadiusCm;
inline constexpr int RoundedCornerSegmentCount = 16;
inline constexpr int RoundedPhysicalRoadCount =
    4 + 4 * RoundedCornerSegmentCount;
inline constexpr double RoundedRoadTopCm = -1.0;
inline constexpr double RoundedRoadCenterHeightCm = -22.0;
inline constexpr double RoundedRoadDepthScale = 0.42;
inline constexpr double RoundedPi = std::numbers::pi_v<double>;
inline constexpr double RoundedHalfPi = RoundedPi / 2.0;
inline constexpr double RoundedStraightOverlapCm = 60.0;
inline constexpr double RoundedArcOverlapCm = 20.0;
inline constexpr double RoundedSouthNorthStraightCm = 2.0 * RoundedHorizontalTangentCm;
inline constexpr double RoundedEastWestStraightCm = 2.0 * RoundedVerticalTangentCm;
inline constexpr double RoundedQuarterArcCm = RoundedHalfPi * RoundedArcRadiusCm;
inline constexpr double RoundedLapLengthCm =
    2.0 * RoundedSouthNorthStraightCm +
    2.0 * RoundedEastWestStraightCm + 4.0 * RoundedQuarterArcCm;
static_assert(RoundedPhysicalRoadCount == 68);
static_assert(RoundedVerticalTangentCm > 0.0);

struct RoundedPhysicalRoadPiece final
{
    CircuitPoint Center{};
    double HeightCm = 0.0;
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.0;
    double YawDegrees = 0.0;
    bool Valid = false;
};

struct RoundedCenterlinePose final
{
    CircuitPose Pose{};
    bool Valid = false;
};

inline constexpr bool IsValidRoundedRoadIndex(int index) noexcept
{
    return index >= 0 && index < RoundedPhysicalRoadCount;
}

inline RoundedPhysicalRoadPiece PlannedRoundedPhysicalRoadPiece(
    int index) noexcept
{
    if (!IsValidRoundedRoadIndex(index)) return {};
    // Four tangential straights; 64 short, overlapping, oriented curve
    // collision slabs. Therlapping covers the small outside-edge wedges
    // between adjacent tangent boxes at a 5.625-degree heading change.
    if (index < 4)
    {
        const double horizontalLength =
            (RoundedSouthNorthStraightCm + 2.0 * RoundedStraightOverlapCm)
            / 100.0;
        const double verticalLength =
            (RoundedEastWestStraightCm + 2.0 * RoundedStraightOverlapCm)
            / 100.0;
        switch (index)
        {
        case 0: return {{0.0, SouthY}, RoundedRoadCenterHeightCm,
                        horizontalLength, 2.0 * RoundedHalfRoadWidthCm / 100.0,
                        RoundedRoadDepthScale, 0.0, true};
        case 1: return {{EastX, 0.0}, RoundedRoadCenterHeightCm,
                        verticalLength, 2.0 * RoundedHalfRoadWidthCm / 100.0,
                        RoundedRoadDepthScale, 90.0, true};
        case 2: return {{0.0, NorthY}, RoundedRoadCenterHeightCm,
                        horizontalLength, 2.0 * RoundedHalfRoadWidthCm / 100.0,
                        RoundedRoadDepthScale, 180.0, true};
        default: return {{WestX, 0.0}, RoundedRoadCenterHeightCm,
                         verticalLength, 2.0 * RoundedHalfRoadWidthCm / 100.0,
                         RoundedRoadDepthScale, 270.0, true};
        }
    }
    constexpr std::array<CircuitPoint, 4> cornerCenters{{
        {RoundedHorizontalTangentCm, -RoundedVerticalTangentCm},
        {RoundedHorizontalTangentCm, RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm, RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm, -RoundedVerticalTangentCm}
    }};
    const int corner = (index - 4) / RoundedCornerSegmentCount;
    const int segment = (index - 4) % RoundedCornerSegmentCount;
    const double step = RoundedHalfPi / RoundedCornerSegmentCount;
    const double angle = -RoundedHalfPi + corner * RoundedHalfPi +
                         (segment + 0.5) * step;
    const double halfChord = RoundedArcRadiusCm * std::sin(step * 0.5);
    const double halfOutsideEdgeWedge =
        RoundedHalfRoadWidthCm * std::sin(step * 0.5);
    const double lengthCm = 2.0 * (halfChord + halfOutsideEdgeWedge +
                                   RoundedArcOverlapCm);
    const auto& center = cornerCenters[static_cast<unsigned>(corner)];
    return {{center.X + RoundedArcRadiusCm * std::cos(angle),
             center.Y + RoundedArcRadiusCm * std::sin(angle)},
            RoundedRoadCenterHeightCm,
            lengthCm / 100.0,
            2.0 * RoundedHalfRoadWidthCm / 100.0,
            RoundedRoadDepthScale,
            (angle + RoundedHalfPi) * 180.0 / RoundedPi,
            true};
}

inline bool IsValidRoundedPhysicalRoadPiece(
    RoundedPhysicalRoadPiece piece) noexcept
{
    return piece.Valid &&
           std::isfinite(piece.Center.X) &&
           std::isfinite(piece.Center.Y) &&
           std::isfinite(piece.HeightCm) &&
           std::isfinite(piece.ScaleX) &&
           std::isfinite(piece.ScaleY) &&
           std::isfinite(piece.ScaleZ) &&
           std::isfinite(piece.YawDegrees) &&
           piece.ScaleX > 0.0 && piece.ScaleY > 0.0 &&
           piece.ScaleZ > 0.0 &&
           std::abs(piece.YawDegrees) <= 360.0 &&
           std::abs(piece.HeightCm + 50.0 * piece.ScaleZ -
                    RoundedRoadTopCm) < 1e-9;
}

inline bool ContainsRoundedPhysicalRoadPoint(
    RoundedPhysicalRoadPiece piece, CircuitPoint point) noexcept
{
    if (!IsValidRoundedPhysicalRoadPiece(piece) ||
        !std::isfinite(point.X) || !std::isfinite(point.Y)) return false;
    const double angle = piece.YawDegrees * RoundedPi / 180.0;
    const double dx = point.X - piece.Center.X;
    const double dy = point.Y - piece.Center.Y;
    const double localX = dx * std::cos(angle) + dy * std::sin(angle);
    const double localY = -dx * std::sin(angle) + dy * std::cos(angle);
    return std::abs(localX) <= 50.0 * piece.ScaleX &&
           std::abs(localY) <= 50.0 * piece.ScaleY;
}

// AUTHORITATIVE for this candidate's union of rotated physical rectangles;
// never reuse the old rectangular IsOnPavedSurface() to award curved-road
// grip. This is a candidate API, NOT connected to the live player pawn.
inline bool IsOnRoundedPhysicalRoad(CircuitPoint point) noexcept
{
    if (!std::isfinite(point.X) || !std::isfinite(point.Y)) return false;
    for (int index = 0; index < RoundedPhysicalRoadCount; ++index)
    {
        if (ContainsRoundedPhysicalRoadPoint(
                PlannedRoundedPhysicalRoadPiece(index), point)) return true;
    }
    return false;
}

// Arc-length parameterized, forward racing-line reference. A verified
// future rival/mini-map must use this together with the physical unions.
// Repeated laps and negative offsets wrap; malformed/nonfinite input fails.
inline RoundedCenterlinePose RoundedRoadCenterlineAt(
    double distanceCm) noexcept
{
    if (!std::isfinite(distanceCm)) return {};
    double s = std::fmod(distanceCm, RoundedLapLengthCm);
    if (s < 0.0) s += RoundedLapLengthCm;

    constexpr std::array<CircuitPoint, 4> centers{{
        {RoundedHorizontalTangentCm, -RoundedVerticalTangentCm},
        {RoundedHorizontalTangentCm, RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm, RoundedVerticalTangentCm},
        {-RoundedHorizontalTangentCm, -RoundedVerticalTangentCm}
    }};
    for (int section = 0; section < 8; ++section)
    {
        if (section % 2 == 0)
        {
            const int straight = section / 2;
            const double length = straight % 2 == 0 ?
                RoundedSouthNorthStraightCm : RoundedEastWestStraightCm;
            if (s < length)
            {
                switch (straight)
                {
                case 0: return {{{-RoundedHorizontalTangentCm + s, SouthY},
                                  0.0}, true};
                case 1: return {{{EastX, -RoundedVerticalTangentCm + s},
                                  90.0}, true};
                case 2: return {{{RoundedHorizontalTangentCm - s, NorthY},
                                  180.0}, true};
                default: return {{{WestX, RoundedVerticalTangentCm - s},
                                   270.0}, true};
                }
            }
            s -= length;
        }
        else
        {
            const int corner = section / 2;
            if (s < RoundedQuarterArcCm || section == 7)
            {
                const double angle = -RoundedHalfPi + corner * RoundedHalfPi
                    + s / RoundedArcRadiusCm;
                const auto& center = centers[static_cast<unsigned>(corner)];
                return {{{center.X + RoundedArcRadiusCm * std::cos(angle),
                          center.Y + RoundedArcRadiusCm * std::sin(angle)},
                         (angle + RoundedHalfPi) * 180.0 / RoundedPi}, true};
            }
            s -= RoundedQuarterArcCm;
        }
    }
    return {};
}
} // namespace latebrake
