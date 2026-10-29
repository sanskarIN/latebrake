#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Optional session-only observation from the rigid-body chassis.
// The signed angle describes travel direction relative to the car's nose,
// NOT wheel slip, tire temperature, contact friction or driving safety.
struct ChassisSlipSample final
{
    double DeltaSeconds = 0.0;
    double ForwardCmPerSecond = 0.0;
    double LateralCmPerSecond = 0.0;
};

class ChassisSlipAngleMeter final
{
public:
    static constexpr double MinSampleSeconds = 0.001;
    static constexpr double MaxSampleSeconds = 0.25;
    static constexpr double MinForwardCmPerSecond = 300.0;
    static constexpr double HighAngleMinForwardCmPerSecond = 900.0;
    static constexpr double HighAngleThresholdDegrees = 12.0;
    static constexpr double MaxPlanarCmPerSecond = 10000.0;
    static constexpr double SmoothingSeconds = 0.12;
    static constexpr double MaxTrackedSeconds = 43200.0;

    void Reset() noexcept
    {
        Suspend();
        PeakAbsoluteDegrees = 0.0;
        HighAngleSeconds = 0.0;
        AcceptedSamples = 0;
    }

    // A loss of driving authority or a teleport must clear live feedback.
    // Historical *run-local* peaks are retained until a genuine restart.
    void Suspend() noexcept
    {
        HaveLiveSample = false;
        LiveDegrees = 0.0;
    }

    bool Observe(const ChassisSlipSample& sample) noexcept
    {
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds < MinSampleSeconds ||
            sample.DeltaSeconds > MaxSampleSeconds ||
            !std::isfinite(sample.ForwardCmPerSecond) ||
            !std::isfinite(sample.LateralCmPerSecond) ||
            sample.ForwardCmPerSecond < MinForwardCmPerSecond ||
            std::hypot(sample.ForwardCmPerSecond,
                       sample.LateralCmPerSecond) > MaxPlanarCmPerSecond)
        {
            Suspend();
            return false;
        }

        constexpr double RadiansToDegrees = 57.2957795130823208768;
        const double signedDegrees =
            std::atan2(sample.LateralCmPerSecond,
                       sample.ForwardCmPerSecond) * RadiansToDegrees;
        if (!std::isfinite(signedDegrees))
        {
            Suspend();
            return false;
        }

        if (!HaveLiveSample)
        {
            LiveDegrees = signedDegrees;
        }
        else
        {
            const double weight = sample.DeltaSeconds /
                (SmoothingSeconds + sample.DeltaSeconds);
            LiveDegrees += weight * (signedDegrees - LiveDegrees);
        }
        HaveLiveSample = true;
        PeakAbsoluteDegrees =
            std::max(PeakAbsoluteDegrees, std::abs(signedDegrees));

        // Accumulate time only while demonstrably driving forwards. There
        // is no checkpoint, reward or save-game authority in this meter.
        if (sample.ForwardCmPerSecond >= HighAngleMinForwardCmPerSecond &&
            std::abs(signedDegrees) >= HighAngleThresholdDegrees)
        {
            HighAngleSeconds = std::min(
                MaxTrackedSeconds, HighAngleSeconds + sample.DeltaSeconds);
        }
        if (AcceptedSamples < std::numeric_limits<int>::max())
        {
            ++AcceptedSamples;
        }
        return true;
    }

    bool HasLiveReading() const noexcept { return HaveLiveSample; }
    double GetLiveDegrees() const noexcept { return LiveDegrees; }
    double GetPeakAbsoluteDegrees() const noexcept
    {
        return PeakAbsoluteDegrees;
    }
    double GetHighAngleSeconds() const noexcept
    {
        return HighAngleSeconds;
    }
    int GetAcceptedSamples() const noexcept { return AcceptedSamples; }

private:
    double LiveDegrees = 0.0;
    double PeakAbsoluteDegrees = 0.0;
    double HighAngleSeconds = 0.0;
    int AcceptedSamples = 0;
    bool HaveLiveSample = false;
};
} // namespace latebrake
