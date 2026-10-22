#pragma once

#include "Career/CareerProgress.h"
#include "Garage/GarageState.h"

namespace latebrake
{
// Pure career launch policy. It only stages profile selection; the caller
// must persist the modified profile BEFORE changing the active race.
// Wrap this call with ProfileWriteCheckpoint so failed writes restore both
// garage and career state instead of leaving an unsaved active event.
enum class CareerLaunchStatus
{
    Ready,
    LockedOrUnknownEvent,
    RequiredVehicleNotOwned
};

inline CareerLaunchStatus StageCareerLaunch(CareerProgress& career,
                                            GarageState& garage,
                                            int eventId) noexcept
{
    const CareerEvent* event = GetCareerEvent(eventId);
    if (!event || career.Status(eventId) == CareerStatus::Locked)
    {
        return CareerLaunchStatus::LockedOrUnknownEvent;
    }
    if (event->RequiredVehicle >= 0 && !garage.Owns(event->RequiredVehicle))
    {
        return CareerLaunchStatus::RequiredVehicleNotOwned;
    }

    // No selection is changed by rejected requests. The ownership check
    // above ensures the required garage selection is always valid.
    if (!career.Select(eventId))
    {
        return CareerLaunchStatus::LockedOrUnknownEvent;
    }
    if (event->RequiredVehicle >= 0)
    {
        garage.Select(event->RequiredVehicle);
    }
    return CareerLaunchStatus::Ready;
}
} // namespace latebrake
