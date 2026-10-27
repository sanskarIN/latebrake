#include "Racing/VehicleGForceMeter.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "VehicleGForceMeter FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double lhs, double rhs, double epsilon = 1e-8)
{
    return std::abs(lhs - rhs) < epsilon;
}
latebrake::VehicleMotionSample Sample(
    double dt, double vx, double vy, double yaw = 0.0, bool brake = false)
{
    return {dt, vx, vy, yaw, brake};
}
}

int main()
{
    using latebrake::VehicleGForceMeter;
    using latebrake::VehicleMotionSample;
    VehicleGForceMeter meter;
    Require(meter.GetAcceptedPairs() == 0 &&
            Near(meter.GetPeakBrakingG(), 0.0) &&
            Near(meter.GetPeakCorneringG(), 0.0),
        "a new session has no phantom motion evidence");

    Require(!meter.Observe(Sample(0.1, 500, 0)),
        "first valid velocity establishes baseline only");
    Require(meter.GetAcceptedPairs() == 0 &&
            Near(meter.GetLiveLongitudinalG(), 0.0),
        "no first-frame acceleration spike");
    Require(meter.Observe(Sample(0.1, 598.0665, 0)),
        "a physical one-g forward acceleration is accepted");
    Require(meter.GetAcceptedPairs() == 1 &&
            meter.GetLiveLongitudinalG() > 0.0 &&
            Near(meter.GetLiveLateralG(), 0.0),
        "longitudinal velocity change is physically projected");

    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 1000.0, 0)),
        "cornering example establishes world velocity baseline");
    Require(meter.Observe(Sample(0.1, 1000.0, 98.0665)),
        "a one-g turn is measured from world XY velocity");
    Require(Near(meter.GetPeakCorneringG(), 1.0) &&
            Near(meter.GetPeakBrakingG(), 0.0),
        "lateral g peaks but steering does not invent braking");
    const double cornerPeak = meter.GetPeakCorneringG();
    meter.Suspend();
    Require(Near(meter.GetLiveLateralG(), 0.0) &&
            Near(meter.GetPeakCorneringG(), cornerPeak),
        "menu/focus interruption removes live impulse but retains session peak");
    Require(!meter.Observe(Sample(0.1, -1000.0, -1000.0)) &&
            meter.GetAcceptedPairs() == 1,
        "first frame after pause/teleport cannot bridge old world velocities");

    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 800.0, 0.0)),
        "brake sample starts a clean pair");
    Require(meter.Observe(Sample(0.1, 701.9335, 0.0, 0.0, true)) &&
            Near(meter.GetPeakBrakingG(), 1.0),
        "actual held brake and forward negative acceleration registers one g");
    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 800.0, 0.0)) &&
            meter.Observe(Sample(0.1, 701.9335, 0.0, 0.0, false)) &&
            Near(meter.GetPeakBrakingG(), 0.0),
        "passive drag is not labelled brake performance");
    meter.Reset();
    Require(!meter.Observe(Sample(0.1, -800.0, 0.0)) &&
            meter.Observe(Sample(0.1, -701.9335, 0.0, 0.0, true)) &&
            Near(meter.GetPeakBrakingG(), 0.0),
        "reverse movement does not masquerade as forward brake deceleration");

    // Rotate the chassis: world +Y is forward at yaw=90 degrees. Measuring
    // derivatives of actor-local projected speed would yield a false turn.
    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 0.0, 700.0, 90.0)) &&
            meter.Observe(Sample(0.1, 0.0, 601.9335, 90.0, true)) &&
            Near(meter.GetPeakBrakingG(), 1.0) &&
            Near(meter.GetPeakCorneringG(), 0.0),
        "braking respects chassis heading without conflating world axes");
    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 0.0, 700.0, 90.0)) &&
            meter.Observe(Sample(0.1, -98.0665, 700.0, 90.0)) &&
            Near(meter.GetPeakCorneringG(), 1.0),
        "world -X acceleration projects to vehicle-right when yaw is 90");

    // An invalid or long frame tears down continuity; it never makes a
    // giant sampled peak on the first valid frame after an interruption.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const VehicleMotionSample bad[] = {
        Sample(0.0, 0, 0), Sample(-0.1, 0, 0),
        Sample(0.0001, 0, 0), Sample(0.251, 0, 0),
        Sample(nan, 0, 0), Sample(0.1, nan, 0),
        Sample(0.1, 0, inf), Sample(0.1, 0, 0, nan),
        Sample(0.1, 0, 0, 1000001.0), Sample(0.1, 10001, 0),
    };
    for (const VehicleMotionSample& invalid : bad)
    {
        meter.Reset();
        Require(!meter.Observe(Sample(0.1, 500.0, 0.0)),
            "valid sample armed before invalid frame");
        Require(!meter.Observe(invalid) && meter.GetAcceptedPairs() == 0 &&
                Near(meter.GetLiveLongitudinalG(), 0.0) &&
                Near(meter.GetLiveLateralG(), 0.0),
            "malformed sample is rejected and tears down stale velocity");
        Require(!meter.Observe(Sample(0.1, 900.0, 0.0)),
            "valid sample after malformed frame re-arms with no phantom g");
    }

    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 0.0, 0.0)) &&
            !meter.Observe(Sample(0.001, 980.665, 0.0)) &&
            meter.GetAcceptedPairs() == 0,
        "unphysically large acceleration spikes invalidate both samples");
    Require(!meter.Observe(Sample(0.1, 700.0, 0.0)),
        "implausible collision impulse cannot bleed into next pair");

    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 200.0, 0.0)) &&
            meter.Observe(Sample(0.1, 101.9335, 0.0, 0.0, true)) &&
            Near(meter.GetPeakBrakingG(), 0.0),
        "very slow motion is not treated as a brake performance measurement");
    meter.Reset();
    Require(!meter.Observe(Sample(0.1, 100.0, 0.0)) &&
            meter.Observe(Sample(0.1, 100.0, 98.0665)) &&
            Near(meter.GetPeakCorneringG(), 0.0),
        "parking-speed sideways jitter does not create a cornering peak");

    meter.Reset();
    for (int i = 0; i < 20000; ++i)
    {
        const double velocity = 700.0 + 2.0 * static_cast<double>(i % 15);
        (void)meter.Observe(Sample(1.0 / 60.0, velocity, 20.0));
    }
    Require(meter.GetAcceptedPairs() == 19999 &&
            std::isfinite(meter.GetLiveLongitudinalG()) &&
            std::isfinite(meter.GetLiveLateralG()) &&
            meter.GetPeakBrakingG() <= VehicleGForceMeter::MaxResultantG &&
            meter.GetPeakCorneringG() <= VehicleGForceMeter::MaxResultantG,
        "long racing telemetry remains finite, bounded and allocation-free");
    meter.Reset();
    Require(meter.GetAcceptedPairs() == 0 &&
            Near(meter.GetPeakCorneringG(), 0.0) &&
            Near(meter.GetPeakBrakingG(), 0.0) &&
            Near(meter.GetLiveLateralG(), 0.0),
        "accepted restart resets all retained g-meter statistics");
    std::cout << "Vehicle G-force telemetry policy passed.\n";
    return EXIT_SUCCESS;
}
