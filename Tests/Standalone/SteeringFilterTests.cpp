#include "Vehicles/SteeringFilter.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Check(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "Steering FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using latebrake::SmoothSteering;
    Check(std::abs(SmoothSteering(0.0, 1.0, 0.1, 3.0) - 0.3) < 1e-9,
          "limits steering rise");
    Check(std::abs(SmoothSteering(1.0, -1.0, 0.1, 2.0) - 0.8) < 1e-9,
          "limits sharp steering reversal");
    Check(SmoothSteering(0.5, 0.6, 1.0, 1.0) == 0.6, "settles at target");
    Check(SmoothSteering(0.5, 1.0, 0.0, 3.0) == 0.5, "ignores paused delta");
    Check(SmoothSteering(0.5, 1.0, -1.0, 3.0) == 0.5, "ignores negative delta");
    Check(SmoothSteering(0.5, 1.0, 1.0, -1.0) == 0.5, "rejects invalid rate");
    Check(SmoothSteering(0.0, 2.0, 1.0, 10.0) == 1.0, "clamps requested value");
    Check(SmoothSteering(0.0, -2.0, 1.0, 10.0) == -1.0, "clamps negative value");
    Check(SmoothSteering(0.2, std::numeric_limits<double>::quiet_NaN(), 1.0, 2.0) == 0.2,
          "rejects nan target");
    Check(SmoothSteering(std::numeric_limits<double>::infinity(), 0.0, 0.1, 2.0) == 0.0,
          "invalid prior reading defaults to zero");
    const double enormous = SmoothSteering(0.2, 1.0, std::numeric_limits<double>::max(), 2.0);
    Check(enormous == 0.2, "overflowing step fails safely");
    double value = 0.0;
    for (int i = 0; i < 10; ++i)
    {
        value = SmoothSteering(value, 1.0, 0.1, 1.0);
    }
    Check(std::abs(value - 1.0) < 1e-9, "repeated steps converge");
    std::cout << "Steering filter tests passed.\n";
    return EXIT_SUCCESS;
}
