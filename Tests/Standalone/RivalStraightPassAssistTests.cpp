#include "AI/RivalStraightPassAssist.h"

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
        std::cerr << "RivalStraightPassAssist FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalPassSample Observe(
    double rivalAlong = 550.0, double playerAhead = 1900.0,
    double rivalLateral = 0.0, double playerLateral = 0.0,
    double rivalSpeed = 2200.0, double playerSpeed = 1800.0,
    int gate = 0)
{
    using namespace latebrake;
    const auto corner = CircuitGate(gate);
    const auto prior = CircuitGate(
        (gate + CircuitGateCount - 1) % CircuitGateCount);
    const double dx = corner.X - prior.X;
    const double dy = corner.Y - prior.Y;
    const double length = std::hypot(dx, dy);
    const double ux = dx / length, uy = dy / length;
    const double nx = -uy, ny = ux;
    return {1.0 / 60.0, gate,
            {prior.X + ux * rivalAlong + nx * rivalLateral,
             prior.Y + uy * rivalAlong + ny * rivalLateral},
            {prior.X + ux * (rivalAlong + playerAhead) + nx * playerLateral,
             prior.Y + uy * (rivalAlong + playerAhead) + ny * playerLateral},
            std::atan2(uy, ux),
            rivalSpeed * ux, rivalSpeed * uy,
            playerSpeed * ux, playerSpeed * uy,
            true, true};
}
}

