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

    void Reset() noexcept
    {
        Captured = false;
        Invalidated = false;
        SawBelowThreshold = false;
        HaveLastSample = false;
        LastSampleSeconds = 0.0;
        TimeToFiveKmhSeconds = 0.0;
    }

    void Invalidate() noexcept { Invalidated = true; }
    bool HasSample() const noexcept { return Captured; }
    bool IsInvalidated() const noexcept { return Invalidated; }
    double SecondsToFiveKmh() const noexcept
    {
        return Captured ? TimeToFiveKmhSeconds : 0.0;
    }

    // No state change for an ineligible or malformed sample. Once captured,
    // the first estimate is immutable until a legitimate race restart.
    bool Observe(double activeSeconds, double forwardKmh,
                 bool qualifiedAfterGreen) noexcept
    {
        if (Captured || Invalidated || !qualifiedAfterGreen ||
            !std::isfinite(activeSeconds) || activeSeconds < 0.0 ||
            !std::isfinite(forwardKmh))
        {
            return false;
        }
        if ((HaveLastSample && activeSeconds < LastSampleSeconds) ||
            activeSeconds > CaptureWindowSeconds)
        {
            Invalidate();
            return false;
        }

        HaveLastSample = true;
        LastSampleSeconds = activeSeconds;
        if (forwardKmh < ThresholdKmh)
        {
            SawBelowThreshold = true;
            return false;
        }
        if (!SawBelowThreshold)
        {
            // The first sample was already in motion: do not assume
            // the car crossed 5 km/h after the actual green flag.
            return false;
        }
        TimeToFiveKmhSeconds = activeSeconds;
        Captured = true;
        return true;
    }

private:
    bool Captured = false;
    bool Invalidated = false;
    bool SawBelowThreshold = false;
    bool HaveLastSample = false;
    double LastSampleSeconds = 0.0;
    double TimeToFiveKmhSeconds = 0.0;
};
} // namespace latebrake
