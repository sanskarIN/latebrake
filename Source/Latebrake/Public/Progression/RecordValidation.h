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
