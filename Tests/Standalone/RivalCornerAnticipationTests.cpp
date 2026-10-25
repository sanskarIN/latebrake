#include "AI/RivalCornerAnticipation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "RivalCornerAnticipation FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using namespace latebrake;
    const PilotCommand base{0.85, 0.25, false, false};
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint target = CircuitGate(gate);
        const CircuitPoint prev = CircuitGate(
            (gate + CircuitGateCount - 1) % CircuitGateCount);
        const double vectorX = target.X - prev.X;
        const double vectorY = target.Y - prev.Y;
        const double length = std::hypot(vectorX, vectorY);
        const CircuitPoint close{
            target.X - vectorX / length * 1000.0,
            target.Y - vectorY / length * 1000.0
        };
        const auto fast = AnticipateRivalCorner(
            base, gate, close, 3200.0);
        Check(fast.Brake && fast.Throttle == 0.0 &&
              fast.Steering == base.Steering && !fast.Arrived,
              "all four turns request early braking without stealing steering");
        const auto slow = AnticipateRivalCorner(
            base, gate, close, 1000.0);
        Check(!slow.Brake && slow.Throttle == base.Throttle,
              "slow rival retains regular AI throttle");
        const auto far = AnticipateRivalCorner(
            base, gate, {target.X - vectorX / length * 2500.0,
                         target.Y - vectorY / length * 2500.0}, 4300.0);
        Check(!far.Brake && far.Throttle == base.Throttle,
              "does not brake far outside approach window");
    }
    const auto priorBrake = AnticipateRivalCorner(
        {0, -0.4, true, false}, 0, {2000, SouthY}, 4000);
    Check(priorBrake.Brake && priorBrake.Throttle == 0 &&
          priorBrake.Steering == -0.4,
          "never overrides a braking decision from main rival planner");
    const auto arrived = AnticipateRivalCorner(
        {0, 0, false, true}, 0, {EastX, SouthY}, 0);
    Check(arrived.Arrived && !arrived.Brake,
          "preserves waypoint arrival state");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        const auto invalid = AnticipateRivalCorner(
            base, 0, {bad, SouthY}, 2500.0);
        const auto invalidSpeed = AnticipateRivalCorner(
            base, 0, {2500, SouthY}, bad);
        Check(invalid.Brake && invalid.Throttle == 0 &&
              invalidSpeed.Brake && invalidSpeed.Throttle == 0,
              "invalid source samples fail closed");
    }
    const auto invalidGate = AnticipateRivalCorner(
        base, -1, {2500, SouthY}, 2300);
    Check(invalidGate.Brake && invalidGate.Throttle == 0,
          "invalid route gate never triggers throttle");
    std::cout << "Rival corner anticipation passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
