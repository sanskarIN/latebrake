#pragma once

#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Advisory-only measured-route audit for the PLANNED 1.7-km original
// circuit. It cannot set throttle, steering, position, physics forces,
// checkpoint credit, race results, achievements, or SaveGame state.
// It must receive actual same-chassis world position and measured velocity
// from a trusted future UE adapter; no such adapter is enabled here.
struct CircuitPaceAuditSample final
{
    std::uint64_t SessionIdentity = 0;
    std::uint64_t ActorIdentity = 0;
    std::uint64_t FrameOrdinal = 0;
    double DeltaSeconds = 0.0;
    double WorldX = 0.0;
    double WorldY = 0.0;
    double WorldVelocityX = 0.0;
    double WorldVelocityY = 0.0;
    double ReportedLapDistanceCm = 0.0;
    double ForwardThrottle = 0.0;
    double SteeringAxis = 0.0;
    bool ServiceBrakeHeld = false;
    bool QualifiedRoadSupport = false;
    bool ActiveHumanRace = false;
};

struct CircuitPaceAuditSnapshot final
{
    bool Valid = false;
    bool RouteValid = false;
    bool MotionContinuous = false;
    bool OnAuthoredRoad = false;
    bool AboveAdvisoryPace = false;
    bool BrakingAbovePace = false;
    double TargetSpeedCmPerSecond = 0.0;
    double MeasuredSpeedCmPerSecond = 0.0;
    double ForwardExcessCmPerSecond = 0.0;
    double SuggestedDecelerationCmPerSecondSquared = 0.0;
    double SignedRoadOffsetCm = 0.0;
    double ContinuousObservedSeconds = 0.0;
    double TimeAbovePaceSeconds = 0.0;
    std::uint64_t AcceptedContinuityFrames = 0;
    std::uint64_t RejectedSamples = 0;
};

// Stateless bounds shared by the audit; unlike a racing result, these are
// advisory thresholds and are NOT evidence that the map is UE-cooked.
inline constexpr double CircuitPaceAuditMinimumFrameSeconds = 0.001;
inline constexpr double CircuitPaceAuditMaximumFrameSeconds = 0.1;
inline constexpr double CircuitPaceAuditMaximumSpeed = 15000.0;
inline constexpr double CircuitPaceAuditRouteProvenanceToleranceCm = 220.0;
inline constexpr double CircuitPaceAuditWorldMotionToleranceCm = 180.0;
inline constexpr double CircuitPaceAuditExcessHysteresisCmPerSecond = 180.0;
inline constexpr double CircuitPaceAuditGuidanceLookAheadCm = 4500.0;

inline double CircularCircuitDistanceDifference(
    double first, double second) noexcept
{
    if (!std::isfinite(first) || !std::isfinite(second))
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (first < 0.0 || second < 0.0 ||
        first >= WorldScaleCircuitLapCm ||
        second >= WorldScaleCircuitLapCm)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    const double diff = std::abs(first - second);
    return std::min(diff, WorldScaleCircuitLapCm - diff);
}

inline double SignedCircuitAdvance(
    double from, double to) noexcept
{
    if (!std::isfinite(from) || !std::isfinite(to) ||
        from < 0.0 || to < 0.0 ||
        from >= WorldScaleCircuitLapCm ||
        to >= WorldScaleCircuitLapCm)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::remainder(to - from, WorldScaleCircuitLapCm);
}

