#pragma once

#include "Racing/RaceEvents.h"
#include "Racing/RaceProgress.h"

namespace latebrake
{
// Rival route progress is telemetry, not an official race finish. The AI
// must nevertheless observe the same ready/running session conditions as
// the player: do not drive on an invalid track, without a player, while a
// menu/pause owns focus, after a finish, or in solo Time Attack events.
// This pure rule also protects manually ticked Unreal worlds.
inline bool CanDriveRival(const RaceProgress& race,
                          EventKind activeEvent,
                          bool circuitReady,
                          bool worldPausedOrMenuOpen,
                          bool localPlayerReady,
                          bool rivalRouteFinished) noexcept
{
    return activeEvent == EventKind::Circuit &&
           circuitReady &&
           !worldPausedOrMenuOpen &&
           localPlayerReady &&
           !rivalRouteFinished &&
           !race.IsFinished();
}
} // namespace latebrake
