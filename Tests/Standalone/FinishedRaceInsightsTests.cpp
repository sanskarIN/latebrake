#include "Racing/FinishedRaceInsights.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Finished race insights FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double actual, double expected)
{
    return std::abs(actual - expected) < 1e-9;
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(2, 3);
    Require(!BuildFinishedRaceInsights(race).Valid,
            "running race cannot expose a finished results card");
    race.Advance(4.0);
    Require(race.PassCheckpoint(0), "lap one sector one");
    race.Advance(6.0);
    Require(race.PassCheckpoint(1), "lap one sector two");
    Require(race.GetTimedLapCount() == 1,
            "one properly timed lap tracked independently of completed laps");
    Require(!BuildFinishedRaceInsights(race).Valid,
            "intermediate lap does not become final results");
    race.Advance(3.0);
    Require(race.PassCheckpoint(0), "lap two first sector");
    race.Advance(5.0);
    Require(race.PassCheckpoint(1), "lap two second sector");
    race.Advance(10.0);
    Require(race.PassCheckpoint(0), "lap three first sector");
    race.Advance(2.0);
    Require(race.PassCheckpoint(1) && race.IsFinished(),
            "three genuine laps complete the race");
    const FinishedRaceInsights report = BuildFinishedRaceInsights(race);
    Require(report.Valid && report.CompletedLaps == 3 && report.TimedLaps == 3,
            "valid timed laps summarized");
    Require(Near(report.BestLapSeconds, 8.0) &&
            Near(report.LastLapSeconds, 12.0),
            "actual best and final lap are sourced from race state");
    Require(report.HasTheoreticalGap &&
            Near(report.TheoreticalBestLapSeconds, 5.0) &&
            Near(report.BestToTheoreticalGapSeconds, 3.0),
            "within-run ideal is not passed off as achieved record");
    Require(report.HasLapSpread && Near(report.LapSpreadSeconds, 4.0),
            "lap consistency displays max minus min measured lap");
    Require(race.GetRecentTimedLapCount() == 3 &&
            race.GetRecentTimedLap(0).CompletedLap == 3 &&
            race.GetRecentTimedLap(0).Seconds == 12.0 &&
            race.GetRecentTimedLap(1).CompletedLap == 2 &&
            race.GetRecentTimedLap(1).Seconds == 8.0 &&
            race.GetRecentTimedLap(2).CompletedLap == 1 &&
            race.GetRecentTimedLap(2).Seconds == 10.0,
            "recent lap history contains actual timed lap ordering");
    Require(report.HasLatestVsPrevious &&
            report.LatestTimedLapNumber == 3 &&
            report.PreviousTimedLapNumber == 2 &&
            Near(report.LatestTimedLapSeconds, 12.0) &&
            Near(report.PreviousTimedLapSeconds, 8.0) &&
            Near(report.LatestVsPreviousDeltaSeconds, 4.0),
            "last two timed laps compare physical laps, not sector bests");
    Require(!race.PassCheckpoint(0) &&
            race.GetTimedLapCount() == 3,
            "finished race cannot fabricate additional timed laps");

    race.Reset();
    Require(race.GetRecentTimedLapCount() == 0 &&
            !race.GetRecentTimedLap(0).Valid(),
            "restart clears all previous history");
    Require(!BuildFinishedRaceInsights(race).Valid &&
            race.GetTimedLapCount() == 0,
            "restart clears insights and counted laps");

    RaceProgress oneLap(2, 1);
    oneLap.Advance(5.0);
    Require(oneLap.PassCheckpoint(0), "single-lap first sector");
    oneLap.Advance(5.0);
    Require(oneLap.PassCheckpoint(1), "single-lap finish");
    const FinishedRaceInsights single = BuildFinishedRaceInsights(oneLap);
    Require(!single.HasLatestVsPrevious &&
            single.LatestTimedLapNumber == 1 &&
            single.PreviousTimedLapNumber == 0,
            "one-lap time attack does not invent previous lap pace");
    Require(single.Valid && single.TimedLaps == 1 &&
            !single.HasLapSpread &&
            single.HasTheoreticalGap &&
            Near(single.BestToTheoreticalGapSeconds, 0.0),
            "one-lap events show no invented multi-lap consistency measure");

    RaceProgress zeroDuration(1, 1);
    Require(zeroDuration.PassCheckpoint(0) && zeroDuration.IsFinished(),
            "zero-time gate sequence may finish underlying ordered model");
    Require(zeroDuration.GetTimedLapCount() == 0 &&
            !BuildFinishedRaceInsights(zeroDuration).Valid,
            "a zero-time finish is never presented as a timed result");

    RaceProgress mixed(1, 2);
    Require(mixed.PassCheckpoint(0) && mixed.GetTimedLapCount() == 0,
            "initial t=0 lap excluded from timed-lap count");
    mixed.Advance(7.0);
    Require(mixed.PassCheckpoint(0), "second physically timed lap");
    const FinishedRaceInsights mixedReport = BuildFinishedRaceInsights(mixed);
    Require(!mixedReport.HasLatestVsPrevious &&
            mixedReport.LatestTimedLapNumber == 2 &&
            mixedReport.PreviousTimedLapNumber == 0 &&
            mixed.GetRecentTimedLapCount() == 1,
            "zero-duration first lap cannot become comparison baseline");
    Require(!mixedReport.Valid &&
            !CanAwardFinishedRace(mixed),
            "a zero-duration first lap cannot become awardable final insights");

    RaceProgress tooLong(1, 1);
    tooLong.Advance(604801.0);
    Require(tooLong.PassCheckpoint(0) &&
            !BuildFinishedRaceInsights(tooLong).Valid,
            "unrecordable elapsed clocks cannot produce a results card");

    RaceProgress limit(1, 1);
    limit.Advance(604800.0);
    Require(limit.PassCheckpoint(0), "seven-day inclusive clock boundary");
    Require(BuildFinishedRaceInsights(limit).Valid,
            "results align with existing SaveGame seven-day boundary");

    std::cout << "Finished race insight policy passed.\n";
    return EXIT_SUCCESS;
}
