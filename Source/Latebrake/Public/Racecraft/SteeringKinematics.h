#pragma once
#include "Racecraft/ChassisLoadTransfer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class SteeringResponseMode : std::uint8_t
{
    Simulation,
    Sport,
    Accessible,
    Recovery
};
struct SteeringRackParameters final
{
    double MaximumRoadWheelAngleRadians = 0.60;
    double MinimumHighSpeedAngleRadians = 0.13;
    double HighSpeedTransitionCmS = 2000.0;
    double HighSpeedFullReductionCmS = 6500.0;
    double MaximumWheelTurningRateRadiansSecond = 1.8;
    double MaximumSteeringRatePerSecond = 4.5;
    double SteeringReturnRatePerSecond = 3.0;
    double SteeringDeadzoneFraction = 0.015;
    double SteeringResponseExponent = 1.5;
    double AckermannFraction = 0.85;
    double FrontToeRadians = 0.001;
    double RearToeRadians = 0.002;
    double WheelbaseCm = 268.0;
    double FrontTrackCm = 158.0;
    double RearTrackCm = 156.0;
    double MinimumTurningRadiusCm = 270.0;
    double MaximumSteerAtSlipRadians = 0.4;
    double SteeringFeedbackStrength = 0.28;
    double SelfCenteringSpeedFactor = 0.32;
    double InputSmoothingSeconds = 0.065;
    double ReactionDelaySeconds = 0.0;
};
struct SteeringObservation final
{
    double DriverRequest = 0.0;
    double SpeedCmS = 0.0;
    double YawRateRadiansSecond = 0.0;
    double SideSlipAngleRadians = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double LateralAccelerationG = 0.0;
    double ContactGripFraction = 1.0;
    double DeltaSeconds = 0.0;
    bool PhysicalSteeringReady = false;
    bool EngineQualified = false;
    bool OriginalTrack = false;
    bool Human = false;
};
struct SteeringWheelOutput final
{
    double RackInput = 0.0;
    double LeftFrontRadians = 0.0;
    double RightFrontRadians = 0.0;
    double LeftRearRadians = 0.0;
    double RightRearRadians = 0.0;
    double MeanFrontRadians = 0.0;
    double OuterWheelRadians = 0.0;
    double InnerWheelRadians = 0.0;
    double KinematicYawRateRadiansSecond = 0.0;
    double EstimatedTurnRadiusCm = 0.0;
    double LateralAccelerationDemandG = 0.0;
    double LateralGripUse = 0.0;
    double UndersteerCompensation = 0.0;
    double RestoringTorqueEstimateNm = 0.0;
    double MaximumAllowedRoadAngleRadians = 0.0;
    bool AckermannQualified = false;
    bool LimitedBySpeed = false;
    bool LimitedByGrip = false;
    bool SteeringAccepted = false;
    bool ModifiesRealPhysics = false;
    bool Valid = false;
};
inline bool ValidSteeringRackParameters(
    SteeringRackParameters s) noexcept
{
    return Finite(s.MaximumRoadWheelAngleRadians) &&
        s.MaximumRoadWheelAngleRadians > 0.0 &&
        s.MaximumRoadWheelAngleRadians < Pi * 0.5 &&
        Finite(s.MinimumHighSpeedAngleRadians) &&
        s.MinimumHighSpeedAngleRadians > 0.0 &&
        s.MinimumHighSpeedAngleRadians <
            s.MaximumRoadWheelAngleRadians &&
        Finite(s.HighSpeedTransitionCmS) &&
        s.HighSpeedTransitionCmS > 0.0 &&
        Finite(s.HighSpeedFullReductionCmS) &&
        s.HighSpeedFullReductionCmS >
            s.HighSpeedTransitionCmS &&
        Finite(s.MaximumWheelTurningRateRadiansSecond) &&
        s.MaximumWheelTurningRateRadiansSecond > 0.0 &&
        Finite(s.MaximumSteeringRatePerSecond) &&
        s.MaximumSteeringRatePerSecond > 0.0 &&
        Finite(s.SteeringReturnRatePerSecond) &&
        s.SteeringReturnRatePerSecond > 0.0 &&
        Finite(s.SteeringDeadzoneFraction) &&
        s.SteeringDeadzoneFraction >= 0.0 &&
        s.SteeringDeadzoneFraction < 0.5 &&
        Finite(s.SteeringResponseExponent) &&
        s.SteeringResponseExponent >= 0.5 &&
        s.SteeringResponseExponent <= 4.0 &&
        Finite(s.AckermannFraction) &&
        s.AckermannFraction >= 0.0 &&
        s.AckermannFraction <= 1.0 &&
        Finite(s.FrontToeRadians) &&
        Finite(s.RearToeRadians) &&
        Finite(s.WheelbaseCm) &&
        s.WheelbaseCm > 100.0 &&
        Finite(s.FrontTrackCm) &&
        s.FrontTrackCm > 75.0 &&
        Finite(s.RearTrackCm) &&
        s.RearTrackCm > 75.0 &&
        Finite(s.MinimumTurningRadiusCm) &&
        s.MinimumTurningRadiusCm > 0.0 &&
        Finite(s.MaximumSteerAtSlipRadians) &&
        s.MaximumSteerAtSlipRadians > 0.0 &&
        Finite(s.SteeringFeedbackStrength) &&
        s.SteeringFeedbackStrength >= 0.0 &&
        Finite(s.SelfCenteringSpeedFactor) &&
        s.SelfCenteringSpeedFactor >= 0.0 &&
        Finite(s.InputSmoothingSeconds) &&
        s.InputSmoothingSeconds >= 0.0;
}
inline double ApplySteeringDeadzone(
    double input, double deadzone, double exponent) noexcept
{
    if (!Finite(input) || !Finite(deadzone) ||
        !Finite(exponent) || deadzone < 0.0 ||
        deadzone >= 1.0 || exponent <= 0.0)
        return 0.0;
    const double magnitude = std::abs(input);
    if (magnitude <= deadzone) return 0.0;
    const double scaled = Clamp01(
        (magnitude - deadzone) / (1.0 - deadzone));
    return Sign(input) * std::pow(scaled, exponent);
}
inline double AllowedWheelAngleAtSpeed(
    double speedCmS, SteeringRackParameters s) noexcept
{
    if (!Finite(speedCmS) || speedCmS < 0.0)
        return 0.0;
    const double factor = Smoothstep(
        s.HighSpeedTransitionCmS,
        s.HighSpeedFullReductionCmS,
        speedCmS);
    return Lerp(s.MaximumRoadWheelAngleRadians,
                s.MinimumHighSpeedAngleRadians,
                factor);
}
inline std::array<double, 2> AckermannAngles(
    double meanAngleRadians,
    double wheelbaseCm, double frontTrackCm,
    double ackermannFraction) noexcept
{
    if (!Finite(meanAngleRadians) ||
        !Finite(wheelbaseCm) || wheelbaseCm <= 0.0 ||
        !Finite(frontTrackCm) || frontTrackCm <= 0.0 ||
        !Finite(ackermannFraction) ||
        ackermannFraction < 0.0 ||
        ackermannFraction > 1.0)
        return {};
    if (std::abs(meanAngleRadians) <= PhysicsEpsilon)
        return {};
    const double radius = std::abs(wheelbaseCm /
        std::tan(meanAngleRadians));
    const double inner = std::atan2(
        wheelbaseCm, std::max(1.0, radius - frontTrackCm * 0.5));
    const double outer = std::atan2(
        wheelbaseCm, radius + frontTrackCm * 0.5);
    const double blendedInner =
        Lerp(std::abs(meanAngleRadians), inner,
             ackermannFraction);
    const double blendedOuter =
        Lerp(std::abs(meanAngleRadians), outer,
             ackermannFraction);
    return meanAngleRadians >= 0.0 ?
        std::array<double, 2>{{blendedOuter, blendedInner}} :
        std::array<double, 2>{{-blendedInner, -blendedOuter}};
}
inline SteeringWheelOutput EvaluateSteering(
    SteeringObservation input,
    SteeringRackParameters parameters,
    double previousRack,
    SteeringResponseMode mode =
        SteeringResponseMode::Simulation) noexcept
{
    SteeringWheelOutput out;
    if (!ValidSteeringRackParameters(parameters) ||
        !input.PhysicalSteeringReady ||
        !input.EngineQualified ||
        !input.OriginalTrack ||
        !Finite(input.DriverRequest) ||
        !Finite(input.SpeedCmS) ||
        !Finite(input.YawRateRadiansSecond) ||
        !Finite(input.SideSlipAngleRadians) ||
        !Finite(input.LateralAccelerationG) ||
        !Finite(input.LongitudinalAccelerationG) ||
        !Finite(input.ContactGripFraction) ||
        !Finite(previousRack) ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        std::abs(input.DriverRequest) > 1.0 ||
        input.SpeedCmS < 0.0 ||
        input.SpeedCmS > MaximumPhysicalSpeedCmS ||
        input.ContactGripFraction < 0.0 ||
        input.ContactGripFraction > 3.0)
        return out;
    const double responseExponent =
        mode == SteeringResponseMode::Accessible ?
            1.0 : mode == SteeringResponseMode::Sport ?
            1.25 : parameters.SteeringResponseExponent;
    double request = ApplySteeringDeadzone(
        input.DriverRequest,
        parameters.SteeringDeadzoneFraction,
        responseExponent);
    if (mode == SteeringResponseMode::Recovery)
        request *= 0.60;
    const double previous = std::clamp(
        previousRack, -1.0, 1.0);
    const double smooth = ExponentialResponse(
        previous, request,
        parameters.InputSmoothingSeconds,
        input.DeltaSeconds);
    const double rate = std::abs(request) < std::abs(previous) ?
        parameters.SteeringReturnRatePerSecond :
        parameters.MaximumSteeringRatePerSecond;
    const double rack = RateLimited(
        previous, smooth, rate, rate,
        input.DeltaSeconds);
    const double speedLimit = AllowedWheelAngleAtSpeed(
        input.SpeedCmS, parameters);
    const double gripCap = std::clamp(
        input.ContactGripFraction, 0.1, 1.0);
    const double slipCap = std::clamp(
        1.0 - 0.4 * SafeDivision(
            std::abs(input.SideSlipAngleRadians),
            parameters.MaximumSteerAtSlipRadians),
        0.45, 1.0);
    const double wheelAngle = rack * speedLimit *
        gripCap * slipCap;
    const auto angles = AckermannAngles(
        wheelAngle, parameters.WheelbaseCm,
        parameters.FrontTrackCm,
        parameters.AckermannFraction);
    out.RackInput = rack;
    out.LeftFrontRadians =
        angles[0] + parameters.FrontToeRadians;
    out.RightFrontRadians =
        angles[1] - parameters.FrontToeRadians;
    out.LeftRearRadians = parameters.RearToeRadians;
    out.RightRearRadians = -parameters.RearToeRadians;
    out.MeanFrontRadians = 0.5 *
        (out.LeftFrontRadians + out.RightFrontRadians);
    out.InnerWheelRadians =
        std::abs(angles[0]) >= std::abs(angles[1]) ?
        angles[0] : angles[1];
    out.OuterWheelRadians =
        std::abs(angles[0]) < std::abs(angles[1]) ?
        angles[0] : angles[1];
    out.KinematicYawRateRadiansSecond =
        SafeDivision(input.SpeedCmS *
            std::tan(wheelAngle),
            parameters.WheelbaseCm);
    out.EstimatedTurnRadiusCm =
        std::abs(wheelAngle) > PhysicsEpsilon ?
        std::abs(parameters.WheelbaseCm /
                 std::tan(wheelAngle)) :
        std::numeric_limits<double>::infinity();
    out.LateralAccelerationDemandG =
        SafeDivision(
            input.SpeedCmS *
            out.KinematicYawRateRadiansSecond,
            GravityCmS2);
    out.LateralGripUse = SafeDivision(
        std::abs(out.LateralAccelerationDemandG),
        std::max(0.1, input.ContactGripFraction));
    out.UndersteerCompensation =
        out.KinematicYawRateRadiansSecond -
        input.YawRateRadiansSecond;
    out.RestoringTorqueEstimateNm =
        -parameters.SteeringFeedbackStrength * rack *
        (1.0 + parameters.SelfCenteringSpeedFactor *
         input.SpeedCmS / 3000.0);
    out.MaximumAllowedRoadAngleRadians = speedLimit;
    out.AckermannQualified = true;
    out.LimitedBySpeed =
        speedLimit <
            parameters.MaximumRoadWheelAngleRadians * 0.99;
    out.LimitedByGrip =
        gripCap * slipCap < 0.98;
    out.SteeringAccepted = true;
    out.ModifiesRealPhysics = false;
    out.Valid = Finite(out.MeanFrontRadians) &&
        Finite(out.KinematicYawRateRadiansSecond) &&
        Finite(out.RestoringTorqueEstimateNm);
    return out;
}
class SteeringRackDynamics final
{
public:
    bool Configure(SteeringRackParameters settings,
                   SteeringResponseMode mode) noexcept
    {
        if (!ValidSteeringRackParameters(settings)) return false;
        Settings = settings;
        Mode = mode;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Rack = 0.0;
        Last = {};
        QualifiedFrames = 0;
        RejectedFrames = 0;
        MaximumDemandG = 0.0;
        SaturatedFrames = 0;
        AccumulatedSteerSeconds = 0.0;
    }
    SteeringWheelOutput Step(SteeringObservation input) noexcept
    {
        const auto output = EvaluateSteering(
            input, Settings, Rack, Mode);
        if (!output.Valid)
        {
            ++RejectedFrames;
            Last = {};
            return {};
        }
        Rack = output.RackInput;
        Last = output;
        ++QualifiedFrames;
        MaximumDemandG = std::max(
            MaximumDemandG,
            std::abs(output.LateralAccelerationDemandG));
        if (output.LimitedByGrip) ++SaturatedFrames;
        if (std::abs(Rack) > 0.05)
            AccumulatedSteerSeconds += input.DeltaSeconds;
        return Last;
    }
    SteeringWheelOutput Snapshot() const noexcept { return Last; }
    double RackPosition() const noexcept { return Rack; }
    std::uint64_t Frames() const noexcept { return QualifiedFrames; }
    std::uint64_t Rejects() const noexcept { return RejectedFrames; }
    std::uint64_t GripLimitedFrames() const noexcept
    {
        return SaturatedFrames;
    }
    double MaximumDemandedLateralG() const noexcept
    {
        return MaximumDemandG;
    }
private:
    SteeringRackParameters Settings{};
    SteeringResponseMode Mode = SteeringResponseMode::Simulation;
    SteeringWheelOutput Last{};
    double Rack = 0.0;
    double MaximumDemandG = 0.0;
    double AccumulatedSteerSeconds = 0.0;
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t SaturatedFrames = 0;
};
} // namespace latebrake::racecraft
