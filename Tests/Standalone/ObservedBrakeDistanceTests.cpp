#include "Racing/ObservedBrakeDistance.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* reason)
{
    if (!value)
    {
        std::cerr << "ObservedBrakeDistance FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1e-7)
{
    return std::abs(a - b) < tolerance;
}
latebrake::BrakeDistanceSample Sample(
    double dt, double speed, double forward, bool braking = true)
{
    return {dt, speed, forward, braking};
}
void Confirm(latebrake::ObservedBrakeDistance& meter)
{
    Check(!meter.Observe(Sample(0.05, 2000.0, 2000.0)) &&
          !meter.HasLiveEstimate(), "one speed does not prove deceleration");
    Check(meter.Observe(Sample(0.05, 1970.0, 1970.0)) &&
          !meter.HasLiveEstimate(), "one pair is not enough");
    Check(meter.Observe(Sample(0.05, 1940.0, 1940.0)) &&
          !meter.HasLiveEstimate(), "two pairs are not enough");
    Check(meter.Observe(Sample(0.05, 1910.0, 1910.0)) &&
          meter.HasLiveEstimate(), "three sustained braking pairs qualify");
}
}

int main()
{
    using latebrake::ObservedBrakeDistance;
    ObservedBrakeDistance meter;
    Check(!meter.HasLiveEstimate() && Near(meter.GetDistanceMeters(), 0.0) &&
          Near(meter.GetObservedDecelerationMetersPerSecondSquared(), 0.0),
          "fresh meter exposes no invented braking information");

    Confirm(meter);
    const double expected = 0.01 * 1910.0 * 1910.0 / (2.0 * 600.0);
    Check(Near(meter.GetDistanceMeters(), expected) &&
          Near(meter.GetObservedDecelerationMetersPerSecondSquared(), 6.0) &&
          meter.GetConsecutivePairs() == 3,
          "distance uses actual speed derivative and correct cm-to-m units");
    Check(!meter.Observe(Sample(0.05, 1880.0, 1880.0, false)) &&
          !meter.HasLiveEstimate() && Near(meter.GetDistanceMeters(), 0.0),
          "released brake immediately hides prior estimate");

    meter.Reset();
    Confirm(meter);
    Check(!meter.Observe(Sample(0.05, 1920.0, -100.0)) &&
          !meter.HasLiveEstimate(), "reverse vehicle motion is rejected");
    Confirm(meter);
    Check(!meter.Observe(Sample(0.05, 1880.0, 950.0)) &&
          !meter.HasLiveEstimate(), "sideways drift cannot fake longitudinal stop");
    Confirm(meter);
    Check(!meter.Observe(Sample(0.05, 1880.0, 2000.0)) &&
          !meter.HasLiveEstimate(), "forward projection exceeding planar speed rejected");
    Confirm(meter);
    Check(!meter.Observe(Sample(0.05, 2000.0, 2000.0)) &&
          !meter.HasLiveEstimate(), "acceleration while braking hides estimate");

    Confirm(meter);
    meter.Suspend();
    Check(!meter.HasLiveEstimate() && meter.GetConsecutivePairs() == 0,
          "pause and focus interruptions discard all previous samples");
    Confirm(meter);
    Check(!meter.Observe(Sample(0.05, 900.0, 900.0)) &&
          !meter.HasLiveEstimate(), "collision-like deceleration spikes rejected");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const latebrake::BrakeDistanceSample invalid[] = {
        Sample(0.0, 1900.0, 1900.0),
        Sample(-0.01, 1900.0, 1900.0),
        Sample(0.0001, 1900.0, 1900.0),
        Sample(0.251, 1900.0, 1900.0),
        Sample(nan, 1900.0, 1900.0),
        Sample(0.05, nan, 1900.0),
        Sample(0.05, inf, 1900.0),
        Sample(0.05, 1900.0, inf),
        Sample(0.05, 1900.0, nan),
        Sample(0.05, 10500.0, 10500.0),
        Sample(0.05, 899.0, 899.0),
    };
    for (const auto& bad : invalid)
    {
        meter.Reset();
        Confirm(meter);
        Check(!meter.Observe(bad) && !meter.HasLiveEstimate() &&
              Near(meter.GetDistanceMeters(), 0.0),
              "invalid physics input must fail closed");
    }

    meter.Reset();
    Check(!meter.Observe(Sample(0.02, 1500.0, 1500.0)),
          "baseline accepted without a fabricated pair");
    // Tiny deceleration should never yield a misleading huge stop estimate.
    for (int i = 1; i < 100; ++i)
    {
        Check(!meter.Observe(Sample(0.02, 1500.0 - i * 0.5,
                                    1500.0 - i * 0.5)),
              "low deceleration does not become trusted");
    }

    // Stress constant-rate braking within the plausible source bounds.
    // Each race segment starts with a fresh sample following a reset.
    for (int run = 0; run < 5000; ++run)
    {
        meter.Reset();
        Confirm(meter);
        Check(std::isfinite(meter.GetDistanceMeters()) &&
              meter.GetDistanceMeters() >= 0.0 &&
              meter.GetDistanceMeters() <=
                  ObservedBrakeDistance::MaxAdvisoryDistanceMeters,
              "long repeated sessions remain bounded and allocation-free");
    }

    meter.Reset();
    Check(!meter.HasLiveEstimate() && meter.GetConsecutivePairs() == 0,
          "restart clears unpersisted braking estimate");
    std::cout << "Observed braking distance policy passed (source only).\n";
    return EXIT_SUCCESS;
}
