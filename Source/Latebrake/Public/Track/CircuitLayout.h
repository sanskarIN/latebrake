#pragma once

#include <array>
#include <cmath>

namespace latebrake
{
// Track distances use Unreal's centimetre units; geometry is engine-neutral.
// Centralizing these coordinates keeps player recovery, checkpoint and rival
// steering positions consistent with the visual road.
struct CircuitPoint final
{
    double X = 0.0;
    double Y = 0.0;
};

struct CircuitPose final
{
    CircuitPoint Position;
    double HeadingDegrees = 0.0;
};

inline constexpr double RoadHalfWidth = 800.0;
inline constexpr double SouthY = -1800.0;
inline constexpr double NorthY = 1800.0;
inline constexpr double EastX = 2800.0;
inline constexpr double WestX = -2800.0;
inline constexpr int CircuitGateCount = 4;

inline constexpr std::array<CircuitPoint, CircuitGateCount> CircuitGates =
{{
    { EastX, SouthY },
    { EastX, NorthY },
    { WestX, NorthY },
    { WestX, SouthY }
}};

inline constexpr CircuitPose PlayerStart = {{-2550.0, SouthY}, 0.0};
inline constexpr CircuitPose RivalStart = {{-2250.0, -1450.0}, 0.0};

inline constexpr std::array<CircuitPose, CircuitGateCount> PlayerRecoveries =
{{
    {{-2550.0, SouthY}, 0.0},
    {{EastX, -1300.0}, 90.0},
    {{2300.0, NorthY}, 180.0},
    {{WestX, 1300.0}, -90.0}
}};

inline constexpr std::array<CircuitPose, CircuitGateCount> RivalRecoveries =
{{
    {RivalStart.Position, RivalStart.HeadingDegrees},
    {{EastX, -1300.0}, 90.0},
    {{2300.0, NorthY}, 180.0},
    {{WestX, 1300.0}, -90.0}
}};

constexpr bool IsCircuitGate(int index) noexcept
{
    return index >= 0 && index < CircuitGateCount;
}

constexpr CircuitPoint CircuitGate(int index) noexcept
{
    return IsCircuitGate(index) ? CircuitGates[static_cast<unsigned>(index)]
                               : CircuitPoint{};
}

constexpr CircuitPose PlayerRecovery(int nextGate) noexcept
{
    return IsCircuitGate(nextGate)
        ? PlayerRecoveries[static_cast<unsigned>(nextGate)] : PlayerStart;
}

constexpr CircuitPose RivalRecovery(int nextGate) noexcept
{
    return IsCircuitGate(nextGate)
        ? RivalRecoveries[static_cast<unsigned>(nextGate)] : RivalStart;
}

inline bool IsOnPavedSurface(CircuitPoint point) noexcept
{
    if (!std::isfinite(point.X) || !std::isfinite(point.Y)) return false;
    const bool horizontal =
        std::abs(point.X) <= 4000.0 &&
        (std::abs(point.Y - SouthY) <= RoadHalfWidth ||
         std::abs(point.Y - NorthY) <= RoadHalfWidth);
    const bool vertical =
        std::abs(point.Y) <= 2600.0 &&
        (std::abs(point.X - EastX) <= RoadHalfWidth ||
         std::abs(point.X - WestX) <= RoadHalfWidth);
    return horizontal || vertical;
}
} // namespace latebrake
