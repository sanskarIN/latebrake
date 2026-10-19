#include "Track/CircuitLayout.h"
#include "Racing/GateValidation.h"
#include "Racing/RaceEvents.h"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* detail)
{
    if (!value)
    {
        std::cerr << "Course layout FAIL: " << detail << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(CircuitGateCount == 4);
    static_assert(CircuitGates[0].X == EastX && CircuitGates[0].Y == SouthY);
    static_assert(CircuitGates[3].X == WestX && CircuitGates[3].Y == SouthY);
    Require(GetRaceEvent(EventKind::Circuit).CheckpointCount == CircuitGateCount, "course check 1");
    Require(GetRaceEvent(EventKind::TimeAttack).CheckpointCount == CircuitGateCount, "course check 2");
    Require(!IsCircuitGate(-1) && !IsCircuitGate(4), "course check 3");
    Require(IsCircuitGate(0) && IsCircuitGate(3), "course check 4");
    Require(CircuitGate(-1).X == 0.0, "course check 5");
    Require(CircuitGate(4).Y == 0.0, "course check 6");
    Require(IsOnPavedSurface(PlayerStart.Position), "course check 7");
    Require(IsOnPavedSurface(RivalStart.Position), "course check 8");
    Require(!IsOnPavedSurface({0.0, 0.0}), "course check 9");
    Require(!IsOnPavedSurface({99999.0, 99999.0}), "course check 10");
    Require(!IsOnPavedSurface({std::numeric_limits<double>::quiet_NaN(), 0.0}), "course check 11");
    Require(!IsOnPavedSurface({0.0, std::numeric_limits<double>::infinity()}), "course check 12");
    for (int index = 0; index < CircuitGateCount; ++index)
    {
        const CircuitPoint gate = CircuitGate(index);
        Require(IsOnPavedSurface(gate), "course check 13");
        Require(IsOnPavedSurface(PlayerRecovery(index).Position), "course check 14");
        Require(IsOnPavedSurface(RivalRecovery(index).Position), "course check 15");
        Require(std::isfinite(PlayerRecovery(index).HeadingDegrees), "course check 16");
        Require(std::isfinite(RivalRecovery(index).HeadingDegrees), "course check 17");
        // The gate's expected velocity is aligned to the shared track order.
        const double vx = index == 0 ? 200.0 : index == 2 ? -200.0 : 0.0;
        const double vy = index == 1 ? 200.0 : index == 3 ? -200.0 : 0.0;
        Require(CanPassCircuitGate(index, vx, vy), "course check 18");
        Require(!CanPassCircuitGate(index, -vx, -vy), "course check 19");
    }
    Require(PlayerRecovery(-1).Position.X == PlayerStart.Position.X, "course check 20");
    Require(RivalRecovery(99).Position.X == RivalStart.Position.X, "course check 21");
    std::cout << "Circuit layout geometry tests passed.\n";
}
