#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Engine-independent guards for old or malformed local SaveGame fields.
// Store zero when a historic best time cannot be interpreted reliably.
inline float SanitizeBestTime(double seconds) noexcept
{
    constexpr double MaxRecordedSeconds = 7.0 * 24.0 * 60.0 * 60.0;
    if (!std::isfinite(seconds) || seconds < 0.0 ||
        seconds > MaxRecordedSeconds)
    {
        return 0.f;
    }
    return static_cast<float>(seconds);
}

// Records, paid finishes and career medals share a single upper bound.
// Without the same limit at *award* time, a clean-race challenge may
// grant a first-completion medal/credits for an impossible clock that
// could never survive SaveGame time-record sanitization.
inline bool IsValidFinishedRaceTime(double seconds) noexcept
{
    return std::isfinite(seconds) && seconds > 0.0 &&
           SanitizeBestTime(seconds) > 0.f;
}

// Only improve a best record with a time that is actually representable
// by the existing SaveGame float schema. Rejected candidates must not
// clobber a previously valid personal best or become Infinity on disk.
inline float ImprovedBestTime(float previous, double candidate) noexcept
{
    const float prior = SanitizeBestTime(previous);
    const float next = SanitizeBestTime(candidate);
    if (next <= 0.f)
    {
        return prior;
    }
    return prior <= 0.f || next < prior ? next : prior;
}

inline int SanitizeCompletedRaces(int count) noexcept
{
    return std::max(0, count);
}

inline int IncrementCompletedRaces(int current) noexcept
{
    const int safe = SanitizeCompletedRaces(current);
    return safe == std::numeric_limits<int>::max() ? safe : safe + 1;
}
} // namespace latebrake
