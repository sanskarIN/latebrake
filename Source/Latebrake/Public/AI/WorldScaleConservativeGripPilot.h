#pragma once

#include "AI/WorldScaleCandidateRivalPilot.h"
#include "Track/WorldScaleVariableGripPace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// A read-only SOURCE-ONLY integration experiment for one future physical
// rival. The sensor-qualified grip planner may only REDUCE the dry pilot's
// power and pace; it has no steering, Chaos, collision, checkpoint or
// official race authority. No native GameMode instantiates this wrapper.
inline constexpr double WorldScaleGripPilotMaxSourceAgeSeconds = 0.75;

struct WorldScaleGripPilotObservation final
{
    WorldScalePilotSample Physical{};
    std::uint64_t SessionIdentity = 0;
    double PhysicsClockSeconds = 0.0;
    bool SourceRoadIdentityVerified = false;
    bool CurrentTireContactVerified = false;
};

struct WorldScaleGripPilotReadout final
{
    bool Valid = false;
    WorldScalePilotDecision Decision{};
    double SourceAgeSeconds = 0.0;
    double MeasuredGripFraction = 0.0;
    double DrySpeedReferenceCmps = 0.0;
    double GripConstrainedReferenceCmps = 0.0;
    bool GripReducedPace = false;
    bool ConservativeBrakeRequested = false;
};

// Stateful only for the original rival pilot's frame-to-frame continuity.
// Plan freshness, session agreement, road and tire contact must ALL hold
// before the dry pilot is advanced. A failed observation never inherits
// earlier continuity. The planner makes no claim that the adapter exists.
class WorldScaleConservativeGripPilot final
{
public:
    void Reset() noexcept { BasePilot.Reset(); }

    WorldScaleGripPilotReadout Step(
        const WorldScalePacePlan& original,
        const WorldScaleVariableGripPacePlan& grip,
        const WorldScaleGripPilotObservation& input) noexcept
    {
        const double age =
            input.PhysicsClockSeconds - grip.SourceClockSeconds;
        if (!original.Valid || !grip.Valid ||
            !input.SourceRoadIdentityVerified ||
            !input.CurrentTireContactVerified ||
            input.SessionIdentity == 0 ||
            input.SessionIdentity != grip.SourceSessionIdentity ||
            !std::isfinite(input.PhysicsClockSeconds) ||
            input.PhysicsClockSeconds < 0.0 ||
            !std::isfinite(grip.SourceClockSeconds) ||
            !std::isfinite(age) || age < 0.0 ||
            age > WorldScaleGripPilotMaxSourceAgeSeconds)
        {
            Reset();
            return {};
        }

        WorldScalePilotDecision originalDecision =
            BasePilot.Step(original, input.Physical);
        if (!originalDecision.Valid)
        {
            Reset();
            return {};
        }

        // Evaluate both the measured station and the actual source-only
        // pilot lookahead; a wet bend cannot become dry merely because
        // the chassis currently occupies a faster preceding straight.
        const auto local = SampleWorldScaleVariableGripPace(
            grip, originalDecision.WrappedLapDistanceCm);
        const auto ahead = SampleWorldScaleVariableGripPace(
            grip, originalDecision.WrappedLapDistanceCm +
                  originalDecision.LookaheadCm);
        if (!local.Valid || !ahead.Valid ||
            !std::isfinite(local.TargetSpeedCmps) ||
            !std::isfinite(ahead.TargetSpeedCmps))
        {
            Reset();
            return {};
        }
        const double reference = std::min({
            originalDecision.SpeedReferenceCmps,
            local.TargetSpeedCmps,
            ahead.TargetSpeedCmps
        });
        if (!std::isfinite(reference) ||
            reference < WorldScalePaceMinSpeedCmps ||
            reference > originalDecision.SpeedReferenceCmps)
        {
            Reset();
            return {};
        }

        const bool reduced =
            reference < originalDecision.SpeedReferenceCmps - 0.00001;
        const bool wetBrake =
            input.Physical.FlatSpeedCmps > reference + 120.0;
        // Retain existing reverse, spun, corner and overspeed braking.
        // Grip cannot reduce a brake already requested by the dry pilot.
        const bool brake = originalDecision.Command.Brake || wetBrake;
        const double throttle = (!brake &&
             input.Physical.FlatSpeedCmps + 120.0 < reference) ?
             originalDecision.Command.Throttle : 0.0;

        const double drySpeed = originalDecision.SpeedReferenceCmps;
        originalDecision.Command.Brake = brake;
        originalDecision.Command.Throttle = throttle;
        originalDecision.SpeedReferenceCmps = reference;
        // The only mutable fields above are brake, throttle and the
        // conservative reference speed; independent yaw/steering,
        // checkpoint and actor identity are never touched.
        return {
            true, originalDecision, age,
            std::min(local.GripAtPreviousNode,
                     ahead.GripAtPreviousNode),
            drySpeed, reference, reduced, wetBrake
        };
    }

private:
    WorldScaleCandidateRivalPilot BasePilot{};
};
} // namespace latebrake
