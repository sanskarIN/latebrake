#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

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
    // World-planar velocity projected onto each body's physical right
    // direction. Heading alignment alone cannot qualify a sliding car.
    double FollowerLateralSpeedCmPerSecond = 0.0;
    double LeadLateralSpeedCmPerSecond = 0.0;
    // Append-only so the pawn's aggregate observation keeps its existing
    // measured lateral-speed ordering. Defaults closed on missing proof.
    bool ClearPhysicalDraftPath = false;
    // Both genuine Chaos Z velocities are required. Missing observations
    // fail closed rather than silently treating unobserved motion as zero.
    double FollowerVerticalSpeedCmPerSecond =
        std::numeric_limits<double>::quiet_NaN();
    double LeadVerticalSpeedCmPerSecond =
        std::numeric_limits<double>::quiet_NaN();
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
    // Prototype aerodynamic help requires predominantly forward motion,
    // not a handbrake drift or rival travelling broadside to the lane.
    static constexpr double MaximumLateralSlipFraction = 0.25;
    static constexpr double MaximumLateralSpeedCmPerSecond = 500.0;
    static constexpr double ConfirmSeconds = 0.18;
    static constexpr double MaxForceExtraFraction = 0.06;
    // Admit ordinary tick-order/velocity-estimation skew without carrying
    // drafting credit across an apparent rival teleport inside the lane.
    static constexpr double MaxRelativeMotionResidualCm = 200.0;
    // A vertical jump can hide inside the wider 3D allowance while
    // staying under the legal 2.3m height envelope. Check Z separately:
    // an unmeasured 0.9m rise in a 50ms frame is NOT normal continuity.
    static constexpr double MaxVerticalMotionResidualCm = 75.0;

    void Reset() noexcept
    {
        EligibleSeconds = 0.0;
        ExtraForceFraction = 0.0;
        HavePreviousOffset = false;
        PreviousOffsetX = 0.0;
        PreviousOffsetY = 0.0;
        PreviousOffsetZ = 0.0;
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
        const double followerLateral =
            sample.FollowerLateralSpeedCmPerSecond;
        const double leadLateral = sample.LeadLateralSpeedCmPerSecond;
        const double followerVertical =
            sample.FollowerVerticalSpeedCmPerSecond;
        const double leadVertical = sample.LeadVerticalSpeedCmPerSecond;
        const double throttle = sample.ForwardThrottle;
        if (!sample.EligibleHumanRace || !sample.ClearPhysicalDraftPath ||
            sample.Braking ||
            !std::isfinite(dt) || dt < MinFrameSeconds ||
            dt > MaxFrameSeconds || !std::isfinite(follower) ||
            !std::isfinite(lead) || follower < MinimumFollowerSpeed ||
            follower > 10000.0 || lead < MinimumLeadSpeed ||
            lead > 10000.0 || follower - lead > MaximumClosingSpeed ||
            !std::isfinite(ax) || !std::isfinite(ay) ||
            !std::isfinite(bx) || !std::isfinite(by) ||
            !std::isfinite(dx) || !std::isfinite(dy) ||
            !std::isfinite(dz) || !std::isfinite(throttle) ||
            !std::isfinite(followerLateral) ||
            !std::isfinite(leadLateral) ||
            !std::isfinite(followerVertical) ||
            !std::isfinite(leadVertical) ||
            std::abs(followerVertical) > 10000.0 ||
            std::abs(leadVertical) > 10000.0 ||
            throttle < 0.30 || throttle > 1.0)
        {
            Reset();
            return 0.0;
        }
        const double followerNorm = ax * ax + ay * ay;
        const double leadNorm = bx * bx + by * by;
        if (followerNorm < 0.95 || followerNorm > 1.05 ||
            leadNorm < 0.95 || leadNorm > 1.05)
        {
            Reset();
            return 0.0;
        }
        // The 5% source tolerance does not authorize scaled heading vectors
        // to fake alignment. Resolve BOTH measured directions to unit planar
        // axes before any angular, gap or world-velocity calculation.
        const double invFollowerNorm = 1.0 / std::sqrt(followerNorm);
        const double invLeadNorm = 1.0 / std::sqrt(leadNorm);
        const double fwdX = ax * invFollowerNorm;
        const double fwdY = ay * invFollowerNorm;
        const double leadX = bx * invLeadNorm;
        const double leadY = by * invLeadNorm;
        if (fwdX * leadX + fwdY * leadY < MinimumHeadingAlignment)
        {
            Reset();
            return 0.0;
        }
        // Both chassis must travel primarily FORWARD rather than merely
        // point in the same direction while sliding sideways. These are
        // actual per-body world velocity projections, not yaw estimates.
        // Qualification is revoked immediately; prior charge cannot
        // survive a single unqualified sideways-slipping physics frame.
        if (std::abs(followerLateral) >
                std::min(MaximumLateralSpeedCmPerSecond,
                         MaximumLateralSlipFraction * follower) ||
            std::abs(leadLateral) >
                std::min(MaximumLateralSpeedCmPerSecond,
                         MaximumLateralSlipFraction * lead))
        {
            Reset();
            return 0.0;
        }
        const double forwardGap = dx * fwdX + dy * fwdY;
        const double lateralGap = std::abs(dx * fwdY - dy * fwdX);
        if (!std::isfinite(forwardGap) || !std::isfinite(lateralGap) ||
            forwardGap < MinForwardDistance ||
            forwardGap >= MaxForwardDistance ||
            lateralGap >= MaxLateralDistance ||
            std::abs(dz) > MaxElevationDistance)
        {
            Reset();
            return 0.0;
        }
        // The pair must remain physically continuous, not just pass each
        // independent in-lane snapshot. Predict relative world displacement
        // from *both* measured 3D velocity vectors over this frame.
        // A recover/teleport can preserve an otherwise eligible 7-38m gap;
        // such a jump must discard the old 0.18s confirmation window.
        if (HavePreviousOffset)
        {
            const double followerVx =
                follower * fwdX - followerLateral * fwdY;
            const double followerVy =
                follower * fwdY + followerLateral * fwdX;
            const double leadVx =
                lead * leadX - leadLateral * leadY;
            const double leadVy =
                lead * leadY + leadLateral * leadX;
            const double predictedX = PreviousOffsetX + dt * (leadVx - followerVx);
            const double predictedY = PreviousOffsetY + dt * (leadVy - followerVy);
            const double predictedZ = PreviousOffsetZ +
                dt * (leadVertical - followerVertical);
            const double residual = std::hypot(
                std::hypot(dx - predictedX, dy - predictedY),
                dz - predictedZ);
            const double verticalResidual = dz - predictedZ;
            if (!std::isfinite(residual) ||
                std::abs(verticalResidual) > MaxVerticalMotionResidualCm ||
                residual > MaxRelativeMotionResidualCm)
            {
                Reset();
                return 0.0;
            }
        }
        PreviousOffsetX = dx;
        PreviousOffsetY = dy;
        PreviousOffsetZ = dz;
        HavePreviousOffset = true;
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
    double PreviousOffsetX = 0.0;
    double PreviousOffsetY = 0.0;
    double PreviousOffsetZ = 0.0;
    bool HavePreviousOffset = false;
};
} // namespace latebrake
