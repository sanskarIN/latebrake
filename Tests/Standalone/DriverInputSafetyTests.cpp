#include "Vehicles/DriverInputSafety.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool pass, const char* description)
{
    if (!pass)
    {
        std::cerr << "Driver input test FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Check(SanitizeDriveAxis(2.0) == 1.0, "positive axis clamped");
    Check(SanitizeDriveAxis(-2.0) == -1.0, "negative axis clamped");
    Check(SanitizeDriveAxis(0.25) == 0.25, "normal axis preserved");
    Check(SanitizeDriveAxis(nan) == 0.0, "NaN axis neutral");
    Check(SanitizeDriveAxis(inf) == 0.0, "infinite axis neutral");
    Check(SanitizeTouchSteering(0.0, 1000.0) == -1.0, "left touch fully left");
    Check(SanitizeTouchSteering(250.0, 1000.0) == 0.0, "left touch center neutral");
    Check(SanitizeTouchSteering(500.0, 1000.0) == 1.0, "left touch right edge");
    Check(SanitizeTouchSteering(-100.0, 1000.0) == -1.0, "negative touch clamped");
    Check(SanitizeTouchSteering(500.0, 0.0) == 0.0, "zero viewport neutral");
    Check(SanitizeTouchSteering(nan, 1000.0) == 0.0, "invalid touch coordinate neutral");
    Check(SanitizeTouchSteering(100.0, inf) == 0.0, "invalid viewport neutral");
    Check(std::abs(SmoothThrottle(0.0, 1.0, 0.1) - 0.4) < 1e-9,
          "rise rate limited");
    Check(std::abs(SmoothThrottle(1.0, -1.0, 0.1) - 0.6) < 1e-9,
          "reverse direction rate limited");
    Check(SmoothThrottle(0.5, 1.0, 0.0) == 0.5, "pause does not change throttle");
    Check(SmoothThrottle(0.5, 1.0, nan) == 0.5, "invalid delta is inert");
    Check(SmoothThrottle(0.5, nan, 1.0) == 0.0, "corrupt input decays to neutral");
    Check(SmoothThrottle(0.5, 1.0, inf) == 0.5, "infinite delta inert");
    Check(SmoothThrottle(0.5, 1.0, std::numeric_limits<double>::max(),
                         std::numeric_limits<double>::max()) == 0.5,
          "overflowing time step inert");
    Check(SmoothThrottle(0.1, 1.0, 1.0) == 1.0, "long valid step reaches target");
    std::cout << "Driver input safety tests passed.\n";
    return EXIT_SUCCESS;
}
