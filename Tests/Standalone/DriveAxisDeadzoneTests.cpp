#include "Vehicles/DriveAxisDeadzone.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Expect(bool okay, const char* message)
{
    if (!okay)
    {
        std::cerr << "DriveAxisDeadzone FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1e-12;
}
}

int main()
{
    using namespace latebrake;
    Expect(Near(FilterHumanSteeringAxis(0.0), 0.0) &&
           Near(FilterHumanThrottleAxis(0.0), 0.0),
           "resting keyboard/gamepad axes produce zero movement");
    for (double x : {-0.12, -0.08, 0.08, 0.12})
    {
        Expect(Near(FilterHumanSteeringAxis(x), 0.0),
               "small positive and negative left-stick noise ignored");
    }
    for (double x : {-0.055, -0.02, 0.02, 0.055})
    {
        Expect(Near(FilterHumanThrottleAxis(x), 0.0),
               "small signed trigger jitter ignored");
    }
    Expect(Near(FilterHumanSteeringAxis(1.0), 1.0) &&
           Near(FilterHumanSteeringAxis(-1.0), -1.0) &&
           Near(FilterHumanThrottleAxis(1.0), 1.0) &&
           Near(FilterHumanThrottleAxis(-1.0), -1.0),
           "A/D, W/S and full analog endpoints retain full authority");
    Expect(Near(FilterHumanSteeringAxis(0.56), 0.5) &&
           Near(FilterHumanSteeringAxis(-0.56), -0.5),
           "beyond steering deadzone remaining travel is rescaled");
    Expect(Near(FilterHumanThrottleAxis(0.5275), 0.5) &&
           Near(FilterHumanThrottleAxis(-0.5275), -0.5),
           "throttle/reverse trigger travel rescaled symmetrically");
    Expect(Near(RescaleDriveAxisDeadzone(0.5, 0.0), 0.5),
           "zero threshold leaves valid input unchanged");
    Expect(Near(FilterHumanSteeringAxis(900.0), 1.0) &&
           Near(FilterHumanThrottleAxis(-900.0), -1.0),
           "out-of-range axes clamped before deadzone math");

    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (double invalid : {inf, -inf, nan})
    {
        Expect(Near(FilterHumanSteeringAxis(invalid), 0.0) &&
               Near(FilterHumanThrottleAxis(invalid), 0.0),
               "malformed input does not accelerate/steer");
        Expect(Near(RescaleDriveAxisDeadzone(0.8, invalid), 0.0),
               "malformed configuration fails closed");
    }
    for (double invalid : {-1.0, 1.0, 1.25})
    {
        Expect(Near(RescaleDriveAxisDeadzone(0.8, invalid), 0.0),
               "negative or full-span deadzone fails closed");
    }
    double prior = 0.0;
    for (int i = 0; i <= 1000; ++i)
    {
        const double raw = static_cast<double>(i) / 1000.0;
        const double positive = FilterHumanSteeringAxis(raw);
        const double negative = FilterHumanSteeringAxis(-raw);
        Expect(std::isfinite(positive) && positive >= prior &&
               positive >= 0.0 && positive <= 1.0 &&
               Near(negative, -positive),
               "dense steering sweep is bounded, smooth and symmetric");
        prior = positive;
    }
    Expect(Near(SanitizeDriveAxis(0.08), 0.08),
           "underlying autonomous/touch sanitizer remains unchanged");
    std::cout << "Analog drive axis deadzone policy passed.\n";
    return EXIT_SUCCESS;
}
