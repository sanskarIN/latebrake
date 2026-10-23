#include "Racing/LiveRacePaceProjection.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool accepted, const char* label)
{
    if (!accepted)
    {
        std::cerr << "Live race pace FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CompleteLap(latebrake::RaceProgress& race, double seconds)
{
    for (int checkpoint = 0; checkpoint < race.GetCheckpointCount(); ++checkpoint)
    {
        race.Advance(seconds / static_cast<double>(race.GetCheckpointCount()));
        Require(race.PassCheckpoint(checkpoint), "ordered checkpoint pass");
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    Require(!ProjectLiveRacePace(race).Valid,
            "no pace is claimed before finishing a real measured lap");

    CompleteLap(race, 60.0);
    const LiveRacePaceProjection first = ProjectLiveRacePace(race);
    Require(first.Valid && first.SampledLaps == 1 && first.LapsRemaining == 2 &&
            first.ReferenceLapSeconds == 60.0 &&
            first.ProjectedFinishSeconds == 180.0,
            "one timed lap gives stable remaining-laps estimate");
    race.Advance(10.0);
    Require(ProjectLiveRacePace(race).ProjectedFinishSeconds == 180.0,
            "time spent during current lap reduces its remaining reference");

    race.Advance(100.0); // Current lap now exceeds the previous lap pace.
    const auto slow = ProjectLiveRacePace(race);
    Require(slow.Valid && slow.ProjectedFinishSeconds >=
            race.GetElapsedSeconds(),
            "estimate never goes back before the current race clock");
    Require(slow.ProjectedFinishSeconds == 230.0,
            "slow current lap does not claim negative remaining time");

    // Complete the second lap at 170 seconds total (110-second lap).
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(race.PassCheckpoint(checkpoint), "second lap gate accepts");
    }
    const auto two = ProjectLiveRacePace(race);
    Require(two.Valid && two.SampledLaps == 2 &&
            two.ReferenceLapSeconds == 85.0 &&
            two.ProjectedFinishSeconds == 255.0,
            "two observed laps average without leaking an old session");
    race.Advance(70.0);
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(race.PassCheckpoint(checkpoint), "final lap gate accepts");
    }
    Require(race.IsFinished() && !ProjectLiveRacePace(race).Valid,
            "never claim a projected time after authoritative finish");
    race.Reset();
    Require(!ProjectLiveRacePace(race).Valid,
            "restart clears old pace samples and predictions");

    RaceProgress zero(4, 2);
    for (int checkpoint = 0; checkpoint < 4; ++checkpoint)
    {
        Require(zero.PassCheckpoint(checkpoint), "zero-time test ordered gate");
    }
    Require(zero.GetTimedLapCount() == 0 && !ProjectLiveRacePace(zero).Valid,
            "zero-duration laps cannot seed a false projection");

    RaceProgress multi(4, 7);
    CompleteLap(multi, 10.0);
    CompleteLap(multi, 20.0);
    CompleteLap(multi, 30.0);
    CompleteLap(multi, 40.0);
    const auto bounded = ProjectLiveRacePace(multi);
    Require(bounded.Valid && bounded.SampledLaps == 3 &&
            bounded.ReferenceLapSeconds == 30.0 &&
            bounded.ProjectedFinishSeconds == 190.0,
            "rolling reference uses only three most recent timed laps");

    RaceProgress overflow(4, 2);
    CompleteLap(overflow, 604800.0);
    Require(!ProjectLiveRacePace(overflow).Valid,
            "unrealistic projection beyond record clock bound is hidden");

    std::cout << "Live race pace projection passed.\n";
    return EXIT_SUCCESS;
}
