#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Run-local advisory estimate from actual chassis planar speed, rather than
// input demand or a presumed friction/road coefficient. In-game only: not
// suitable for real vehicle braking decisions, awards or saved records.
struct BrakeDistanceSample final
{
    double DeltaSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double ForwardSpeedCmPerSecond = 0.0;
    bool Braking = false;
};

class ObservedBrakeDistance final
{
public:
    static constexpr double MinFrameSeconds = 0.001;
    static constexpr double MaxFrameSeconds = 0.25;
    static constexpr double MinPlanarSpeedCmPerSecond = 900.0;
    static constexpr double MaxPlanarSpeedCmPerSecond = 10000.0;
    static constexpr double MinimumForwardFraction = 0.88;
    static constexpr double MinDecelerationCmPerSecondSquared = 75.0;
    static constexpr double MaxDecelerationCmPerSecondSquared = 2000.0;
    static constexpr double SmoothingSeconds = 0.22;
    static constexpr double MinimumConfirmedSeconds = 0.12;
    static constexpr double MaxAdvisoryDistanceMeters = 2000.0;

    void Reset() noexcept { Suspend(); }

    // No delayed, stale estimate after release, drift, collision, invalid
    // physics, recover, restart, pause or OS focus interruption.
    void Suspend() noexcept
    {
        HasPrevious = false;
        PreviousPlanarSpeed = 0.0;
        SmoothedDeceleration = 0.0;
        ConfirmedSeconds = 0.0;
        ConsecutivePairs = 0;
        LiveDistanceMeters = 0.0;
        LiveEstimate = false;
    }

    bool Observe(const BrakeDistanceSample sample) noexcept
    {
        const double speed = sample.PlanarSpeedCmPerSecond;
        const double forward = sample.ForwardSpeedCmPerSecond;
        if (!sample.Braking || !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds < MinFrameSeconds ||
            sample.DeltaSeconds > MaxFrameSeconds ||
            !std::isfinite(speed) ||
            speed < MinPlanarSpeedCmPerSecond ||
            speed > MaxPlanarSpeedCmPerSecond ||
            !std::isfinite(forward) || forward < 0.0 ||
            forward > speed + 5.0 ||
            forward < speed * MinimumForwardFraction)
        {
            Suspend();
            return false;
        }

        if (!HasPrevious)
        {
            PreviousPlanarSpeed = speed;
            HasPrevious = true;
            return false; // one instantaneous velocity is not deceleration
        }

        const double deceleration =
            (PreviousPlanarSpeed - speed) / sample.DeltaSeconds;
        PreviousPlanarSpeed = speed;
        if (!std::isfinite(deceleration) ||
            deceleration < MinDecelerationCmPerSecondSquared ||
            deceleration > MaxDecelerationCmPerSecondSquared)
        {
            Suspend();
            return false; // spike, acceleration or stalled deceleration
        }

        if (ConsecutivePairs == 0)
        {
            SmoothedDeceleration = deceleration;
        }
        else
        {
            const double weight = sample.DeltaSeconds /
                (SmoothingSeconds + sample.DeltaSeconds);
            SmoothedDeceleration +=
                weight * (deceleration - SmoothedDeceleration);
        }
        if (ConsecutivePairs < std::numeric_limits<int>::max())
        {
            ++ConsecutivePairs;
        }
        ConfirmedSeconds = std::min(300.0,
            ConfirmedSeconds + sample.DeltaSeconds);

        // v^2 / 2a is only a *current-rate* braking projection. It does not
        // know upcoming corner geometry, impacts, grip changes or latency.
        const double meters = 0.01 * speed * speed /
            (2.0 * SmoothedDeceleration);
        if (!std::isfinite(meters) || meters < 0.0 ||
            meters > MaxAdvisoryDistanceMeters)
        {
            Suspend();
            return false;
        }
        LiveEstimate = ConsecutivePairs >= 3 &&
            ConfirmedSeconds >= MinimumConfirmedSeconds;
        LiveDistanceMeters = LiveEstimate ? meters : 0.0;
        return true;
    }

    bool HasLiveEstimate() const noexcept { return LiveEstimate; }
    double GetDistanceMeters() const noexcept
    {
        return LiveEstimate ? LiveDistanceMeters : 0.0;
    }
    double GetObservedDecelerationMetersPerSecondSquared() const noexcept
    {
        return LiveEstimate ? SmoothedDeceleration * 0.01 : 0.0;
    }
    int GetConsecutivePairs() const noexcept { return ConsecutivePairs; }

private:
    double PreviousPlanarSpeed = 0.0;
    double SmoothedDeceleration = 0.0;
    double ConfirmedSeconds = 0.0;
    double LiveDistanceMeters = 0.0;
    int ConsecutivePairs = 0;
    bool HasPrevious = false;
    bool LiveEstimate = false;
};
} // namespace latebrake
