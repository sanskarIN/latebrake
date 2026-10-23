#include "Racing/RecoveryCharge.h"
#include "Racing/PlayerRaceClockReadiness.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "Player recovery readiness FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;

    RaceProgress race(4, 1);
    int recoveries = 0;
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, false, false, true),
            "incomplete circuit cannot charge a recovery");
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, true, true, true),
            "world pause or active menu cannot charge a recovery");
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, true, false, false),
            "absent or unpossessed local player cannot charge a recovery");
    Require(recoveries == 0 && race.GetElapsedSeconds() == 0.0,
            "all readiness rejections preserve clock and recovery count");

    // The actual checkpoint/timer gate and recovery authorization must agree
    // on the same readiness flags, independently of Unreal's pause callbacks.
    for (int circuit = 0; circuit != 2; ++circuit)
    {
        for (int inactive = 0; inactive != 2; ++inactive)
        {
            for (int driver = 0; driver != 2; ++driver)
            {
                const bool mayScore = CanScorePlayerCheckpoint(
                    race, circuit != 0, inactive != 0, driver != 0);
                const double before = race.GetElapsedSeconds();
                const int oldCount = recoveries;
                const bool recovered = TryChargePlayerRecoveryIfReady(
                    race, recoveries, circuit != 0, inactive != 0, driver != 0);
                Require(mayScore == recovered,
                        "active unfinished race readiness matches gate scoring");
                if (recovered)
                {
                    Require(recoveries == oldCount + 1 &&
                            race.GetElapsedSeconds() == before + RecoveryPenaltySeconds,
                            "authorized recovery charges entire five-second fee");
                }
                else
                {
                    Require(recoveries == oldCount &&
                            race.GetElapsedSeconds() == before,
                            "blocked recovery never changes either counter or clock");
                }
            }
        }
    }

    const double beforePause = race.GetElapsedSeconds();
    const int countBeforePause = recoveries;
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, true, true, true) &&
            race.GetElapsedSeconds() == beforePause &&
            recoveries == countBeforePause,
            "pause after a paid recovery does not allow another payment");
    Require(TryChargePlayerRecoveryIfReady(race, recoveries, true, false, true) &&
            recoveries == countBeforePause + 1 &&
            race.GetElapsedSeconds() == beforePause + 5.0,
            "unpaused race permits another independently charged recovery");

    for (int gate = 0; gate < 4; ++gate)
    {
        Require(race.PassCheckpoint(gate), "finish requires correctly ordered gates");
    }
    Require(race.IsFinished(), "race is now finished");
    const double finalClock = race.GetElapsedSeconds();
    const int finalCount = recoveries;
    Require(!TryChargePlayerRecoveryIfReady(race, recoveries, true, false, true) &&
            recoveries == finalCount && race.GetElapsedSeconds() == finalClock,
            "finished race cannot teleport or modify locked result");

    race.Reset();
    recoveries = 0;
    Require(TryChargePlayerRecoveryIfReady(race, recoveries, true, false, true) &&
            recoveries == 1 && race.GetElapsedSeconds() == 5.0,
            "legitimate new race can charge a recovery after restart");

    RaceProgress saturated(1, 1);
    int countLimit = std::numeric_limits<int>::max();
    Require(!TryChargePlayerRecoveryIfReady(saturated, countLimit, true, false, true) &&
            countLimit == std::numeric_limits<int>::max() &&
            saturated.GetElapsedSeconds() == 0.0,
            "counter overflow rejects a nominally playable recovery");

    RaceProgress extreme(1, 1);
    extreme.Advance(std::ldexp(1.0, 54));
    int enormousCount = 1;
    const double enormousClock = extreme.GetElapsedSeconds();
    Require(!TryChargePlayerRecoveryIfReady(extreme, enormousCount, true, false, true) &&
            enormousCount == 1 && extreme.GetElapsedSeconds() == enormousClock,
            "unrepresentable full penalty cannot authorize teleport");

    std::cout << "Player recovery readiness tests passed.\n";
    return EXIT_SUCCESS;
}
