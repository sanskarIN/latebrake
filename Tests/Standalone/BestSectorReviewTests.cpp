#include "Racing/BestSectorReview.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool yes, const char* msg)
{
    if (!yes)
    {
        std::cerr << "BestSectorReview FAIL: " << msg << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-8;
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(3, 2);
    Check(!BuildBestSectorReview(race).Valid,
          "running race not eligible for finished sector analysis");
    race.Advance(4.0);
    Check(race.PassCheckpoint(0), "lap one gate one");
    race.Advance(5.0);
    Check(race.PassCheckpoint(1), "lap one gate two");
    race.Advance(6.0);
    Check(race.PassCheckpoint(2), "lap one gate three");
    race.Advance(3.0);
    Check(race.PassCheckpoint(0), "lap two gate one");
    race.Advance(7.0);
    Check(race.PassCheckpoint(1), "lap two gate two");
    race.Advance(4.0);
    Check(race.PassCheckpoint(2), "lap two gate three");
    const auto review = BuildBestSectorReview(race);
    Check(review.Valid && review.Count == 3 &&
          Near(review.SectorSeconds[0], 3.0) &&
          Near(review.SectorSeconds[1], 5.0) &&
          Near(review.SectorSeconds[2], 4.0) &&
          Near(review.MixedBestSectorsSeconds, 12.0) &&
          Near(review.SecondsBehindAchievedBest, 2.0),
          "three actual best gate splits, mixed-lap theoretical not official");
    Check(!race.PassCheckpoint(0), "finished cannot accept another split");
    race.Reset();
    Check(!BuildBestSectorReview(race).Valid,
          "new race cannot inherit earlier best gates");

    RaceProgress oneGate(1, 1);
    oneGate.Advance(8.0);
    Check(oneGate.PassCheckpoint(0), "single-gate complete race");
    const auto single = BuildBestSectorReview(oneGate);
    Check(single.Valid && single.Count == 1 &&
          Near(single.MixedBestSectorsSeconds, 8.0) &&
          Near(single.SecondsBehindAchievedBest, 0.0),
          "single gate run has no speculative improvement");

    RaceProgress penalty(1, 1);
    Check(penalty.AddTimePenalty(5.0) && penalty.PassCheckpoint(0),
          "ordered fee-only race completes");
    Check(!BuildBestSectorReview(penalty).Valid,
          "penalty-only finish must not be awarded sector review");

    RaceProgress invalid(2, 2);
    Check(invalid.PassCheckpoint(0) && invalid.PassCheckpoint(1),
          "zero-time first lap");
    invalid.Advance(5.0);
    Check(invalid.PassCheckpoint(0) && invalid.PassCheckpoint(1),
          "second lap");
    Check(!BuildBestSectorReview(invalid).Valid,
          "unrecordable two-lap finish does not get sector claims");

    std::cout << "Best sector review passed.\n";
}
