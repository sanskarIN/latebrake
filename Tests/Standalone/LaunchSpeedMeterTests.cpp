#include "Racing/LaunchSpeedMeter.h"
#include "Racing/RaceStartLights.h"
#include "Racing/RaceProgress.h"

#include <cmath>

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

    // Exercise the production green-light and race clock together. Red
    // frames cannot produce official elapsed time or a launch observation.
    latebrake::RaceStartLights lights;
    latebrake::RaceProgress race(4, 3);
    LaunchSpeedMeter integrated;
    for (int i = 0; i < 5; ++i)
    {
        race.Advance(lights.Advance(0.5, true));
    }
    Require(!lights.IsGreen() && race.GetElapsedSeconds() == 0.0,
        "five red-light frames cannot advance production race clock");
    race.Advance(lights.Advance(0.5, true));
    Require(lights.IsGreen() && race.GetElapsedSeconds() == 0.0,
        "exact green boundary starts with zero official time");
    Require(!integrated.Observe(
        race.GetElapsedSeconds() - race.GetTotalPenaltySeconds(),
        0.0, lights.IsGreen()),
        "first post-green stationary sample arms telemetry");
    race.Advance(lights.Advance(0.3, true));
    Require(integrated.Observe(
        race.GetElapsedSeconds() - race.GetTotalPenaltySeconds(),
        5.5, lights.IsGreen()) &&
        std::abs(integrated.SecondsToFiveKmh() - 0.3) < 1e-9,
        "launch estimate uses genuine production green-light race seconds");
    Require(race.AddTimePenalty(5.0) &&
        std::abs((race.GetElapsedSeconds() - race.GetTotalPenaltySeconds()) - 0.3) < 1e-9,
        "recovery fees are excluded from advisory elapsed driving time");
    integrated.Invalidate();
    Require(std::abs(integrated.SecondsToFiveKmh() - 0.3) < 1e-9,
        "a recorded launch remains immutable after an unrelated recovery");

    std::cout << "Launch speed telemetry policy passed.\n";
    return EXIT_SUCCESS;
}