inline bool IsFiniteCircuitPaceAuditSample(
    const CircuitPaceAuditSample& sample) noexcept
{
    if (sample.SessionIdentity == 0 || sample.ActorIdentity == 0 ||
        sample.FrameOrdinal == 0 || !sample.ActiveHumanRace ||
        !sample.QualifiedRoadSupport)
    {
        return false;
    }
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds < CircuitPaceAuditMinimumFrameSeconds ||
        sample.DeltaSeconds > CircuitPaceAuditMaximumFrameSeconds)
    {
        return false;
    }
    if (!std::isfinite(sample.WorldX) ||
        !std::isfinite(sample.WorldY) ||
        !std::isfinite(sample.WorldVelocityX) ||
        !std::isfinite(sample.WorldVelocityY) ||
        !std::isfinite(sample.ReportedLapDistanceCm) ||
        !std::isfinite(sample.ForwardThrottle) ||
        !std::isfinite(sample.SteeringAxis))
    {
        return false;
    }
    if (std::abs(sample.WorldX) > 1000000.0 ||
        std::abs(sample.WorldY) > 1000000.0 ||
        std::abs(sample.WorldVelocityX) > CircuitPaceAuditMaximumSpeed ||
        std::abs(sample.WorldVelocityY) > CircuitPaceAuditMaximumSpeed ||
        std::hypot(sample.WorldVelocityX, sample.WorldVelocityY) >
            CircuitPaceAuditMaximumSpeed ||
        std::abs(sample.ForwardThrottle) > 1.0 ||
        std::abs(sample.SteeringAxis) > 1.0 ||
        sample.ReportedLapDistanceCm < 0.0 ||
        sample.ReportedLapDistanceCm >= WorldScaleCircuitLapCm)
    {
        return false;
    }
    return true;
}

class CircuitPaceAudit final
{
public:
    CircuitPaceAudit() noexcept
        : Reference(BuildWorldScalePacePlan())
    {
        Reset();
    }

    void Reset() noexcept
    {
        Session = 0;
        Actor = 0;
        PreviousOrdinal = 0;
        PreviousX = 0.0;
        PreviousY = 0.0;
        PreviousVx = 0.0;
        PreviousVy = 0.0;
        PreviousProgress = 0.0;
        HaveBaseline = false;
        ContinuousSeconds = 0.0;
        AbovePaceSeconds = 0.0;
        AcceptedFrames = 0;
        RejectedFrames = 0;
        Last = {};
    }

    void Suspend() noexcept
    {
        // A pause, unloaded actor, focus loss, new track, or corrupted
        // frame discards every previously qualified continuity baseline.
        Reset();
    }

    const WorldScalePacePlan& Plan() const noexcept
    {
        return Reference;
    }

    CircuitPaceAuditSnapshot Readout() const noexcept
    {
        return Last;
    }

