#include "Racing/RaceProgress.h"
#include "Racing/RecoveryCharge.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Lap penalty attribution FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(2, 3);
    Require(race.GetCurrentLapPenaltySeconds() == 0.0 &&
            race.GetLastLapPenaltySeconds() == 0.0,
            "no invented lap recovery fees");
    race.Advance(4.0);
    Require(race.AddTimePenalty(5.0) &&
            race.GetCurrentLapPenaltySeconds() == 5.0 &&
            race.GetLastLapPenaltySeconds() == 0.0,
            "live lap owns a committed recovery fee");
    Require(race.PassCheckpoint(0), "first gate");
    Require(race.GetCurrentLapPenaltySeconds() == 5.0,
            "sector gate does not prematurely reset the lap fee");
    race.Advance(3.0);
    Require(race.PassCheckpoint(1) &&
            race.GetLastLapSeconds() == 12.0 &&
            race.GetLastLapPenaltySeconds() == 5.0 &&
            race.GetCurrentLapPenaltySeconds() == 0.0,
            "ordered finish snapshots and clears per-lap attribution");
    Require(race.GetTotalPenaltySeconds() == 5.0,
            "completing lap does not erase total event fees");

    // A penalty-free subsequent lap must not inherit the preceding fee.
    race.Advance(3.0);
    Require(race.PassCheckpoint(0), "lap 2 first gate");
    race.Advance(4.0);
    Require(race.PassCheckpoint(1), "lap 2 final gate");
    Require(race.GetLastLapPenaltySeconds() == 0.0 &&
            race.GetTotalPenaltySeconds() == 5.0 &&
            race.GetCurrentLapPenaltySeconds() == 0.0,
            "clean lap is not mislabeled as having earlier penalties");

    race.Advance(1.0);
    Require(race.AddTimePenalty(0.125) && race.AddTimePenalty(5.0),
            "multiple representable fees on final lap");
    Require(race.GetCurrentLapPenaltySeconds() == 5.125 &&
            race.GetTotalPenaltySeconds() == 10.125,
            "current attribution sums only final lap transactions");
    const double before = race.GetCurrentLapPenaltySeconds();
    Require(!race.AddTimePenalty(std::numeric_limits<double>::infinity()) &&
            !race.AddTimePenalty(-1.0) &&
            race.GetCurrentLapPenaltySeconds() == before,
            "malformed charges cannot contaminate a lap");
    Require(race.PassCheckpoint(0), "final lap first gate");
    race.Advance(2.0);
    Require(race.PassCheckpoint(1) && race.IsFinished() &&
            race.GetLastLapPenaltySeconds() == 5.125 &&
            race.GetCurrentLapPenaltySeconds() == 0.0,
            "last finished lap retains attribution, current resets");
    Require(!race.AddTimePenalty(5.0) &&
            race.GetLastLapPenaltySeconds() == 5.125,
            "finished race cannot rewrite final-lap penalty");
    race.Reset();
    Require(race.GetTotalPenaltySeconds() == 0.0 &&
            race.GetCurrentLapPenaltySeconds() == 0.0 &&
            race.GetLastLapPenaltySeconds() == 0.0 &&
            race.GetTimePenaltyCount() == 0,
            "new event drops every penalty attribution field");

    RaceProgress single(1, 2);
    int recoveryCount = 0;
    Require(TryChargePlayerRecovery(single, recoveryCount) &&
            single.PassCheckpoint(0) &&
            single.GetLastLapPenaltySeconds() == 5.0,
            "single-sector races snapshot fees too");
    Require(single.GetCurrentLapPenaltySeconds() == 0.0,
            "single-gate next lap starts clean");

    std::cout << "Lap penalty attribution passed (portable source).\n";
    return EXIT_SUCCESS;
}
