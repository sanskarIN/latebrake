#include "AI/RivalSpinThrottleGuard.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool success, const char* message)
{
    if (!success)
    {
        std::cerr << "RivalSpinThrottleGuard FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RivalSpinThrottleGuard guard;
    const PilotCommand drive{0.85, -0.6, false, false};
    const RivalSpinObservation stable{0.04, 2400, 100, 0.15, true};
    const RivalSpinObservation slide{0.04, 2400, 650, -1.25, true};
    const RivalSpinObservation spin{0.04, 2400, -1150, 2.25, true};

    auto result = guard.Apply(drive, stable);
    Check(result.Throttle == drive.Throttle &&
          result.Steering == drive.Steering && !result.Brake &&
          guard.RemainingHoldSeconds() == 0.0,
          "ordinary straight driving is unchanged");

    result = guard.Apply(drive, slide);
    Check(result.Throttle == 0.30 && result.Steering == drive.Steering &&
          !result.Brake && !result.Arrived &&
          guard.RemainingHoldSeconds() == RivalSpinThrottleGuard::HoldSeconds,
          "measured lateral slide caps throttle without applying brakes");
    result = guard.Apply({0.10, 0.3, false, false}, slide);
    Check(result.Throttle == 0.10 && result.Steering == 0.3,
          "guard never increases propulsion or changes steering");

    result = guard.Apply(drive, spin);
    Check(result.Throttle == 0.0 && result.Steering == drive.Steering &&
          !result.Brake && guard.RemainingHoldSeconds() > 0.0,
          "severe physical spin cuts propulsion without hard braking");
    for (int frame = 0; frame < 3; ++frame)
    {
        result = guard.Apply(drive, stable);
        Check(result.Throttle == 0.0,
              "severe spin hold prevents brief sensor chatter");
    }
    result = guard.Apply(drive, stable);
    Check(result.Throttle == drive.Throttle &&
          guard.RemainingHoldSeconds() == 0.0,
          "bounded hold expires so rival can accelerate again");

    guard.Reset();
    result = guard.Apply(drive, {0.04, 2400, 1100, 0.4, true});
    Check(result.Throttle == drive.Throttle,
          "lateral drift without real yaw rate cannot fake a spin");
    result = guard.Apply(drive, {0.04, 2400, 100, 2.5, true});
    Check(result.Throttle == drive.Throttle,
          "yaw alone cannot trigger a spin without measured slip");
    result = guard.Apply(drive, {0.04, 500, 300, 3.0, true});
    Check(result.Throttle == drive.Throttle,
          "low-speed maneuvering retains driver authority");

    guard.Apply(drive, spin);
    const PilotCommand priorBrake{0.0, -0.25, true, false};
    result = guard.Apply(priorBrake, stable);
    Check(result.Brake && result.Throttle == 0.0 &&
          result.Steering == priorBrake.Steering,
          "existing traffic or corner brakes remain authoritative");
    result = guard.Apply({0.0, -0.4, false, true}, stable);
    Check(result.Arrived && result.Steering == -0.4 &&
          guard.RemainingHoldSeconds() == 0.0,
          "arrived decision clears old spin state");

    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity()})
    {
        guard.Apply(drive, spin);
        result = guard.Apply(drive, {0.04, 2400, 700, invalid, true});
        Check(result.Throttle == 0.0 && result.Steering == 0.0 &&
              result.Brake && guard.RemainingHoldSeconds() == 0.0,
              "nonfinite yaw fails closed and resets state");
        result = guard.Apply(drive, {0.04, 2400, invalid, 2.0, true});
        Check(result.Brake && result.Throttle == 0.0,
              "nonfinite lateral speed fails closed");
        result = guard.Apply(drive, {invalid, 2400, 700, 2.0, true});
        Check(result.Brake && result.Throttle == 0.0,
              "nonfinite frame time fails closed");
    }
    for (const auto invalid : {
             RivalSpinObservation{0.15, 2400, 700, 2.0, true},
             RivalSpinObservation{0.0, 2400, 700, 2.0, true},
             RivalSpinObservation{0.04, -10, 0, 2.0, true},
             RivalSpinObservation{0.04, 2400, 2800, 2.0, true},
             RivalSpinObservation{0.04, 2400, 700, 12.0, true},
             RivalSpinObservation{0.04, 2400, 700, 2.0, false}})
    {
        result = guard.Apply(drive, invalid);
        Check(result.Throttle == 0.0 && result.Steering == 0.0 &&
              result.Brake && guard.RemainingHoldSeconds() == 0.0,
              "unqualified physics never authorizes motor power");
    }
    result = guard.Apply({1.2, 0.2, false, false}, stable);
    Check(result.Throttle == 0.0 && result.Brake,
          "invalid upstream motor demand fails closed");

    // Stress pure numerical composition, including fresh races and
    // throttle/steering authority over deterministic finite samples.
    for (int frame = 0; frame < 250000; ++frame)
    {
        if (frame % 971 == 0) guard.Reset();
        const double speed = 250.0 + (frame % 97) * 70.0;
        const double lateral = speed * ((frame % 53) / 52.0);
        const double yaw = ((frame % 47) - 23) * 0.105;
        const double throttle = (frame % 17) / 16.0;
        const double steering = ((frame % 21) - 10) / 10.0;
        const PilotCommand input{throttle, steering, false, false};
        const auto output = guard.Apply(
            input, {0.01 + (frame % 4) * 0.01, speed, lateral, yaw, true});
        Check(std::isfinite(output.Throttle) &&
              output.Throttle >= 0.0 && output.Throttle <= input.Throttle &&
              output.Steering == input.Steering && !output.Brake &&
              !output.Arrived &&
              guard.RemainingHoldSeconds() >= 0.0 &&
              guard.RemainingHoldSeconds() <= RivalSpinThrottleGuard::HoldSeconds,
              "deterministic spin policy never adds motor, steering, brake or lap authority");
    }
    std::cout << "Rival spin throttle guard passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
