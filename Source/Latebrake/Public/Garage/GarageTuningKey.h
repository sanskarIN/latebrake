#pragma once

#include "Garage/GarageState.h"

namespace latebrake
{
// Credits and ownership never affect live physics; only the selected
// profile and tuning levels do. This prevents needless per-frame tuning.
struct GarageTuningKey final
{
    int Selected = -1;
    int Engine = -1;
    int Grip = -1;

    bool operator==(const GarageTuningKey& other) const noexcept
    {
        return Selected == other.Selected && Engine == other.Engine &&
               Grip == other.Grip;
    }
};

inline GarageTuningKey MakeGarageTuningKey(const GarageSnapshot& snapshot) noexcept
{
    return {snapshot.SelectedVehicle, snapshot.EngineLevel, snapshot.GripLevel};
}
} // namespace latebrake
