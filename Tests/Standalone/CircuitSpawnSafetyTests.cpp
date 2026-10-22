#include "Track/CircuitSpawnSafety.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Spawn clearance FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    Require(AreCircuitSpawnLocationsSafe(),
            "all original player/rival starts and recovery points are on pavement clear of barriers");
    Require(EvaluateCircuitSpawnPose(PlayerStart) == CircuitSpawnDecision::Safe &&
            EvaluateCircuitSpawnPose(RivalStart) == CircuitSpawnDecision::Safe,
            "player/rival starter poses are valid");
    for (int i = 0; i < CircuitGateCount; ++i)
    {
        Require(EvaluateCircuitSpawnPose(PlayerRecovery(i)) == CircuitSpawnDecision::Safe &&
                EvaluateCircuitSpawnPose(RivalRecovery(i)) == CircuitSpawnDecision::Safe,
                "all planned recovery points are safe under original collision plan");
    }
    Require(EvaluateCircuitSpawnPose({{9000.0, 9000.0}, 0.0}) ==
            CircuitSpawnDecision::OffPavement,
            "off-road recovery target rejected");
    Require(EvaluateCircuitSpawnPose({{0.0, 2550.0}, 0.0}) ==
            CircuitSpawnDecision::BarrierOverlap,
            "north perimeter barrier clearance is enforced within paved verge");
    Require(EvaluateCircuitSpawnPose({{0.0, -2550.0}, 0.0}) ==
            CircuitSpawnDecision::BarrierOverlap,
            "south perimeter barrier clearance is enforced within paved verge");
    Require(EvaluateCircuitSpawnPose({{0.0, 0.0}, 0.0}) ==
            CircuitSpawnDecision::OffPavement,
            "center dirt cannot become valid start");
    Require(EvaluateCircuitSpawnPose(
                {{std::numeric_limits<double>::quiet_NaN(), SouthY}, 0.0}) ==
            CircuitSpawnDecision::InvalidLocation,
            "invalid coordinate rejected before geometry query");
    Require(EvaluateCircuitSpawnPose(
                {{PlayerStart.Position.X, SouthY},
                 std::numeric_limits<double>::infinity()}) ==
            CircuitSpawnDecision::InvalidLocation,
            "nonfinite rotation rejected");
    Require(EvaluateCircuitSpawnPose(PlayerStart, -10.0) ==
            CircuitSpawnDecision::InvalidLocation,
            "negative clearance cannot silently authorize unsafe starts");
    Require(EvaluateCircuitSpawnPose(PlayerStart, 15000.0) ==
            CircuitSpawnDecision::InvalidLocation,
            "unreasonably large clearance rejected instead of overflowing comparisons");
    std::cout << "Original circuit spawn plan XY preflight passed.\n";
    return EXIT_SUCCESS;
}
