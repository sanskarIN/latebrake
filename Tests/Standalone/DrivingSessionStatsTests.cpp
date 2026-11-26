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
    // Class boundaries are lower-inclusive, upper-exclusive except the
    // final 360 km/h endpoint. Quantiles retain the containing interval.
    for (std::size_t i=0;i<SampledSpeedBandCount;++i)
    {
        SampledSpeedBand band{};
        Check(ClassifySampledSpeed(SampledSpeedEdgesKmH[i],band) &&
            static_cast<std::size_t>(band)==i,"exact lower band boundary");
        const double below=std::nextafter(SampledSpeedEdgesKmH[i+1],0.0);
        Check(ClassifySampledSpeed(below,band) &&
            static_cast<std::size_t>(band)==i,"exact upper band boundary");
    }
    SampledSpeedBand band{};
    Check(ClassifySampledSpeed(360.0,band) &&
        band==SampledSpeedBand::From280To360,"inclusive top speed");
    Check(!ClassifySampledSpeed(361.0,band) &&
        !ClassifySampledSpeed(-1.0,band) &&
        !ClassifySampledSpeed(std::numeric_limits<double>::quiet_NaN(),band),
        "invalid band inputs rejected");
    Check(stats.Observe({0.25,1000.0,0.0,true}),"slow first sample");
    Check(stats.Observe({0.05,9000.0,0.0,false}),"fast brief sample");
    const auto weighted=stats.GetSpeedDistribution();
    Check(weighted.Valid && weighted.TotalFrames==2 &&
        Near(weighted.TotalSeconds,0.30) && Near(weighted.TotalDistanceCm,700.0) &&
        Near(weighted.TotalBrakingSeconds,0.25) &&
        Near(weighted.AverageSpeedKmH,84.0) &&
        weighted.MedianBand==SampledSpeedBand::From20To60 &&
        weighted.P90Band==SampledSpeedBand::From280To360,
        "time-weighted quantiles use actual durations rather than frame counts");
    Check(Near(weighted.Rows[1].BrakingSeconds,0.25) &&
        weighted.Rows[6].BrakingSeconds==0.0,"per-band brake exposure");
    stats.Suspend();
    Check(!stats.Observe({0.4,9000.0,0.0,true}),"hitch rejected");
    Check(stats.GetSpeedDistribution().TotalFrames==2,"hitch changes no bins");
    stats.Reset();
    Check(!stats.GetSpeedDistribution().Valid,"reset clears speed bands");
    Check(stats.Observe({0.25,0.0,0.0,false}) &&
        stats.Observe({0.25,0.0,0.0,false}),"idle telemetry accepted");
    const auto idle=stats.GetSpeedDistribution();
    Check(idle.Valid && idle.AverageSpeedKmH==0.0 &&
        idle.MedianBand==SampledSpeedBand::Below20 &&
        idle.P90Band==SampledSpeedBand::Below20,"idle has zero mean without division by zero");

    SampledSpeedAccumulator bounded;
    for (int i=0;i<400000;++i)
        Check(bounded.Observe(0.25,10000.0,false),"bounded distance admission");
    Check(bounded.Review(400000,100000.0,1000000000.0,0.0).Valid,
        "exact distance saturation boundary still reconciles");
    Check(!bounded.Observe(0.25,10000.0,false),"distance overflow refused");
    Check(!bounded.Review(400000,100000.0,1000000000.0,0.0).Valid,
        "saturated distribution cannot be presented as complete");
    bounded.Reset();
    Check(bounded.Observe(0.25,1000.0,false) &&
        bounded.Observe(0.25,1000.0,false),"fresh bounded samples");
    Check(!bounded.Review(2,0.6,500.0,0.0).Valid &&
        !bounded.Review(3,0.5,500.0,0.0).Valid &&
        !bounded.Review(2,0.5,499.0,0.0).Valid &&
        !bounded.Review(2,0.5,500.0,0.1).Valid,
        "mismatched authoritative totals withhold distribution");
    std::cout << "Driving session statistics and time-weighted speed bands passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