    CircuitPaceAuditSnapshot Observe(
        const CircuitPaceAuditSample& sample) noexcept
    {
        if (!Reference.Valid || !IsFiniteCircuitPaceAuditSample(sample))
        {
            RejectAndBreakContinuity();
            return Last;
        }

        const CircuitPoint position{sample.WorldX, sample.WorldY};
        const auto projection = ProjectWorldScaleRoute(Reference, position);
        if (!projection.Valid || !projection.InsidePhysicalCandidateRoad ||
            !std::isfinite(projection.SuggestedSpeedCmps) ||
            CircularCircuitDistanceDifference(
                sample.ReportedLapDistanceCm,
                projection.LapDistanceCm) >
                    CircuitPaceAuditRouteProvenanceToleranceCm)
        {
            RejectAndBreakContinuity();
            return Last;
        }

        // A new source identity NEVER inherits another race's or actor's
        // accumulated time, and even an identical pose is just baseline.
        if (Session != sample.SessionIdentity ||
            Actor != sample.ActorIdentity)
        {
            Reset();
            Session = sample.SessionIdentity;
            Actor = sample.ActorIdentity;
        }

        const double speed = std::hypot(
            sample.WorldVelocityX, sample.WorldVelocityY);
        CircuitPaceAuditSnapshot next{};
        next.Valid = true;
        next.RouteValid = true;
        next.OnAuthoredRoad = true;
        next.TargetSpeedCmPerSecond = projection.SuggestedSpeedCmps;
        next.MeasuredSpeedCmPerSecond = speed;
        next.SignedRoadOffsetCm = projection.SignedLateralCm;
        next.ForwardExcessCmPerSecond = std::max(
            0.0, speed - projection.SuggestedSpeedCmps);

        bool continuous = false;
        if (HaveBaseline && sample.FrameOrdinal == PreviousOrdinal + 1)
        {
            // The trapezoidal world-XY trajectory must agree with genuine
            // consecutive observed actor poses. Mere reported speed and
            // a plausible lane offset cannot grant new pace evidence.
            const double expectedX = PreviousX +
                0.5 * sample.DeltaSeconds *
                    (PreviousVx + sample.WorldVelocityX);
            const double expectedY = PreviousY +
                0.5 * sample.DeltaSeconds *
                    (PreviousVy + sample.WorldVelocityY);
            const double residual = std::hypot(
                sample.WorldX - expectedX, sample.WorldY - expectedY);
            const double forwardAdvance = SignedCircuitAdvance(
                PreviousProgress, sample.ReportedLapDistanceCm);
            const double allowableResidual = std::max(
                CircuitPaceAuditWorldMotionToleranceCm,
                0.15 * speed * sample.DeltaSeconds);

            // Route progress must be forward-coherent with the distance
            // implied by real XY velocity. Small negative track-distance
            // jitter is tolerated, but teleports/recovery never score.
            const double expectedAdvance = 0.5 * sample.DeltaSeconds *
                (std::hypot(PreviousVx, PreviousVy) + speed);
            const double progressResidual = std::abs(
                forwardAdvance - expectedAdvance);
            continuous = std::isfinite(residual) &&
                std::isfinite(forwardAdvance) &&
                residual <= allowableResidual &&
                forwardAdvance >= -25.0 &&
                progressResidual <=
                    CircuitPaceAuditRouteProvenanceToleranceCm;
        }

        // The current point may be a valid new baseline even after a
        // frame gap; do not retroactively credit its unobserved dt.
        PreviousOrdinal = sample.FrameOrdinal;
        PreviousX = sample.WorldX;
        PreviousY = sample.WorldY;
        PreviousVx = sample.WorldVelocityX;
        PreviousVy = sample.WorldVelocityY;
        PreviousProgress = sample.ReportedLapDistanceCm;
        HaveBaseline = true;

        if (continuous)
        {
            ContinuousSeconds += sample.DeltaSeconds;
            ++AcceptedFrames;
            const bool above = next.ForwardExcessCmPerSecond >
                CircuitPaceAuditExcessHysteresisCmPerSecond;
            if (above) AbovePaceSeconds += sample.DeltaSeconds;
            next.AboveAdvisoryPace = above;
            next.BrakingAbovePace = above && sample.ServiceBrakeHeld;

            // Pure advisory deceleration from current observed velocity
            // to local target over an explicit, fixed planning horizon.
            // Bounded by the same candidate circuit's stated brake budget.
            const double deltaV2 = std::max(0.0,
                speed * speed -
                projection.SuggestedSpeedCmps *
                    projection.SuggestedSpeedCmps);
            next.SuggestedDecelerationCmPerSecondSquared = std::min(
                WorldScalePaceBrakeAccelCmps2,
                deltaV2 / (2.0 * CircuitPaceAuditGuidanceLookAheadCm));
        }
        else
        {
            // A position discontinuity or first baseline cannot fabricate
            // lap seconds or an actionable brake suggestion.
            ContinuousSeconds = 0.0;
            AbovePaceSeconds = 0.0;
            AcceptedFrames = 0;
            next.SuggestedDecelerationCmPerSecondSquared = 0.0;
        }

        next.MotionContinuous = continuous;
        next.ContinuousObservedSeconds = ContinuousSeconds;
        next.TimeAbovePaceSeconds = AbovePaceSeconds;
        next.AcceptedContinuityFrames = AcceptedFrames;
        next.RejectedSamples = RejectedFrames;
        Last = next;
        return Last;
    }

private:
    void RejectAndBreakContinuity() noexcept
    {
        ++RejectedFrames;
        HaveBaseline = false;
        PreviousOrdinal = 0;
        ContinuousSeconds = 0.0;
        AbovePaceSeconds = 0.0;
        AcceptedFrames = 0;
        Last = {};
        Last.RejectedSamples = RejectedFrames;
    }

    WorldScalePacePlan Reference{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t PreviousOrdinal = 0;
    double PreviousX = 0.0;
    double PreviousY = 0.0;
    double PreviousVx = 0.0;
    double PreviousVy = 0.0;
    double PreviousProgress = 0.0;
    bool HaveBaseline = false;
    double ContinuousSeconds = 0.0;
    double AbovePaceSeconds = 0.0;
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    CircuitPaceAuditSnapshot Last{};
};
} // namespace latebrake
