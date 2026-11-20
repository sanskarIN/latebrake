#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Advisory source-only model: a finite jerk ramp followed by constant
// deceleration. All quantities are centimeters, seconds and their powers.
// This does not issue brake torque or replace tire/road grip qualification.
struct JerkBrakeEnvelopeSettings final
{
    double MaximumDecelerationCmps2=520.0;
    double MaximumJerkCmps3=1800.0;
    double ReactionSeconds=0.27;
    double VehicleLengthAllowanceCm=410.0;
    double MinimumClearanceCm=75.0;
};

struct JerkBrakeEnvelopeResult final
{
    bool Valid=false;
    bool RequiresEmergencyBrake=true;
    double ClosingSpeedCmps=0.0;
    double ReactionDistanceCm=0.0;
    double RampDistanceCm=0.0;
    double ConstantBrakeDistanceCm=0.0;
    double RequiredGapCm=0.0;
    double RampDurationSeconds=0.0;
    double StopDurationSeconds=0.0;
};

inline bool ValidJerkBrakeEnvelopeSettings(
    const JerkBrakeEnvelopeSettings& s) noexcept
{
    return std::isfinite(s.MaximumDecelerationCmps2) &&
        s.MaximumDecelerationCmps2>=100.0 &&
        s.MaximumDecelerationCmps2<=2000.0 &&
        std::isfinite(s.MaximumJerkCmps3) &&
        s.MaximumJerkCmps3>=100.0 &&
        s.MaximumJerkCmps3<=20000.0 &&
        std::isfinite(s.ReactionSeconds) &&
        s.ReactionSeconds>=0.0 && s.ReactionSeconds<=2.0 &&
        std::isfinite(s.VehicleLengthAllowanceCm) &&
        s.VehicleLengthAllowanceCm>=0.0 &&
        s.VehicleLengthAllowanceCm<=2000.0 &&
        std::isfinite(s.MinimumClearanceCm) &&
        s.MinimumClearanceCm>=0.0 &&
        s.MinimumClearanceCm<=1000.0;
}

// Conservative relative-motion envelope for a stationary or constant-speed
// obstacle. No assumption is made that the obstacle brakes with the player.
// A closing speed of zero still requires geometric body clearance.
inline JerkBrakeEnvelopeResult EvaluateJerkBrakeEnvelope(
    double egoForwardSpeedCmps,
    double obstacleForwardSpeedCmps,
    double measuredForwardGapCm,
    JerkBrakeEnvelopeSettings settings={}) noexcept
{
    JerkBrakeEnvelopeResult out{};
    if(!ValidJerkBrakeEnvelopeSettings(settings) ||
       !std::isfinite(egoForwardSpeedCmps) ||
       !std::isfinite(obstacleForwardSpeedCmps) ||
       !std::isfinite(measuredForwardGapCm) ||
       egoForwardSpeedCmps<0.0 || egoForwardSpeedCmps>20000.0 ||
       obstacleForwardSpeedCmps<0.0 ||
       obstacleForwardSpeedCmps>20000.0 ||
       measuredForwardGapCm<0.0 || measuredForwardGapCm>1000000.0)
        return out;

    const double closing=std::max(0.0,
        egoForwardSpeedCmps-obstacleForwardSpeedCmps);
    const double acceleration=settings.MaximumDecelerationCmps2;
    const double jerk=settings.MaximumJerkCmps3;
    const double fullRamp=acceleration/jerk;
    const double rampVelocityLoss=0.5*jerk*fullRamp*fullRamp;
    const bool stopsDuringRamp=closing<=rampVelocityLoss;
    const double rampTime=stopsDuringRamp
        ? std::sqrt(2.0*closing/jerk) : fullRamp;
    const double rampDistance=closing*rampTime-
        jerk*rampTime*rampTime*rampTime/6.0;
    const double remaining=std::max(0.0,
        closing-0.5*jerk*rampTime*rampTime);
    const double constantDistance=remaining*remaining/
        (2.0*acceleration);
    const double reactionDistance=
        closing*settings.ReactionSeconds;
    const double required=settings.VehicleLengthAllowanceCm+
        settings.MinimumClearanceCm+reactionDistance+
        rampDistance+constantDistance;
    const double stopTime=settings.ReactionSeconds+rampTime+
        remaining/acceleration;
    if(!std::isfinite(required) || !std::isfinite(stopTime) ||
       required<0.0 || rampDistance<0.0)
        return out;

    out.Valid=true;
    out.ClosingSpeedCmps=closing;
    out.ReactionDistanceCm=reactionDistance;
    out.RampDistanceCm=rampDistance;
    out.ConstantBrakeDistanceCm=constantDistance;
    out.RequiredGapCm=required;
    out.RampDurationSeconds=rampTime;
    out.StopDurationSeconds=stopTime;
    out.RequiresEmergencyBrake=measuredForwardGapCm<=required;
    return out;
}
} // namespace latebrake
