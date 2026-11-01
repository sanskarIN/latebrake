#include "Racing/RacePitService.h"
#include "Track/CircuitMap.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* description)
{
    if (!ok)
    {
        std::cerr << "RacePitRadar FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::RacePitBay;
    const auto pit = latebrake::NormalizeCircuitMap(RacePitBay);
    const auto southLine = latebrake::NormalizeCircuitMap({0.0, latebrake::SouthY});
    Require(latebrake::IsOnPavedSurface(RacePitBay),
            "pit is on actual southern strip of paved track");
    Require(latebrake::IsInRacePitBay(RacePitBay.X, RacePitBay.Y),
            "radar marker uses real service center");
    Require(std::abs(pit.U - 0.5) < 1.0e-12,
            "pit appears at horizontal map midpoint");
    Require(std::abs(pit.V - 1700.0 / 6000.0) < 1.0e-12,
            "north-up vertical mapping matches world position");
    Require(pit.V > southLine.V,
            "inside shoulder is north of the racing center line");
    Require(pit.U >= 0.0 && pit.U <= 1.0 &&
            pit.V >= 0.0 && pit.V <= 1.0,
            "pit marker stays inside radar viewport");
    for (int i = 0; i < latebrake::CircuitGateCount; ++i)
    {
        const auto gate = latebrake::NormalizeCircuitMap(latebrake::CircuitGate(i));
        const double distanceSquared =
            (pit.U - gate.U) * (pit.U - gate.U) +
            (pit.V - gate.V) * (pit.V - gate.V);
        Require(distanceSquared > 0.04,
                "pit icon is not a checkpoint gate icon");
    }
    for (double bad : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()})
    {
        const auto invalid = latebrake::NormalizeCircuitMap({bad, RacePitBay.Y});
        Require(invalid.U == 0.5 && invalid.V == 0.5,
                "malformed sampled locations have safe map fallback");
    }
    std::cout << "RacePitRadar: portable waypoint mapping passed\n";
    return EXIT_SUCCESS;
}
