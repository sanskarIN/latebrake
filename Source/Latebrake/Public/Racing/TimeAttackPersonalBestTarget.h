#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceEvents.h"
#include <algorithm>
#include <cmath>

namespace latebrake
{
// Read-only remaining race-clock budget against the SAVED solo Time Attack PB.
// Includes actual official clock penalties. Not a prediction, new record or award.
struct TimeAttackPersonalBestTarget final
{
    bool Valid = false;
    bool DeadlineReached = false;
    double SavedBestSeconds = 0.0;
    double SecondsRemaining = 0.0;
    double SecondsOver = 0.0;
};

inline TimeAttackPersonalBestTarget MeasureTimeAttackPersonalBestTarget(
    EventKind kind, bool careerRace, bool activeRace, bool finishedRace,
    double elapsedSeconds, double savedBestSeconds) noexcept
{
    TimeAttackPersonalBestTarget result;
    // A finished run may have just overwritten the record: never compare
    // Results against that new value. Career time trials are separate.
    if (kind != EventKind::TimeAttack || careerRace ||
        !activeRace || finishedRace || !std::isfinite(elapsedSeconds) ||
        elapsedSeconds < 0.0 || elapsedSeconds > 604800.0)
    {
        return result;
    }
    const float prior = SanitizeBestTime(savedBestSeconds);
    if (prior <= 0.f)
    {
        return result;
    }
    const double target = static_cast<double>(prior);
    result.Valid = true;
    result.SavedBestSeconds = target;
    result.DeadlineReached = elapsedSeconds >= target;
    result.SecondsRemaining = std::max(0.0, target - elapsedSeconds);
    result.SecondsOver = std::max(0.0, elapsedSeconds - target);
    return result;
}
} // namespace latebrake
