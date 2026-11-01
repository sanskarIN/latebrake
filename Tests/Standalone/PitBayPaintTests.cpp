#include "Track/PitBayPaintLayout.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
}

int main()
{
    using latebrake::PitBayPaintMarkCount;
    using latebrake::PlannedPitBayPaintMark;
    using latebrake::RacePitBay;
    Check(PitBayPaintMarkCount == 8, "eight small ring dashes");
    Check(!PlannedPitBayPaintMark(-1).Valid &&
          !PlannedPitBayPaintMark(8).Valid &&
          !PlannedPitBayPaintMark(100000).Valid,
          "out-of-range marks never appear");
    const auto empty = PlannedPitBayPaintMark(-1);
    Check(empty.Center.X == 0.0 && empty.Center.Y == 0.0,
          "malformed index has no pit-relative coordinates");

    for (int i = 0; i < PitBayPaintMarkCount; ++i)
    {
        const auto mark = PlannedPitBayPaintMark(i);
        Check(mark.Valid, "all marked bay dashes are present");
        Check(std::isfinite(mark.Center.X) &&
              std::isfinite(mark.Center.Y) &&
              std::isfinite(mark.YawDegrees) &&
              std::isfinite(mark.Height),
              "authored marker geometry is finite");
        Check(latebrake::IsOnPavedSurface(mark.Center) &&
              latebrake::IsInRacePitBay(mark.Center.X, mark.Center.Y),
              "each dash is on actual paved serviceable bay");
        const double dx = mark.Center.X - RacePitBay.X;
        const double dy = mark.Center.Y - RacePitBay.Y;
        const double d = std::sqrt(dx * dx + dy * dy);
        // The 70cm strip's outer tip must still be INSIDE the physical
        // 250cm service radius, including rounding at diagonal marks.
        Check(d > 180.0 && d + mark.ScaleX * 50.0 <
              latebrake::RacePitBayRadiusCm,
              "entire painted strip fits inside the real pit-service zone");
        Check(mark.Height >= 0.0 && mark.Height <= 4.0 &&
              mark.ScaleX > 0.0 && mark.ScaleX < 0.9 &&
              mark.ScaleY > 0.0 && mark.ScaleY < 0.2 &&
              mark.ScaleZ > 0.0 && mark.ScaleZ < 0.05,
              "marker is thin decorative road paint, not a wall");
        for (int j = 0; j < i; ++j)
        {
            const auto other = PlannedPitBayPaintMark(j);
            const double sx = other.Center.X - mark.Center.X;
            const double sy = other.Center.Y - mark.Center.Y;
            Check(sx * sx + sy * sy > 100.0 * 100.0,
                  "separate dashes have generous visual space");
        }
    }
    for (int i = 0; i < 100000; ++i)
    {
        const auto x = PlannedPitBayPaintMark(i % PitBayPaintMarkCount);
        const auto y = PlannedPitBayPaintMark(i % PitBayPaintMarkCount);
        Check(x.Center.X == y.Center.X && x.Center.Y == y.Center.Y &&
              x.YawDegrees == y.YawDegrees,
              "read-only map paint plan remains deterministic");
    }
    std::cout << "PitBayPaint: visual markers match the physical bay\n";
    return 0;
}
