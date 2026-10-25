#include "Racing/FinishTimingEligibility.h"
#include "Racing/RecoveryCharge.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Finish timing eligibility FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void PassLap(latebrake::RaceProgress& race)
{
    Require(race.PassCheckpoint(0) && race.PassCheckpoint(1),
            "ordered two-gate lap accepted");
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceProgress clean(2, 2);
    Require(ValidateFinishTiming(clean) == FinishTimingEligibility::RaceIncomplete,
            "new race cannot award");
    clean.Advance(4.0);
    Require(clean.PassCheckpoint(0), "first gate");
    Require(ValidateFinishTiming(clean) == FinishTimingEligibility::RaceIncomplete,
            "partial lap cannot award");
    Require(clean.PassCheckpoint(1), "first lap finish");
    Require(clean.GetActiveTimedLapCount() == 1,
            "one lap has a real race-clock advancement");
    clean.Advance(3.0);
    Require(clean.PassCheckpoint(0), "next lap first gate");
    Require(clean.AddTimePenalty(5.0), "legitimate fee within active lap");
    clean.Advance(2.0);
    Require(clean.PassCheckpoint(1) && clean.IsFinished(), "race finishes");
    Require(clean.GetTimedLapCount() == 2 &&
            clean.GetActiveTimedLapCount() == 2 &&
            CanAwardFinishedRace(clean),
            "both driven laps award even with recovery penalty");

    RaceProgress zero(2, 2);
    PassLap(zero); // zero-time lap is physically complete but not clocked.
    zero.Advance(4.0);
    PassLap(zero);
    Require(zero.IsFinished() && zero.GetElapsedSeconds() == 4.0 &&
            zero.GetBestLapSeconds() == 4.0,
            "a positive final lap alone can make earlier checks appear valid");
    Require(zero.GetTimedLapCount() == 1 &&
            zero.GetActiveTimedLapCount() == 1,
            "missing first-lap evidence counted accurately");
    Require(ValidateFinishTiming(zero) ==
                FinishTimingEligibility::MissingActiveLapTiming &&
            !CanAwardFinishedRace(zero),
            "zero-time first lap cannot grant gold, credits or records");

    RaceProgress penaltyOnly(2, 2);
    Require(penaltyOnly.AddTimePenalty(5.0), "penalty increases race elapsed");
    PassLap(penaltyOnly);
    Require(penaltyOnly.GetTimedLapCount() == 1 &&
            penaltyOnly.GetActiveTimedLapCount() == 0,
            "penalty-only positive lap is NOT a real active clock sample");
    penaltyOnly.Advance(3.0);
    PassLap(penaltyOnly);
    Require(penaltyOnly.IsFinished() &&
            penaltyOnly.GetTimedLapCount() == 2 &&
            penaltyOnly.GetActiveTimedLapCount() == 1 &&
            !CanAwardFinishedRace(penaltyOnly),
            "penalty-only first lap cannot be laundered by later valid lap");

    RaceProgress allPenalty(2, 1);
    Require(allPenalty.AddTimePenalty(5.0), "penalty-only race charges fee");
    PassLap(allPenalty);
    Require(allPenalty.IsFinished() &&
            ValidateFinishTiming(allPenalty) ==
                FinishTimingEligibility::MissingActiveLapTiming,
            "entirely penalized run has no observed active clock");
    allPenalty.Reset();
    Require(allPenalty.GetTimedLapCount() == 0 &&
            allPenalty.GetActiveTimedLapCount() == 0 &&
            !allPenalty.IsFinished(),
            "restart clears all real-clock evidence and eligibility");

    RaceProgress invalid(2, 1);
    invalid.Advance(std::numeric_limits<double>::quiet_NaN());
    invalid.Advance(std::numeric_limits<double>::infinity());
    invalid.Advance(-10.0);
    Require(invalid.AddTimePenalty(5.0), "invalid deltas did not advance time");
    PassLap(invalid);
    Require(invalid.GetActiveTimedLapCount() == 0 &&
            !CanAwardFinishedRace(invalid),
            "invalid Advance arguments do not establish timing evidence");

    RaceProgress overlong(2, 1);
    overlong.Advance(604801.0);
    PassLap(overlong);
    Require(overlong.GetActiveTimedLapCount() == 1 &&
            ValidateFinishTiming(overlong) ==
                FinishTimingEligibility::UnrecordableClock,
            "overlong genuine lap is not representable by save schema");

    RaceProgress oneGate(1, 1);
    oneGate.Advance(1.5);
    Require(oneGate.PassCheckpoint(0) &&
            CanAwardFinishedRace(oneGate),
            "a genuine one-gate timed event remains eligible");

    // Active evidence is per-lap, not inherited from another lap.
    RaceProgress absentFinal(2, 2);
    absentFinal.Advance(4.0);
    PassLap(absentFinal);
    Require(absentFinal.AddTimePenalty(5.0), "second lap penalty");
    PassLap(absentFinal);
    Require(absentFinal.GetTimedLapCount() == 2 &&
            absentFinal.GetActiveTimedLapCount() == 1 &&
            !CanAwardFinishedRace(absentFinal),
            "zero-driving final lap cannot inherit first-lap timing");

    std::cout << "Finished race clock evidence validated (portable source).\n";
    return EXIT_SUCCESS;
}
