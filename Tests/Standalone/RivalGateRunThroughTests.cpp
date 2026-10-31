#include "AI/RivalGateRunThrough.h"
#include "AI/RivalPilot.h"
#include "Racing/CheckpointMotionTrace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "RivalGateRunThrough FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double x, double y) { return std::abs(x - y) < 1e-9; }
latebrake::CircuitPoint Inbound(int gate, double offset)
{
    const auto p = latebrake::CircuitGate(gate);
    switch (gate)
    {
    case 0: return {p.X + offset, p.Y};
    case 1: return {p.X, p.Y + offset};
    case 2: return {p.X - offset, p.Y};
    default: return {p.X, p.Y - offset};
    }
}
double InboundHeading(int gate)
{
    return static_cast<double>(gate) * std::numbers::pi / 2.0;
}
latebrake::PilotCommand Plan(int gate, latebrake::CircuitPoint p,
                            double speed)
{
    const auto target = latebrake::RivalGateRunThroughPoint(gate);
    return latebrake::PlanRivalTurn(
        p.X, p.Y, InboundHeading(gate), speed,
        target.X, target.Y, 440.0, false);
}
} // namespace

int main()
{
    using namespace latebrake;

    // The default generic planner contract is preserved. A physical rival
    // MUST use the explicit opt-out; only the gate trace may award arrival.
    const auto ordinary = PlanRivalTurn(0, 0, 0, 600, 100, 0);
    Check(ordinary.Arrived && ordinary.Throttle == 0.0,
          "original distance-only waypoint caller still declares arrival");
    const auto optOut = PlanRivalTurn(
        0, 0, 0, 600, 100, 0, 440.0, false);
    Check(!optOut.Arrived && optOut.Throttle > 0.0,
          "real gate planner never stops simply because it is within 440cm");
    Check(PlanRivalTurn(0, 0, 0, 600, 100, 0, 440.0, true).Arrived,
          "explicitly enabled generic arrival keeps old behavior");

    Check(!IsCircuitGate(-1) && !IsCircuitGate(4),
          "invalid next-gate index rejected by real rival readiness");
    for (int gate = 0; gate < 4; ++gate)
    {
        const auto center = CircuitGate(gate);
        const auto aim = RivalGateRunThroughPoint(gate);
        const auto direction = Inbound(gate, RivalGateRunThroughDistanceCm);
        Check(Close(aim.X, direction.X) && Close(aim.Y, direction.Y),
              "run-through aim is exactly 550cm beyond the real gate");
        Check(IsOnPavedSurface(aim),
              "run-through aim must be on original circuit pavement");
        Check(Close(std::hypot(aim.X - center.X, aim.Y - center.Y), 550.0),
              "aim offset uses actual incoming gate direction");

        // Before the gate, at the plane, and beyond it but still inside
        // the former 440cm arrival range: none is an AI arrival event.
        for (double along : {-220.0, 0.0, 180.0, 410.0, 530.0})
        {
            const auto car = Inbound(gate, along);
            const auto cmd = Plan(gate, car, 600);
            Check(!cmd.Arrived && !cmd.Brake && cmd.Throttle > 0.0 &&
                  std::isfinite(cmd.Steering),
                  "actual AI cannot deadlock near a real unverified plane");
        }

        // A verified forward crossing comes from measured vehicle motion,
        // not a planner proximity or a fabricated arrival flag.
        CheckpointMotionTrace trace;
        const auto from = Inbound(gate, -180.0);
        const auto to = Inbound(gate, 150.0);
        const CircuitGateSample prior{{from.X, from.Y}, 180.0};
        const CircuitGateSample next{{to.X, to.Y}, 180.0};
        const auto directionX = gate == 0 ? 1600.0 : gate == 2 ? -1600.0 : 0.0;
        const auto directionY = gate == 1 ? 1600.0 : gate == 3 ? -1600.0 : 0.0;
        Check(!trace.Sample(gate, 0, prior, directionX, directionY),
              "first physical sample cannot award an AI checkpoint");
        Check(trace.Sample(gate, 0, next, directionX, directionY),
              "real forward plane sweep is accepted for each gate");
        Check(!Plan(gate, to, 600).Arrived,
              "same frame is not misread as planner-arrived on the far side");

        const auto fast = Plan(gate, Inbound(gate, -220.0), 4700);
        Check(!fast.Arrived && fast.Throttle == 0.0 && fast.Brake,
              "fast approach keeps existing braking without false arrival");

        const auto far = Plan(gate, Inbound(gate, -1800.0), 1200.0);
        Check(!far.Arrived && !far.Brake && far.Throttle > 0.0,
              "ordinary straight approach maintains old throttle planning");
    }

    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (double bad : {nan, std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        const auto rejected = PlanRivalTurn(0, 0, bad, 600, 100, 0,
                                           440.0, false);
        Check(rejected.Brake && rejected.Throttle == 0.0,
              "corrupt heading still fails closed when arrival is disabled");
        const auto badPosition = PlanRivalTurn(bad, 0, 0, 600, 100, 0,
                                               440.0, false);
        Check(badPosition.Brake && badPosition.Throttle == 0.0,
              "corrupt car position cannot authorize run-through drive");
        const auto badSpeed = PlanRivalTurn(0, 0, 0, bad, 100, 0,
                                           440.0, false);
        Check(badSpeed.Brake && badSpeed.Throttle == 0.0,
              "corrupt velocity cannot authorize run-through drive");
    }
    Check(PlanRivalTurn(0, 0, 0, 600, 100, 0, -1.0, false).Brake,
          "invalid arrival-radius parameter remains rejected");
    Check(PlanRivalTurn(0, 0, 0, 600, 100, 0, 0.0, false).Brake,
          "zero arrival-radius parameter remains rejected");

    // 200000 deterministic actual paved approach samples, including the
    // entire old proximity-stop window and higher corner speeds.
    for (int i = 0; i < 200000; ++i)
    {
        const int gate = i % 4;
        const double offset = static_cast<double>((i * 37) % 1601) - 700.0;
        const double sideways = static_cast<double>((i * 17) % 801) - 400.0;
        auto position = Inbound(gate, offset);
        if (gate == 0 || gate == 2) position.Y += sideways;
        else position.X += sideways;
        const double speed = 100.0 + static_cast<double>((i * 29) % 4800);
        const auto planned = Plan(gate, position, speed);
        Check(!planned.Arrived && std::isfinite(planned.Throttle) &&
              std::isfinite(planned.Steering) &&
              planned.Throttle >= 0.0 && planned.Throttle <= 0.85 &&
              std::abs(planned.Steering) <= 1.0 &&
              (!planned.Brake || planned.Throttle == 0.0),
              "200000 real nearby racing samples never stop by proximity");
    }

    std::cout << "RivalGateRunThrough: 200000 samples passed (not UE)\n";
    return EXIT_SUCCESS;
}
