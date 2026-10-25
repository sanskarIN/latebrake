#include "Racing/DrivingSessionStats.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* name)
{
    if (!ok)
    {
        std::cerr << "DrivingSessionStats FAIL: " << name << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double left, double right)
{
    return std::abs(left - right) <= 1e-9;
}
}
int main()
{
    using namespace latebrake;
    DrivingSessionStats stats;
    Check(stats.GetDistanceKm() == 0.0 &&
          stats.GetPeakSpeedKmH() == 0.0 &&
          stats.GetBrakeApplications() == 0,
          "fresh run has no fabricated telemetry");
    Check(stats.Observe({0.1, 2000.0, 100.0, false}),
          "valid first sample accepted");
    Check(Near(stats.GetDistanceKm(), 0.002) &&
          Near(stats.GetPeakSpeedKmH(), 72.0) &&
          Near(stats.GetActiveSampledSeconds(), 0.1),
          "distance uses centimeters, speed uses km/h");
    Check(stats.Observe({0.2, 3000.0, 1500.0, true}) &&
          stats.Observe({0.1, 2500.0, 1100.0, true}) &&
          stats.GetBrakeApplications() == 1,
          "held brake only counts one distinct application");
    Check(Near(stats.GetBrakingSeconds(), 0.3) &&
          Near(stats.GetHighSlipSeconds(), 0.3) &&
          Near(stats.GetPeakSpeedKmH(), 108.0),
          "braking, sideways drift and maximum are measured separately");
    stats.Suspend();
    Check(stats.Observe({0.1, 1500.0, 50.0, true}) &&
          stats.GetBrakeApplications() == 2,
          "post-teleport or pause brake starts a new valid sequence");
    const double oldDistance = stats.GetDistanceKm();
    const int oldCount = stats.GetAcceptedSamples();
    const double invalid[] = {std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::infinity(),
                              -std::numeric_limits<double>::infinity()};
    for (double broken : invalid)
    {
        Check(!stats.Observe({broken, 1000, 0, false}) &&
              !stats.Observe({0.1, broken, 0, false}) &&
              !stats.Observe({0.1, 1000, broken, false}),
              "all invalid speed/delta measurements are discarded");
    }
    Check(!stats.Observe({-0.1, 1000, 0, false}) &&
          !stats.Observe({0.0, 1000, 0, false}) &&
          !stats.Observe({0.4, 1000, 0, false}) &&
          !stats.Observe({0.1, -1.0, 0, false}) &&
          !stats.Observe({0.1, 10001.0, 0, false}),
          "negative, oversized, hitch and impossible readings ignored");
    Check(stats.GetAcceptedSamples() == oldCount &&
          Near(stats.GetDistanceKm(), oldDistance),
          "bad frames never contaminate distance or sample count");
    Check(stats.Observe({0.1, 0.0, 0.0, false}),
          "valid idle frame is still a valid observation");
    Check(stats.GetBrakeApplications() == 2 &&
          stats.GetHighSlipSeconds() <= stats.GetActiveSampledSeconds(),
          "no phantom brake edge or time inflation");
    for (int i = 0; i < 30000; ++i)
    {
        Check(stats.Observe({0.2, 6000.0, 2200.0, (i % 3) == 0}),
              "bounded repeated racing samples are accepted");
    }
    Check(std::isfinite(stats.GetDistanceKm()) &&
          stats.GetPeakSpeedKmH() <= 360.0 &&
          stats.GetBrakingSeconds() <= stats.GetActiveSampledSeconds() &&
          stats.GetHighSlipSeconds() <= stats.GetActiveSampledSeconds(),
          "long session remains bounded and internally consistent");
    stats.Reset();
    Check(stats.GetDistanceKm() == 0.0 &&
          stats.GetPeakSpeedKmH() == 0.0 &&
          stats.GetActiveSampledSeconds() == 0.0 &&
          stats.GetBrakeApplications() == 0 &&
          stats.GetAcceptedSamples() == 0,
          "new race clears all advisory statistics");
    std::cout << "Driving session statistics passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
