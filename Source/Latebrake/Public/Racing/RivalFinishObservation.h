#pragma once

#include "Racing/RaceEvents.h"

namespace latebrake
{
// An in-memory observation of which verified gate sequence was complete
// at the player's actual final-gate callback. Not official race placement:
// Unreal's same-frame actor order may not reflect the crossing order.
enum class RivalFinishObservationState
{
    Unavailable,
    RivalStillOnRoute,
    RivalRouteAlreadyComplete
};

struct RivalFinishObservationSample final
{
    EventKind Event = EventKind::Circuit;
    bool Career = false;
    bool PlayerFinished = false;
    bool EligibleTiming = false;
    int PlayerGates = 0;
    int PlayerLaps = 0;
    bool RivalPresent = false;
    bool RivalPhysicsReady = false;
    int RivalGates = 0;
    int RivalLaps = 0;
    int RivalCompletedLaps = 0;
    int RivalNextGate = -1;
};

struct RivalFinishObservation final
{
    RivalFinishObservationState State = RivalFinishObservationState::Unavailable;
};

constexpr RivalFinishObservation ObserveRivalFinish(
    RivalFinishObservationSample sample) noexcept
{
    constexpr RaceEvent quick = GetRaceEvent(EventKind::Circuit);
    // No career classification, solo Time Attack, absent rival, incorrect
    // configured lap counts or incomplete/unqualified player finish.
    if (sample.Event != EventKind::Circuit || sample.Career ||
        !sample.PlayerFinished || !sample.EligibleTiming ||
        !sample.RivalPresent || !sample.RivalPhysicsReady ||
        sample.PlayerGates != quick.CheckpointCount ||
        sample.PlayerLaps != quick.RequiredLaps ||
        sample.RivalGates != sample.PlayerGates ||
        sample.RivalLaps != sample.PlayerLaps ||
        sample.RivalCompletedLaps < 0 ||
        sample.RivalCompletedLaps > sample.RivalLaps ||
        sample.RivalNextGate < 0 || sample.RivalNextGate >= sample.RivalGates ||
        (sample.RivalCompletedLaps == sample.RivalLaps &&
         sample.RivalNextGate != 0))
        return {};
    return {sample.RivalCompletedLaps == sample.RivalLaps
        ? RivalFinishObservationState::RivalRouteAlreadyComplete
        : RivalFinishObservationState::RivalStillOnRoute};
}
} // namespace latebrake
