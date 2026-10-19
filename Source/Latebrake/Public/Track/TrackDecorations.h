#pragma once

#include "Track/CircuitLayout.h"

namespace latebrake
{
// Layout-only descriptions for inexpensive geometry markers. Production
// builds should eventually batch these into instanced meshes in Unreal.
struct TrackMarker final
{
    CircuitPoint Center;
    double ScaleX = 0.0; // cube mesh (100 cm) scale multipliers
    double ScaleY = 0.0;
    bool Valid = false;
};

inline constexpr int MarkersPerStraight = 7;
inline constexpr int StraightCount = 4;

constexpr TrackMarker LaneStripe(int straight, int marker) noexcept
{
    if (straight < 0 || straight >= StraightCount ||
        marker < 0 || marker >= MarkersPerStraight) return {};
    if (straight == 0 || straight == 2)
    {
        return {{-2400.0 + marker * 800.0,
                 straight == 0 ? SouthY : NorthY}, 2.5, 0.1, true};
    }
    return {{straight == 1 ? EastX : WestX,
             -1500.0 + marker * 500.0}, 0.1, 2.5, true};
}

constexpr TrackMarker TrackCurb(int straight, int marker, bool outer) noexcept
{
    if (straight < 0 || straight >= StraightCount ||
        marker < 0 || marker >= MarkersPerStraight) return {};

    // Curbs stay INSIDE the paved boundary (800 cm half-width).
    const double offset = outer ? 705.0 : -705.0;
    if (straight == 0 || straight == 2)
    {
        return {{-2400.0 + marker * 800.0,
                 (straight == 0 ? SouthY : NorthY) + offset}, 4.0, 0.18, true};
    }
    return {{(straight == 1 ? EastX : WestX) + offset,
             -1500.0 + marker * 500.0}, 0.18, 3.0, true};
}

constexpr int GrandstandCount = 2;

constexpr CircuitPoint GrandstandCenter(int index) noexcept
{
    return index == 0 ? CircuitPoint{-1100.0, 3420.0}
         : index == 1 ? CircuitPoint{1100.0, -3420.0}
         : CircuitPoint{};
}

constexpr bool ValidGrandstand(int index) noexcept
{
    return index >= 0 && index < GrandstandCount;
}
} // namespace latebrake
