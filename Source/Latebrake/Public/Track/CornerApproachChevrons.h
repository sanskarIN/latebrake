#pragma once

#include "Track/CircuitLayout.h"

#include <array>

namespace latebrake
{
// Original, asset-free painted chevrons near the inside edge of each clockwise
// approach. They are directional decoration, NOT a line to drive, a gate,
// collision geometry, braking instruction, shortcut or scoring authority.
// Units are Unreal centimeters; transforms use the stock 100 cm cube.
struct CornerApproachChevron final
{
    CircuitPoint Center{};
    double HeightCm = 0.0;
    double YawDegrees = 0.0;
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.0;
    int Corner = -1;
    int Row = -1;        // 0 far, 1 middle, 2 near
    int Arm = -1;        // two diagonal strokes meet at the forward tip
    double BeforeGateCm = 0.0;
    bool Valid = false;
};

inline constexpr int ChevronCorners = CircuitGateCount;
inline constexpr int ChevronRowsPerCorner = 3;
inline constexpr int ChevronArmsPerRow = 2;
inline constexpr int CornerChevronPieceCount =
    ChevronCorners * ChevronRowsPerCorner * ChevronArmsPerRow;
inline constexpr double ChevronLeftOffsetCm = 430.0;
inline constexpr std::array<double, ChevronRowsPerCorner> ChevronApproachesCm{{
    1600.0, 1100.0, 600.0
}};

// A readable forward-pointing V made from two shallow stock-cube stripes.
// Both arms are on the LEFT-side paved shoulder, safely opposite the
// right-verge braking boards. The two stroke tips nearly meet.
constexpr CornerApproachChevron PlannedCornerApproachChevron(
    int index) noexcept
{
    if (index < 0 || index >= CornerChevronPieceCount) return {};

    const int corner = index / (ChevronRowsPerCorner * ChevronArmsPerRow);
    const int row = (index / ChevronArmsPerRow) % ChevronRowsPerCorner;
    const int arm = index % ChevronArmsPerRow;
    const double forwardX = corner == 0 ? 1.0 : corner == 2 ? -1.0 : 0.0;
    const double forwardY = corner == 1 ? 1.0 : corner == 3 ? -1.0 : 0.0;
    const double leftX = -forwardY;
    const double leftY = forwardX;
    const double side = arm == 0 ? 1.0 : -1.0;
    const double distance =
        ChevronApproachesCm[static_cast<unsigned>(row)];
    const CircuitPoint gate = CircuitGate(corner);

    // 190cm diagonal bars: ±32-degree headings and the 80cm backward /
    // 51cm side offset give a visibly pointed pair without a complex mesh.
    const CircuitPoint center{
        gate.X - forwardX * (distance + 80.0) +
            leftX * (ChevronLeftOffsetCm + side * 51.0),
        gate.Y - forwardY * (distance + 80.0) +
            leftY * (ChevronLeftOffsetCm + side * 51.0)
    };
    const double heading = corner == 0 ? 0.0 : corner == 1 ? 90.0 :
                           corner == 2 ? 180.0 : -90.0;
    return {center, 1.5, heading + (arm == 0 ? -32.0 : 32.0),
            1.90, 0.19, 0.03, corner, row, arm, distance, true};
}
} // namespace latebrake
