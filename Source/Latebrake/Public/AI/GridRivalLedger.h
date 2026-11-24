#pragma once

#include "Track/CircuitStartGrid.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Read-only per-actor observations for up to five distinct physical rivals.
// Slot 0 belongs to the human and is never a valid rival. These records DO
// NOT grant gates, rank cars, teleport actors, command throttle or write saves.
// The current Unreal race still spawns only the primary rival in slot 1.
struct GridRivalStatus final
{
    bool Bound = false;
    int Slot = -1;
    std::uint64_t ActorToken = 0;
    int LastGateOrdinal = 0;
    double LastGateSeconds = 0.0;
    int RecoveryAttempts = 0;
    int SuccessfulRecoveries = 0;
    int FailedRecoveries = 0;
    bool Finished = false;
};

class GridRivalLedger final
{
public:
    static constexpr int FirstRivalSlot = PrimaryRivalGridSlot;
    static constexpr int LastRivalSlot = StartGridCapacity - 1;
    static constexpr int MaxGateCount = 256;
    static constexpr int MaxRaceLaps = 100;
    static constexpr int MaxRecoveryAttempts = 100000;
    static constexpr double MaxObservedSeconds = 7.0 * 24.0 * 3600.0;

    void Reset() noexcept
    {
        Entries.fill({});
        Configured = false;
        CheckpointCount = 0;
        TotalGates = 0;
    }

    bool Configure(int checkpointCount, int requiredLaps) noexcept
    {
        Reset();
        if (checkpointCount < 1 || checkpointCount > MaxGateCount ||
            requiredLaps < 1 || requiredLaps > MaxRaceLaps)
        {
            return false;
        }
        Configured = true;
        CheckpointCount = checkpointCount;
        TotalGates = checkpointCount * requiredLaps;
        return true;
    }

    bool IsConfigured() const noexcept { return Configured; }

    // The caller supplies the engine actor identity (not a grid index as a
    // fake token). An active actor may never occupy two ledger slots. Rebind
    // after a session reset, or explicitly release a destroyed actor first.
    bool Bind(int slot, std::uint64_t actorToken) noexcept
    {
        if (!Configured || !ValidSlot(slot) || actorToken == 0)
        {
            return false;
        }
        GridRivalStatus& status = Entries[static_cast<std::size_t>(slot)];
        if (status.Bound)
        {
            return status.ActorToken == actorToken;
        }
        for (int other = FirstRivalSlot; other <= LastRivalSlot; ++other)
        {
            if (other != slot &&
                Entries[static_cast<std::size_t>(other)].Bound &&
                Entries[static_cast<std::size_t>(other)].ActorToken == actorToken)
            {
                return false;
            }
        }
        status = {};
        status.Bound = true;
        status.Slot = slot;
        status.ActorToken = actorToken;
        return true;
    }

    bool Release(int slot, std::uint64_t actorToken) noexcept
    {
        if (!Matches(slot, actorToken))
        {
            return false;
        }
        Entries[static_cast<std::size_t>(slot)] = {};
        return true;
    }

    bool Matches(int slot, std::uint64_t actorToken) const noexcept
    {
        return Configured && ValidSlot(slot) && actorToken != 0 &&
            Entries[static_cast<std::size_t>(slot)].Bound &&
            Entries[static_cast<std::size_t>(slot)].ActorToken == actorToken;
    }

    // Called AFTER a real directional-gate pass is independently accepted
    // by that actor's OWN route tracker. Require every intervening gate,
    // not only a larger ordinal, and strictly increasing positive clock.
    // This telemetry never substitutes for the actor's verified gate trace.
    bool RecordAcceptedGate(int slot, std::uint64_t actorToken,
                            int ordinal, double elapsedSeconds) noexcept
    {
        if (!Matches(slot, actorToken) ||
            !std::isfinite(elapsedSeconds) ||
            elapsedSeconds <= 0.0 ||
            elapsedSeconds > MaxObservedSeconds)
        {
            return false;
        }
        GridRivalStatus& status = Entries[static_cast<std::size_t>(slot)];
        if (status.Finished || ordinal != status.LastGateOrdinal + 1 ||
            ordinal < 1 || ordinal > TotalGates ||
            elapsedSeconds <= status.LastGateSeconds)
        {
            return false;
        }
        status.LastGateOrdinal = ordinal;
        status.LastGateSeconds = elapsedSeconds;
        status.Finished = ordinal == TotalGates;
        return true;
    }

    // Recovery counters only record an already attempted/verified actor
    // teleport: this NEVER calls Unreal movement and cannot clear lap data.
    // Each accepted attempt ordinal must be exactly the previous + 1.
    bool RecordRecoveryResult(int slot, std::uint64_t actorToken,
                              int attemptOrdinal,
                              bool enginePlacementVerified) noexcept
    {
        if (!Matches(slot, actorToken))
        {
            return false;
        }
        GridRivalStatus& status = Entries[static_cast<std::size_t>(slot)];
        if (attemptOrdinal < 1 ||
            attemptOrdinal > MaxRecoveryAttempts ||
            attemptOrdinal != status.RecoveryAttempts + 1)
        {
            return false;
        }
        ++status.RecoveryAttempts;
        if (enginePlacementVerified)
        {
            ++status.SuccessfulRecoveries;
        }
        else
        {
            ++status.FailedRecoveries;
        }
        return true;
    }

    // Invalid/unoccupied slots return a zeroed and unbound value; never
    // report a phantom position, finish or result to the caller.
    GridRivalStatus Snapshot(int slot) const noexcept
    {
        return Configured && ValidSlot(slot)
            ? Entries[static_cast<std::size_t>(slot)] : GridRivalStatus{};
    }

    int GateCount() const noexcept { return CheckpointCount; }
    int TotalGateCount() const noexcept { return TotalGates; }

private:
    static constexpr bool ValidSlot(int slot) noexcept
    {
        return slot >= FirstRivalSlot && slot <= LastRivalSlot;
    }

    std::array<GridRivalStatus, StartGridCapacity> Entries{};
    int CheckpointCount = 0;
    int TotalGates = 0;
    bool Configured = false;
};
} // namespace latebrake
