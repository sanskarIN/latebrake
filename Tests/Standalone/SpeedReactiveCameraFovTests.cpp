#include "Vehicles/SpeedReactiveCameraFov.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void Check(bool ok, const char* reason)
{
    if (!ok) { std::cerr << "Speed reactive FOV: " << reason << '\n'; std::exit(1); }
}
bool Near(double a, double b, double tolerance = 1.0e-8)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    latebrake::SpeedReactiveCameraFov fov;
    Check(!fov.HasActiveSample(), "default is inactive");
    Check(Near(fov.Sample(90.0, 0.0, 1.0 / 60.0, true, false), 90.0),
          "stationary car never widens");
    Check(Near(fov.Sample(90.0, 190.0, 1.0 / 60.0, true, false), 90.0 + 6.0 * (1.0 - std::exp(-(1.0 / 60.0) / 0.3))),
          "first speed frame uses a bounded exponential easing");
    for (int i = 0; i < 20000; ++i)
    {
        double value = fov.Sample(90.0, 250.0, 1.0 / 120.0, true, false);
        Check(std::isfinite(value) && value >= 90.0 && value <= 96.0,
              "long sessions remain bounded");
    }
    Check(fov.Sample(90.0, 0.0, 0.016, false, false) == 90.0 &&
          !fov.HasActiveSample(), "pause resets visual effect");
    Check(fov.Sample(92.0, 250.0, 0.016, true, true) == 92.0 &&
          !fov.HasActiveSample(), "lookback uses fixed preset framing");
    Check(fov.Sample(90.0, -20.0, 0.016, true, false) == 90.0,
          "reverse motion cannot widen FOV");
    Check(fov.Sample(90.0, std::numeric_limits<double>::infinity(), 0.016, true, false) == 90.0,
          "nonfinite velocity rejected");
    Check(fov.Sample(90.0, 400.0, 0.016, true, false) == 90.0,
          "implausible velocity rejected");
    Check(fov.Sample(90.0, 120.0, 0.5, true, false) == 90.0,
          "large hitch resets effect");
    Check(fov.Sample(std::numeric_limits<double>::quiet_NaN(), 100.0, 0.016, true, false) == 90.0,
          "nonfinite base preset fails closed");
    fov.Reset();
    const double fast1 = fov.Sample(78.0, 190.0, 0.10, true, false);
    const double fast2 = fov.Sample(78.0, 190.0, 0.10, true, false);
    fov.Reset();
    const double slow1 = fov.Sample(78.0, 190.0, 0.05, true, false);
    const double slow2 = fov.Sample(78.0, 190.0, 0.05, true, false);
    const double slow3 = fov.Sample(78.0, 190.0, 0.05, true, false);
    const double slow4 = fov.Sample(78.0, 190.0, 0.05, true, false);
    // The very first sample already advances the visual easing state.
    // Match equivalent elapsed totals with two half-sized time steps.
    Check(fast1 > 78.0 && slow1 > 78.0 &&
          Near(fast1, slow2, 1e-9) &&
          Near(fast2, slow4, 1e-9),
          "equal elapsed time produces consistent FOV across frame rates");
    Check(slow2 > slow1 && slow3 > slow2 && slow4 > slow3,
          "widening progresses monotonically without steps");
    fov.Reset();
    Check(Near(fov.Sample(103.0, 190.0, 0.016, true, false),
               103.0 + 2.0 * (1.0 - std::exp(-0.016 / 0.3))),
          "high-base preset begins bounded easing on the first frame");
    for (int i = 0; i < 120; ++i)
    {
        Check(fov.Sample(103.0, 190.0, 0.016, true, false) <= 105.0,
              "maximum camera field of view remains safe");
    }
    std::cout << "Speed-reactive camera FOV portable tests passed.\n";
    return 0;
}
