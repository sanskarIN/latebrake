#include "Racing/RecentLapReview.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "Recent lap review FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double actual, double expected)
{
    return std::abs(actual - expected) < 1e-9;
}

void Lap(latebrake::RaceProgress& race, double seconds)
{
    race.Advance(seconds / 2.0);
    Require(race.PassCheckpoint(0), "first sector accepted");
    race.Advance(seconds / 2.0);
    Require(race.PassCheckpoint(1), "last sector accepted");
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(2, 3);
    Require(!BuildRecentLapReview(race).Valid,
            "unfinished race cannot claim a finished lap review");
    Lap(race, 10.0);
    Lap(race, 8.0);
    Require(!BuildRecentLapReview(race).Valid,
            "partially completed event cannot publish a results review");
    Lap(race, 12.0);
    const RecentLapReview report = BuildRecentLapReview(race);
    Require(race.IsFinished() && report.Valid && report.Count == 3,
            "complete actively timed finish shows three real laps");
    Require(report.Rows[0].CompletedLap == 3 &&
            Near(report.Rows[0].LapSeconds, 12.0) &&
            Near(report.Rows[0].SecondsBehindRunBest, 4.0) &&
            report.Rows[0].HasPrevious &&
            Near(report.Rows[0].DeltaToPreviousSeconds, 4.0),
            "newest lap has best gap and positive slower-than-previous delta");
    Require(report.Rows[1].CompletedLap == 2 &&
            report.Rows[1].EqualsRunBest &&
            Near(report.Rows[1].SecondsBehindRunBest, 0.0) &&
            report.Rows[1].HasPrevious &&
            Near(report.Rows[1].DeltaToPreviousSeconds, -2.0),
            "run best is marked and faster-than-previous delta is negative");
    Require(report.Rows[2].CompletedLap == 1 &&
            Near(report.Rows[2].LapSeconds, 10.0) &&
            !report.Rows[2].HasPrevious,
            "oldest recorded lap never invents an earlier comparison");
    Require(report.Rows[3].CompletedLap == 0 &&
            !report.Rows[3].HasPrevious,
            "unused fixed-size rows remain empty");
    race.Reset();
    Require(!BuildRecentLapReview(race).Valid,
            "restart removes prior-session review and best lap");

    RaceProgress single(2, 1);
    Lap(single, 9.0);
    const auto one = BuildRecentLapReview(single);
    Require(one.Valid && one.Count == 1 &&
            one.Rows[0].EqualsRunBest && !one.Rows[0].HasPrevious,
            "single lap shows best without phantom previous");
    Require(!single.PassCheckpoint(0),
            "finished race cannot write fictitious subsequent laps");

    RaceProgress penaltyOnly(2, 1);
    Require(penaltyOnly.AddTimePenalty(5.0), "penalty may be charged");
    Require(penaltyOnly.PassCheckpoint(0) &&
            penaltyOnly.PassCheckpoint(1),
            "ordered but not actively driven finish can occur");
    Require(penaltyOnly.IsFinished() &&
            !BuildRecentLapReview(penaltyOnly).Valid,
            "penalty-only finish must never masquerade as timed result");

    RaceProgress zeroLap(2, 2);
    Require(zeroLap.PassCheckpoint(0) &&
            zeroLap.PassCheckpoint(1),
            "zero-time lap may complete in pure ordered model");
    Lap(zeroLap, 7.0);
    Require(zeroLap.IsFinished() &&
            !BuildRecentLapReview(zeroLap).Valid,
            "invalid initial lap poisons finish result eligibility");

    RaceProgress many(2, 12);
    for (int i = 1; i <= 12; ++i)
    {
        Lap(many, static_cast<double>(i) + 1.0);
    }
    const auto bounded = BuildRecentLapReview(many);
    Require(bounded.Valid && bounded.Count == RecentTimedLaps::Capacity &&
            bounded.Rows[0].CompletedLap == 12 &&
            bounded.Rows[7].CompletedLap == 5,
            "long races show exactly eight physical laps after history wrap");
    Require(!bounded.Rows[7].HasPrevious &&
            bounded.Rows[0].HasPrevious &&
            Near(bounded.Rows[0].DeltaToPreviousSeconds, 1.0) &&
            Near(bounded.Rows[0].SecondsBehindRunBest, 11.0),
            "history boundary never fabricates evicted prior lap data");

    std::cout << "Recent lap review source tests passed.\n";
    return EXIT_SUCCESS;
}
