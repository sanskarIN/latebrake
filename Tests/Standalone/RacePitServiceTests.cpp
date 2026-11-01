#include "Racing/RacePitService.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
}

int main()
{
    using latebrake::RacePitService;
    using latebrake::PitServiceObservation;
    using latebrake::IsInRacePitBay;

    Check(IsInRacePitBay(0.0, -1300.0), "bay center is on the road");
    Check(latebrake::IsOnPavedSurface(latebrake::RacePitBay),
          "pit is in the existing paved circuit");
    Check(!IsInRacePitBay(0.0, -1800.0),
          "rival center-line cannot count as pit service");
    Check(!IsInRacePitBay(0.0, -1000.0), "bay radius is bounded");
    Check(!IsInRacePitBay(std::numeric_limits<double>::quiet_NaN(), -1300.0),
          "nonfinite position rejected");

    RacePitService pit;
    PitServiceObservation input{
        0.05, 0.0, -1300.0, 0.0, 0.0, true, false, true, true};
    Check(pit.StopCount() == 0 && pit.IsArmed(), "initial arm");

    // Fail closed on official countdown/pause/menu/restart unavailability.
    input.Authorized = false;
    for (int i = 0; i < 60; ++i)
        Check(!pit.Observe(input), "unauthorized hold must not complete");
    Check(pit.ProgressFraction() == 0.0, "ineligible progress never accrues");
    input.Authorized = true;
    input.X = 0.0; input.Y = -1800.0;
    for (int i = 0; i < 60; ++i)
        Check(!pit.Observe(input), "main racing line cannot pit");
    Check(!pit.IsInBay(), "outside bay");
    input.Y = -1300.0;

    input.PlanarSpeedCmPerSecond = 300.0;
    Check(!pit.Observe(input), "rolling at speed disallowed");
    input.PlanarSpeedCmPerSecond = 0.0;
    input.RequestedThrottle = 0.2;
    Check(!pit.Observe(input), "held acceleration disallowed");
    input.RequestedThrottle = 0.0;
    input.NitroHeld = true;
    Check(!pit.Observe(input), "held nitro disallowed");
    input.NitroHeld = false;
    input.ButtonHeld = false;
    Check(!pit.Observe(input), "must physically hold service key");
    input.ButtonHeld = true;
    input.MaintenanceNeeded = false;
    Check(!pit.Observe(input), "fully healthy car cannot create extra stop");
    input.MaintenanceNeeded = true;

    for (int i = 0; i < 49; ++i)
        Check(!pit.Observe(input), "service cannot finish early");
    Check(pit.IsInBay() && pit.ProgressFraction() > 0.97 &&
              pit.ProgressFraction() < 1.0,
          "2.45 seconds earns no early service");
    Check(pit.Observe(input), "2.50s stationary hold completes one service");
    Check(pit.StopCount() == 1 && !pit.IsArmed() &&
              pit.ProgressFraction() == 0.0, "first stop is latched");

    for (int i = 0; i < 200; ++i)
        Check(!pit.Observe(input), "no infinite repairs on one pit visit");
    Check(pit.StopCount() == 1, "holding in the same bay never repeats");
    input.X = 500.0;
    Check(!pit.Observe(input) && pit.IsArmed(), "physical exit rearms");
    input.X = 0.0;
    for (int i = 0; i < 50; ++i)
        Check(pit.Observe(input) == (i == 49), "second timed service");
    Check(pit.StopCount() == 2, "return visit counts one stop");

    input.X = 500.0;
    Check(!pit.Observe(input), "exit twice");
    input.X = 0.0;
    Check(!pit.Observe(input), "begin third service");
    input.ButtonHeld = false;
    Check(!pit.Observe(input), "release cancels progress");
    Check(pit.ProgressFraction() == 0.0, "release is not resume-able");
    input.ButtonHeld = true;
    for (int i = 0; i < 20; ++i) Check(!pit.Observe(input), "hold begins");
    input.DeltaSeconds = 0.4;
    Check(!pit.Observe(input), "large frame refuses credit");
    Check(pit.ProgressFraction() == 0.0, "long frame resets hold");

    const double invalid[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(), -1.0};
    for (double n : invalid)
    {
        input.DeltaSeconds = 0.05;
        input.PlanarSpeedCmPerSecond = n;
        Check(!pit.Observe(input), "invalid chassis velocity");
        Check(pit.ProgressFraction() == 0.0, "invalid velocity clears progress");
        input.PlanarSpeedCmPerSecond = 0.0;
        input.RequestedThrottle = n;
        Check(!pit.Observe(input), "invalid driver axis");
        input.RequestedThrottle = 0.0;
    }
    pit.Reset();
    Check(pit.IsArmed() && !pit.IsInBay() && pit.StopCount() == 0 &&
              pit.ProgressFraction() == 0.0,
          "new race erases pit history");

    std::cout << "RacePitService: all scenarios passed\n";
    return 0;
}