int main()
{
    using namespace latebrake;
    const PilotCommand driving{0.72, 0.0, false, false};
    RivalStraightPassAssist controller;

    // Only the long east/west straights have enough paved runway for
    // a real single-chassis lane change. On the two short north/south
    // straights, the old source-only pass was an unsafe late dive.
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        controller.Reset();
        const auto approach = Observe(550.0, 1900.0, 0.0, 0.0,
                                      2200.0, 1800.0, gate);
        const auto pass = controller.Apply(driving, approach);
        if (gate % 2 != 0)
        {
            Check(!pass.Passing && !controller.IsActive() &&
                  pass.Command.Steering == driving.Steering,
                  "short original straights cannot support safe passing runway");
            continue;
        }
        Check(pass.Passing && controller.IsActive() &&
              pass.Command.Steering < 0.0 &&
              pass.Command.Steering >= -0.65 &&
              pass.Command.Throttle == driving.Throttle &&
              !pass.Command.Brake &&
              pass.TargetLaneOffsetCm == -440.0,
              "long paved straights choose a bounded RIGHT passing lane");
        const auto shifted = controller.Apply(
            driving, Observe(640.0, 1800.0, -395.0, 0.0,
                             2200.0, 1800.0, gate));
        Check(shifted.Passing &&
              std::abs(shifted.Command.Steering) <
                  std::abs(pass.Command.Steering),
              "holding lane is smoother once actually shifted");
        const auto ahead = controller.Apply(
            driving, Observe(850.0, -600.0, -420.0, 0.0,
                             2200.0, 1800.0, gate));
        Check(!ahead.Passing && !controller.IsActive() &&
              ahead.Command.Steering == driving.Steering,
              "finished pass returns original gate steering after clearance");

        controller.Reset();
        const auto leftPass = controller.Apply(
            driving, Observe(550.0, 1900.0, 0.0, -120.0,
                             2200.0, 1800.0, gate));
        Check(leftPass.Passing && leftPass.Command.Steering > 0.0 &&
              leftPass.TargetLaneOffsetCm == 440.0,
              "player on right chooses left opposite passing lane");
    }

    controller.Reset();
    Check(!controller.Apply(driving, Observe(
        550.0, 1150.0, 0.0, 0.0, 2500.0, 1500.0)).Passing,
        "old close/high-relative-speed pass cannot activate into emergency brake");
    controller.Reset();
    Check(!controller.Apply(driving, Observe(
        550.0, 1750.0, 0.0, 0.0, 2600.0, 1500.0)).Passing,
        "large closing speed cannot finish lane shift before contact");
    controller.Reset();
    Check(controller.Apply(driving, Observe(
        550.0, 1900.0, 0.0, 0.0, 2200.0, 1800.0)).Passing,
        "well-separated lower-relative-speed start is still allowed");

    // No attempt around a turn, or when there is no safe track clearance.
    controller.Reset();
    auto tooCloseToCorner = Observe(4100.0);
    const auto corner = controller.Apply(driving, tooCloseToCorner);
    Check(!corner.Passing && corner.Command.Steering == 0.0,
          "no lane changing near original circuit turn");
    controller.Reset();
    Check(!controller.Apply(driving, Observe(550.0, 500.0)).Passing,
          "no dive into a nearby player before there is room to pass");
    controller.Reset();
    Check(!controller.Apply(driving, Observe(550.0, 2500.0)).Passing,
          "no invented long-range passing target");
    controller.Reset();
    Check(!controller.Apply(driving, Observe(550.0, 1900.0,
                                           0.0, 0.0, 1800.0, 1700.0)).Passing,
          "matched speeds never cause pointless lane weaving");
    controller.Reset();
    Check(!controller.Apply(
        {0.0, -0.4, true, false}, Observe()).Passing,
        "already braking for traffic or corner cannot start a pass");
    controller.Reset();
    const auto arrived = controller.Apply(
        {0.0, 0.0, false, true}, Observe());
    Check(!arrived.Passing && arrived.Command.Arrived,
          "gate-arrival authority cannot be transformed into passing");

    controller.Reset();
    const auto begun = controller.Apply(driving, Observe());
    Check(begun.Passing, "valid normal pass begins");
    const auto occupied = controller.Apply(
        driving, Observe(650.0, 950.0, -280.0, -450.0));
    Check(!occupied.Passing && occupied.Command.Brake &&
          occupied.Command.Throttle == 0.0 && !controller.IsActive(),
          "player moves into passing lane: yield instead of collide");

    controller.Reset();
    Check(controller.Apply(driving, Observe()).Passing, "pass latched");
    auto invalid = Observe();
    invalid.Qualified = false;
    const auto cancelled = controller.Apply(driving, invalid);
    Check(!cancelled.Passing && !controller.IsActive() &&
          cancelled.Command.Brake && cancelled.Command.Throttle == 0.0,
          "unqualified physics resets lane and fails closed");
    controller.Reset();
    Check(controller.Apply(driving, Observe()).Passing, "pass starts again");
    const auto changedGate = controller.Apply(
        driving, Observe(550.0, 1900.0, 0.0, 0.0, 2200.0, 1800.0, 1));
    Check(changedGate.Command.Throttle <= driving.Throttle,
          "new gate cannot preserve stale direction/extra propulsion");
    controller.Reset();
    auto longFrame = Observe();
    longFrame.DeltaSeconds = 0.25;
    const auto badFrame = controller.Apply(driving, longFrame);
    Check(!badFrame.Passing && badFrame.Command.Brake,
          "frame hitch blocks steering and motor and resets state");
    auto corrupt = Observe();
    corrupt.Player.X = std::numeric_limits<double>::quiet_NaN();
    Check(!controller.Apply(driving, corrupt).Passing &&
          controller.Apply(driving, corrupt).Command.Brake,
          "nonfinite player geometry fails closed");
    corrupt = Observe();
    corrupt.RivalHeadingRadians = std::numeric_limits<double>::infinity();
    Check(controller.Apply(driving, corrupt).Command.Brake,
          "nonfinite rival heading fails closed");
    corrupt = Observe();
    corrupt.NextGate = 99;
    Check(controller.Apply(driving, corrupt).Command.Brake,
          "invalid gate cannot choose off-track lane");

    // Thousands of monotone variations; no new acceleration, road writes,
    // route authority, NaNs, or unbounded steering permitted.
    for (int i = 0; i < 20000; ++i)
    {
        controller.Reset();
        const int gate = i % CircuitGateCount;
        const double playerOffset = static_cast<double>(i % 240) - 120.0;
        const double aheadDistance = 1200.0 + static_cast<double>(i % 1200);
        const auto o = controller.Apply(
            driving, Observe(250.0 + static_cast<double>(i % 850),
                             aheadDistance, 0.0, playerOffset,
                             2200.0 + (i % 100),
                             1700.0 + (i % 100), gate));
        Check(std::isfinite(o.Command.Steering) &&
              std::abs(o.Command.Steering) <= 0.65 + 1.0e-9 &&
              o.Command.Throttle >= 0.0 &&
              o.Command.Throttle <= driving.Throttle &&
              !o.Command.Arrived,
              "20k varied physical passing observations remain safe/bounded");
    }
    std::cout << "Rival paved-straight passing assists passed.\n";
    return EXIT_SUCCESS;
}
