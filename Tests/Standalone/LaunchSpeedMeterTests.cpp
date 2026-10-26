#include "Racing/LaunchSpeedMeter.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "LaunchSpeedMeter FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::LaunchSpeedMeter;
    LaunchSpeedMeter meter;
    Require(!meter.HasSample() && !meter.IsInvalidated(),
        "new race has no fabricated launch estimate");

    Require(!meter.Observe(0.0, 0.0, false) &&
            !meter.Observe(0.0, 100.0, false) && !meter.HasSample(),
        "red lights, pause and unqualified frames never produce an estimate");
    Require(!meter.Observe(0.02, 8.0, true) && !meter.HasSample(),
        "first already-moving sample cannot invent a start crossing");
    Require(!meter.Observe(0.03, -12.0, true) &&
            !meter.Observe(0.12, 4.99, true),
        "reversing and sub-threshold speed do not complete forward launch");
    Require(!meter.Observe(0.12, 100.0, false),
        "inactive frame cannot finish the launch");
    Require(meter.Observe(0.18, 5.0, true) &&
            meter.HasSample() && meter.SecondsToFiveKmh() == 0.18,
        "first forward 5 km/h is an approximate session-only launch");
    Require(!meter.Observe(1.5, 100.0, true) &&
            meter.SecondsToFiveKmh() == 0.18,
        "subsequent speed never replaces the first captured launch");

    meter.Invalidate();
    Require(meter.HasSample() && meter.SecondsToFiveKmh() == 0.18,
        "recovery cannot retrospectively rewrite an already captured estimate");
    meter.Reset();
    Require(!meter.HasSample() && !meter.IsInvalidated() &&
            meter.SecondsToFiveKmh() == 0.0,
        "accepted restart clears previous race's estimate");

    const double bad = std::numeric_limits<double>::quiet_NaN();
    Require(!meter.Observe(bad, 0.0, true) &&
            !meter.Observe(0.0, bad, true) &&
            !meter.Observe(std::numeric_limits<double>::infinity(), 0.0, true) &&
            !meter.Observe(-1.0, 0.0, true),
        "invalid clock or velocity samples are ignored");
    Require(!meter.Observe(0.0, 0.0, true) &&
            !meter.Observe(0.2, 4.0, true),
        "valid samples can resume after malformed physics");
    Require(!meter.Observe(0.1, 10.0, true) && meter.IsInvalidated(),
        "clock rollback invalidates the whole launch observation");
    Require(!meter.Observe(0.3, 10.0, true) && !meter.HasSample(),
        "invalidated estimate cannot revive without restart");

    meter.Reset();
    Require(!meter.Observe(0.0, 0.0, true) &&
            !meter.Observe(30.01, 5.0, true) &&
            meter.IsInvalidated(),
        "late speed crossing is not called a race launch");
    meter.Reset();
    Require(!meter.Observe(0.0, 0.0, true),
        "new attempt can arm again");
    meter.Invalidate();
    Require(!meter.Observe(0.6, 5.0, true) && !meter.HasSample(),
        "verified recovery discards the current launch attempt");
    meter.Reset();
    Require(!meter.Observe(0.0, 0.0, true) &&
            meter.Observe(0.5, 5.2, true) &&
            meter.SecondsToFiveKmh() == 0.5,
        "fresh restart permits a new estimate");

    std::cout << "Launch speed telemetry policy passed.\n";
    return EXIT_SUCCESS;
}
