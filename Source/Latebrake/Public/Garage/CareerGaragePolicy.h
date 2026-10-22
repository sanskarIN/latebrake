#pragma once

#include "Racing/RaceProgress.h"

namespace latebrake
{
// Career medals and event objectives are evaluated against the vehicle that
// started the event. Letting garage hotkeys swap cars or upgrade handling in
// the middle of a timed career race invalidates that comparison.
// Quick races and already finished career races may still use the garage;
// restarting a finished career race re-engages the lock.
inline bool CanEditGarageDuringRace(bool isCareerRace,
                                     const RaceProgress& race) noexcept
{
    return !isCareerRace || race.IsFinished();
}
} // namespace latebrake
