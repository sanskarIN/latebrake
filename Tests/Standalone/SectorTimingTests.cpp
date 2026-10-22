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
        std::cerr << "Sector timing FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double actual, double expected)
{
    return std::abs(actual - expected) < 1.e-9;
}
}

int main()
{
    using latebrake::RaceProgress;
    RaceProgress r(2, 3);
    Require(r.GetLastSectorIndex() == -1 && !r.HasLastSectorComparison(),
            "fresh race has no sector observation");
    Require(r.GetTheoreticalBestLapSeconds() == 0.0 &&
            r.GetBestSectorSeconds(0) == 0.0, "incomplete sectors have no ideal lap");
    Require(r.GetBestSectorSeconds(-1) == 0.0 &&
            r.GetBestSectorSeconds(2) == 0.0, "out-of-range lookups are safe");

    r.Advance(4.0);
    Require(Near(r.GetCurrentSectorSeconds(), 4.0), "running sector clock");
    Require(r.PassCheckpoint(0), "first sector finish");
    Require(r.GetLastSectorIndex() == 0 && !r.HasLastSectorComparison() &&
            Near(r.GetBestSectorSeconds(0), 4.0), "first record has no prior delta");
    Require(Near(r.GetCurrentSectorSeconds(), 0.0) &&
            r.GetTheoreticalBestLapSeconds() == 0.0, "wait for final sector");
    r.Advance(6.0);
    Require(r.PassCheckpoint(1), "first lap finish");
    Require(Near(r.GetTheoreticalBestLapSeconds(), 10.0), "first lap is initial ideal");
    Require(Near(r.GetBestLapSeconds(), 10.0), "ordinary fastest lap unchanged");
    Require(!r.HasLastSectorComparison(), "initial final split has no comparison");

    r.Advance(3.0);
    Require(!r.PassCheckpoint(1), "wrong checkpoint ignored");
    Require(Near(r.GetLastSectorSeconds(), 6.0) &&
            r.GetLastSectorIndex() == 1, "invalid gate leaves prior split intact");
    Require(r.PassCheckpoint(0), "second lap sector one");
    Require(r.HasLastSectorComparison() && r.GetLastSectorIndex() == 0 &&
            Near(r.GetLastSectorDeltaSeconds(), -1.0) &&
            Near(r.GetBestSectorSeconds(0), 3.0), "new sector best shows negative delta");
    r.Advance(5.0);
    Require(r.PassCheckpoint(1), "second lap sector two");
    Require(r.HasLastSectorComparison() &&
            Near(r.GetLastSectorDeltaSeconds(), -1.0) &&
            Near(r.GetBestSectorSeconds(1), 5.0), "sector 2 compares with sector 2 only");
    Require(Near(r.GetTheoreticalBestLapSeconds(), 8.0) &&
            Near(r.GetBestLapSeconds(), 8.0), "ideal and best lap update consistently");

    r.Advance(10.0);
    Require(r.PassCheckpoint(0), "slower third lap sector one");
    Require(Near(r.GetLastSectorDeltaSeconds(), 7.0) &&
            Near(r.GetBestSectorSeconds(0), 3.0), "slower sector cannot overwrite best");
    r.Advance(2.0);
    Require(r.PassCheckpoint(1) && r.IsFinished(), "finish third lap");
    Require(Near(r.GetLastSectorDeltaSeconds(), -3.0) &&
            Near(r.GetBestSectorSeconds(1), 2.0), "faster final sector updates");
    Require(Near(r.GetTheoreticalBestLapSeconds(), 5.0) &&
            Near(r.GetBestLapSeconds(), 8.0),
            "theoretical best combines sectors; never falsely called an achieved lap");
    Require(!r.PassCheckpoint(0), "finished race cannot modify sector timing");
    Require(Near(r.GetTheoreticalBestLapSeconds(), 5.0), "finish freezes best sectors");

    r.Reset();
    Require(r.GetLastSectorIndex() == -1 &&
            !r.HasLastSectorComparison() &&
            r.GetLastSectorDeltaSeconds() == 0.0 &&
            r.GetBestSectorSeconds(0) == 0.0 &&
            r.GetBestSectorSeconds(1) == 0.0 &&
            r.GetTheoreticalBestLapSeconds() == 0.0, "restart clears run-local splits");

    // A t=0 gate is not a valid sector best. It cannot become unbeatable.
    RaceProgress zero(1, 2);
    Require(zero.PassCheckpoint(0) && !zero.HasLastSectorComparison(),
            "zero-second sector doesn't manufacture a fastest time");
    Require(zero.GetBestSectorSeconds(0) == 0.0, "zero isn't stored as best");
    zero.Advance(7.0);
    Require(zero.PassCheckpoint(0) && zero.IsFinished(), "finish second lap");
    Require(Near(zero.GetBestSectorSeconds(0), 7.0) &&
            !zero.HasLastSectorComparison(), "first positive measurement starts baseline");

    // Recovery penalties remain in the real race clock and hence in that split.
    RaceProgress penalty(1, 1);
    penalty.Advance(5.0);
    Require(penalty.AddTimePenalty(5.0) && penalty.PassCheckpoint(0),
            "valid penalty and sector");
    Require(Near(penalty.GetLastSectorSeconds(), 10.0) &&
            Near(penalty.GetBestSectorSeconds(0), 10.0),
            "penalized time is not silently dropped");

    RaceProgress large(std::numeric_limits<int>::max(), 1);
    Require(large.GetCheckpointCount() == RaceProgress::MaxCheckpoints,
            "hostile count is bounded to fixed storage");
    Require(large.GetBestSectorSeconds(RaceProgress::MaxCheckpoints) == 0.0,
            "max boundary lookup stays safe");

    RaceProgress nearMax(2, 1);
    nearMax.Advance(std::numeric_limits<double>::max());
    Require(nearMax.PassCheckpoint(0), "max finite first split can be recorded");
    Require(nearMax.GetBestSectorSeconds(0) ==
            std::numeric_limits<double>::max(), "very large split stays finite");
    Require(nearMax.GetTheoreticalBestLapSeconds() == 0.0,
            "missing second sector doesn't form fake ideal");
    nearMax.Advance(5.0);
    Require(nearMax.PassCheckpoint(1), "no overflow when clock already max");
    Require(nearMax.GetBestSectorSeconds(1) == 0.0 &&
            nearMax.GetTheoreticalBestLapSeconds() == 0.0,
            "overflow/no-time sectors cannot fabricate an ideal");
    std::cout << "Sector timing tests passed.\n";
    return EXIT_SUCCESS;
}
