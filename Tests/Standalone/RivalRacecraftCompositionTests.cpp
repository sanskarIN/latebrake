#include "AI/RivalGripPace.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalFollowingBrakeHold.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "RivalRacecraftComposition FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::RivalTrafficObservation Traffic(
    double rivalSpeed, double playerForward, double ahead = 400.0,
    double lateral = 0.0, bool playerReady = true)
{
    return {
        0.0, 0.0, 0.0, rivalSpeed, ahead, lateral,
        playerForward, 0.0, playerReady, rivalSpeed, 0.0
    };
}

// Exact source-order analogue of the real autonomous rival's policy chain.
// This never simulates Unreal, track collision or physical tire dynamics.
class Pipeline final
{
public:
    latebrake::PilotCommand Step(
        latebrake::PilotCommand route,
        latebrake::RivalGripPaceSample tire,
        latebrake::RivalTrafficObservation player)
    {
        const auto grip = Pace.Apply(route, tire);
        const auto traffic = latebrake::YieldRivalToPlayer(grip, player);
        return Hold.Apply(traffic, tire.FrameSeconds, player.PlayerReady,
                          traffic.Brake && !grip.Brake);
    }

    void Reset()
    {
        Pace.Reset();
        Hold.Reset();
    }

    double CornerHold() const { return Pace.RemainingHoldSeconds(); }
    double TrafficHold() const { return Hold.GetRemainingSeconds(); }

private:
    latebrake::RivalGripPace Pace;
    latebrake::RivalFollowingBrakeHold Hold;
};
}

int main()
{
    using latebrake::PilotCommand;
    using latebrake::RivalGripPaceSample;
    Pipeline pipeline;
    const PilotCommand route{0.85, 0.70, false, false};

    // Grip braking wins before traffic evaluation, preserving the route
    // steering and not manufacturing a fresh player-traffic brake hold.
    auto out = pipeline.Step(route, {0.05, 2400.0, 550.0, 0.78, true},
                             Traffic(2400.0, 700.0, 420.0));
    Require(out.Brake && out.Throttle == 0.0 &&
            out.Steering == route.Steering && pipeline.CornerHold() > 0.0 &&
            pipeline.TrafficHold() == 0.0,
            "corner braking must not be mistaken for a new traffic brake");

    pipeline.Reset();
    // Less severe tire loss permits route throttle, but a closing physical
    // player in the same lane must override it with ordinary brake.
    out = pipeline.Step(route, {0.05, 1700.0, 320.0, 0.85, true},
                        Traffic(1700.0, 900.0, 370.0));
    Require(out.Brake && out.Throttle == 0.0 &&
            out.Steering == route.Steering &&
            pipeline.CornerHold() == 0.0 && pipeline.TrafficHold() > 0.0,
            "qualified player traffic must override permitted grip throttle");

    // Changing lane no longer requests new traffic braking, but a bounded
    // hold protects against single-frame lane/velocity boundary chatter.
    for (int i = 0; i < 3; ++i)
    {
        out = pipeline.Step(route, {0.05, 1700.0, 0.0, 1.0, true},
                            Traffic(1700.0, 900.0, 900.0, 280.0));
        Require(out.Brake && out.Throttle == 0.0 &&
                out.Steering == route.Steering,
                "traffic brake hysteresis must not remove steering");
    }
    out = pipeline.Step(route, {0.05, 1700.0, 0.0, 1.0, true},
                        Traffic(1700.0, 900.0, 900.0, 280.0));
    Require(!out.Brake && out.Throttle == route.Throttle &&
            pipeline.TrafficHold() == 0.0,
            "bounded traffic hold must expire and restore authorized throttle");

    // Authoritative checkpoint arrival passes through all three policies.
    pipeline.Reset();
    out = pipeline.Step({0.0, -0.2, false, true},
                        {0.05, 1000.0, 80.0, 0.98, true},
                        Traffic(1000.0, 100.0, 100.0));
    Require(out.Arrived && !out.Brake && out.Throttle == 0.0 &&
            out.Steering == -0.2 && pipeline.CornerHold() == 0.0 &&
            pipeline.TrafficHold() == 0.0,
            "checkpoint arrival must not create traffic or grip braking");

    pipeline.Reset();
    out = pipeline.Step(route, {0.05, 1800.0, 200.0, 0.80, false},
                        Traffic(1800.0, 600.0));
    Require(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0 &&
            pipeline.TrafficHold() == 0.0,
            "missing prior real tire sample must neutralize rival motor");
    pipeline.Reset();
    out = pipeline.Step(route, {0.05, 1800.0, 180.0, 0.90, true},
                        Traffic(1800.0, 900.0, 400.0, 0.0, false));
    Require(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0,
            "unready physical player cannot enable phantom rival traffic");
    pipeline.Reset();
    out = pipeline.Step(route,
                        {std::numeric_limits<double>::quiet_NaN(),
                         1800.0, 180.0, 0.90, true},
                        Traffic(1800.0, 900.0));
    Require(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0 &&
            pipeline.CornerHold() == 0.0 &&
            pipeline.TrafficHold() == 0.0,
            "invalid time cannot preserve or invent acceleration");

    // Repeated races and moving player targets. All policy decisions
    // must remain bounded, finite and incapable of adding route power,
    // changing route steering, creating an arrival or leaking holds.
    for (int session = 0; session < 1000; ++session)
    {
        pipeline.Reset();
        for (int frame = 0; frame < 100; ++frame)
        {
            const double speed = 800.0 + (frame % 130) * 25.0;
            const double lateral = 0.1 * speed;
            const double grip = 0.72 + (frame % 28) * 0.01;
            const double steer = frame % 2 ? 0.6 : -0.4;
            const PilotCommand planned{0.85, steer, false, false};
            const auto result = pipeline.Step(
                planned, {0.02, speed, lateral, grip, true},
                Traffic(speed, 0.45 * speed, 340.0 + (frame % 100) * 18.0,
                        frame % 3 ? 0.0 : 290.0));
            Require(std::isfinite(result.Throttle) &&
                    std::isfinite(result.Steering) &&
                    result.Throttle >= 0.0 &&
                    result.Throttle <= planned.Throttle &&
                    result.Steering == planned.Steering && !result.Arrived,
                    "composed rival policies must not amplify motor or steer");
        }
        pipeline.Reset();
        Require(pipeline.CornerHold() == 0.0 &&
                pipeline.TrafficHold() == 0.0,
                "race boundaries clear both independent brake holds");
    }

    std::cout << "Rival racecraft composition passed 100000 frames.\n";
    return EXIT_SUCCESS;
}
