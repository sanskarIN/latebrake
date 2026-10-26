#include "Racing/SampledDrivingReview.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "SampledDrivingReview FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-6;
}
}

int main()
{
    using namespace latebrake;
    DrivingSessionStats stats;
    Check(!BuildSampledDrivingReview(stats).Valid,
          "no driving evidence cannot produce telemetry");
    for (int n = 0; n < 10; ++n)
    {
        const bool brake = n < 5;
        Check(stats.Observe({0.2, 1500.0, n < 3 ? 800.0 : 100.0, brake}),
              "valid real speed, slip and brake sample");
    }
    const auto r = BuildSampledDrivingReview(stats);
    Check(r.Valid && r.AcceptedFrames == 10 &&
          r.BrakeApplications == 1 &&
          Near(r.SampledSeconds, 2.0) &&
          Near(r.EstimatedDistanceKm, 0.03) &&
          Near(r.PeakSpeedKmH, 54.0) &&
          Near(r.BrakingPercent, 50.0) &&
          Near(r.HighLateralSlipPercent, 30.0),
          "speed-integrated distance, brake time and high-slip sample ratios");
    stats.Suspend();
    Check(stats.Observe({0.2, 1000.0, 0.0, true}),
          "braking after interrupted sampling");
    Check(BuildSampledDrivingReview(stats).BrakeApplications == 2,
          "suspended brake cannot merge a new press");
    Check(!stats.Observe({std::numeric_limits<double>::infinity(),
                          1000.0, 0.0, true}),
          "invalid time ignored");
    Check(BuildSampledDrivingReview(stats).Valid,
          "malformed sample cannot overwrite valid accumulated data");
    stats.Reset();
    Check(!BuildSampledDrivingReview(stats).Valid,
          "race restart clears sampled stats");
    DrivingSessionStats shortRun;
    Check(shortRun.Observe({0.1, 500.0, 0.0, false}),
          "one valid but insufficient short frame");
    Check(!BuildSampledDrivingReview(shortRun).Valid,
          "too-short one-frame telemetry remains hidden");
    std::cout << "Sampled driving results passed.\n";
}
