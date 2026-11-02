#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Simplified lateral grip budget on the actual player/rival Unreal Chaos
// chassis. Forces oppose measured body-right slip, never an input-axis
// animation. This is NOT a per-wheel suspension, tire contact, ABS,
// chassis load-transfer or a validated physical tire model.
struct LateralGripSample final
{
    double DeltaSeconds = 0.0;
    double LateralCmPerSecond = 0.0;
    double ChassisMassKg = 0.0;
    // Current driver intent filtered through HandbrakeDriveProfile.
    double SlipDampingRatePerSecond = 0.0;
    // Product of observed surface/tire/impact grip and garage handling
    // tuning; multiplied here ONCE by the original damping request.
    double GripScale = 1.0;
    // Actual SAME FRAME bounded longitudinal service/handbrake response,
    // from CalculateGripLimitedBraking, not a guessed brake-pedal value.
    double RequestedBrakeDecelerationCmPerSecondSquared = 0.0;
    bool ArcadeHandling = true;
    bool HandbrakeActive = false;
};

struct LateralGripOutput final
{
    bool Valid = false;
    // Signed body-right physical AddForce magnitude (kg*cm/s^2).
    double RightForce = 0.0;
    double RequestedLateralDeceleration = 0.0;
    double AppliedLateralDeceleration = 0.0;
    double AvailableLateralDeceleration = 0.0;
    bool GripLimited = false;
};

inline constexpr double MaximumLateralFrameSeconds = 0.10;
inline constexpr double ArcadeLateralAccelerationBudget = 1250.0;
inline constexpr double SimulationLateralAccelerationBudget = 1050.0;
inline constexpr double ArcadeCombinedGripBudget = 1550.0;
inline constexpr double SimulationCombinedGripBudget = 1350.0;
inline constexpr double HandbrakeLateralGripFraction = 0.42;

inline LateralGripOutput CalculateLateralGripForce(
    LateralGripSample sample) noexcept
{
    if (!std::isfinite(sample.DeltaSeconds) ||
        sample.DeltaSeconds <= 0.0 ||
        sample.DeltaSeconds > MaximumLateralFrameSeconds ||
        !std::isfinite(sample.LateralCmPerSecond) ||
        std::abs(sample.LateralCmPerSecond) > 15000.0 ||
        !std::isfinite(sample.ChassisMassKg) ||
        sample.ChassisMassKg < 100.0 ||
        sample.ChassisMassKg > 15000.0 ||
        !std::isfinite(sample.SlipDampingRatePerSecond) ||
        sample.SlipDampingRatePerSecond < 0.0 ||
        sample.SlipDampingRatePerSecond > 50.0 ||
        !std::isfinite(sample.GripScale) ||
        sample.GripScale < 0.05 ||
        sample.GripScale > 4.0 ||
        !std::isfinite(sample.RequestedBrakeDecelerationCmPerSecondSquared) ||
        sample.RequestedBrakeDecelerationCmPerSecondSquared < 0.0 ||
        sample.RequestedBrakeDecelerationCmPerSecondSquared > 5000.0)
    {
        return {};
    }

    const double lateral = std::abs(sample.LateralCmPerSecond);
    const double rawRequest = lateral *
        sample.SlipDampingRatePerSecond * sample.GripScale;
    const double nominalLateral =
        (sample.ArcadeHandling ? ArcadeLateralAccelerationBudget
                               : SimulationLateralAccelerationBudget) *
        sample.GripScale;
    const double totalBudget =
        (sample.ArcadeHandling ? ArcadeCombinedGripBudget
                               : SimulationCombinedGripBudget) *
        sample.GripScale;
    // A coarse friction-circle budget: braking and cornering must draw
    // from the same contact-grip allowance, not independently demand
    // extreme longitudinal AND lateral acceleration in the same frame.
    const double usedLongitudinal = std::min(
        sample.RequestedBrakeDecelerationCmPerSecondSquared, totalBudget);
    const double remaining = std::sqrt(std::max(0.0,
        totalBudget * totalBudget - usedLongitudinal * usedLongitudinal));
    const double handbrakeScale = sample.HandbrakeActive
        ? HandbrakeLateralGripFraction : 1.0;
    const double available = std::min(nominalLateral, remaining) *
        handbrakeScale;

    // Even on low-grip roads or long qualified frames the *additional*
    // resistive force cannot reverse the sampled sideways velocity in one
    // frame. Existing Chaos collision impulses remain independent.
    const double frameLimit = sample.DeltaSeconds > 0.0
        ? 0.8 * lateral / sample.DeltaSeconds : 0.0;
    const double applied = std::min({rawRequest, available, frameLimit});
    const double force = lateral <= 1.0e-9 ? 0.0 :
        -std::copysign(applied * sample.ChassisMassKg,
            sample.LateralCmPerSecond);
    return {true, force, rawRequest, applied, available,
            rawRequest > available + 1.0e-6 && lateral > 1.0};
}
} // namespace latebrake
