#pragma once

#include <cmath>

namespace latebrake
{
// A session-only estimate of the time from authoritative GO to the first
// observed forward speed of at least 5 km/h. It is NOT driver reaction time,
// a physics certification, a lap time, a reward or saved progression.
//
// Sampling happens after the real clock's successful post-GO advance. The
// caller must subtract separately charged recovery penalties and supply
// genuine, finite chassis-forward speed, not speedometer magnitude.
// We require a measured below-threshold sample before the crossing to
// avoid inventing an instant launch from a spawned/moving replacement car.
class LaunchSpeedMeter final
{
public:
    static constexpr double ThresholdKmh = 5.0;
    static constexpr double CaptureWindowSeconds = 30.0;
    // Refuse to interpolate across physics stalls, pause or dropped samples.
    static constexpr double MaxInterpolationGapSeconds = 0.25;

    void Reset() noexcept
    {
        Captured = false;
        Invalidated = false;
        SawBelowThreshold = false;
        HaveLastSample = false;
        LastSampleSeconds = 0.0;
        LastForwardKmh = 0.0;
        ContinuousSamples = false;
        TimeToFiveKmhSeconds = 0.0;
    }

    void Invalidate() noexcept { Invalidated = true; }
    bool HasSample() const noexcept { return Captured; }
    bool IsInvalidated() const noexcept { return Invalidated; }
    double SecondsToFiveKmh() const noexcept
    {
        return Captured ? TimeToFiveKmhSeconds : 0.0;
    }

    // Malformed/ineligible samples break interpolation continuity, without
    // themselves fabricating a launch or erasing a valid earlier result.
    // This is an approximate sampled crossing, NOT a reaction-time measure.
    bool Observe(double activeSeconds, double forwardKmh,
                 bool qualifiedAfterGreen) noexcept
    {
        if (Captured || Invalidated)
        {
            return false;
        }
        if (!qualifiedAfterGreen || !std::isfinite(activeSeconds) ||
            activeSeconds < 0.0 || !std::isfinite(forwardKmh))
        {
            ContinuousSamples = false;
            return false;
        }
        if ((HaveLastSample && activeSeconds < LastSampleSeconds) ||
            activeSeconds > CaptureWindowSeconds)
        {
            Invalidate();
            return false;
        }
        // A late first physics observation already in motion cannot prove
        // acceleration from the original grid. It cannot be revived by
        // slowing down and trying again during this same race.
        if (!HaveLastSample && forwardKmh >= ThresholdKmh)
        {
            Invalidate();
            return false;
        }
        if (forwardKmh >= ThresholdKmh)
        {
            // Distinct speeds at the same clock instant do not establish a
            // real physical crossing; reject instead of recording 0 seconds.
            if (!SawBelowThreshold || activeSeconds <= LastSampleSeconds)
            {
                Invalidate();
                return false;
            }
            double estimateSeconds = activeSeconds; // conservative fallback
            const double gap = activeSeconds - LastSampleSeconds;
            const double speedChange = forwardKmh - LastForwardKmh;
            if (ContinuousSamples && gap <= MaxInterpolationGapSeconds &&
                LastForwardKmh < ThresholdKmh &&
                std::isfinite(speedChange) && speedChange > 0.0)
            {
                const double fraction =
                    (ThresholdKmh - LastForwardKmh) / speedChange;
                if (std::isfinite(fraction) &&
                    fraction >= 0.0 && fraction <= 1.0)
                {
                    // Bounded inside two observed valid post-GO samples.
                    // It is never extrapolated before the first observation.
                    estimateSeconds = LastSampleSeconds + gap * fraction;
                }
            }
            TimeToFiveKmhSeconds = estimateSeconds;
            Captured = true;
            return true;
        }
        SawBelowThreshold = true;
        HaveLastSample = true;
        LastSampleSeconds = activeSeconds;
        LastForwardKmh = forwardKmh;
        ContinuousSamples = true;
        return false;
    }

private:
    bool Captured = false;
    bool Invalidated = false;
    bool SawBelowThreshold = false;
    bool HaveLastSample = false;
    double LastSampleSeconds = 0.0;
    double LastForwardKmh = 0.0;
    bool ContinuousSamples = false;
    double TimeToFiveKmhSeconds = 0.0;
};
} // namespace latebrake
