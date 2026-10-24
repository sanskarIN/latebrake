#include "Racing/RaceProgress.h"
#include "Racing/RecoveryCharge.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* label)
{
    if (!ok)
    {
        std::cerr << "Penalty accounting FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(2, 1);
    Require(race.GetTimePenaltyCount() == 0 &&
            race.GetTotalPenaltySeconds() == 0.0,
            "new race starts without fabricated penalties");
    race.Advance(2.25);
    Require(race.AddTimePenalty(5.0), "first legal penalty accepted");
    Require(race.GetTimePenaltyCount() == 1 &&
            race.GetTotalPenaltySeconds() == 5.0 &&
            race.GetElapsedSeconds() == 7.25,
            "first charge is reflected in time and accounting");
    Require(race.AddTimePenalty(0.125), "subsecond fee accepted");
    Require(race.GetTimePenaltyCount() == 2 &&
            race.GetTotalPenaltySeconds() == 5.125 &&
            race.GetElapsedSeconds() == 7.375,
            "independent fees are accumulated without lap drift");
    Require(!race.AddTimePenalty(0.0) &&
            !race.AddTimePenalty(-5.0) &&
            !race.AddTimePenalty(std::numeric_limits<double>::infinity()) &&
            !race.AddTimePenalty(std::numeric_limits<double>::quiet_NaN()),
            "invalid fees rejected");
    Require(race.GetTimePenaltyCount() == 2 &&
            race.GetTotalPenaltySeconds() == 5.125 &&
            race.GetElapsedSeconds() == 7.375,
            "rejected fees are fully atomic");
    Require(race.PassCheckpoint(0) &&
            race.GetLastSectorSeconds() == 7.375,
            "sector and lap timing still include charged penalties");
    race.Advance(2.0);
    Require(race.PassCheckpoint(1) && race.IsFinished(),
            "legitimate lap can finish");
    Require(race.GetLastLapSeconds() == 9.375 &&
            race.GetTotalPenaltySeconds() == 5.125,
            "penalty telemetry does not alter race clock or split semantics");
    Require(!race.AddTimePenalty(5.0) &&
            race.GetTimePenaltyCount() == 2,
            "finished event cannot accept more penalties");
    race.Reset();
    Require(race.GetTimePenaltyCount() == 0 &&
            race.GetTotalPenaltySeconds() == 0.0 &&
            race.GetElapsedSeconds() == 0.0,
            "restart drops penalty history");

    RaceProgress recover(4, 1);
    int recoveries = 0;
    Require(TryChargePlayerRecoveryIfReady(
        recover, recoveries, true, false, true), "ready recovery succeeds");
    Require(recoveries == 1 &&
            recover.GetTimePenaltyCount() == 1 &&
            recover.GetTotalPenaltySeconds() == RecoveryPenaltySeconds,
            "successful teleport authorization pays exactly once");
    Require(!TryChargePlayerRecoveryIfReady(
        recover, recoveries, true, true, true) &&
            recoveries == 1 && recover.GetTimePenaltyCount() == 1,
            "pause rejects both teleport authorization and accounting");

    RaceProgress precision(1, 1);
    const double enormous = std::ldexp(1.0, 54);
    precision.Advance(enormous);
    const double before = precision.GetElapsedSeconds();
    Require(!precision.AddTimePenalty(5.0) &&
            precision.GetElapsedSeconds() == before &&
            precision.GetTimePenaltyCount() == 0 &&
            precision.GetTotalPenaltySeconds() == 0.0,
            "undercharged huge-clock fee cannot corrupt audit total");
    std::cout << "Penalty accounting passed (portable source).\n";
    return EXIT_SUCCESS;
}
