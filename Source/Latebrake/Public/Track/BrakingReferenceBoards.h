#pragma once

#include "Track/CircuitLayout.h"

namespace latebrake
{
// Original, asset-free braking reference furniture for the four existing
// right-angle corners. All measurements are Unreal centimetres. These
// boards are VISUAL, not turn authority, road collision or brake assist.
struct BrakingReferenceBoard final
{
    CircuitPoint Center{};
    double HeightCm = 0.0;
    double YawDegrees = 0.0;
    double ScaleX = 0.0;
    double ScaleY = 0.0;
    double ScaleZ = 0.0;
    int Corner = -1;
    int Zone = -1;             // 0/1/2 = far/middle/near
    int BarNumber = -1;
    int BarsInZone = 0;        // three, two, one
    double DistanceToGateCm = 0.0;
    bool Valid = false;
};

inline constexpr int BrakingBoardCorners = CircuitGateCount;
inline constexpr int BrakingBoardBarsPerCorner = 6;
inline constexpr int BrakingBoardMarkCount =
    BrakingBoardCorners * BrakingBoardBarsPerCorner;
inline constexpr double BrakingBoardRightOffsetCm = 580.0;

// For travel east -> north -> west -> south, the "right-hand" verge
// rotates with the car. The first far-zone group has THREE closely
// spaced vertical bars, then two, then one near the upcoming corner.
constexpr BrakingReferenceBoard PlannedBrakingReferenceBoard(int index) noexcept
{
    if (index < 0 || index >= BrakingBoardMarkCount) return {};
    const int corner = index / BrakingBoardBarsPerCorner;
    const int local = index % BrakingBoardBarsPerCorner;
    const int zone = local < 3 ? 0 : local < 5 ? 1 : 2;
    const int bar = local < 3 ? local : local < 5 ? local - 3 : 0;
    const double distance = 1200.0 - static_cast<double>(zone) * 400.0 +
        static_cast<double>(bar) * 38.0;

    // The canonical clockwise circuit sequence follows the verified gate
    // normals: east (+X), north (+Y), west (-X), south (-Y).
    const double forwardX = corner == 0 ? 1.0 : corner == 2 ? -1.0 : 0.0;
    const double forwardY = corner == 1 ? 1.0 : corner == 3 ? -1.0 : 0.0;
    const CircuitPoint gate = CircuitGate(corner);
    const CircuitPoint center{
        gate.X - forwardX * distance + forwardY * BrakingBoardRightOffsetCm,
        gate.Y - forwardY * distance - forwardX * BrakingBoardRightOffsetCm
    };

    // Cube dimensions are 100 cm. Bottom edge at 9 cm, clear of
    // asphalt top ~-1 cm; thin slabs face each approaching driver.
    return {center, 104.0, corner % 2 == 0 ? 0.0 : 90.0,
            0.15, 0.75, 1.90, corner, zone, bar, 3 - zone,
            distance, true};
}
} // namespace latebrake
