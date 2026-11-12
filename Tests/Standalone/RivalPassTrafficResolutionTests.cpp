#include "AI/RivalPassTrafficResolution.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalFollowingBrakeHold.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "RivalPassTrafficResolution FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalTrafficObservation Traffic(
    double distance, double side, double rivalSpeed = 2000.0,
    double playerSpeed = 1500.0)
{
    // Match the qualified previous-tick brake estimate supplied by
    // the physical rival's corner-forecast pipeline in Unreal.
    return {0, 0, 0, rivalSpeed, distance, side, playerSpeed, 0,
            true, rivalSpeed, 0, 684.0};
}
}

int main()
{
    using namespace latebrake;
    const PilotCommand route{0.76, 0.08, false, false};
    const RivalPassOutput passing{{0.76, -0.42, false, false}, true, -440.0};

    // The original passing manoeuvre tried 900-1600cm with high closing
    // speed, immediately entering the independent traffic emergency.
    // If braking occurs, cancel the lane attempt and brake on the
    // previously planned route, not a sideways passing trajectory.
    const auto emergency = YieldRivalToPlayer(passing.Command,
                                              Traffic(1050.0, 0, 2450, 1100));
    Require(emergency.Brake && emergency.Throttle == 0.0,
            "real same-lane emergency still wins during attempted pass");
    const auto resolved = ResolveRivalPassTraffic(route, passing, emergency);
    Require(resolved.AbortPassing && resolved.Command.Brake &&
            resolved.Command.Throttle == 0.0 &&
            resolved.Command.Steering == route.Steering,
            "brake event aborts passing and restores safe route steering");

    // Already-separated cars keep the passing steer; the following
    // policy sees genuine physical lane clearance, no override is made.
    const auto separated = YieldRivalToPlayer(passing.Command,
                                               Traffic(1250, 270));
    const auto safe = ResolveRivalPassTraffic(route, passing, separated);
    Require(!separated.Brake && safe.Command.Steering == -0.42 &&
            !safe.AbortPassing && safe.Command.Throttle <= route.Throttle,
            "genuine lateral clearance permits continued passing");

    const auto ordinary = ResolveRivalPassTraffic(
        route, {route, false, 0.0},
        YieldRivalToPlayer(route, Traffic(420.0, 0)));
    Require(!ordinary.AbortPassing && ordinary.Command.Brake &&
            ordinary.Command.Steering == route.Steering,
            "nonpassing traffic braking must stay unchanged");

    RivalFollowingBrakeHold hold;
    const auto initial = YieldRivalToPlayer(passing.Command,
                                              Traffic(410.0, 0));
    const auto held = hold.Apply(initial, 1.0 / 60.0, true,
                                  initial.Brake && !passing.Command.Brake);
    const auto mid = ResolveRivalPassTraffic(route, passing, held);
    Require(mid.AbortPassing && mid.Command.Brake &&
            mid.Command.Steering == route.Steering &&
            hold.GetRemainingSeconds() > 0.0,
            "0.18s real traffic brake hysteresis cannot retain overtaking steer");
    hold.Reset();

    const PilotCommand originalBrake{0.0, 0.15, true, false};
    const auto prior = ResolveRivalPassTraffic(
        originalBrake, {originalBrake, false, 0.0}, originalBrake);
    Require(!prior.AbortPassing && prior.Command.Brake &&
            prior.Command.Steering == originalBrake.Steering,
            "prior corner braking unaffected");
    const PilotCommand routeFinish{0, 0, false, true};
    const auto finished = ResolveRivalPassTraffic(
        routeFinish, {routeFinish, false, 0.0}, routeFinish);
    Require(finished.Command.Arrived && !finished.AbortPassing,
            "verified original gate completion unaffected");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const auto invalid = ResolveRivalPassTraffic(
        route, passing, {nan, 0.3, false, false});
    Require(invalid.AbortPassing && invalid.Command.Brake &&
            invalid.Command.Throttle == 0.0,
            "untrusted traffic control cannot authorize propulsion");
    const auto badLane = ResolveRivalPassTraffic(
        route, {passing.Command, true, 500.0}, passing.Command);
    Require(badLane.AbortPassing && badLane.Command.Brake,
            "invalid passing lane cannot override emergency guard");

    for (int i = 0; i < 20000; ++i)
    {
        const double side = static_cast<double>(i % 530) - 265.0;
        const double distance = static_cast<double>(i % 1801) + 200.0;
        const double rivalSpeed = 1100.0 + static_cast<double>(i % 1600);
        const double playerSpeed = static_cast<double>(i % 1000);
        const auto traffic = YieldRivalToPlayer(
            passing.Command,
            Traffic(distance, side, rivalSpeed, playerSpeed));
        const auto outcome = ResolveRivalPassTraffic(route, passing, traffic);
        Require(std::isfinite(outcome.Command.Throttle) &&
                std::isfinite(outcome.Command.Steering) &&
                outcome.Command.Throttle >= 0.0 &&
                outcome.Command.Throttle <= route.Throttle &&
                std::abs(outcome.Command.Steering) <= 1.0,
                "20k independent traffic encounters keep control finite");
        if (traffic.Brake)
        {
            Require(outcome.AbortPassing && outcome.Command.Brake &&
                    outcome.Command.Steering == route.Steering,
                    "any genuine traffic brake cancels active passing steer");
        }
    }
    std::cout << "Rival passing/traffic braking composition verified.\n";
    return EXIT_SUCCESS;
}
