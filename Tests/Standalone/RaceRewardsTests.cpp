#include "Racing/RaceRewards.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "Rewards test FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::CalculateRaceCredits;
    using latebrake::EventKind;
    Require(CalculateRaceCredits(EventKind::Circuit, 120.0, 0) == 650, "gold clean circuit");
    Require(CalculateRaceCredits(EventKind::Circuit, 200.0, 1) == 525, "silver circuit with recovery");
    Require(CalculateRaceCredits(EventKind::Circuit, 400.0, 5) == 450, "circuit participation");
    Require(CalculateRaceCredits(EventKind::TimeAttack, 50.0, 0) == 380, "gold clean time attack");
    Require(CalculateRaceCredits(EventKind::TimeAttack, 100.0, 1) == 180, "slow time attack");
    Require(CalculateRaceCredits(EventKind::TimeAttack, 90.0, 0) == 305, "silver boundary time attack");
    Require(CalculateRaceCredits(EventKind::Circuit, 0.0, 0) == 0, "zero time invalid");
    Require(CalculateRaceCredits(EventKind::Circuit, -1.0, 0) == 0, "negative time invalid");
    Require(CalculateRaceCredits(EventKind::Circuit, 100.0, -1) == 0, "negative recovery count invalid");
    Require(CalculateRaceCredits(EventKind::Circuit,
        std::numeric_limits<double>::infinity(), 0) == 0, "infinite time invalid");
    // A volatile runtime value prevents MSVC C4756 from constant-folding
    // an out-of-range float conversion in the sanitizer's dead branch.
    volatile double enormousElapsed = 1e100;
    Require(CalculateRaceCredits(EventKind::Circuit, 604800.0, 0) == 500,
            "seven-day boundary retains capped participation plus clean bonus");
    Require(CalculateRaceCredits(EventKind::Circuit, 604800.01, 0) == 0,
            "finite overlong circuit cannot grant participation credits");
    Require(CalculateRaceCredits(EventKind::TimeAttack, enormousElapsed, 0) == 0,
            "unrepresentable time-attack clock cannot earn currency");
    Require(CalculateRaceCredits(EventKind::Circuit, 1e-100, 0) == 0,
            "positive subnormal clock cannot be used for artificial gold");
    Require(CalculateRaceCredits(static_cast<EventKind>(123), 100.0, 0) == 0,
        "unknown event does not receive credits");
    std::cout << "Race rewards tests passed.\n";
    return EXIT_SUCCESS;
}
