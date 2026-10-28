#include "AI/RivalTrafficAwareness.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "RivalTrafficAwareness FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalTrafficObservation Traffic(
    double distance, double side, double rivalSpeed = 2200.0,
    double playerSpeed = 900.0)
{
    return {0.0, 0.0, 0.0, rivalSpeed, distance, side,
            playerSpeed, 0.0, true, rivalSpeed, 0.0};
}
}

int main()
{
    using namespace latebrake;
    const PilotCommand cruise{0.85, -0.25, false, false};
    const auto close = YieldRivalToPlayer(cruise, Traffic(420, 12));
    Require(close.Brake && close.Throttle == 0.0 &&
            close.Steering == cruise.Steering && !close.Arrived,
            "closing on a player in the same lane triggers courtesy braking");

    const auto approach = YieldRivalToPlayer(cruise, Traffic(1400, 0));
    Require(!approach.Brake && approach.Throttle >= 0.15 &&
            approach.Throttle < cruise.Throttle &&
            approach.Steering == cruise.Steering,
            "a distant approach reduces rival motor power without stealing steering");

    for (const auto& observation : {
        Traffic(-300, 0), Traffic(0, 0), Traffic(1800, 0),
        Traffic(500, 250), Traffic(500, -250),
        Traffic(400, 10, 350.0, 0.0),
        Traffic(400, 0, 1800.0, 1700.0),
        Traffic(400, 0, 1400.0, 1900.0)
    })
    {
        const auto untouched = YieldRivalToPlayer(cruise, observation);
        Require(untouched.Throttle == cruise.Throttle &&
                !untouched.Brake && untouched.Steering == cruise.Steering,
                "player behind, adjacent, far or not closing cannot trigger brake");
    }

    const auto overlapApproach = YieldRivalToPlayer(cruise, Traffic(90, 0));
    Require(overlapApproach.Brake && overlapApproach.Throttle == 0.0,
            "a nearby in-lane player ahead cannot bypass emergency braking");

    // At fast closing speed, the original 600 cm-only emergency threshold
    // could react too late even in a straight, occupied lane.
    const auto predictive = YieldRivalToPlayer(cruise, Traffic(1050, 0));
    Require(predictive.Brake && predictive.Throttle == 0.0 &&
            predictive.Steering == cruise.Steering,
            "predicted short time-to-contact brakes before 600 cm threshold");
    const auto fastCatch = YieldRivalToPlayer(
        cruise, Traffic(1400, 0, 3400.0, 500.0));
    Require(fastCatch.Brake && fastCatch.Throttle == 0.0,
            "high closing speed triggers early braking even at fourteen meters");
    const auto slowCatch = YieldRivalToPlayer(
        cruise, Traffic(1050, 0, 1500.0, 900.0));
    Require(!slowCatch.Brake && slowCatch.Throttle > 0.0 &&
            slowCatch.Throttle < cruise.Throttle,
            "plenty of time before contact keeps a smooth throttle reduction");
    const auto outsideLane = YieldRivalToPlayer(
        cruise, Traffic(1050, 226.0, 3400.0, 500.0));
    Require(!outsideLane.Brake && outsideLane.Throttle == cruise.Throttle,
            "prediction cannot brake for a player outside the lane corridor");
    const auto mildClose = YieldRivalToPlayer(
        cruise, Traffic(350, 0, 1600.0, 1400.0));
    Require(!mildClose.Brake && mildClose.Throttle < cruise.Throttle,
            "relative approach below emergency threshold is gentle, not full stop");

    const auto priorCornerBrake = YieldRivalToPlayer(
        {0.0, 0.25, true, false}, Traffic(400, 0));
    Require(priorCornerBrake.Brake && priorCornerBrake.Throttle == 0.0 &&
            priorCornerBrake.Steering == 0.25,
            "already braking for a corner is preserved");
    const auto arrival = YieldRivalToPlayer(
        {0.0, 0.0, false, true}, Traffic(350, 0));
    Require(arrival.Arrived && !arrival.Brake && arrival.Throttle == 0.0,
            "waypoint arrival is never reclassified as traffic");
    const auto idle = YieldRivalToPlayer(
        {0.0, 0.4, false, false}, Traffic(380, 0));
    Require(idle.Brake && idle.Throttle == 0.0 &&
            idle.Steering == 0.4,
            "a coasting but fast-closing rival still brakes and never adds throttle");

    constexpr double HalfPi = 1.57079632679489661923;
    const RivalTrafficObservation ninety{
        1200.0, -700.0, HalfPi, 2000.0,
        1190.0, -300.0, 0.0, 900.0, true, 0.0, 2000.0
    };
    const auto rotating = YieldRivalToPlayer(cruise, ninety);
    Require(rotating.Brake && rotating.Throttle == 0.0 &&
            rotating.Steering == cruise.Steering,
            "90-degree world-Y driving uses correct forward lane projections");

    // A sideways-sliding rival has genuine planar speed but no forward
    // approach. Treating speed magnitude as forward motion invents contact.
    auto sliding = Traffic(850, 0, 2200.0, 0.0);
    sliding.RivalWorldVelocityX = 0.0;
    sliding.RivalWorldVelocityY = 2200.0;
    const auto lateral = YieldRivalToPlayer(cruise, sliding);
    Require(!lateral.Brake && lateral.Throttle == cruise.Throttle &&
            lateral.Steering == cruise.Steering,
            "sideways speed alone cannot invent forward collision risk");
    auto reversing = Traffic(850, 0, 2200.0, 0.0);
    reversing.RivalWorldVelocityX = -2200.0;
    const auto backing = YieldRivalToPlayer(cruise, reversing);
    Require(!backing.Brake && backing.Throttle == cruise.Throttle,
            "physical reversing motion does not register as closing");
    auto inconsistent = Traffic(850, 0, 2200.0, 0.0);
    inconsistent.RivalWorldVelocityX = 200.0;
    const auto invalidSpeed = YieldRivalToPlayer(cruise, inconsistent);
    Require(invalidSpeed.Brake && invalidSpeed.Throttle == 0.0,
            "mismatched rival speed and world velocity fail closed");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const RivalTrafficObservation invalid[] = {
        {0, 0, 0, 2000, 420, 0, 0, 0, false},
        {nan, 0, 0, 2000, 420, 0, 0, 0, true},
        {0, inf, 0, 2000, 420, 0, 0, 0, true},
        {0, 0, nan, 2000, 420, 0, 0, 0, true},
        {0, 0, 8, 2000, 420, 0, 0, 0, true},
        {0, 0, 0, -1, 420, 0, 0, 0, true},
        {0, 0, 0, 15001, 420, 0, 0, 0, true},
        {0, 0, 0, 2000, nan, 0, 0, 0, true},
        {0, 0, 0, 2000, 420, inf, 0, 0, true},
        {0, 0, 0, 2000, 420, 0, inf, 0, true},
        {0, 0, 0, 2000, 420, 0, 15001, 0, true},
    };
    for (const auto& corrupt : invalid)
    {
        const auto result = YieldRivalToPlayer(cruise, corrupt);
        Require(result.Throttle == 0.0 && result.Brake &&
                result.Steering == 0.0 && !result.Arrived,
                "missing player or invalid physics must fail closed to no motor");
    }
    const auto badPilot = YieldRivalToPlayer(
        {nan, 0, false, false}, Traffic(400, 0));
    Require(badPilot.Brake && badPilot.Throttle == 0.0,
            "invalid pilot commands never authorize motor output");

    for (int d = 110; d <= 1600; ++d)
    {
        const auto output = YieldRivalToPlayer(cruise, Traffic(d, 0));
        Require(std::isfinite(output.Throttle) &&
                output.Throttle >= 0 && output.Throttle <= cruise.Throttle &&
                output.Steering == cruise.Steering && !output.Arrived,
                "every observed distance retains safe bounded speed and steering");
    }
    std::cout << "Rival lane-approach safety policy passed.\n";
    return EXIT_SUCCESS;
}
