#include "Track/OffTrackAdvisor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes, const char* name)
{
    if (!yes)
    {
        std::cerr << "OffTrackAdvisor FAIL: " << name << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;

    for (CircuitPoint road : {
        PlayerStart.Position, RivalStart.Position,
        CircuitPoint{0.0, SouthY}, CircuitPoint{0.0, NorthY},
        CircuitPoint{EastX, 0.0}, CircuitPoint{WestX, 0.0},
        CircuitPoint{0.0, SouthY + RoadHalfWidth},
        CircuitPoint{0.0, NorthY - RoadHalfWidth}
    })
    {
        const auto advice = AdviseReturnToCircuit(road);
        Check(IsOnPavedSurface(road) && advice.Valid &&
              !advice.OffPavement && Near(advice.DistanceMeters, 0.0) &&
              Near(advice.UnitDirection.X, 0.0) &&
              Near(advice.UnitDirection.Y, 0.0),
              "paved circuit never displays return direction");
    }

    const auto below = AdviseReturnToCircuit({0.0, SouthY - 1000.0});
    Check(below.Valid && below.OffPavement &&
          Near(below.DistanceMeters, 2.0) &&
          Near(below.UnitDirection.X, 0.0) &&
          Near(below.UnitDirection.Y, 1.0),
          "south shoulder points 2m toward south road");
    const auto above = AdviseReturnToCircuit({0.0, NorthY + 1050.0});
    Check(above.Valid && above.OffPavement &&
          Near(above.DistanceMeters, 2.5) &&
          Near(above.UnitDirection.X, 0.0) &&
          Near(above.UnitDirection.Y, -1.0),
          "north shoulder points toward northern road");
    const auto east = AdviseReturnToCircuit({EastX + 1050.0, 0.0});
    Check(east.Valid && east.OffPavement &&
          Near(east.DistanceMeters, 2.5) &&
          Near(east.UnitDirection.X, -1.0),
          "eastern shoulder points to vertical road");
    const auto west = AdviseReturnToCircuit({WestX - 1050.0, 0.0});
    Check(west.Valid && west.OffPavement &&
          Near(west.DistanceMeters, 2.5) &&
          Near(west.UnitDirection.X, 1.0),
          "western shoulder points to vertical road");

    const auto diagonal = AdviseReturnToCircuit({4700.0, 3300.0});
    Check(diagonal.Valid && diagonal.OffPavement &&
          diagonal.DistanceMeters > 0.0 &&
          Near(std::hypot(diagonal.UnitDirection.X,
                          diagonal.UnitDirection.Y), 1.0),
          "diagonal outside point has a normalized return vector");

    for (double invalid : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::max()
    })
    {
        Check(!AdviseReturnToCircuit({invalid, 0.0}).Valid &&
              !AdviseReturnToCircuit({0.0, invalid}).Valid,
              "invalid or extreme physics must never offer false guidance");
    }

    OffTrackIndicator warning;
    const CircuitPoint off{0.0, SouthY - 1000.0};
    for (int i = 0; i < 3; ++i)
    {
        Check(!warning.Update(off, 0.2, true),
              "brief off-track driving is not a warning");
    }
    Check(warning.Update(off, 0.2, true) &&
          warning.GetOffPavementSeconds() >= 0.8,
          "warning requires sustained off-track samples");
    Check(!warning.Update(PlayerStart.Position, 0.02, true) &&
          warning.GetOffPavementSeconds() == 0.0,
          "return to pavement immediately clears warning");
    Check(!warning.Update(off, 0.1, true) &&
          !warning.Update(off, 0.1, false) &&
          warning.GetOffPavementSeconds() == 0.0,
          "paused/menu/finished race resets warning");
    for (double dt : {
        0.0, -0.1, 0.5, std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    })
    {
        Check(!warning.Update(off, dt, true) &&
              warning.GetOffPavementSeconds() == 0.0,
              "zero, negative, hitch and bad frames cannot accumulate");
    }
    for (int i = 0; i < 100000; ++i)
    {
        warning.Update(off, 0.1, true);
    }
    Check(warning.GetOffPavementSeconds() <= 2.0 &&
          warning.GetOffPavementSeconds() >= 0.8,
          "long sessions have bounded alert state");
    warning.Reset();
    Check(warning.GetOffPavementSeconds() == 0.0,
          "teleport and session reset removes stale warnings");

    std::cout << "Off-track geometry/advisory tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
