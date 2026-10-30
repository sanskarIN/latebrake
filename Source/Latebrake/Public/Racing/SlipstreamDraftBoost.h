#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Single-rival arcade slipstream, inferred from LIVE rigid-body motion.
// Not an aerodynamic CFD model, AI overtaking system, or scoring input.
struct DraftingSample final
{
    double FrameSeconds = 0.0;
    double FollowerSpeedCmPerSecond = 0.0;
    double LeadSpeedCmPerSecond = 0.0;
    double FollowerForwardX = 1.0;
    double FollowerForwardY = 0.0;
    double LeadForwardX = 1.0;
    double LeadForwardY = 0.0;
    double LeadOffsetX = 0.0;
    double LeadOffsetY = 0.0;
    double LeadOffsetZ = 0.0;
    double ForwardThrottle = 0.0;
    bool Braking = false;
    bool EligibleHumanRace = false;
};

class SlipstreamDraftBoost final
{
public:
    static constexpr double MinFrameSeconds = 0.001;
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MinimumFollowerSpeed = 1200.0;
    static constexpr double MinimumLeadSpeed = 1000.0;
    static constexpr double MinForwardDistance = 700.0;
    static constexpr double MaxForwardDistance = 3800.0;
    static constexpr double MaxLateralDistance = 380.0;
    static constexpr double MaxElevationDistance = 230.0;
    static constexpr double MinimumHeadingAlignment = 0.94;
    static constexpr double MaximumClosingSpeed = 1000.0;
    static constexpr double ConfirmSeconds = 0.18;
    static constexpr double MaxForceExtraFraction = 0.06;

    void Reset() noexcept
    {
        EligibleSeconds = 0.0;
        ExtraForceFraction = 0.0;
    }
    void Suspend() noexcept { Reset(); }

    double Observe(const DraftingSample& sample) noexcept
    {
        // Invalid telemetry clears qualification rather than holding a
        // previous boost through a teleport, lost rival, pause or hitch.
        const double dt = sample.FrameSeconds;
        const double follower = sample.FollowerSpeedCmPerSecond;
        const double lead = sample.LeadSpeedCmPerSecond;
        const double ax = sample.FollowerForwardX;
        const double ay = sample.FollowerForwardY;
        const double bx = sample.LeadForwardX;
        const double by = sample.LeadForwardY;
        const double dx = sample.LeadOffsetX;
        const double dy = sample.LeadOffsetY;
        const double dz = sample.LeadOffsetZ;
        const double throttle = sample.ForwardThrottle;
        if (!sample.EligibleHumanRace || sample.Braking ||
            !std::isfinite(dt) || dt < MinFrameSeconds ||
            dt > MaxFrameSeconds || !std::isfinite(follower) ||
            !std::isfinite(lead) || follower < MinimumFollowerSpeed ||
            follower > 10000.0 || lead < MinimumLeadSpeed ||
            lead > 10000.0 || follower - lead > MaximumClosingSpeed ||
            !std::isfinite(ax) || !std::isfinite(ay) ||
            !std::isfinite(bx) || !std::isfinite(by) ||
            !std::isfinite(dx) || !std::isfinite(dy) ||
            !std::isfinite(dz) || !std::isfinite(throttle) ||
            throttle < 0.30 || throttle > 1.0)
        {
            Reset();
            return 0.0;
        }
        const double followerNorm = ax * ax + ay * ay;
        const double leadNorm = bx * bx + by * by;
        if (followerNorm < 0.95 || followerNorm > 1.05 ||
            leadNorm < 0.95 || leadNorm > 1.05 ||
            ax * bx + ay * by < MinimumHeadingAlignment)
        {
            Reset();
            return 0.0;
        }
        const double forwardGap = dx * ax + dy * ay;
        const double lateralGap = std::abs(dx * ay - dy * ax);
        if (!std::isfinite(forwardGap) || !std::isfinite(lateralGap) ||
            forwardGap < MinForwardDistance ||
            forwardGap >= MaxForwardDistance ||
            lateralGap >= MaxLateralDistance ||
            std::abs(dz) > MaxElevationDistance)
        {
            Reset();
            return 0.0;
        }
        EligibleSeconds = std::min(ConfirmSeconds, EligibleSeconds + dt);
        if (EligibleSeconds < ConfirmSeconds)
        {
            ExtraForceFraction = 0.0;
            return 0.0;
        }
        // Smaller forward/lateral gaps offer a mild bounded gameplay bonus.
        const double distanceWeight = (MaxForwardDistance - forwardGap) /
            (MaxForwardDistance - MinForwardDistance);
        const double laneWeight = 1.0 - lateralGap / MaxLateralDistance;
        ExtraForceFraction = MaxForceExtraFraction *
            std::clamp(distanceWeight * laneWeight, 0.0, 1.0);
        return ExtraForceFraction;
    }

    bool IsActive() const noexcept { return ExtraForceFraction > 0.0; }
    double ExtraForce() const noexcept { return ExtraForceFraction; }
    double QualifiedSeconds() const noexcept { return EligibleSeconds; }

private:
    double EligibleSeconds = 0.0;
    double ExtraForceFraction = 0.0;
};
} // namespace latebrake
