#include "Track/CircuitLayout.h"
#include "Racing/GateValidation.h"
#include "Racing/RaceEvents.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    using namespace latebrake;
    static_assert(CircuitGateCount == 4);
    static_assert(CircuitGates[0].X == EastX && CircuitGates[0].Y == SouthY);
    static_assert(CircuitGates[3].X == WestX && CircuitGates[3].Y == SouthY);
    assert(GetRaceEvent(EventKind::Circuit).CheckpointCount == CircuitGateCount);
    assert(GetRaceEvent(EventKind::TimeAttack).CheckpointCount == CircuitGateCount);
    assert(!IsCircuitGate(-1) && !IsCircuitGate(4));
    assert(IsCircuitGate(0) && IsCircuitGate(3));
    assert(CircuitGate(-1).X == 0.0);
    assert(CircuitGate(4).Y == 0.0);
    assert(IsOnPavedSurface(PlayerStart.Position));
    assert(IsOnPavedSurface(RivalStart.Position));
    assert(!IsOnPavedSurface({0.0, 0.0}));
    assert(!IsOnPavedSurface({99999.0, 99999.0}));
    assert(!IsOnPavedSurface({std::numeric_limits<double>::quiet_NaN(), 0.0}));
    assert(!IsOnPavedSurface({0.0, std::numeric_limits<double>::infinity()}));
    for (int index = 0; index < CircuitGateCount; ++index)
    {
        const CircuitPoint gate = CircuitGate(index);
        assert(IsOnPavedSurface(gate));
        assert(IsOnPavedSurface(PlayerRecovery(index).Position));
        assert(IsOnPavedSurface(RivalRecovery(index).Position));
        assert(std::isfinite(PlayerRecovery(index).HeadingDegrees));
        assert(std::isfinite(RivalRecovery(index).HeadingDegrees));
        // The gate's expected velocity is aligned to the shared track order.
        const double vx = index == 0 ? 200.0 : index == 2 ? -200.0 : 0.0;
        const double vy = index == 1 ? 200.0 : index == 3 ? -200.0 : 0.0;
        assert(CanPassCircuitGate(index, vx, vy));
        assert(!CanPassCircuitGate(index, -vx, -vy));
    }
    assert(PlayerRecovery(-1).Position.X == PlayerStart.Position.X);
    assert(RivalRecovery(99).Position.X == RivalStart.Position.X);
    std::cout << "Circuit layout geometry tests passed.\n";
}
