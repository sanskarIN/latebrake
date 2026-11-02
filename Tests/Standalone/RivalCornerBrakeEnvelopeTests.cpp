#include "AI/RivalCornerBrakeEnvelope.h"
#include "Vehicles/GripLimitedBraking.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "RivalCornerBrakeEnvelope FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::CircuitPoint Approach(int gate, double distance,
                                 double laneOffset = 0.0)
{
    using namespace latebrake;
    const auto corner = CircuitGate(gate);
    const auto previous = CircuitGate(
        (gate + CircuitGateCount - 1) % CircuitGateCount);
    const double dx = corner.X - previous.X;
    const double dy = corner.Y - previous.Y;
    const double length = std::hypot(dx, dy);
    return {corner.X - dx / length * distance - dy / length * laneOffset,
            corner.Y - dy / length * distance + dx / length * laneOffset};
}
latebrake::RivalCornerBrakeSample Sample(int gate, double distance,
                                         double speed, double grip = 1.0,
                                         double heat = 1.0)
{
    return {gate, Approach(gate, distance), speed, grip, 1.0, 1.0,
            heat, true, true};
}
}

int main()
{
    using namespace latebrake;
    const PilotCommand driving{0.85, 0.4, false, false};
    static_assert(RivalBrakeForecastSafetyScale < 1.0);
    static_assert(RivalEarlyBrakeMaximumApproachCm > 2200.0);
    static_assert(RivalCornerExitSpeedCmPerSecond > 1000.0);

    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const auto fast = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, 3650.0, 4200.0));
        Check(fast.Valid && fast.InboundCorner &&
              fast.AnticipatoryBrake && fast.Command.Brake &&
              fast.Command.Throttle == 0.0 &&
              fast.Command.Steering == driving.Steering,
              "all four turns use speed/distance braking well before 2200cm");
        const auto moderate = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, 3650.0, 1800.0));
        Check(moderate.Valid && moderate.InboundCorner &&
              !moderate.Command.Brake &&
              moderate.Command.Throttle == driving.Throttle,
              "an already slow car must not brake prematurely");
        const auto worn = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, 3650.0, 2900.0, 0.50, 0.75));
        const auto clean = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, 3650.0, 2900.0));
        Check(worn.Valid && clean.Valid &&
              worn.BrakeEnvelopeSpeedCmPerSecond <
                  clean.BrakeEnvelopeSpeedCmPerSecond &&
              worn.SafeDecelerationCmPerSecondSquared <
                  clean.SafeDecelerationCmPerSecondSquared &&
              worn.AnticipatoryBrake && !clean.AnticipatoryBrake,
              "worn/offroad/heated brakes must start reducing speed sooner");
        const auto closer = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, 1500.0, 2800.0));
        Check(closer.Valid && closer.InboundCorner &&
              closer.BrakeEnvelopeSpeedCmPerSecond <
                  clean.BrakeEnvelopeSpeedCmPerSecond,
              "safe corner entry speed decreases smoothly as gate approaches");
        auto far = Sample(gate, 5700.0, 9000.0);
        const auto farAway = ApplyRivalCornerBrakeEnvelope(driving, far);
        Check(farAway.Valid && !farAway.InboundCorner &&
              !farAway.Command.Brake && farAway.Command.Throttle == 0.85,
              "out-of-window target never invents early braking");
        auto past = Sample(gate, 700.0, 4000.0);
        past.Position = Approach(gate, -700.0);
        const auto passed = ApplyRivalCornerBrakeEnvelope(driving, past);
        Check(passed.Valid && !passed.InboundCorner &&
              passed.Command.Throttle == driving.Throttle,
              "already crossed gate never treated as inbound");
        auto offLane = Sample(gate, 2700.0, 4000.0);
        offLane.Position = Approach(gate, 2700.0, 750.0);
        const auto lane = ApplyRivalCornerBrakeEnvelope(driving, offLane);
        Check(lane.Valid && !lane.InboundCorner && !lane.Command.Brake,
              "off-center shoulder never invents a gate-braking authority");
        const auto keepingBrake = ApplyRivalCornerBrakeEnvelope(
            {0.0, -0.7, true, false}, Sample(gate, 3650.0, 4000.0));
        Check(keepingBrake.Valid && keepingBrake.Command.Brake &&
              keepingBrake.Command.Steering == -0.7 &&
              keepingBrake.Command.Throttle == 0.0,
              "existing planner brake decisions are never undone");
        const auto arrived = ApplyRivalCornerBrakeEnvelope(
            {0.0, 0.0, false, true}, Sample(gate, 550.0, 1000.0));
        Check(arrived.Valid && arrived.Command.Arrived && !arrived.Command.Brake,
              "actual gate arrival never overwritten");
    }

    const auto service = CalculateGripLimitedBraking({
        0.016, 3500.0, 1100.0, true, false, true,
        0.72, 0.90, 0.95, 0.80
    });
    const auto forecast = ApplyRivalCornerBrakeEnvelope(driving,
        {0, Approach(0, 3000.0), 3500.0,
         0.72, 0.90, 0.95, 0.80, true, true});
    Check(service.Valid && forecast.Valid &&
          forecast.SafeDecelerationCmPerSecondSquared <
              service.DecelerationCmPerSecondSquared &&
          forecast.SafeDecelerationCmPerSecondSquared > 0.0,
          "brake forecast has real shared physics source and safe reserve");
    auto invalid = Sample(0, 3000.0, 3500.0);
    invalid.PriorPhysicsQualified = false;
    Check(!ApplyRivalCornerBrakeEnvelope(driving, invalid).Valid &&
          ApplyRivalCornerBrakeEnvelope(driving, invalid).Command.Brake,
          "invalid preceding actual Chaos sample fails to safe brake");
    invalid = Sample(0, 3000.0, 3500.0);
    invalid.NextGate = 12;
    Check(!ApplyRivalCornerBrakeEnvelope(driving, invalid).Valid,
          "nonexistent gates must never authorize motor acceleration");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double bad : {nan, inf, -inf})
    {
        for (int field = 0; field < 6; ++field)
        {
            auto s = Sample(0, 3000.0, 3500.0);
            switch (field)
            {
            case 0: s.Position.X = bad; break;
            case 1: s.PlanarSpeedCmPerSecond = bad; break;
            case 2: s.SurfaceGrip = bad; break;
            case 3: s.TireGrip = bad; break;
            case 4: s.ImpactGrip = bad; break;
            default: s.BrakeHeatMultiplier = bad; break;
            }
            const auto result = ApplyRivalCornerBrakeEnvelope(driving, s);
            Check(!result.Valid && result.Command.Brake &&
                  result.Command.Throttle == 0.0,
                  "bad measured chassis/brake fields fail closed");
        }
    }

    for (int i = 0; i < 20000; ++i)
    {
        const int gate = i % CircuitGateCount;
        const double remaining = 440.0 + static_cast<double>(i % 5001);
        const double speed = static_cast<double>((i * 37) % 10001);
        const double grip = 0.40 + static_cast<double>(i % 100) / 125.0;
        const auto result = ApplyRivalCornerBrakeEnvelope(
            driving, Sample(gate, remaining, speed, grip, 0.75));
        Check(result.Valid &&
              result.Command.Throttle >= 0.0 &&
              result.Command.Throttle <= driving.Throttle &&
              result.Command.Steering == driving.Steering &&
              result.BrakeEnvelopeSpeedCmPerSecond >= 0.0 &&
              std::isfinite(result.SafeDecelerationCmPerSecondSquared),
              "20k geometry/speed/grip trials remain bounded and steer-free");
        if (result.AnticipatoryBrake)
        {
            Check(result.Command.Brake && result.Command.Throttle == 0.0,
                  "early brake request always removes motor throttle");
        }
    }
    std::cout << "Rival real-brake distance forecast passed.\n";
    return EXIT_SUCCESS;
}
