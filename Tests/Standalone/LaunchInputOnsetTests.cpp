#include "Racing/LaunchInputOnset.h"
#include "Racing/RaceProgress.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>

namespace
{
void Check(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "LaunchInputOnset FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;
    LaunchInputOnset meter;
    RaceProgress race(2, 1);
    Check(!meter.HasInputOnset() && Near(meter.FirstForwardInputSeconds(), 0),
          "no invented launch action before first human command");

    for (double invalid : {-1.0, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN(), 121.0})
    {
        Check(!meter.Observe(invalid, 0.8, false, true),
              "invalid and out-of-window elapsed times rejected");
    }
    for (double invalid : {0.0, -0.5, 0.199,
                           std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), 1.2})
    {
        Check(!meter.Observe(0.1, invalid, false, true),
              "small, reverse, nonfinite and out-of-range throttle rejected");
    }
    Check(!meter.Observe(0.1, 0.8, true, true),
          "braking does not count as clean forward launch input");
    Check(!meter.Observe(0.1, 0.8, false, false),
          "AI or unpossessed pawn never records a human start response");
    Check(!meter.HasInputOnset(), "no invalid sample can seed timing");

    race.Advance(0.25);
    Check(meter.Observe(race.GetElapsedSeconds(), 0.75, false, true) &&
          meter.HasInputOnset() &&
          Near(meter.FirstForwardInputSeconds(), 0.25),
          "actual authoritative race clock records first qualifying command");
    race.Advance(1.5);
    Check(!meter.Observe(race.GetElapsedSeconds(), 1.0, false, true) &&
          Near(meter.FirstForwardInputSeconds(), 0.25),
          "later commands never overwrite first observed onset");
    meter.Reset();
    race.Reset();
    Check(!meter.HasInputOnset() &&
          Near(meter.FirstForwardInputSeconds(), 0.0),
          "race restart erases old player's launch measurement");
    Check(meter.Observe(0.0, 0.2, false, true) &&
          Near(meter.FirstForwardInputSeconds(), 0.0),
          "held forward throttle at GO may correctly register zero delay");
    meter.Reset();
    Check(meter.Observe(120.0, 1.0, false, true) &&
          Near(meter.FirstForwardInputSeconds(), 120.0),
          "closed tracked boundary allowed without unbounded metrics");
    meter.Reset();
    Check(!meter.Observe(120.001, 1.0, false, true),
          "late input is omitted, not invented or silently clipped");

    std::cout << "First forward input-onset analysis passed.\n";
    return EXIT_SUCCESS;
}
