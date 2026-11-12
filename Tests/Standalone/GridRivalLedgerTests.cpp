#include "AI/GridRivalLedger.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* description)
{
    if (!pass)
    {
        std::cerr << "GridRivalLedger FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    GridRivalLedger ledger;
    Require(!ledger.IsConfigured() && !ledger.Bind(1, 101),
            "unconfigured ledger rejects any phantom rival");
    for (const int invalidCount : {-1, 0, 257, 1000000})
    {
        Require(!ledger.Configure(invalidCount, 3),
                "invalid checkpoint count rejects event");
    }
    for (const int invalidLaps : {-1, 0, 101, 1000000})
    {
        Require(!ledger.Configure(4, invalidLaps),
                "invalid lap target rejects event");
    }
    Require(ledger.Configure(4, 3) && ledger.GateCount() == 4 &&
            ledger.TotalGateCount() == 12, "valid race geometry configured");
    Require(!ledger.Bind(HumanGridSlot, 101) &&
            !ledger.Bind(StartGridCapacity, 101) &&
            !ledger.Bind(-1, 101) &&
            !ledger.Bind(1, 0), "only nonzero identities in five rival slots");
    Require(ledger.Bind(1, 101) && ledger.Bind(2, 202) &&
            ledger.Bind(3, 303) && ledger.Bind(4, 404) &&
            ledger.Bind(5, 505), "all five potential rival identities separate");
    Require(ledger.Bind(1, 101), "same actor bind is idempotent");
    Require(!ledger.Bind(1, 999) && !ledger.Bind(2, 101),
            "occupied slot and cross-slot actor reuse both rejected");
    for (int slot = 1; slot <= 5; ++slot)
    {
        const GridRivalStatus status = ledger.Snapshot(slot);
        Require(status.Bound && status.Slot == slot &&
                status.ActorToken == static_cast<std::uint64_t>(slot * 101) &&
                status.LastGateOrdinal == 0 && !status.Finished,
                "every slot starts empty of progress");
    }
    Require(!ledger.RecordAcceptedGate(1, 999, 1, 0.5) &&
            !ledger.RecordAcceptedGate(0, 101, 1, 0.5) &&
            !ledger.RecordAcceptedGate(1, 101, 2, 0.5) &&
            !ledger.RecordAcceptedGate(1, 101, 1, 0.0) &&
            !ledger.RecordAcceptedGate(1, 101, 1,
                std::numeric_limits<double>::quiet_NaN()) &&
            !ledger.RecordAcceptedGate(1, 101, 1,
                std::numeric_limits<double>::infinity()),
            "unbound, skipped and nonfinite/zero gates fail closed");
    for (int gate = 1; gate <= 12; ++gate)
    {
        Require(ledger.RecordAcceptedGate(1, 101, gate, 0.5 * gate),
                "primary gate ordinals can advance only sequentially");
        Require(!ledger.RecordAcceptedGate(1, 101, gate, 0.5 * gate),
                "duplicate gate from same actor not counted twice");
        if (gate < 12)
        {
            Require(!ledger.RecordAcceptedGate(1, 101, gate + 1, 0.5 * gate),
                    "gate clock must increase across accepted gates");
        }
    }
    Require(ledger.Snapshot(1).Finished &&
            ledger.Snapshot(1).LastGateOrdinal == 12 &&
            !ledger.RecordAcceptedGate(1, 101, 13, 6.5),
            "finish freezes the race observer");
    Require(ledger.Snapshot(2).LastGateOrdinal == 0 &&
            ledger.RecordAcceptedGate(2, 202, 1, 20.0) &&
            ledger.RecordAcceptedGate(3, 303, 1, 5.0) &&
            ledger.Snapshot(3).LastGateSeconds == 5.0 &&
            ledger.Snapshot(2).LastGateSeconds == 20.0,
            "multiple actor gates advance independent histories");

    Require(!ledger.RecordRecoveryResult(2, 999, 1, true) &&
            !ledger.RecordRecoveryResult(2, 202, 2, true) &&
            ledger.RecordRecoveryResult(2, 202, 1, false) &&
            !ledger.RecordRecoveryResult(2, 202, 1, true) &&
            ledger.RecordRecoveryResult(2, 202, 2, true),
            "only actor-owned exact-ordinal recovery receipts count");
    const GridRivalStatus second = ledger.Snapshot(2);
    Require(second.RecoveryAttempts == 2 && second.SuccessfulRecoveries == 1 &&
            second.FailedRecoveries == 1 &&
            second.LastGateOrdinal == 1 && !second.Finished,
            "failed recovery does not erase independent gate progress");
    Require(!ledger.Release(2, 404) && ledger.Release(2, 202) &&
            !ledger.Matches(2, 202) &&
            !ledger.RecordAcceptedGate(2, 202, 2, 30.0) &&
            !ledger.RecordRecoveryResult(2, 202, 3, true),
            "stale destroyed actor cannot write a released slot");
    Require(ledger.Bind(2, 999) &&
            ledger.RecordAcceptedGate(2, 999, 1, 1.5) &&
            ledger.Snapshot(2).RecoveryAttempts == 0,
            "reused grid slot begins with clean state for a new physical actor");

    // Test all valid sizes and complete their own routes with no shared
    // race ordinal, recovery counter or actor identity.
    for (int gates = 1; gates <= 16; ++gates)
    {
        for (int laps = 1; laps <= 10; ++laps)
        {
            Require(ledger.Configure(gates, laps), "event reset accepted");
            for (int slot = 1; slot <= 5; ++slot)
            {
                const std::uint64_t actor = static_cast<std::uint64_t>(slot + 500);
                Require(ledger.Bind(slot, actor), "independent actor bound");
                for (int ordinal = 1; ordinal <= gates * laps; ++ordinal)
                {
                    Require(ledger.RecordAcceptedGate(
                        slot, actor, ordinal, ordinal * (slot + 0.1)),
                        "all finite sequential gate events accepted");
                }
                Require(ledger.Snapshot(slot).Finished, "all planned laps complete");
            }
        }
    }
    Require(ledger.Configure(4, 2), "restarting race clears prior finish");
    Require(!ledger.Snapshot(1).Bound && !ledger.Snapshot(2).Bound &&
            ledger.Snapshot(5).LastGateOrdinal == 0,
            "restart invalidates all old actor identities");
    Require(!ledger.RecordAcceptedGate(1, 101, 1, 9.0),
            "unbound prior-session actor cannot forge a new gate");
    Require(!ledger.Snapshot(0).Bound &&
            !ledger.Snapshot(6).Bound &&
            !ledger.Snapshot(-10).Bound,
            "no phantom read-only observations");
    ledger.Reset();
    Require(!ledger.IsConfigured() && !ledger.Bind(5, 505),
            "explicit reset fails closed");

    std::cout << "Five-slot independent rival telemetry contracts passed.\n";
    return EXIT_SUCCESS;
}
