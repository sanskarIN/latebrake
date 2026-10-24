#include "Racing/LiveSectorTarget.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool condition, const char* label)
{
    if (!condition)
    {
        std::cerr << "Live sector target FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-8;
}
void CompleteLap(latebrake::RaceProgress& race,
                 double a, double b, double c, double d)
{
    const double splits[]{a, b, c, d};
    for (int gate = 0; gate != 4; ++gate)
    {
        race.Advance(splits[gate]);
        Require(race.PassCheckpoint(gate), "ordered lap");
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 3);
    Require(!MeasureLiveSectorTarget(race).Valid,
            "no unearned sector best before a timed lap");
    CompleteLap(race, 10.0, 8.0, 6.0, 6.0);
    const auto initial = MeasureLiveSectorTarget(race);
    Require(initial.Valid && initial.SectorIndex == 0 &&
            Near(initial.BestSectorSeconds, 10.0) &&
            Near(initial.SecondsUntilBest, 10.0) && !initial.OverBest,
            "fresh lap compares to identical measured sector");

    race.Advance(4.0);
    const auto partial = MeasureLiveSectorTarget(race);
    Require(partial.Valid && Near(partial.ElapsedSectorSeconds, 4.0) &&
            Near(partial.SecondsUntilBest, 6.0) &&
            Near(partial.SecondsOverBest, 0.0),
            "current unfinished split gets remaining measured time");

    race.Advance(6.0);
    Require(Near(MeasureLiveSectorTarget(race).SecondsUntilBest, 0.0) &&
            !MeasureLiveSectorTarget(race).OverBest,
            "exact best is not falsely labeled over");
    race.Advance(2.5);
    const auto over = MeasureLiveSectorTarget(race);
    Require(over.Valid && over.OverBest &&
            Near(over.SecondsOverBest, 2.5) &&
            Near(over.SecondsUntilBest, 0.0),
            "overtime is positive only after the real reference");
    Require(race.PassCheckpoint(0), "second lap first gate");
    const auto next = MeasureLiveSectorTarget(race);
    Require(next.Valid && next.SectorIndex == 1 &&
            Near(next.BestSectorSeconds, 8.0) &&
            Near(next.SecondsUntilBest, 8.0),
            "next sector uses its own record not another gate's");

    race.Advance(3.0);
    Require(race.PassCheckpoint(1), "new faster second sector");
    Require(Near(MeasureLiveSectorTarget(race).BestSectorSeconds, 6.0),
            "later sector has distinct best");
    race.Advance(1.0);
    Require(race.PassCheckpoint(2), "new faster third sector");
    race.Advance(2.0);
    Require(race.PassCheckpoint(3), "second lap completed");
    Require(Near(MeasureLiveSectorTarget(race).BestSectorSeconds, 10.0),
            "sector target remains bound to its individual gate index");

    CompleteLap(race, 1.0, 1.0, 1.0, 1.0);
    Require(race.IsFinished() && !MeasureLiveSectorTarget(race).Valid,
            "finished race has no future sector target");
    race.Reset();
    Require(!MeasureLiveSectorTarget(race).Valid,
            "restart cannot carry previous best");

    RaceProgress missing(4, 3);
    // A full timed lap can complete with a zero-duration second sector.
    missing.Advance(5.0);
    Require(missing.PassCheckpoint(0), "missing setup first gate");
    Require(missing.PassCheckpoint(1), "missing setup zero-second gate");
    missing.Advance(5.0);
    Require(missing.PassCheckpoint(2) && missing.PassCheckpoint(3),
            "missing setup completes a timed lap");
    Require(missing.GetTimedLapCount() == 1, "first timed lap completed");
    Require(missing.PassCheckpoint(0), "next lap first gate");
    Require(!MeasureLiveSectorTarget(missing).Valid,
            "missing positive sector reference never becomes zero-time target");

    RaceProgress single(1, 2);
    single.Advance(12.0);
    Require(single.PassCheckpoint(0), "single-gate timed lap");
    Require(MeasureLiveSectorTarget(single).Valid &&
            Near(MeasureLiveSectorTarget(single).BestSectorSeconds, 12.0),
            "single sector event supported");

    RaceProgress impossible(4, 3);
    CompleteLap(impossible, 1.0, 1.0, 1.0, 1.0);
    impossible.Advance(604800.0);
    Require(!MeasureLiveSectorTarget(impossible).Valid,
            "out-of-bounds elapsed clock does not display misleading comparison");

    std::cout << "Live sector target passed.\n";
    return EXIT_SUCCESS;
}
