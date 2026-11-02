#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceEvents.h"
#include <cmath>

namespace latebrake
{
// Read-only post-finish UI. Rewards, records and SaveGame do not consume this.
enum class QuickRaceRecordState
{
    Unavailable, SaveFailed, FirstRecord, Improved, Matched, Slower
};

struct QuickRaceRecordReview final
{
    QuickRaceRecordState State = QuickRaceRecordState::Unavailable;
    double SavedBestSeconds = 0.0;
    double DeltaSeconds = 0.0;
};

inline QuickRaceRecordReview ReviewQuickRaceRecord(
    EventKind event, bool career, bool eligibleFinish, bool saveFailed,
    double elapsedSeconds, double preRunBest, double persistedBest) noexcept
{
    QuickRaceRecordReview result;
    if (!IsValidEvent(event) || career || !eligibleFinish ||
        !IsValidFinishedRaceTime(elapsedSeconds))
        return result;
    if (saveFailed)
    {
        result.State = QuickRaceRecordState::SaveFailed;
        return result;
    }
    const float before = SanitizeBestTime(preRunBest);
    const float after = SanitizeBestTime(persistedBest);
    if (after <= 0.f || (before > 0.f && after > before))
        return result; // A missing or worsened saved record is untrustworthy.
    result.SavedBestSeconds = static_cast<double>(after);
    if (before <= 0.f || after < before)
    {
        // The official race clock contains recovery penalties. The new
        // stored float should be the same attempt within float precision.
        if (std::abs(static_cast<double>(after) - elapsedSeconds) > 0.0625)
            return {};
        if (before <= 0.f)
            result.State = QuickRaceRecordState::FirstRecord;
        else
        {
            result.State = QuickRaceRecordState::Improved;
            result.DeltaSeconds =
                static_cast<double>(before) - static_cast<double>(after);
        }
    }
    else if (elapsedSeconds <= static_cast<double>(before) + 0.01)
        result.State = QuickRaceRecordState::Matched;
    else
    {
        result.State = QuickRaceRecordState::Slower;
        result.DeltaSeconds = elapsedSeconds - static_cast<double>(before);
    }
    return result;
}
} // namespace latebrake
