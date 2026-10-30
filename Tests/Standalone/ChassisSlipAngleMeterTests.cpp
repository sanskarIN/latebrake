#include "Racing/ChassisSlipAngleMeter.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "ChassisSlipAngleMeter FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}
latebrake::ChassisSlipSample Sample(double dt, double forward, double lateral)
{
    return {dt, forward, lateral};
}
}

int main()
{
    using latebrake::ChassisSlipAngleMeter;
    ChassisSlipAngleMeter meter;
    Require(!meter.HasLiveReading() && meter.GetAcceptedSamples() == 0 &&
            Near(meter.GetPeakAbsoluteDegrees(), 0.0),
            "new session starts without invented slip");

    Require(meter.Observe(Sample(0.1, 1000.0, 0.0)) &&
            meter.HasLiveReading() && Near(meter.GetLiveDegrees(), 0.0),
            "straight forward motion gives zero chassis drift angle");
    Require(meter.Observe(Sample(0.1, 1000.0, 1000.0)) &&
            meter.GetLiveDegrees() > 0.0 &&
            Near(meter.GetPeakAbsoluteDegrees(), 45.0) &&
            Near(meter.GetHighAngleSeconds(), 0.1),
            "45-degree lateral motion is measured and smoothed");
    const double peak = meter.GetPeakAbsoluteDegrees();
    meter.Suspend();
    Require(!meter.HasLiveReading() && Near(meter.GetLiveDegrees(), 0.0) &&
            Near(meter.GetPeakAbsoluteDegrees(), peak),
            "pause clears live direction without erasing run peak");
    Require(meter.Observe(Sample(0.1, 1000.0, -1000.0)) &&
            Near(meter.GetLiveDegrees(), -45.0),
            "fresh sample after interruption never blends old sign");
    Require(Near(meter.GetHighAngleSeconds(), 0.2),
            "opposite-sign drift contributes to unsigned session duration");

    meter.Reset();
    Require(meter.GetAcceptedSamples() == 0 &&
            Near(meter.GetPeakAbsoluteDegrees(), 0.0) &&
            Near(meter.GetHighAngleSeconds(), 0.0) &&
            !meter.HasLiveReading(),
            "race restart clears all live and historical observations");

    Require(!meter.Observe(Sample(0.1, 299.0, 500.0)) &&
            !meter.Observe(Sample(0.1, -1200.0, 200.0)) &&
            !meter.Observe(Sample(0.1, 0.0, 0.0)),
            "near-stationary and reverse movement cannot masquerade as drift");
    Require(meter.Observe(Sample(0.1, 800.0, 1000.0)) &&
            Near(meter.GetHighAngleSeconds(), 0.0),
            "low forward speed is excluded from high-angle session time");
    meter.Reset();
    Require(meter.Observe(Sample(0.1, 1000.0, 100.0)) &&
            Near(meter.GetHighAngleSeconds(), 0.0),
            "small observed sideways motion does not register a high angle");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const latebrake::ChassisSlipSample bad[] = {
        Sample(0.0, 1000.0, 0.0),
        Sample(-0.1, 1000.0, 0.0),
        Sample(0.0001, 1000.0, 0.0),
        Sample(0.251, 1000.0, 0.0),
        Sample(nan, 1000.0, 0.0),
        Sample(0.1, nan, 0.0),
        Sample(0.1, inf, 0.0),
        Sample(0.1, 1000.0, -inf),
        Sample(0.1, 8000.0, 8000.0)
    };
    for (const auto& invalid : bad)
    {
        meter.Reset();
        Require(meter.Observe(Sample(0.1, 1000.0, 400.0)),
                "baseline valid before rejected sample");
        const double priorPeak = meter.GetPeakAbsoluteDegrees();
        Require(!meter.Observe(invalid) && !meter.HasLiveReading() &&
                Near(meter.GetLiveDegrees(), 0.0) &&
                Near(meter.GetPeakAbsoluteDegrees(), priorPeak),
                "invalid sample hides live angle and preserves valid peak");
        Require(meter.Observe(Sample(0.1, 1000.0, -400.0)) &&
                meter.GetLiveDegrees() < 0.0,
                "recovery resumes with fresh signed direction");
    }

    meter.Reset();
    for (int i = 0; i < 20000; ++i)
    {
        const double lateral = (i % 2 == 0) ? 800.0 : -800.0;
        Require(meter.Observe(Sample(0.05, 1200.0, lateral)),
                "20k ordinary velocity samples remain admissible");
    }
    Require(meter.GetAcceptedSamples() == 20000 &&
            meter.GetHighAngleSeconds() > 999.0 &&
            meter.GetHighAngleSeconds() <= 1000.0 + 1e-7 &&
            std::isfinite(meter.GetLiveDegrees()) &&
            meter.GetPeakAbsoluteDegrees() < 90.0,
            "long session stays finite, bounded and allocation-free");

    std::cout << "Chassis slip angle telemetry tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
