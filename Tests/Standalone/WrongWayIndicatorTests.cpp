#include "Racing/WrongWayIndicator.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool result, const char* description)
{
    if (!result)
    {
        std::cerr << "Wrong-way indicator FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    const CircuitPoint centers[] = {
        {0.0, SouthY}, {EastX, 0.0},
        {0.0, NorthY}, {WestX, 0.0}
    };
    const CircuitPoint forward[] = {
        {950.0, 0.0}, {0.0, 950.0},
        {-950.0, 0.0}, {0.0, -950.0}
    };
    const CircuitTravelHeading expected[] = {
        CircuitTravelHeading::East, CircuitTravelHeading::North,
        CircuitTravelHeading::West, CircuitTravelHeading::South
    };
    for (int i = 0; i < 4; ++i)
    {
        const CircuitPoint opposite{-forward[i].X, -forward[i].Y};
        Require(ExpectedCircuitHeading(centers[i]) == expected[i],
                "all four middle straights have correct ordered directions");
        Require(AssessCircuitTravel(centers[i], forward[i]) ==
                    CircuitTravelAssessment::WithRoute,
                "normal circuit direction remains safe");
        Require(AssessCircuitTravel(centers[i], opposite) ==
                    CircuitTravelAssessment::AgainstRoute,
                "reverse traversal identifies wrong-way movement");
        Require(AssessCircuitTravel(centers[i],
                {opposite.X * 0.1, opposite.Y * 0.1}) ==
                    CircuitTravelAssessment::Unknown,
                "slow reversing/maneuvering should not produce warning");
    }

    for (CircuitPoint corner : {CircuitPoint{EastX, SouthY},
                                CircuitPoint{EastX, NorthY},
                                CircuitPoint{WestX, NorthY},
                                CircuitPoint{WestX, SouthY}})
    {
        Require(ExpectedCircuitHeading(corner) ==
                    CircuitTravelHeading::Unavailable &&
                AssessCircuitTravel(corner, {-2000.0, 0.0}) ==
                    CircuitTravelAssessment::Unknown,
                "unambiguous straight only; never guess at 90-degree corners");
    }
    Require(ExpectedCircuitHeading({10000.0, 10000.0}) ==
                CircuitTravelHeading::Unavailable &&
            AssessCircuitTravel({10000.0, 10000.0}, {-800.0, 0.0}) ==
                CircuitTravelAssessment::Unknown,
            "off-road position does not generate wrong-way hint");
    Require(ExpectedCircuitHeading({0.0, SouthY + 451.0}) ==
                CircuitTravelHeading::Unavailable,
            "outer road shoulder is outside advisory corridor");
    Require(AssessCircuitTravel(centers[0], {0.0, -900.0}) ==
                CircuitTravelAssessment::Unknown,
            "sideways slide is not misclassified as intentional wrong way");
    Require(AssessCircuitTravel(centers[0], {-600.0, 0.0}) ==
                CircuitTravelAssessment::AgainstRoute,
            "speed threshold is inclusive at exactly 600cm/s");

    WrongWayIndicator indicator;
    Require(!indicator.Update(centers[0], {-950.0, 0.0}, 0.2, true) &&
            !indicator.Update(centers[0], {-950.0, 0.0}, 0.2, true) &&
            !indicator.Update(centers[0], {-950.0, 0.0}, 0.2, true),
            "short reverse interval is not enough to warn");
    Require(indicator.Update(centers[0], {-950.0, 0.0}, 0.2, true),
            "sustained wrong-way driving displays a warning after 0.8s");
    Require(indicator.GetOppositeSeconds() >= 0.8,
            "diagnostic timer is bounded and tracks eligible motion");
    Require(!indicator.Update(centers[0], {950.0, 0.0}, 0.05, true) &&
            indicator.GetOppositeSeconds() == 0.0,
            "warning resets as soon as correct direction resumes");
    Require(!indicator.Update(centers[0], {-950.0, 0.0}, 0.1, true) &&
            !indicator.Update(centers[0], {-950.0, 0.0}, 0.1, false) &&
            indicator.GetOppositeSeconds() == 0.0,
            "paused/menu/finished/not-ready race clears warning state");
    for (double dt : {0.0, -0.1, 0.5,
                      std::numeric_limits<double>::quiet_NaN(),
                      std::numeric_limits<double>::infinity()})
    {
        Require(!indicator.Update(centers[0], {-950.0, 0.0}, dt, true) &&
                indicator.GetOppositeSeconds() == 0.0,
                "invalid or hitch-length frame cannot create a warning");
    }
    const double bad[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };
    for (double value : bad)
    {
        Require(AssessCircuitTravel({value, SouthY}, {-1000.0, 0.0}) ==
                    CircuitTravelAssessment::Unknown &&
                AssessCircuitTravel(centers[0], {value, 0.0}) ==
                    CircuitTravelAssessment::Unknown,
                "invalid positions and velocities are never classified");
    }

    // Repeated HUD draws cannot accumulate an unbounded elapsed timer.
    for (int i = 0; i < 100000; ++i)
    {
        indicator.Update(centers[1], {0.0, -950.0}, 0.04, true);
    }
    Require(indicator.GetOppositeSeconds() <= 2.0,
            "opposite-direction diagnostic time saturates at two seconds");
    indicator.Reset();
    Require(indicator.GetOppositeSeconds() == 0.0,
            "teleport/changed session/pawn reset has no stale warning");

    std::cout << "Circuit wrong-way hint passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
