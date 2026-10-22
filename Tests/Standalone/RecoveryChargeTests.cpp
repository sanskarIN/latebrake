#include "Racing/RecoveryCharge.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Recovery charge FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 1);
    int recoveries = 0;
    race.Advance(3.0);

    Check(TryChargePlayerRecovery(race, recoveries),
          "first recovery applies penalty");
    Check(recoveries == 1 &&
          race.GetElapsedSeconds() == 8.0 &&
          race.GetCurrentSectorSeconds() == 8.0,
          "recovery charge advances counter, race clock and sector");
    Check(TryChargePlayerRecovery(race, recoveries) &&
          recoveries == 2 && race.GetElapsedSeconds() == 13.0,
          "a second legitimate recovery pays its own full fee");
    Check(race.PassCheckpoint(0) && race.GetLastSectorSeconds() == 13.0,
          "last recorded split includes recovery fees");

    race.PassCheckpoint(1);
    race.PassCheckpoint(2);
    Check(race.PassCheckpoint(3) && race.IsFinished(),
          "complete all four gates and finish");
    const double finishedClock = race.GetElapsedSeconds();
    Check(!TryChargePlayerRecovery(race, recoveries) &&
          recoveries == 2 && race.GetElapsedSeconds() == finishedClock,
          "finished race cannot charge or authorize teleport");

    RaceProgress overflow(1, 1);
    overflow.Advance(std::numeric_limits<double>::max());
    int unaffected = 0;
    Check(!TryChargePlayerRecovery(overflow, unaffected) &&
          unaffected == 0 &&
          std::isfinite(overflow.GetElapsedSeconds()),
          "clock overflow rejects free teleport and preserves count");

    RaceProgress saturated(1, 1);
    int maxCount = std::numeric_limits<int>::max();
    Check(!TryChargePlayerRecovery(saturated, maxCount) &&
          maxCount == std::numeric_limits<int>::max() &&
          saturated.GetElapsedSeconds() == 0.0,
          "saturated recovery counter cannot permit unaccounted penalty");
    int almostMaxCount = std::numeric_limits<int>::max() - 1;
    Check(TryChargePlayerRecovery(saturated, almostMaxCount) &&
          almostMaxCount == std::numeric_limits<int>::max() &&
          saturated.GetElapsedSeconds() == 5.0,
          "last representable charge is valid exactly once");
    Check(!TryChargePlayerRecovery(saturated, almostMaxCount) &&
          saturated.GetElapsedSeconds() == 5.0,
          "cannot overflow counter on next request");

    RaceProgress negative(1, 1);
    int invalid = -1;
    Check(!TryChargePlayerRecovery(negative, invalid) &&
          invalid == -1 && negative.GetElapsedSeconds() == 0.0,
          "negative corrupt recovery counter fails closed");

    std::cout << "Recovery charge tests passed.\n";
    return EXIT_SUCCESS;
}
