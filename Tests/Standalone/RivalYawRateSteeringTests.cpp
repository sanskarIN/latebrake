#include "AI/RivalYawRateSteering.h"

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
        std::cerr << "RivalYawRateSteering FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Close(double a, double b, double tolerance = 1e-10)
{
    return std::abs(a - b) <= tolerance;
}

latebrake::RivalYawRateSample Sample(double speed, double yaw)
{
    return {0.05, speed, yaw, true};
}
} // namespace

int main()
{
    using latebrake::ModerateRivalYawRate;
    using latebrake::PilotCommand;
    const PilotCommand turnLeft{0.85, 1.0, false, false};
    const PilotCommand turnRight{0.40, -0.8, false, false};

    // Actual spinning in the REQUESTED turn direction can overshoot the
    // planned waypoint; never invert the request or invent countersteer.
    auto left = ModerateRivalYawRate(turnLeft, Sample(4000.0, 2.0));
    Require(Close(left.Steering, 0.35) &&
            Close(left.Throttle, turnLeft.Throttle) &&
            !left.Brake && !left.Arrived,
            "fast same-direction left yaw removes at most 65% steering");
    auto right = ModerateRivalYawRate(turnRight, Sample(4000.0, -2.0));
    Require(Close(right.Steering, -0.8 * 0.35) &&
            Close(right.Throttle, turnRight.Throttle),
            "fast right yaw is symmetrically damped without inversion");

    // The driver still needs correction when yaw is opposite the requested
    // waypoint turn. Neutral input must never create unwanted torque.
    left = ModerateRivalYawRate(turnLeft, Sample(4000.0, -2.0));
    Require(Close(left.Steering, 1.0),
            "opposing spin preserves full correcting left authority");
    right = ModerateRivalYawRate(turnRight, Sample(4000.0, 2.0));
    Require(Close(right.Steering, -0.8),
            "opposing spin preserves full correcting right authority");
    const PilotCommand neutral{0.5, 0.0, false, false};
    Require(Close(ModerateRivalYawRate(neutral, Sample(3500.0, 2.0)).Steering,
                  0.0),
            "neutral rival steering cannot invent countersteer");

    for (double yaw : {0.0, 0.15, 0.30})
    {
        Require(Close(ModerateRivalYawRate(turnLeft, Sample(4000.0, yaw))
                      .Steering, 1.0),
                "ordinary chassis yaw retains full steering");
    }
    for (double speed : {0.0, 500.0, 950.0})
    {
        Require(Close(ModerateRivalYawRate(turnLeft, Sample(speed, 3.0))
                      .Steering, 1.0),
                "launch, crawling and parking preserve full steering");
    }
    Require(Close(ModerateRivalYawRate(turnLeft, Sample(2050.0, 0.9))
                  .Steering, 1.0 - 0.65 * 0.5 * 0.5),
            "mid-speed, mid-yaw modulation is continuous and proportional");

    const PilotCommand braking{0.0, 0.75, true, false};
    const auto brakingTurn = ModerateRivalYawRate(
        braking, Sample(3500.0, 1.75));
    Require(brakingTurn.Brake && brakingTurn.Throttle == 0.0 &&
            brakingTurn.Steering > 0.0 &&
            brakingTurn.Steering < braking.Steering,
            "prior corner/traffic braking is never canceled");
    const PilotCommand arrived{0.0, -0.75, false, true};
    const auto arrivedTurn = ModerateRivalYawRate(
        arrived, Sample(3600.0, -2.0));
    Require(arrivedTurn.Arrived && arrivedTurn.Steering == arrived.Steering,
            "route arrival is preserved without new steering changes");

    // Source policy takes actual engine-measured yaw, not computed
    // delta-heading, so corrupt input should fail closed before force.
    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity()})
    {
        for (auto bad : {
                latebrake::RivalYawRateSample{0.05, 2000.0, invalid, true},
                latebrake::RivalYawRateSample{0.05, invalid, 0.4, true},
                latebrake::RivalYawRateSample{invalid, 2000.0, 0.4, true}
            })
        {
            const auto out = ModerateRivalYawRate(turnLeft, bad);
            Require(out.Brake && out.Throttle == 0.0 &&
                    out.Steering == 0.0,
                    "invalid finite physics sample denies rival drive");
        }
    }
    for (auto bad : {
            latebrake::RivalYawRateSample{0.0, 2200.0, 0.5, true},
            latebrake::RivalYawRateSample{0.11, 2200.0, 0.5, true},
            latebrake::RivalYawRateSample{0.05, -1.0, 0.5, true},
            latebrake::RivalYawRateSample{0.05, 10001.0, 0.5, true},
            latebrake::RivalYawRateSample{0.05, 2400.0, 10.01, true},
            latebrake::RivalYawRateSample{0.05, 2400.0, 0.5, false}
        })
    {
        const auto out = ModerateRivalYawRate(turnLeft, bad);
        Require(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0,
                "unqualified frame/speed/yaw cannot authorize motor or steering");
    }
    for (auto malformed : {
            PilotCommand{1.1, 0.8, false, false},
            PilotCommand{-0.1, 0.8, false, false},
            PilotCommand{0.5, 1.1, false, false},
            PilotCommand{0.5, -1.1, false, false},
            PilotCommand{std::numeric_limits<double>::quiet_NaN(),
                         0.8, false, false},
            PilotCommand{0.5, std::numeric_limits<double>::infinity(),
                         false, false}
        })
    {
        const auto out = ModerateRivalYawRate(
            malformed, Sample(2200.0, 0.7));
        Require(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0,
                "malformed pilot commands cannot authorize any drive");
    }

    // Boundary curves cannot increase steering when yaw or speed rises.
    for (bool leftSide : {false, true})
    {
        double previous = 1.0;
        for (int i = 0; i <= 10000; ++i)
        {
            const double yaw = static_cast<double>(i) * 0.0002;
            const auto out = ModerateRivalYawRate(
                leftSide ? turnLeft : turnRight,
                Sample(4500.0, leftSide ? yaw : -yaw));
            const double magnitude = std::abs(out.Steering);
            Require(magnitude >= 0.35 - 1e-12 &&
                    magnitude <= previous + 1e-12,
                    "same-direction steering attenuation is bounded/monotone");
            previous = magnitude;
        }
    }

    // 200,000 deterministic samples preserve every already-decided
    // brake/throttle/arrival outcome while changing at most steering size.
    for (int i = 0; i < 200000; ++i)
    {
        const double speed = static_cast<double>((i * 37) % 10001);
        const double yaw = static_cast<double>((i % 801) - 400) * 0.0125;
        const double steer = static_cast<double>((i % 1999) - 999) / 999.0;
        const bool brake = (i % 7) == 0;
        const PilotCommand original{
            brake ? 0.0 : static_cast<double>(i % 101) / 100.0,
            steer, brake, (i % 397) == 0
        };
        const auto out = ModerateRivalYawRate(
            original, {0.05, speed, yaw, true});
        Require(std::isfinite(out.Steering) &&
                out.Throttle == original.Throttle &&
                out.Brake == original.Brake &&
                out.Arrived == original.Arrived &&
                std::abs(out.Steering) <=
                    std::abs(original.Steering) + 1e-12 &&
                out.Steering * original.Steering >= 0.0,
                "200000 real-sample yaw decisions cannot boost or reverse controls");
    }

    std::cout << "RivalYawRateSteering: 200000 bounded samples passed (no UE)\n";
    return EXIT_SUCCESS;
}
