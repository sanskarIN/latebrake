#include "AI/RivalFollowingBrakeHold.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool okay, const char* message)
{
    if (!okay)
    {
        std::cerr << "RivalFollowingBrakeHold FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RivalFollowingBrakeHold hold;
    const PilotCommand cruise{0.85, -0.25, false, false};
    const PilotCommand trafficBrake{0.0, -0.25, true, false};

    Require(hold.GetRemainingSeconds() == 0.0, "initial hold is empty");
    const auto urgent = hold.Apply(trafficBrake, 0.016, true, true);
    Require(urgent.Brake && urgent.Throttle == 0.0 &&
            urgent.Steering == cruise.Steering &&
            hold.GetRemainingSeconds() == RivalFollowingBrakeHold::HoldSeconds,
            "a real newly required traffic brake arms short hold");

    for (int i = 0; i < 7; ++i)
    {
        const auto filtered = hold.Apply(cruise, 0.02, true, false);
        Require(filtered.Brake && filtered.Throttle == 0.0 &&
                filtered.Steering == cruise.Steering,
                "held brake prevents frame-to-frame motor chatter");
    }
    Require(hold.GetRemainingSeconds() > 0.0,
            "brake hold still active before the full window expires");
    const auto released = hold.Apply(cruise, 0.05, true, false);
    Require(!released.Brake && released.Throttle == cruise.Throttle &&
            released.Steering == cruise.Steering &&
            hold.GetRemainingSeconds() == 0.0,
            "fresh safe observations eventually restore original cruise");

    hold.Apply(trafficBrake, 0.016, true, true);
    hold.Apply(cruise, 0.02, true, false);
    const auto repeat = hold.Apply(trafficBrake, 0.016, true, true);
    Require(repeat.Brake && repeat.Throttle == 0.0 &&
            hold.GetRemainingSeconds() == RivalFollowingBrakeHold::HoldSeconds,
            "new genuine emergency refreshes short hold");

    // Route completion never inherits a stale brake window.
    const PilotCommand arrived{0.0, 0.0, false, true};
    const auto completed = hold.Apply(arrived, 0.016, true, false);
    Require(completed.Arrived && !completed.Brake &&
            hold.GetRemainingSeconds() == 0.0,
            "completed waypoint keeps its existing arrival command");

    hold.Apply(trafficBrake, 0.016, true, true);
    const auto corner = hold.Apply(
        {0.0, 0.5, true, false}, 0.02, true, false);
    Require(corner.Brake && corner.Throttle == 0.0 &&
            corner.Steering == 0.5,
            "existing corner brake and steering are never weakened");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (const double badTime : {nan, inf, -inf, 0.0, -0.01, 0.251})
    {
        hold.Apply(trafficBrake, 0.016, true, true);
        const auto bad = hold.Apply(cruise, badTime, true, false);
        Require(bad.Brake && bad.Throttle == 0.0 &&
                bad.Steering == 0.0 && !bad.Arrived &&
                hold.GetRemainingSeconds() == 0.0,
                "invalid or excessive frame duration stops motor and clears latch");
    }

    hold.Apply(trafficBrake, 0.016, true, true);
    const auto interrupted = hold.Apply(cruise, 0.016, false, false);
    Require(interrupted.Brake && interrupted.Throttle == 0.0 &&
            hold.GetRemainingSeconds() == 0.0,
            "focus/pause/lost player invalidates traffic hold and motor");

    for (const auto badCommand : {
        PilotCommand{nan, 0, false, false},
        PilotCommand{0.5, inf, false, false},
        PilotCommand{2, 0, false, false},
        PilotCommand{0.5, -2, false, false}
    })
    {
        hold.Apply(trafficBrake, 0.016, true, true);
        const auto blocked = hold.Apply(badCommand, 0.016, true, false);
        Require(blocked.Brake && blocked.Throttle == 0.0 &&
                blocked.Steering == 0.0 &&
                hold.GetRemainingSeconds() == 0.0,
                "corrupt command cannot retain a live motor or brake latch");
    }

    hold.Apply(trafficBrake, 0.016, true, true);
    hold.Reset();
    Require(hold.GetRemainingSeconds() == 0.0 &&
            hold.Apply(cruise, 0.016, true, false).Throttle == cruise.Throttle,
            "event reset starts with clean, unmodified steering and motor");

    std::cout << "Rival bounded following-brake hold tests passed.\n";
    return EXIT_SUCCESS;
}
