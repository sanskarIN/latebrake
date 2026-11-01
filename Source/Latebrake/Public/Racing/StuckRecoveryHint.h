#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Advisory ONLY. This cannot initiate a teleport, charge a penalty,
// affect gate credit, declare a race result or alter saved progression.
struct StuckRecoverySample final
{
    double DeltaSeconds = 0.0;
    double PositionXcm = 0.0;
    double PositionYcm = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    bool ForwardThrottleRequested = false;
    bool DrivingEligible = false;
};

class StuckRecoveryHint final
{
public:
    static constexpr double RequiredSeconds = 4.0;
    static constexpr double MaximumSampleSeconds = 0.15;
    static constexpr double SlowSpeedCmPerSecond = 80.0;
    static constexpr double MaximumAnchorDisplacementCm = 150.0;

    void Reset() noexcept
    {
        StillSeconds = 0.0;
        HasAnchor = false;
        AnchorX = 0.0;
        AnchorY = 0.0;
    }

    bool Observe(const StuckRecoverySample& sample) noexcept
    {
        // Only admitted, continuously sampled, locally driven physics can
        // create this hint. Unqualified samples discard the whole history.
        if (!sample.DrivingEligible || !sample.ForwardThrottleRequested ||
            !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > MaximumSampleSeconds ||
            !std::isfinite(sample.PositionXcm) ||
            !std::isfinite(sample.PositionYcm) ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond >= SlowSpeedCmPerSecond)
        {
            Reset();
            return false;
        }
        if (!HasAnchor)
        {
            AnchorX = sample.PositionXcm;
            AnchorY = sample.PositionYcm;
            HasAnchor = true;
        }

        // Slow movement is not synonymous with being stuck: crossing
        // 1.5 metres from the anchor proves movement despite low speed.
        // hypot avoids intermediate squared-distance overflow.
        const double displacement = std::hypot(
            sample.PositionXcm - AnchorX, sample.PositionYcm - AnchorY);
        if (!std::isfinite(displacement) ||
            displacement > MaximumAnchorDisplacementCm)
        {
            Reset();
            return false;
        }
        StillSeconds = std::min(RequiredSeconds,
            StillSeconds + sample.DeltaSeconds);
        return StillSeconds >= RequiredSeconds;
    }

    double QualifiedStillSeconds() const noexcept { return StillSeconds; }

private:
    double StillSeconds = 0.0;
    double AnchorX = 0.0;
    double AnchorY = 0.0;
    bool HasAnchor = false;
};
} // namespace latebrake
