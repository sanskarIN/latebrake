#include "Racing/RecentLapPaceStatistics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>

namespace
{
void Check(bool ok, const char* text)
{
    if (!ok)
    {
        std::cerr << "RecentLapPaceStatistics FAIL: " << text << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double actual, double target)
{
    return std::abs(actual - target) < 1e-7;
}
void Lap(latebrake::RaceProgress& race, double seconds)
{
    race.Advance(seconds * 0.25);
    Check(race.PassCheckpoint(0), "first gate");
    race.Advance(seconds * 0.75);
    Check(race.PassCheckpoint(1), "second gate");
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(2, 3);
    Check(!CalculateRecentLapPaceStatistics(race).Valid,
          "running race must not expose post-race summary");
    Lap(race, 10.0);
    Lap(race, 8.0);
    Lap(race, 12.0);
    const auto s = CalculateRecentLapPaceStatistics(race);
    Check(s.Valid && s.SampleCount == 3 &&
          s.OldestLap == 1 && s.NewestLap == 3,
          "real completed lap window");
    Check(Near(s.AverageSeconds, 10.0) &&
          Near(s.MedianSeconds, 10.0) &&
          Near(s.FastestSampleSeconds, 8.0) &&
          Near(s.SlowestSampleSeconds, 12.0) &&
          Near(s.PopulationDeviationSeconds, std::sqrt(8.0 / 3.0)),
          "mean, median and population deviation from actual lap history");
    Check(Near(s.NewestVsAverageSeconds, 2.0) &&
          s.ConsecutiveImprovements == 0,
          "newest lap slower than average, no improvement streak");

    RaceProgress improvement(2, 4);
    for (double t : {10.0, 9.0, 8.0, 7.0}) Lap(improvement, t);
    const auto i = CalculateRecentLapPaceStatistics(improvement);
    Check(i.Valid && i.SampleCount == 4 &&
          Near(i.MedianSeconds, 8.5) &&
          Near(i.AverageSeconds, 8.5) &&
          i.ConsecutiveImprovements == 3 &&
          Near(i.NewestVsAverageSeconds, -1.5),
          "even median and three strictly improving consecutive laps");

    RaceProgress tie(2, 2);
    Lap(tie, 8.0);
    Lap(tie, 8.0);
    Check(CalculateRecentLapPaceStatistics(tie).ConsecutiveImprovements == 0,
          "equal consecutive laps are not improvements");

    RaceProgress single(2, 1);
    Lap(single, 9.0);
    Check(!CalculateRecentLapPaceStatistics(single).Valid,
          "single lap has insufficient samples for variability");

    RaceProgress invalid(2, 2);
    Check(invalid.PassCheckpoint(0) && invalid.PassCheckpoint(1),
          "zero-time sequence accepted by pure gate model");
    Lap(invalid, 6.0);
    Check(invalid.IsFinished() &&
          !CalculateRecentLapPaceStatistics(invalid).Valid,
          "unawardable finish does not produce a statistics card");

    RaceProgress wrapped(2, 12);
    for (int t = 2; t <= 13; ++t) Lap(wrapped, static_cast<double>(t));
    const auto w = CalculateRecentLapPaceStatistics(wrapped);
    Check(w.Valid && w.SampleCount == 8 &&
          w.OldestLap == 5 && w.NewestLap == 12 &&
          Near(w.AverageSeconds, 9.5) &&
          Near(w.MedianSeconds, 9.5) &&
          Near(w.NewestVsAverageSeconds, 3.5),
          "12-lap race reports last eight only, not an invented all-run average");
    wrapped.Reset();
    Check(!CalculateRecentLapPaceStatistics(wrapped).Valid,
          "restart discards previous session evidence");

    std::cout << "Recent lap pace statistics passed.\n";
}
