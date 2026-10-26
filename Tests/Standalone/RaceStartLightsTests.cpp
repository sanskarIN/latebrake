#include "Racing/RaceStartLights.h"
#include "Racing/RaceProgress.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* what)
{
    if (!ok)
    {
        std::cerr << "RaceStartLights FAIL: " << what << '\n';
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
    RaceStartLights start;
    RaceProgress race(2, 1);
    Require(!start.IsGreen() &&
            start.Phase() == RaceStartPhase::Three &&
            Near(start.SecondsUntilGreen(), 3.0),
            "new session starts with three red lights");

    for (double bad : {-0.1, 0.0, 0.51,
                       std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()})
    {
        Require(Near(start.Advance(bad, true), 0.0) &&
                start.Phase() == RaceStartPhase::Three,
                "nonfinite, negative or large hitch frame never starts race");
    }

    for (int n = 0; n < 4; ++n)
    {
        Require(Near(start.Advance(0.5, false), 0.0),
                "paused/unqualified game cannot count down");
    }
    Require(Near(start.SecondsUntilGreen(), 3.0),
            "blocked frames do not consume lights");

    for (int n = 0; n < 5; ++n)
    {
        Require(Near(start.Advance(0.4, true), 0.0),
                "pre-green frames yield zero race-clock credit");
        if (n == 2)
        {
            Require(start.Phase() == RaceStartPhase::Two,
                    "second red light after one qualified second");
        }
    }
    Require(start.Phase() == RaceStartPhase::One &&
            Near(start.SecondsUntilGreen(), 1.0),
            "two accumulated seconds show final red light");

    Require(Near(start.Advance(0.45, false), 0.0) &&
            Near(start.SecondsUntilGreen(), 1.0),
            "app interruption freezes final red light");
    Require(Near(start.Advance(0.5, true), 0.0) &&
            start.Phase() == RaceStartPhase::One,
            "red light remains one at 2.5 eligible seconds");
    Require(Near(start.Advance(0.4, true), 0.0),
            "final red light still denies lap-clock progression");
    Require(!start.IsGreen(), "race has not begun before green");

    // Cross 3.0s with a fractional frame; 0.1s is countdown time,
    // remaining 0.2s is the only legitimate player clock delta.
    const double live = start.Advance(0.3, true);
    Require(start.IsGreen() && start.Phase() == RaceStartPhase::Go &&
            Near(live, 0.2), "transition frame credits only post-GO time");
    race.Advance(live);
    Require(Near(race.GetElapsedSeconds(), 0.2),
            "authoritative clock excludes all earlier lights");

    Require(Near(start.Advance(0.5, false), 0.0) &&
            start.Phase() == RaceStartPhase::Go,
            "pause also freezes green GO announcement");
    Require(Near(start.Advance(0.5, true), 0.5) &&
            start.Phase() == RaceStartPhase::Go,
            "running frame credits full elapsed time after green");
    Require(Near(start.Advance(0.2, true), 0.2) &&
            start.Phase() == RaceStartPhase::Racing,
            "brief GO cue eventually disappears");

    start.Reset();
    race.Reset();
    Require(!start.IsGreen() &&
            start.Phase() == RaceStartPhase::Three &&
            Near(start.SecondsUntilGreen(), 3.0) &&
            Near(race.GetElapsedSeconds(), 0.0),
            "race restart resets both light gate and timer");

    // Exact boundary has no free race-clock increment on that frame.
    for (int n = 0; n < 6; ++n)
    {
        Require(Near(start.Advance(0.5, true), 0.0),
                "exact start boundary never contributes time");
    }
    Require(start.IsGreen() &&
            Near(start.Advance(0.1, true), 0.1),
            "following frame advances only after green");
    Require(Near(start.SecondsUntilGreen(), 0.0),
            "green signal reports zero remaining countdown");

    std::cout << "Three-light race-start policy passed.\n";
    return EXIT_SUCCESS;
}
