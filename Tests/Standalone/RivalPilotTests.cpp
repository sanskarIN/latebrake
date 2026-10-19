#include "AI/RivalPilot.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Rival pilot FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::PlanRivalTurn;
    const auto straight = PlanRivalTurn(0.0, 0.0, 0.0, 1000.0, 3000.0, 0.0);
    Require(!straight.Arrived && !straight.Brake, "straight waypoint should accelerate");
    Require(straight.Throttle > 0.0 && straight.Steering == 0.0,
            "straight heading produces throttle without turning");

    const auto turnLeft = PlanRivalTurn(0.0, 0.0, 0.0, 1000.0, 0.0, 3000.0);
    Require(turnLeft.Steering > 0.0, "north target turns left");
    const auto turnRight = PlanRivalTurn(0.0, 0.0, 0.0, 1000.0, 0.0, -3000.0);
    Require(turnRight.Steering < 0.0, "south target turns right");

    const auto fastCorner = PlanRivalTurn(0.0, 0.0, 0.0, 3500.0, 0.0, 3000.0);
    Require(fastCorner.Brake && fastCorner.Throttle == 0.0,
            "overspeed at corner requests braking");
    Require(std::abs(fastCorner.Steering) <= 1.0, "steering clamped");

    const auto reached = PlanRivalTurn(0.0, 0.0, 0.0, 500.0, 200.0, 0.0);
    Require(reached.Arrived && reached.Throttle == 0.0, "waypoint arrival signaled");

    const auto wrap = PlanRivalTurn(0.0, 0.0, 3.10, 500.0, -3000.0, -80.0);
    Require(wrap.Steering > 0.0 && wrap.Steering < 1.0,
            "heading wraps continuously at plus/minus pi");

    const auto invalid = PlanRivalTurn(
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 500.0, 3000.0, 0.0);
    Require(invalid.Throttle == 0.0 && invalid.Brake,
            "invalid sensor readings fail safely");

    const auto farOverflow = PlanRivalTurn(0.0, 0.0, 0.0, 0.0,
        std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    Require(farOverflow.Brake, "overflowing distance fails safely");

    const auto noRadius = PlanRivalTurn(0.0, 0.0, 0.0, 500.0, 3000.0, 0.0, 0.0);
    Require(noRadius.Brake, "invalid waypoint arrival radius rejected");

    std::cout << "Rival pilot logic tests passed.\n";
    return EXIT_SUCCESS;
}
