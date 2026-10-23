#include "Racing/PlayerRaceClockReadiness.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Player race clock readiness FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress race(1, 1);
    Require(!CanScorePlayerCheckpoint(race, true, false, false) &&
            !CanScorePlayerCheckpoint(race, false, false, true) &&
            !CanScorePlayerCheckpoint(race, true, true, true) &&
            CanScorePlayerCheckpoint(race, true, false, true),
            "no player, incomplete track or menu pause cannot score gates");

    Require(!AdvancePlayerRaceClockIfReady(race, true, false, false, 5.0),
            "fully built track without local player cannot run the clock");
    Require(!AdvancePlayerRaceClockIfReady(race, false, false, true, 5.0),
            "player cannot time an incomplete track");
    Require(!AdvancePlayerRaceClockIfReady(race, true, true, true, 5.0),
            "paused world cannot time the local player");
    Require(!AdvancePlayerRaceClockIfReady(race, true, false, true,
                std::numeric_limits<double>::quiet_NaN()),
            "malformed frame time is rejected");
    Require(!AdvancePlayerRaceClockIfReady(race, true, false, true, 0.0) &&
            race.GetElapsedSeconds() == 0.0,
            "blocked steps do not accumulate any time");

    Require(AdvancePlayerRaceClockIfReady(race, true, false, true, 2.25) &&
            race.GetElapsedSeconds() == 2.25,
            "real local player can advance a ready active clock");
    Require(!AdvancePlayerRaceClockIfReady(race, true, false, false, 100.0) &&
            race.GetElapsedSeconds() == 2.25,
            "losing the player during a race freezes accumulated time");
    Require(AdvancePlayerRaceClockIfReady(race, true, false, true, 1.75) &&
            race.GetElapsedSeconds() == 4.0,
            "returning player resumes only active frame time");

    Require(race.PassCheckpoint(0) && race.IsFinished() &&
            race.GetBestLapSeconds() == 4.0,
            "legitimate ordered finish uses only playable race time");
    Require(!AdvancePlayerRaceClockIfReady(race, true, false, true, 15.0) &&
            race.GetElapsedSeconds() == 4.0,
            "finished result remains frozen even with a valid player");

    race.Reset();
    Require(!AdvancePlayerRaceClockIfReady(race, true, false, false, 8.0) &&
            race.GetElapsedSeconds() == 0.0,
            "restart while missing a player cannot produce ghost elapsed time");
    Require(AdvancePlayerRaceClockIfReady(race, true, false, true, 1.0) &&
            race.GetElapsedSeconds() == 1.0,
            "clock resumes after valid player returns to restarted race");

    std::cout << "Player race clock readiness policy passed.\n";
    return EXIT_SUCCESS;
}
