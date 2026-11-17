#include "AI/WorldScaleCandidateRivalPilot.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "WorldScaleCandidateRivalPilot FAILED: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double eps = 1e-5)
{
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a - b) <= eps;
}
latebrake::WorldScalePilotSample OnRoad(
    double distanceCm, double velocity, double delta = 1.0 / 60.0,
    double lateral = 0.0)
{
    const auto pose = latebrake::WorldScaleCenterlineAt(distanceCm);
    const double rad = pose.Pose.HeadingDegrees *
                       latebrake::WorldScalePaceRadPerDegree;
    return {{
        pose.Pose.Position.X - lateral * std::sin(rad),
        pose.Pose.Position.Y + lateral * std::cos(rad)
    }, rad, velocity, velocity, delta, true, true};
}
}

int main()
{
    using namespace latebrake;
    const auto plan = BuildWorldScalePacePlan();
    Check(plan.Valid, "actual 1.7km candidate road plan must be valid");
    WorldScaleCandidateRivalPilot pilot;
    const auto grid = OnRoad(5000.0, 0.0);
    const auto launch = pilot.Step(plan, grid);
    Check(launch.Valid && !launch.Command.Brake &&
          launch.Command.Throttle > 0.0 &&
          Near(launch.Command.Steering, 0.0, 0.001) &&
          launch.SpeedReferenceCmps > 3000.0,
          "on-track, aligned, stationary rival has safe forward launch");
    pilot.Reset();
    auto tooFast = OnRoad(5000.0, 7000.0);
    const auto limit = pilot.Step(plan, tooFast);
    Check(limit.Valid && limit.Command.Brake &&
          limit.Command.Throttle == 0.0 &&
          std::abs(limit.Command.Steering) <= 1.0,
          "real overspeed orders braking, no power");

    pilot.Reset();
    auto left = OnRoad(5000.0, 1000.0, 1.0/60.0, 200.0);
    auto turnRight = pilot.Step(plan, left);
    Check(turnRight.Valid && turnRight.SignedCrossTrackCm > 0.0 &&
          turnRight.Command.Steering < 0.0,
          "a car left of the straight steers back toward centre");
    pilot.Reset();
    auto right = OnRoad(5000.0, 1000.0, 1.0/60.0, -200.0);
    auto turnLeft = pilot.Step(plan, right);
    Check(turnLeft.Valid && turnLeft.SignedCrossTrackCm < 0.0 &&
          turnLeft.Command.Steering > 0.0,
          "a car right of the straight steers back toward centre");

    pilot.Reset();
    auto wrong = OnRoad(5000.0, 2200.0);
    wrong.HeadingRadians += WorldScalePilotRadiansPi;
    const auto wrongResult = pilot.Step(plan, wrong);
    Check(wrongResult.Valid && wrongResult.Command.Brake &&
          wrongResult.Command.Throttle == 0.0,
          "opposite-direction rival must brake, not drive down road");
    pilot.Reset();
    auto stationaryWrongWay = OnRoad(5000.0, 0.0);
    stationaryWrongWay.HeadingRadians += WorldScalePilotRadiansPi;
    const auto stationary = pilot.Step(plan, stationaryWrongWay);
    Check(stationary.Valid && stationary.Command.Brake &&
          stationary.Command.Throttle == 0.0,
          "stationary reverse-facing car must not launch backwards");
    pilot.Reset();
    auto reverse = OnRoad(5000.0, 700.0);
    reverse.ForwardSpeedCmps = -700.0;
    const auto reverseResult = pilot.Step(plan, reverse);
    Check(reverseResult.Valid && reverseResult.Command.Brake &&
          reverseResult.Command.Throttle == 0.0,
          "measured real reverse velocity cannot receive drive power");

    pilot.Reset();
    const auto first = pilot.Step(plan, OnRoad(5000.0, 2500.0));
    Check(first.Valid, "first physical sample accepted");
    auto jump = pilot.Step(plan, OnRoad(15500.0, 2500.0));
    Check(!jump.Valid && jump.Command.Brake &&
          jump.Command.Throttle == 0.0,
          "unverified 105m teleport cannot count as continuous travel");
    Check(pilot.Step(plan, OnRoad(15500.0, 2500.0)).Valid,
          "safe independent reacquisition after discontinuity");

    // True 1.7km start-finish seam is a continuous segment. A car may
    // legally pass it without being mistaken for a teleport.
    pilot.Reset();
    Check(pilot.Step(plan, OnRoad(WorldScaleCircuitLapCm - 95.0,
                                 2200.0, 0.1)).Valid,
          "near-seam car accepted");
    Check(pilot.Step(plan, OnRoad(45.0, 2200.0, 0.1)).Valid,
          "one physical across-seam forward step accepted");

    for (double fps : {30.0, 60.0, 120.0})
    {
        pilot.Reset();
        const double dt = 1.0 / fps;
        const double speed = 2200.0;
        const double start = WorldScaleCircuitLapCm - 300.0;
        for (unsigned frame = 0; frame < 300; ++frame)
        {
            const auto sample = OnRoad(start + frame * speed * dt, speed, dt);
            const auto out = pilot.Step(plan, sample);
            Check(out.Valid &&
                  std::isfinite(out.HeadingErrorRadians) &&
                  out.LookaheadCm >= WorldScalePilotMinLookaheadCm &&
                  out.LookaheadCm <= WorldScalePilotMaxLookaheadCm &&
                  out.SpeedReferenceCmps <= WorldScalePaceMaxSpeedCmps &&
                  out.Command.Throttle >= 0.0 &&
                  out.Command.Throttle <= 1.0 &&
                  out.Command.Steering >= -1.0 &&
                  out.Command.Steering <= 1.0 &&
                  (!out.Command.Brake || out.Command.Throttle == 0.0),
                  "30/60/120 FPS on-track future AI command stays bounded");
        }
    }

    const double bad = std::numeric_limits<double>::quiet_NaN();
    for (const auto invalid : {
        WorldScalePilotSample{},
        WorldScalePilotSample{grid.Position, bad, 1000, 1000, 1.0/60.0, true, true},
        WorldScalePilotSample{grid.Position, 0.0, bad, bad, 1.0/60.0, true, true},
        WorldScalePilotSample{grid.Position, 0.0, 1000, 1000, 0.2, true, true},
        WorldScalePilotSample{grid.Position, 0.0, 1000, 1000, -0.1, true, true},
        WorldScalePilotSample{grid.Position, 0.0, 1000, 1000, 1.0/60.0, false, true},
        WorldScalePilotSample{grid.Position, 0.0, 1000, 1000, 1.0/60.0, true, false},
        WorldScalePilotSample{{0.0, 0.0}, 0.0, 1000, 1000, 1.0/60.0, true, true},
        WorldScalePilotSample{grid.Position, 0.0, 150, 800, 1.0/60.0, true, true}
    })
    {
        const auto out = pilot.Step(plan, invalid);
        Check(!out.Valid && out.Command.Brake &&
              out.Command.Throttle == 0.0,
              "invalid physics, race phase or road denies all drive power");
    }
    Check(!pilot.Step(WorldScalePacePlan{}, grid).Valid,
          "unverified route plan fails closed");
    std::cout << "WorldScaleCandidateRivalPilot: 30/60/120 FPS, "
                 "real 1.7km seam, steering, overspeed, teleports and "
                 "invalid-physics safety PASS (source only).\n";
}
