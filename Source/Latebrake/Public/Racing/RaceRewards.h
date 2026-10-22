#pragma once

#include "Racing/RaceEvents.h"
#include "Progression/RecordValidation.h"

#include <cmath>

namespace latebrake
{
// Offline event rewards are deterministic, capped and never purchased.
// The caller awards once on RaceProgress's transition to Finished.
inline int CalculateRaceCredits(EventKind event, double finishedSeconds,
                                int recoveryCount) noexcept
{
    if (!IsValidEvent(event) || !IsValidFinishedRaceTime(finishedSeconds) ||
        recoveryCount < 0)
    {
        return 0;
    }

    const bool circuit = event == EventKind::Circuit;
    const int participation = circuit ? 450 : 180;
    const double goldTime = circuit ? 180.0 : 60.0;
    const double silverTime = circuit ? 240.0 : 90.0;
    const int speedBonus = (finishedSeconds <= goldTime) ? 150 :
                           (finishedSeconds <= silverTime) ? 75 : 0;
    const int cleanBonus = recoveryCount == 0 ? 50 : 0;
    return participation + speedBonus + cleanBonus;
}
} // namespace latebrake
