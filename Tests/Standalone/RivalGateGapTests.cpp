#include "Racing/RivalGateGap.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "Rival credited gate gap FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    const auto compare = [](int playerLaps, int playerNext,
                            int rivalLaps, int rivalNext) {
        return CompareRivalCreditedGates({
            4, 3, playerLaps, playerNext, rivalLaps, rivalNext,
            true, true, true});
    };

    const auto start = compare(0, 0, 0, 0);
    Require(start.Valid && start.Kind == RivalGateGapKind::SameCreditedGateCount &&
            start.CreditedGateDelta == 0 && start.PlayerCreditedGates == 0,
            "no completed gate is not a physical shared position");

    const auto playerAhead = compare(1, 2, 0, 3);
    Require(playerAhead.Valid &&
            playerAhead.Kind == RivalGateGapKind::PlayerMoreCreditedGates &&
            playerAhead.PlayerCreditedGates == 6 &&
            playerAhead.RivalCreditedGates == 3 &&
            playerAhead.CreditedGateDelta == 3,
            "compare cumulative lap and gate count without geometric claims");
    const auto rivalAhead = compare(0, 3, 1, 2);
    Require(rivalAhead.Valid &&
            rivalAhead.Kind == RivalGateGapKind::RivalMoreCreditedGates &&
            rivalAhead.CreditedGateDelta == -3,
            "reversal should reverse signed credited gate gap");
    const auto acrossLap = compare(1, 0, 0, 3);
    Require(acrossLap.Valid && acrossLap.CreditedGateDelta == 1,
            "lap rollover counts exactly one gate, not four extra");
    const auto evenDifferentLaps = compare(1, 0, 1, 0);
    Require(evenDifferentLaps.Valid &&
            evenDifferentLaps.Kind == RivalGateGapKind::SameCreditedGateCount,
            "same accepted counts cannot authorize race position");

    const auto finished = compare(3, 0, 2, 3);
    Require(finished.Valid && finished.PlayerCreditedGates == 12 &&
            finished.CreditedGateDelta == 1,
            "qualified finished route has exactly all credited gates");
    const auto bothFinish = compare(3, 0, 3, 0);
    Require(bothFinish.Valid && bothFinish.CreditedGateDelta == 0,
            "equal finished gate totals are not official finish placement");

    for (int player = 0; player <= 12; ++player)
    {
        for (int rival = 0; rival <= 12; ++rival)
        {
            const auto value = compare(player / 4, player % 4,
                                       rival / 4, rival % 4);
            Require(value.Valid && value.CreditedGateDelta == player - rival,
                    "complete 3-lap gate grid yields exact signed difference");
            const auto swapped = compare(rival / 4, rival % 4,
                                         player / 4, player % 4);
            Require(swapped.Valid &&
                    swapped.CreditedGateDelta == -value.CreditedGateDelta,
                    "comparison stays antisymmetric for both drivers");
        }
    }

    RivalGateGapSample sample{4, 3, 1, 2, 0, 1, true, true, true};
    Require(CompareRivalCreditedGates(sample).Valid,
            "well-formed circuit and real physical cars allow advisory data");
    for (int* field : {&sample.GateCount, &sample.RequiredLaps,
                       &sample.PlayerCompletedLaps, &sample.PlayerNextGate,
                       &sample.RivalCompletedLaps, &sample.RivalNextGate})
    {
        const int original = *field;
        *field = -1;
        Require(!CompareRivalCreditedGates(sample).Valid,
                "negative/invalid configuration or progress fails closed");
        *field = std::numeric_limits<int>::max();
        Require(!CompareRivalCreditedGates(sample).Valid,
                "oversized unbounded configuration or progress fails closed");
        *field = original;
    }
    sample.PlayerCompletedLaps = 3;
    sample.PlayerNextGate = 1;
    Require(!CompareRivalCreditedGates(sample).Valid,
            "finished player must have next gate reset to zero");
    sample = {4, 3, 1, 2, 3, 2, true, true, true};
    Require(!CompareRivalCreditedGates(sample).Valid,
            "finished rival must have next gate reset to zero");

    sample = {4, 3, 1, 2, 0, 1, true, true, true};
    for (bool* flag : {&sample.CircuitRacingEnabled,
                       &sample.PlayerPhysicsReady,
                       &sample.RivalPhysicsReady})
    {
        *flag = false;
        const auto rejected = CompareRivalCreditedGates(sample);
        Require(!rejected.Valid &&
                rejected.Kind == RivalGateGapKind::Unavailable &&
                rejected.CreditedGateDelta == 0,
                "Time Attack, missing player or invalid rival never show a gap");
        *flag = true;
    }
    sample.GateCount = 257;
    Require(!CompareRivalCreditedGates(sample).Valid,
            "oversized gate catalogs cannot overflow a lead readout");
    sample.GateCount = 256;
    sample.RequiredLaps = 100;
    sample.PlayerCompletedLaps = 100;
    sample.PlayerNextGate = 0;
    sample.RivalCompletedLaps = 99;
    sample.RivalNextGate = 255;
    const auto maxRange = CompareRivalCreditedGates(sample);
    Require(maxRange.Valid && maxRange.CreditedGateDelta == 1 &&
            maxRange.PlayerCreditedGates == 25600,
            "largest supported event uses bounded 64-bit progress arithmetic");

    int checksum = 0;
    for (int i = 0; i < 200000; ++i)
    {
        const int player = (i * 17) % 13;
        const int rival = (i * 11) % 13;
        const auto out = compare(player / 4, player % 4, rival / 4, rival % 4);
        Require(out.Valid && out.CreditedGateDelta == player - rival,
                "200k deterministic advisory frames retain exact gate arithmetic");
        checksum += out.CreditedGateDelta;
    }
    Require(checksum != std::numeric_limits<int>::min(),
            "stress comparison remains finite and bounded");

    std::cout << "Rival verified credited-gate gap policy passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
