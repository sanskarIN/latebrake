#pragma once

#include "Racing/RacePitService.h"

#include <array>

namespace latebrake
{
// Eight separate non-colliding asphalt paint dashes mark the actual
// physical PIT service area. The band is intentionally INSIDE the 250 cm
// pit radius so it does not visually promise service outside the bay.
// World positions are computed from RacePitBay rather than a second
// hand-authored landmark. These are decorative cubes, not collision zones.
struct PitBayPaintMark final
{
    CircuitPoint Center{};
    double Height = 3.0;
    double YawDegrees = 0.0;
    double ScaleX = 0.70;
    double ScaleY = 0.10;
    double ScaleZ = 0.025;
    bool Valid = false;
};

inline constexpr std::array<CircuitPoint, 8> PitBayMarkOffsets{{
    {190.0, 0.0}, {134.0, 134.0}, {0.0, 190.0},
    {-134.0, 134.0}, {-190.0, 0.0}, {-134.0, -134.0},
    {0.0, -190.0}, {134.0, -134.0}
}};
inline constexpr std::array<double, 8> PitBayMarkYaw{{
    90.0, 135.0, 0.0, -135.0,
    90.0, -45.0, 0.0, 45.0
}};
inline constexpr int PitBayPaintMarkCount =
    static_cast<int>(PitBayMarkOffsets.size());
static_assert(PitBayPaintMarkCount == 8,
              "Review visible source-authored service bay pattern");

constexpr PitBayPaintMark PlannedPitBayPaintMark(int index) noexcept
{
    if (index < 0 || index >= PitBayPaintMarkCount) return {};
    const auto offset = PitBayMarkOffsets[static_cast<unsigned>(index)];
    return {{RacePitBay.X + offset.X, RacePitBay.Y + offset.Y},
            3.0, PitBayMarkYaw[static_cast<unsigned>(index)],
            0.70, 0.10, 0.025, true};
}
} // namespace latebrake
