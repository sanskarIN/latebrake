#pragma once

#include "AI/JerkBrakeEnvelope.h"

#include <cmath>

namespace latebrake
{
// Advisory inverse of the finite-jerk stopping envelope. The returned speed
// is a relative closing-speed ceiling, not an ego speed or brake actuator.
// All input gaps and speed quantities use centimeters and seconds.
struct JerkBrakeSpeedLimit final
{
    bool Valid=false;
    bool HasSafeClosingSpeed=false;
    double MaximumClosingSpeedCmps=0.0;
    double RequiredGapAtLimitCm=0.0;
};

// Compute the largest conservative closing speed that fits the measured gap.
// A geometric overlap is explicitly unsafe even when both cars are stopped.
// The finite bisection returns the lower bound; never round upward.
inline JerkBrakeSpeedLimit SolveJerkBrakeClosingSpeedLimit(
    double measuredForwardGapCm,
    JerkBrakeEnvelopeSettings settings={}) noexcept
{
    JerkBrakeSpeedLimit out{};
    if(!ValidJerkBrakeEnvelopeSettings(settings) ||
       !std::isfinite(measuredForwardGapCm) ||
       measuredForwardGapCm<0.0 || measuredForwardGapCm>1000000.0)
        return out;

    out.Valid=true;
    const double clearance=settings.VehicleLengthAllowanceCm+
        settings.MinimumClearanceCm;
    if(measuredForwardGapCm<=clearance)
        return out;

    // A relative speed of 20,000 cm/s is the maximum supported input of
    // EvaluateJerkBrakeEnvelope. Its exact threshold is still emergency.
    const auto ceiling=EvaluateJerkBrakeEnvelope(
        20000.0,0.0,measuredForwardGapCm,settings);
    if(!ceiling.Valid)
    {
        out.Valid=false;
        return out;
    }
    if(!ceiling.RequiresEmergencyBrake)
    {
        out.HasSafeClosingSpeed=true;
        out.MaximumClosingSpeedCmps=20000.0;
        out.RequiredGapAtLimitCm=ceiling.RequiredGapCm;
        return out;
    }

    double lower=0.0;
    double upper=20000.0;
    for(int iteration=0;iteration<64;++iteration)
    {
        const double middle=lower+(upper-lower)*0.5;
        const auto trial=EvaluateJerkBrakeEnvelope(
            middle,0.0,measuredForwardGapCm,settings);
        if(!trial.Valid)
        {
            out.Valid=false;
            return out;
        }
        if(trial.RequiresEmergencyBrake)
            upper=middle;
        else
            lower=middle;
    }

    const auto result=EvaluateJerkBrakeEnvelope(
        lower,0.0,measuredForwardGapCm,settings);
    if(!result.Valid || result.RequiresEmergencyBrake)
    {
        out.Valid=false;
        return out;
    }
    out.HasSafeClosingSpeed=true;
    out.MaximumClosingSpeedCmps=lower;
    out.RequiredGapAtLimitCm=result.RequiredGapCm;
    return out;
}
} // namespace latebrake
