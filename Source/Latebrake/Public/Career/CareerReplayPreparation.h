#pragma once

#include "Career/CareerLaunchSelection.h"

namespace latebrake
{
// The garage is editable after a career race finishes. On a replay we must
// take a fresh starting-vehicle snapshot and re-equip required vehicles,
// without silently restarting a completed race before storage accepts any
// necessary selection change.
//
// Wrap this staging call in ProfileWriteCheckpoint. A NeedSave result must
// be saved successfully before restarting the timer/pawn/rival; if the save
// fails, roll back and leave the finished race untouched.
struct CareerReplayPreparation final
{
    CareerLaunchStatus Status = CareerLaunchStatus::LockedOrUnknownEvent;
    bool NeedsSave = false;
    int StartingVehicle = -1;
};

inline CareerReplayPreparation StageCareerReplay(CareerProgress& career,
                                                  GarageState& garage) noexcept
{
    const int eventId = career.SelectedEvent();
    const int previousVehicle = garage.SelectedVehicle();
    const CareerLaunchStatus status = StageCareerLaunch(career, garage, eventId);
    if (status != CareerLaunchStatus::Ready)
    {
        return {status, false, -1};
    }
    return {status, garage.SelectedVehicle() != previousVehicle,
            garage.SelectedVehicle()};
}
} // namespace latebrake
