#include "Racing/CurrentLapPotential.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Current lap potential FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1e-8;
}
void Lap(latebrake::RaceProgress& race, double a, double b,
         double c, double d)
{
    const double splits[] = {a, b, c, d};
    for (int sector = 0; sector < 4; ++sector)
    {
        race.Advance(splits[sector]);
        Require(race.PassCheckpoint(sector), "ordered gate");
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    Require(!EstimateCurrentLapPotential(race).Valid,
            "no estimate before a completed timed lap");

    Lap(race, 10.0, 12.0, 8.0, 10.0);
    const auto initial = EstimateCurrentLapPotential(race);
    Require(initial.Valid && initial.RemainingSectors == 4 &&
            Near(initial.CurrentLapSeconds, 0.0) &&
            Near(initial.PotentialLapSeconds, 40.0) &&
            Near(initial.DeltaToBestLapSeconds, 0.0),
            "new lap starts with actual same-run sector reference");

    race.Advance(4.0);
    Require(Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 40.0),
            "time spent below current split best does not double-count");
    race.Advance(18.0);
    const auto slow = EstimateCurrentLapPotential(race);
    Require(slow.Valid && Near(slow.PotentialLapSeconds, 52.0) &&
            Near(slow.DeltaToBestLapSeconds, 12.0),
            "slower current sector cannot project elapsed time backwards");

    Require(race.PassCheckpoint(0), "second lap first gate");
    Require(EstimateCurrentLapPotential(race).RemainingSectors == 3 &&
            Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 52.0),
            "passed sector is no longer included in remaining bests");
    race.Advance(6.0);
    Require(Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 52.0),
            "in-progress faster segment leaves estimated minimum unchanged");
    Require(race.PassCheckpoint(1), "second lap second gate");
    Require(Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 46.0),
            "new observed best split improves potential without rewriting laps");

    race.Advance(13.0);
    Require(Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 51.0),
            "slow third sector uses only future best, not impossible rewind");
    Require(race.PassCheckpoint(2), "second lap third gate");
    race.Advance(9.0);
    Require(Near(EstimateCurrentLapPotential(race).PotentialLapSeconds, 51.0),
            "final unfinished sector includes only its remaining best time");
    Require(race.PassCheckpoint(3), "second lap finish");
    const auto third = EstimateCurrentLapPotential(race);
    Require(third.Valid && Near(third.PotentialLapSeconds, 33.0) &&
            Near(third.DeltaToBestLapSeconds, -7.0) &&
            Near(race.GetBestLapSeconds(), 40.0),
            "theoretical potential may beat achieved best without awarding it");

    Lap(race, 1.0, 1.0, 1.0, 1.0);
    Require(race.IsFinished() && !EstimateCurrentLapPotential(race).Valid,
            "no live prediction after the official finish");
    race.Reset();
    Require(!EstimateCurrentLapPotential(race).Valid,
            "reset cannot reuse old sector reference");

    RaceProgress missing(4, 2);
    missing.Advance(5.0);
    Require(missing.PassCheckpoint(0), "first split");
    Require(missing.PassCheckpoint(1), "zero second split");
    missing.Advance(5.0);
    Require(missing.PassCheckpoint(2) && missing.PassCheckpoint(3),
            "zero-split lap can still complete");
    Require(missing.GetTimedLapCount() == 1 &&
            !EstimateCurrentLapPotential(missing).Valid,
            "missing positive best sector invalidates proposed lower bound");

    RaceProgress single(1, 3);
    single.Advance(5.0);
    Require(single.PassCheckpoint(0), "single-gate lap completes");
    const auto one = EstimateCurrentLapPotential(single);
    Require(one.Valid && one.RemainingSectors == 1 &&
            Near(one.PotentialLapSeconds, 5.0),
            "single checkpoint configuration remains safe");

    RaceProgress expired(4, 3);
    Lap(expired, 10.0, 12.0, 8.0, 10.0);
    expired.Advance(604800.0);
    Require(!EstimateCurrentLapPotential(expired).Valid,
            "seven-day record clock cap also limits live estimate");

    std::cout << "Current lap sector potential passed.\n";
    return EXIT_SUCCESS;
}
