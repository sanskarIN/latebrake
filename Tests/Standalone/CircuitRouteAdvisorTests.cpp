#include "Track/CircuitRouteAdvisor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "CircuitRouteAdvisor FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using namespace latebrake;
    const CircuitRouteAdvice start = AdviseCircuitRoute(
        0, PlayerStart.Position, 0.0, true);
    Check(start.Valid && start.Hint == RouteHint::Straight &&
          std::abs(start.DirectDistanceMeters - 53.5) < 1e-9 &&
          !start.Approaching, "start gives grounded direct range east");
    const auto near = AdviseCircuitRoute(
        0, {EastX - 850.0, SouthY}, 0.0, true);
    Check(near.Valid && near.Approaching &&
          near.Hint == RouteHint::Straight,
          "approach window offers straight-ahead gate");
    const auto left = AdviseCircuitRoute(
        0, {EastX - 850.0, SouthY}, -90.0, true);
    const auto right = AdviseCircuitRoute(
        0, {EastX - 850.0, SouthY}, 90.0, true);
    Check(left.Valid && left.Hint == RouteHint::Left &&
          right.Valid && right.Hint == RouteHint::Right,
          "relative steering direction is determined by current yaw");
    const auto turn = AdviseCircuitRoute(
        0, {EastX - 850.0, SouthY}, 179.0, true);
    Check(turn.Valid && turn.Hint == RouteHint::TurnAround,
          "opposing orientation uses turn-around hint");
    const auto wrap = AdviseCircuitRoute(
        0, {EastX - 850.0, SouthY}, 360.0, true);
    Check(wrap.Valid && wrap.Hint == RouteHint::Straight &&
          std::abs(wrap.HeadingErrorDegrees) < 1e-9,
          "360-degree yaw wraps to straight");
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const auto pose = CircuitGate(gate);
        const auto atGate = AdviseCircuitRoute(gate, pose, 0.0, true);
        Check(atGate.Valid && atGate.Approaching &&
              atGate.DirectDistanceMeters == 0.0,
              "all four target gates have defined zero range");
    }
    Check(!AdviseCircuitRoute(-1, PlayerStart.Position, 0, true).Valid &&
          !AdviseCircuitRoute(4, PlayerStart.Position, 0, true).Valid &&
          !AdviseCircuitRoute(0, PlayerStart.Position, 0, false).Valid,
          "invalid gate or paused/blocked race returns no hint");
    Check(!AdviseCircuitRoute(0, {14000, 14000}, 0, true).Valid,
          "off-track cannot fabricate meaningful route range");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        Check(!AdviseCircuitRoute(0, {bad, 0}, 0, true).Valid &&
              !AdviseCircuitRoute(0, PlayerStart.Position, bad, true).Valid,
              "nonfinite pose/yaw cannot produce a bearing");
    }
    std::cout << "Circuit route advisory passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
