#pragma once

#include "Career/CareerProgress.h"
#include "Garage/GarageState.h"

namespace latebrake
{
// A single Unreal SaveGame slot contains all of these fields. If the engine
// reports that its write failed, the local session must not continue with
// rewards or records which it just failed to persist.
//
// This checkpoint does NOT guarantee disk-level atomicity. SaveGameToSlot can
// fail for OS/storage reasons, and an existing slot must be treated as
// untrusted until a later verified load. It only rolls back in-memory state.
class ProfileWriteCheckpoint final
{
public:
    ProfileWriteCheckpoint(GarageState& garage, CareerProgress& career,
                           int& completedRaces, float& bestRaceSeconds,
                           float& bestLapSeconds, float& bestTimeAttackSeconds) noexcept
        : Garage(garage), Career(career),
          CompletedRaces(completedRaces), BestRaceSeconds(bestRaceSeconds),
          BestLapSeconds(bestLapSeconds), BestTimeAttackSeconds(bestTimeAttackSeconds),
          PreviousGarage(garage.Snapshot()), PreviousCareer(career.Snapshot()),
          PreviousCompletedRaces(completedRaces), PreviousBestRaceSeconds(bestRaceSeconds),
          PreviousBestLapSeconds(bestLapSeconds),
          PreviousBestTimeAttackSeconds(bestTimeAttackSeconds)
    {
    }

    ProfileWriteCheckpoint(const ProfileWriteCheckpoint&) = delete;
    ProfileWriteCheckpoint& operator=(const ProfileWriteCheckpoint&) = delete;
    ProfileWriteCheckpoint(ProfileWriteCheckpoint&&) = delete;
    ProfileWriteCheckpoint& operator=(ProfileWriteCheckpoint&&) = delete;

    // Only call after Unreal SaveGameToSlot explicitly returned success.
    void Commit() noexcept { Pending = false; }

    // Idempotent; safe to call explicitly before the scope exits, so game
    // code can recompute pawn tuning and display a failed-save warning.
    void Rollback() noexcept
    {
        if (!Pending) return;
        Garage.Load(PreviousGarage);
        Career.Load(PreviousCareer);
        CompletedRaces = PreviousCompletedRaces;
        BestRaceSeconds = PreviousBestRaceSeconds;
        BestLapSeconds = PreviousBestLapSeconds;
        BestTimeAttackSeconds = PreviousBestTimeAttackSeconds;
        Pending = false;
    }

    ~ProfileWriteCheckpoint() { Rollback(); }

private:
    GarageState& Garage;
    CareerProgress& Career;
    int& CompletedRaces;
    float& BestRaceSeconds;
    float& BestLapSeconds;
    float& BestTimeAttackSeconds;
    const GarageSnapshot PreviousGarage;
    const CareerSnapshot PreviousCareer;
    const int PreviousCompletedRaces;
    const float PreviousBestRaceSeconds;
    const float PreviousBestLapSeconds;
    const float PreviousBestTimeAttackSeconds;
    bool Pending = true;
};
} // namespace latebrake
