#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake
{
// Pure C++20, source-only replay timing. Distances must be measurements of
// the SAME immutable course and start/finish origin. This module has no
// access to a live Unreal race, an official best lap or ghost playback.
struct ReplayDistanceTime final
{
    double DistanceMeters = 0.0;
    double SecondsFromStart = 0.0;
};

struct ReplayTimeEstimate final
{
    bool Valid = false;
    double DistanceMeters = 0.0;
    double EstimatedSeconds = 0.0;
    std::size_t LowerSampleIndex = 0;
    std::size_t UpperSampleIndex = 0;
    bool Interpolated = false;
};

class PortableReplayDeltaTrace final
{
public:
    static constexpr std::size_t Capacity = 128;
    static constexpr double MaximumLapMeters = 100000.0;
    static constexpr double MaximumLapSeconds = 604800.0;

    bool Reset(double lapMeters) noexcept
    {
        Count = 0;
        LapMeters = 0.0;
        if (!std::isfinite(lapMeters) ||
            lapMeters < 10.0 || lapMeters > MaximumLapMeters)
        {
            return false;
        }
        LapMeters = lapMeters;
        return true;
    }

    bool Append(ReplayDistanceTime point) noexcept
    {
        if (LapMeters <= 0.0 || Count == Capacity ||
            !std::isfinite(point.DistanceMeters) ||
            !std::isfinite(point.SecondsFromStart) ||
            point.DistanceMeters < 0.0 ||
            point.DistanceMeters > LapMeters ||
            point.SecondsFromStart < 0.0 ||
            point.SecondsFromStart > MaximumLapSeconds)
        {
            return false;
        }
        if (Count == 0)
        {
            if (point.DistanceMeters != 0.0 ||
                point.SecondsFromStart != 0.0)
            {
                return false;
            }
        }
        else
        {
            const auto& prior = Samples[Count - 1];
            if (point.DistanceMeters <= prior.DistanceMeters ||
                point.SecondsFromStart <= prior.SecondsFromStart ||
                prior.DistanceMeters == LapMeters)
            {
                return false;
            }
        }
        Samples[Count++] = point;
        return true;
    }

    bool Complete() const noexcept
    {
        return Count >= 2 && LapMeters > 0.0 &&
            Samples[0].DistanceMeters == 0.0 &&
            Samples[Count - 1].DistanceMeters == LapMeters;
    }

    std::size_t Size() const noexcept { return Count; }
    double CourseMeters() const noexcept { return LapMeters; }

    ReplayTimeEstimate Estimate(double distance) const noexcept
    {
        ReplayTimeEstimate out;
        if (Count < 2 || !std::isfinite(distance) ||
            distance < 0.0 || distance > Samples[Count - 1].DistanceMeters)
        {
            return out;
        }
        // The bounded binary search avoids scanning 128 telemetry records
        // per frame; no heap allocation and no seconds extrapolation.
        std::size_t low = 0;
        std::size_t high = Count - 1;
        while (low < high)
        {
            const std::size_t middle = low + (high - low) / 2;
            if (Samples[middle].DistanceMeters < distance)
            {
                low = middle + 1;
            }
            else
            {
                high = middle;
            }
        }
        const std::size_t upper = low;
        const std::size_t lower = upper == 0 ? 0 : upper - 1;
        out.DistanceMeters = distance;
        out.LowerSampleIndex = lower;
        out.UpperSampleIndex = upper;
        if (Samples[upper].DistanceMeters == distance)
        {
            out.Valid = true;
            out.EstimatedSeconds = Samples[upper].SecondsFromStart;
            return out;
        }
        const auto& a = Samples[lower];
        const auto& b = Samples[upper];
        const double span = b.DistanceMeters - a.DistanceMeters;
        if (span <= 0.0)
        {
            return {};
        }
        const double fraction = (distance - a.DistanceMeters) / span;
        const double estimate = a.SecondsFromStart +
            (b.SecondsFromStart - a.SecondsFromStart) * fraction;
        if (!std::isfinite(estimate) || estimate < a.SecondsFromStart ||
            estimate > b.SecondsFromStart)
        {
            return {};
        }
        out.Valid = true;
        out.Interpolated = true;
        out.EstimatedSeconds = estimate;
        return out;
    }

private:
    std::array<ReplayDistanceTime, Capacity> Samples{};
    std::size_t Count = 0;
    double LapMeters = 0.0;
};

struct PortableReplayDeltaResult final
{
    bool Valid = false;
    double ReferenceSeconds = 0.0;
    double ChallengerSeconds = 0.0;
    // Positive means the challenger reached this sampled distance later.
    double ChallengerBehindSeconds = 0.0;
};

inline PortableReplayDeltaResult CompareReplayAtDistance(
    const PortableReplayDeltaTrace& reference,
    const PortableReplayDeltaTrace& challenger,
    double distanceMeters) noexcept
{
    if (!reference.Complete() || !challenger.Complete() ||
        reference.CourseMeters() != challenger.CourseMeters())
    {
        return {};
    }
    const auto a = reference.Estimate(distanceMeters);
    const auto b = challenger.Estimate(distanceMeters);
    if (!a.Valid || !b.Valid)
    {
        return {};
    }
    const double delta = b.EstimatedSeconds - a.EstimatedSeconds;
    if (!std::isfinite(delta) || std::abs(delta) > 604800.0)
    {
        return {};
    }
    return {true, a.EstimatedSeconds, b.EstimatedSeconds, delta};
}
} // namespace latebrake
