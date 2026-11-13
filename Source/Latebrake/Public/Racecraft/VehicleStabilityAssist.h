#pragma once
#include "Racecraft/TireForceModel.h"
#include "Racecraft/SteeringKinematics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
enum class StabilityMode : std::uint8_t
{
    Off, Sport, Road, WetWeather, Recovery
};
struct StabilityParameters final
{
    double SlipRatioOnset = 0.16;
    double SevereWheelSpin = 0.55;
    double BrakeLockRatio = -0.19;
    double YawErrorOnsetRadS = 0.32;
    double MaximumYawErrorRadS = 2.5;
    double MaximumSlipAngleRadians = 0.30;
    double MinimumPhysicalSpeedCmS = 250.0;
    double MaximumControlSpeedCmS = MaximumPhysicalSpeedCmS;
    double MaximumInterventionBrakeFraction = 0.60;
    double MaximumInterventionThrottleCut = 0.90;
    double MaximumPerFrameThrottleRecovery = 3.0;
    double MaximumPerFrameBrakeRecovery = 6.0;
    double InterventionHoldSeconds = 0.15;
    double SteeringGripAllowance = 0.85;
    double WetWeatherAdditionalReserve = 0.15;
    double MaximumAccelerationG = 2.2;
    double MinimumGroundedWheels = 3.0;
    double LateralSlipDampingGain = 0.32;
    double TorqueReductionForWheelLift = 0.80;
    bool BrakeIndividualWheels = true;
    bool LimitEngineTorque = true;
    bool NeverIncreaseDriverThrottle = true;
    bool RequireOriginalPhysicalWheelContact = true;
};
struct StabilityInput final
{
    PhysicalVehicleFrame Car{};
    SteeringWheelOutput Steering{};
    std::array<TireForceOutput, PhysicalWheelCount> TireForces{};
    double TargetYawRateRadS = 0.0;
    double AvailableGripG = 0.0;
    double DeltaSeconds = 0.0;
    bool OriginalWheelMeasurementsVerified = false;
    bool SessionRaceGreen = false;
};
struct StabilityIntervention final
{
    std::array<double, 4> IndividualBrakeFractions{};
    double ThrottleCeiling = 1.0;
    double CombinedBrakeFloor = 0.0;
    double SteeringCeilingFraction = 1.0;
    double MaximumSpeedCmS = MaximumPhysicalSpeedCmS;
    double WheelSpinSeverity = 0.0;
    double WheelLockSeverity = 0.0;
    double YawErrorRadS = 0.0;
    double SlipAngleRadians = 0.0;
    double GripUseFraction = 0.0;
    double WheelLiftFraction = 0.0;
    double InterventionHoldSeconds = 0.0;
    int SpinningWheels = 0;
    int LockedWheels = 0;
    int LiftedWheels = 0;
    int BrakedWheels = 0;
    bool AntiLockNeeded = false;
    bool TractionControlNeeded = false;
    bool StabilityCorrectionNeeded = false;
    bool MustReducePower = false;
    bool RestrictionOnly = true;
    bool SentActualChaosForce = false;
    bool Valid = false;
};
inline bool ValidStabilityParameters(StabilityParameters s) noexcept
{
    return Finite(s.SlipRatioOnset) &&
        s.SlipRatioOnset > 0.0 &&
        Finite(s.SevereWheelSpin) &&
        s.SevereWheelSpin > s.SlipRatioOnset &&
        Finite(s.BrakeLockRatio) &&
        s.BrakeLockRatio < 0.0 &&
        Finite(s.YawErrorOnsetRadS) &&
        s.YawErrorOnsetRadS > 0.0 &&
        Finite(s.MaximumYawErrorRadS) &&
        s.MaximumYawErrorRadS > s.YawErrorOnsetRadS &&
        Finite(s.MaximumSlipAngleRadians) &&
        s.MaximumSlipAngleRadians > 0.0 &&
        Finite(s.MinimumPhysicalSpeedCmS) &&
        s.MinimumPhysicalSpeedCmS >= 0.0 &&
        Finite(s.MaximumControlSpeedCmS) &&
        s.MaximumControlSpeedCmS >
            s.MinimumPhysicalSpeedCmS &&
        Finite(s.MaximumInterventionBrakeFraction) &&
        s.MaximumInterventionBrakeFraction > 0.0 &&
        s.MaximumInterventionBrakeFraction <= 1.0 &&
        Finite(s.MaximumInterventionThrottleCut) &&
        s.MaximumInterventionThrottleCut >= 0.0 &&
        s.MaximumInterventionThrottleCut <= 1.0 &&
        Finite(s.MaximumPerFrameThrottleRecovery) &&
        s.MaximumPerFrameThrottleRecovery > 0.0 &&
        Finite(s.MaximumPerFrameBrakeRecovery) &&
        s.MaximumPerFrameBrakeRecovery > 0.0 &&
        Finite(s.InterventionHoldSeconds) &&
        s.InterventionHoldSeconds >= 0.0 &&
        Finite(s.SteeringGripAllowance) &&
        s.SteeringGripAllowance > 0.0 &&
        Finite(s.WetWeatherAdditionalReserve) &&
        s.WetWeatherAdditionalReserve >= 0.0 &&
        Finite(s.MinimumGroundedWheels) &&
        s.MinimumGroundedWheels >= 1.0 &&
        s.MinimumGroundedWheels <= 4.0;
}
inline StabilityIntervention EvaluateStabilityIntervention(
    const StabilityInput& input,
    StabilityParameters settings,
    StabilityMode mode) noexcept
{
    StabilityIntervention out;
    if (!ValidStabilityParameters(settings) ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        ValidateVehicleFrame(input.Car,
            settings.RequireOriginalPhysicalWheelContact) !=
                SnapshotFault::None ||
        !input.OriginalWheelMeasurementsVerified ||
        !input.SessionRaceGreen ||
        !Finite(input.TargetYawRateRadS) ||
        !Finite(input.AvailableGripG) ||
        input.AvailableGripG <= 0.0 ||
        !input.Steering.Valid)
        return out;
    out.Valid = true;
    if (mode == StabilityMode::Off) return out;
    const Vec2 velocity{
        input.Car.Chassis.VelocityCmS.X,
        input.Car.Chassis.VelocityCmS.Y
    };
    const double speed = Length(velocity);
    if (speed < settings.MinimumPhysicalSpeedCmS) return out;
    const double slipAngle = PlanarSlipAngle(
        velocity,
        input.Car.Chassis.PlanarPose.HeadingRadians);
    const double yawError = input.TargetYawRateRadS -
        input.Car.Chassis.AngularVelocityRadS.Z;
    out.SlipAngleRadians = slipAngle;
    out.YawErrorRadS = yawError;
    double worstSpin = 0.0;
    double worstLock = 0.0;
    double maximumGripDemand = 0.0;
    for (std::size_t i = 0; i < 4; ++i)
    {
        const WheelObservation& w = input.Car.Wheels[i];
        const double wheelSlip = w.SlipRatio;
        const bool spinning = wheelSlip >
            settings.SlipRatioOnset && w.Driven;
        const bool locked =
            wheelSlip < settings.BrakeLockRatio;
        out.SpinningWheels += spinning ? 1 : 0;
        out.LockedWheels += locked ? 1 : 0;
        out.LiftedWheels += w.PhysicalContact ? 0 : 1;
        worstSpin = std::max(worstSpin,
            spinning ? wheelSlip : 0.0);
        worstLock = std::max(worstLock,
            locked ? -wheelSlip : 0.0);
        maximumGripDemand = std::max(
            maximumGripDemand,
            input.TireForces[i].Valid ?
                input.TireForces[i].CombinedTractionUse : 0.0);
    }
    out.WheelSpinSeverity = Clamp01(SafeDivision(
        worstSpin - settings.SlipRatioOnset,
        settings.SevereWheelSpin -
            settings.SlipRatioOnset));
    out.WheelLockSeverity = Clamp01(SafeDivision(
        worstLock - std::abs(settings.BrakeLockRatio),
        1.0 - std::abs(settings.BrakeLockRatio)));
    out.GripUseFraction = maximumGripDemand;
    out.WheelLiftFraction = out.LiftedWheels / 4.0;
    const double modeGain =
        mode == StabilityMode::Sport ? 0.55 :
        mode == StabilityMode::WetWeather ? 1.3 :
        mode == StabilityMode::Recovery ? 1.2 : 1.0;
    const double wetReserve = mode ==
        StabilityMode::WetWeather ?
        settings.WetWeatherAdditionalReserve : 0.0;
    const double yawSeverity = Smoothstep(
        settings.YawErrorOnsetRadS,
        settings.MaximumYawErrorRadS,
        std::abs(yawError));
    const double sideSeverity = Smoothstep(
        0.5 * settings.MaximumSlipAngleRadians,
        settings.MaximumSlipAngleRadians,
        std::abs(slipAngle));
    const double combined = Clamp01(
        std::max({out.WheelSpinSeverity,
                  yawSeverity, sideSeverity}) *
        modeGain + wetReserve);
    const double wheelSupport =
        static_cast<double>(4 - out.LiftedWheels);
    const double liftVeto = wheelSupport <
        settings.MinimumGroundedWheels ?
        settings.TorqueReductionForWheelLift : 0.0;
    out.ThrottleCeiling = Clamp01(
        1.0 -
        std::max(combined *
            settings.MaximumInterventionThrottleCut,
            liftVeto));
    out.AntiLockNeeded = out.LockedWheels > 0;
    out.TractionControlNeeded = out.SpinningWheels > 0;
    out.StabilityCorrectionNeeded =
        yawSeverity > 0.0 || sideSeverity > 0.0;
    out.MustReducePower = out.ThrottleCeiling < 0.98;
    const double brakeDemand =
        settings.MaximumInterventionBrakeFraction *
        Clamp01(std::max(yawSeverity, sideSeverity) *
                modeGain);
    out.CombinedBrakeFloor = brakeDemand;
    if (settings.BrakeIndividualWheels &&
        brakeDemand > 0.0)
    {
        const bool needRight =
            yawError < 0.0;
        const std::size_t front = needRight ? 1 : 0;
        const std::size_t rear = needRight ? 3 : 2;
        out.IndividualBrakeFractions[front] =
            brakeDemand;
        out.IndividualBrakeFractions[rear] =
            brakeDemand * 0.45;
        out.BrakedWheels = 2;
    }
    out.SteeringCeilingFraction = std::clamp(
        1.0 - 0.55 * std::max(yawSeverity, sideSeverity),
        0.4, 1.0);
    out.MaximumSpeedCmS = std::min(
        settings.MaximumControlSpeedCmS,
        speed * (1.0 -
            0.4 * std::max(sideSeverity, yawSeverity)));
    out.InterventionHoldSeconds = combined > 0.0 ?
        settings.InterventionHoldSeconds : 0.0;
    out.RestrictionOnly = true;
    out.SentActualChaosForce = false;
    return out;
}
class StabilityAssistController final
{
public:
    bool Configure(StabilityParameters parameters,
                   StabilityMode mode) noexcept
    {
        if (!ValidStabilityParameters(parameters)) return false;
        Parameters = parameters;
        Mode = mode;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Previous = {};
        HoldRemaining = 0.0;
        Accepted = 0;
        Rejected = 0;
        InterventionFrames = 0;
        TractionControlFrames = 0;
        AntiLockFrames = 0;
        AccumulatedInterventionSeconds = 0.0;
    }
    StabilityIntervention Step(StabilityInput input) noexcept
    {
        const auto proposal =
            EvaluateStabilityIntervention(input, Parameters, Mode);
        if (!proposal.Valid)
        {
            ++Rejected;
            ResetControlState();
            return {};
        }
        StabilityIntervention output = proposal;
        ++Accepted;
        const double dt = input.DeltaSeconds;
        if (proposal.InterventionHoldSeconds > 0.0)
        {
            HoldRemaining = std::max(
                HoldRemaining,
                proposal.InterventionHoldSeconds);
        }
        else
        {
            HoldRemaining = std::max(
                0.0, HoldRemaining - dt);
        }
        if (HoldRemaining > 0.0 &&
            proposal.ThrottleCeiling >
            Previous.ThrottleCeiling)
        {
            output.ThrottleCeiling =
                std::min(proposal.ThrottleCeiling,
                    Previous.ThrottleCeiling +
                    Parameters.MaximumPerFrameThrottleRecovery * dt);
        }
        output.CombinedBrakeFloor = std::min(
            proposal.CombinedBrakeFloor,
            std::max(0.0,
                Previous.CombinedBrakeFloor +
                Parameters.MaximumPerFrameBrakeRecovery * dt));
        output.InterventionHoldSeconds = HoldRemaining;
        const bool intervening =
            output.ThrottleCeiling < 0.99 ||
            output.CombinedBrakeFloor > 0.01;
        if (intervening)
        {
            ++InterventionFrames;
            AccumulatedInterventionSeconds += dt;
        }
        TractionControlFrames +=
            output.TractionControlNeeded ? 1U : 0U;
        AntiLockFrames +=
            output.AntiLockNeeded ? 1U : 0U;
        Previous = output;
        return output;
    }
    StabilityIntervention Snapshot() const noexcept
    {
        return Previous;
    }
    std::uint64_t Frames() const noexcept { return Accepted; }
    std::uint64_t Rejections() const noexcept { return Rejected; }
    std::uint64_t ActiveInterventionFrames() const noexcept
    {
        return InterventionFrames;
    }
    double InterventionSeconds() const noexcept
    {
        return AccumulatedInterventionSeconds;
    }
private:
    void ResetControlState() noexcept
    {
        Previous = {};
        HoldRemaining = 0.0;
    }
    StabilityParameters Parameters{};
    StabilityMode Mode = StabilityMode::Road;
    StabilityIntervention Previous{};
    double HoldRemaining = 0.0;
    double AccumulatedInterventionSeconds = 0.0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t InterventionFrames = 0;
    std::uint64_t TractionControlFrames = 0;
    std::uint64_t AntiLockFrames = 0;
};
} // namespace latebrake::racecraft
