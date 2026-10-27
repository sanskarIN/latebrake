#include "Vehicles/WheelVisualMotion.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* reason)
{
    if (!value)
    {
        std::cerr << "WheelVisualMotion: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1e-8)
{
    return std::abs(a - b) <= tolerance;
}
latebrake::WheelVisualSample Sample(
    double dt, double speed, double steer = 0.0, double radius = 29.0)
{
    return {dt, speed, steer, radius};
}
}

int main()
{
    using latebrake::WheelVisualMotion;
    WheelVisualMotion wheel;
    Require(Near(wheel.Current().RollDegrees, 0.0) &&
            Near(wheel.Current().FrontSteerDegrees, 0.0),
            "new vehicle has neutral visual wheels");

    constexpr double Pi = 3.14159265358979323846;
    const double QuarterTurnSpeed = 29.0 * (Pi / 2.0) / 0.1;
    Require(wheel.Advance(Sample(0.1, QuarterTurnSpeed, 0.5)),
            "forward chassis movement animates wheel");
    Require(Near(wheel.Current().RollDegrees, 90.0) &&
            Near(wheel.Current().FrontSteerDegrees, 15.0),
            "correct rolling radius and bounded visual steering angle");
    Require(wheel.Advance(Sample(0.1, -QuarterTurnSpeed, -0.5)),
            "real reverse motion is accepted");
    Require(Near(wheel.Current().RollDegrees, 0.0) &&
            Near(wheel.Current().FrontSteerDegrees, -15.0),
            "reverse rolls backward, not always forward");

    wheel.Reset();
    Require(wheel.Advance(Sample(0.1, 0.0, 1.0)) &&
            Near(wheel.Current().RollDegrees, 0.0) &&
            Near(wheel.Current().FrontSteerDegrees, 30.0),
            "parking wheel steering is visible even when stationary");
    wheel.Suspend();
    Require(Near(wheel.Current().FrontSteerDegrees, 0.0),
            "pause/focus loss neutralizes visual steering");
    Require(wheel.Advance(Sample(0.1, 0.0, -1.0)) &&
            Near(wheel.Current().FrontSteerDegrees, -30.0),
            "driver can select opposite steering on valid resumed frames");

    wheel.Reset();
    Require(wheel.Advance(Sample(0.1, QuarterTurnSpeed, 0.2)) &&
            Near(wheel.Current().RollDegrees, 90.0),
            "first wheel revolution segment");
    wheel.Suspend();
    Require(Near(wheel.Current().RollDegrees, 90.0),
            "suspending does not pop decorative wheel phase");
    Require(wheel.Advance(Sample(0.1, QuarterTurnSpeed, 0.0)) &&
            Near(std::abs(wheel.Current().RollDegrees), 180.0),
            "after resume only measured new frame contributes rotation");
    wheel.Reset();
    Require(Near(wheel.Current().RollDegrees, 0.0) &&
            Near(wheel.Current().FrontSteerDegrees, 0.0),
            "teleport/restart removes phase and steering history");

    const double bad = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const latebrake::WheelVisualSample invalid[] = {
        Sample(0.0, 10.0), Sample(-0.1, 10.0),
        Sample(0.251, 10.0), Sample(bad, 10.0),
        Sample(0.1, bad), Sample(0.1, inf),
        Sample(0.1, 15000.01), Sample(0.1, -15000.01),
        Sample(0.1, 100.0, bad), Sample(0.1, 100.0, 1.01),
        Sample(0.1, 100.0, -1.01),
        Sample(0.1, 100.0, 0.5, bad),
        Sample(0.1, 100.0, 0.5, 4.99),
        Sample(0.1, 100.0, 0.5, 100.01),
    };
    for (const auto& sample : invalid)
    {
        wheel.Reset();
        Require(wheel.Advance(Sample(0.1, QuarterTurnSpeed, 1.0)),
                "arm valid pose before malicious physics sample");
        const double previous = wheel.Current().RollDegrees;
        Require(!wheel.Advance(sample),
                "invalid timing/velocity/steering/radius rejected");
        Require(Near(wheel.Current().RollDegrees, previous) &&
                Near(wheel.Current().FrontSteerDegrees, 0.0),
                "invalid frame does not animate or persist steering");
    }

    wheel.Reset();
    Require(wheel.Advance(Sample(0.25, 10000.0, 1.0, 5.0)) &&
            std::isfinite(wheel.Current().RollDegrees) &&
            std::abs(wheel.Current().RollDegrees) <= 180.0 &&
            Near(wheel.Current().FrontSteerDegrees, 30.0),
            "maximum allowed finite speed/radius/time remains bounded");

    wheel.Reset();
    for (int n = 0; n < 100000; ++n)
    {
        const double signedSpeed = n % 2 == 0 ? 1000.0 : -1000.0;
        Require(wheel.Advance(Sample(1.0 / 60.0, signedSpeed, 0.25)),
                "long session samples remain valid");
        Require(std::isfinite(wheel.Current().RollDegrees) &&
                std::abs(wheel.Current().RollDegrees) <= 180.0,
                "rolling phase stays bounded for long sessions");
    }
    Require(Near(wheel.Current().RollDegrees, 0.0, 1e-6),
            "equal forward/reverse frame distances cancel");
    std::cout << "Wheel visuals source policy passed.\n";
    return EXIT_SUCCESS;
}
