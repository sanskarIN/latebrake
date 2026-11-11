#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
struct AeroParameters final
{
    double FrontalAreaM2 = 2.2;
    double DragCoefficient = 0.32;
    double LiftCoefficient = -0.28;
    double FrontDownforceFraction = 0.43;
    double AirDensityKgM3 = 1.225;
    double DownforceGroundEffectGain = 0.35;
    double GroundEffectOptimumRideHeightCm = 8.0;
    double GroundEffectCutoffHeightCm = 24.0;
    double MaximumGroundEffectMultiplier = 1.65;
    double MaximumFrontDownforceN = 15000.0;
    double MaximumRearDownforceN = 20000.0;
    double MaximumDragForceN = 15000.0;
    double BrakeCoolingCoefficient = 0.014;
    double EngineCoolingCoefficient = 0.020;
    double TireCoolingCoefficient = 0.009;
    double ActiveAeroDragIncrease = 0.30;
    double ActiveAeroDownforceIncrease = 0.45;
    double ActiveAeroDeploySeconds = 0.15;
    double ActiveAeroRetractSeconds = 0.12;
    double WingDamageGripReduction = 0.70;
    double WingDamageDragIncrease = 0.35;
    double RideHeightPitchSensitivity = 0.5;
    double RideHeightRollSensitivity = 0.2;
    double MaximumWindSpeedCmS = 15000.0;
    double MaximumQualifiedVehicleSpeedCmS = MaximumPhysicalSpeedCmS;
};
struct AeroObservation final
{
    Vec2 VehicleVelocityCmS{};
    Vec2 WindVelocityCmS{};
    double YawRadians = 0.0;
    double PitchRadians = 0.0;
    double RollRadians = 0.0;
    double RideHeightCm = 12.0;
    double BodyDamageFraction = 0.0;
    double FrontWingDamageFraction = 0.0;
    double RearWingDamageFraction = 0.0;
    double AirTemperatureC = 25.0;
    double AmbientAirDensityKgM3 = 1.225;
    double ActiveAeroDemand = 0.0;
    double DeltaSeconds = 0.0;
    bool OriginalVehicleObserved = false;
    bool AeroBodyPhysicallyPresent = false;
};
struct AeroForces final
{
    Vec2 DragForceWorldN{};
    Vec2 SideForceWorldN{};
    double LongitudinalDragN = 0.0;
    double LateralDragN = 0.0;
    double FrontDownforceN = 0.0;
    double RearDownforceN = 0.0;
    double TotalDownforceN = 0.0;
    double EffectiveDragCoefficient = 0.0;
    double EffectiveLiftCoefficient = 0.0;
    double GroundEffectMultiplier = 1.0;
    double ActiveAeroFraction = 0.0;
    double RelativeWindCmS = 0.0;
    double RelativeYawRadians = 0.0;
    double DynamicPressurePa = 0.0;
    double AerodynamicPowerLossKw = 0.0;
    double BrakeCoolingCoefficient = 0.0;
    double EngineCoolingCoefficient = 0.0;
    double TireCoolingCoefficient = 0.0;
    double AeroBalanceFrontFraction = 0.0;
    double DownforceVariationFromDamageN = 0.0;
    bool RideHeightUnderfloorRisk = false;
    bool FlowSeparated = false;
    bool AuthoritativeChaosForce = false;
    bool Valid = false;
};
inline bool ValidAeroParameters(AeroParameters s) noexcept
{
    return Finite(s.FrontalAreaM2) && s.FrontalAreaM2 > 0.0 &&
        s.FrontalAreaM2 < 15.0 &&
        Finite(s.DragCoefficient) &&
        s.DragCoefficient >= 0.0 &&
        s.DragCoefficient <= 4.0 &&
        Finite(s.LiftCoefficient) &&
        std::abs(s.LiftCoefficient) <= 5.0 &&
        Finite(s.FrontDownforceFraction) &&
        s.FrontDownforceFraction >= 0.0 &&
        s.FrontDownforceFraction <= 1.0 &&
        Finite(s.AirDensityKgM3) &&
        s.AirDensityKgM3 >= 0.1 &&
        s.AirDensityKgM3 <= 2.0 &&
        Finite(s.DownforceGroundEffectGain) &&
        s.DownforceGroundEffectGain >= 0.0 &&
        Finite(s.GroundEffectOptimumRideHeightCm) &&
        s.GroundEffectOptimumRideHeightCm >= 0.0 &&
        Finite(s.GroundEffectCutoffHeightCm) &&
        s.GroundEffectCutoffHeightCm >
            s.GroundEffectOptimumRideHeightCm &&
        Finite(s.MaximumGroundEffectMultiplier) &&
        s.MaximumGroundEffectMultiplier >= 1.0 &&
        Finite(s.MaximumFrontDownforceN) &&
        s.MaximumFrontDownforceN >= 0.0 &&
        Finite(s.MaximumRearDownforceN) &&
        s.MaximumRearDownforceN >= 0.0 &&
        Finite(s.MaximumDragForceN) &&
        s.MaximumDragForceN > 0.0 &&
        Finite(s.ActiveAeroDragIncrease) &&
        s.ActiveAeroDragIncrease >= 0.0 &&
        Finite(s.ActiveAeroDownforceIncrease) &&
        s.ActiveAeroDownforceIncrease >= 0.0 &&
        Finite(s.ActiveAeroDeploySeconds) &&
        s.ActiveAeroDeploySeconds > 0.0 &&
        Finite(s.ActiveAeroRetractSeconds) &&
        s.ActiveAeroRetractSeconds > 0.0 &&
        Finite(s.WingDamageGripReduction) &&
        s.WingDamageGripReduction >= 0.0 &&
        Finite(s.WingDamageDragIncrease) &&
        s.WingDamageDragIncrease >= 0.0;
}
inline bool ValidAeroObservation(
    AeroObservation s, AeroParameters parameters) noexcept
{
    return s.OriginalVehicleObserved &&
        s.AeroBodyPhysicallyPresent &&
        ValidFrameDelta(s.DeltaSeconds) &&
        Finite(s.VehicleVelocityCmS) &&
        Finite(s.WindVelocityCmS) &&
        Finite(s.YawRadians) &&
        Finite(s.PitchRadians) &&
        Finite(s.RollRadians) &&
        Finite(s.RideHeightCm) &&
        Finite(s.BodyDamageFraction) &&
        Finite(s.FrontWingDamageFraction) &&
        Finite(s.RearWingDamageFraction) &&
        Finite(s.AirTemperatureC) &&
        Finite(s.AmbientAirDensityKgM3) &&
        Finite(s.ActiveAeroDemand) &&
        Length(s.VehicleVelocityCmS) <=
            parameters.MaximumQualifiedVehicleSpeedCmS &&
        Length(s.WindVelocityCmS) <=
            parameters.MaximumWindSpeedCmS &&
        std::abs(s.PitchRadians) <= Pi * 0.5 &&
        std::abs(s.RollRadians) <= Pi * 0.5 &&
        s.RideHeightCm >= 0.0 &&
        s.RideHeightCm <= 100.0 &&
        s.BodyDamageFraction >= 0.0 &&
        s.BodyDamageFraction <= 1.0 &&
        s.FrontWingDamageFraction >= 0.0 &&
        s.FrontWingDamageFraction <= 1.0 &&
        s.RearWingDamageFraction >= 0.0 &&
        s.RearWingDamageFraction <= 1.0 &&
        s.AmbientAirDensityKgM3 > 0.0 &&
        s.AmbientAirDensityKgM3 <= 2.0 &&
        s.ActiveAeroDemand >= 0.0 &&
        s.ActiveAeroDemand <= 1.0;
}
inline double AeroGroundEffectFraction(
    double rideHeightCm, AeroParameters p) noexcept
{
    if (!Finite(rideHeightCm) || rideHeightCm < 0.0)
        return 0.0;
    const double lower = Smoothstep(
        0.0, p.GroundEffectOptimumRideHeightCm,
        rideHeightCm);
    const double upper = 1.0 - Smoothstep(
        p.GroundEffectOptimumRideHeightCm,
        p.GroundEffectCutoffHeightCm,
        rideHeightCm);
    return std::clamp(
        std::min(lower, upper), 0.0, 1.0);
}
inline AeroForces CalculateAerodynamicForces(
    AeroObservation sample,
    AeroParameters params,
    double activeAeroState) noexcept
{
    AeroForces out;
    if (!ValidAeroParameters(params) ||
        !ValidAeroObservation(sample, params) ||
        !Finite(activeAeroState) ||
        activeAeroState < 0.0 ||
        activeAeroState > 1.0) return out;
    const Vec2 relativeWind =
        sample.VehicleVelocityCmS - sample.WindVelocityCmS;
    const double speedCmS = Length(relativeWind);
    const double speedMS =
        speedCmS / CentimetersPerMeter;
    const double pressure = 0.5 *
        sample.AmbientAirDensityKgM3 * speedMS * speedMS;
    const double yaw = WrappedAngle(
        AngleOf(relativeWind) - sample.YawRadians);
    const double sideRatio =
        std::clamp(std::abs(std::sin(yaw)), 0.0, 1.0);
    const double rideCorrection =
        sample.RideHeightCm +
        std::abs(sample.PitchRadians) *
            params.RideHeightPitchSensitivity * 100.0 +
        std::abs(sample.RollRadians) *
            params.RideHeightRollSensitivity * 100.0;
    const double groundEffect =
        1.0 + params.DownforceGroundEffectGain *
        AeroGroundEffectFraction(rideCorrection, params);
    const double frontDamage =
        1.0 - params.WingDamageGripReduction *
        sample.FrontWingDamageFraction;
    const double rearDamage =
        1.0 - params.WingDamageGripReduction *
        sample.RearWingDamageFraction;
    const double bodyDrag =
        1.0 + params.WingDamageDragIncrease *
        sample.BodyDamageFraction;
    const double activeDrag =
        1.0 + params.ActiveAeroDragIncrease *
        activeAeroState;
    const double activeLift =
        1.0 + params.ActiveAeroDownforceIncrease *
        activeAeroState;
    const double effectiveCd =
        params.DragCoefficient * bodyDrag *
        activeDrag * (1.0 + 0.55 * sideRatio);
    const double rawDrag = pressure *
        params.FrontalAreaM2 * effectiveCd;
    const double drag = std::min(
        params.MaximumDragForceN, rawDrag);
    const double baseDownforce = std::max(
        0.0, -params.LiftCoefficient *
        pressure * params.FrontalAreaM2);
    out.FrontDownforceN = std::min(
        params.MaximumFrontDownforceN,
        baseDownforce * groundEffect * activeLift *
        params.FrontDownforceFraction * frontDamage);
    out.RearDownforceN = std::min(
        params.MaximumRearDownforceN,
        baseDownforce * groundEffect * activeLift *
        (1.0 - params.FrontDownforceFraction) * rearDamage);
    out.TotalDownforceN =
        out.FrontDownforceN + out.RearDownforceN;
    out.DragForceWorldN =
        Normalized(relativeWind) * (-drag);
    out.SideForceWorldN =
        PerpendicularLeft(HeadingVector(sample.YawRadians)) *
        (-0.18 * pressure * params.FrontalAreaM2 *
         std::sin(yaw));
    out.LongitudinalDragN = drag * std::cos(yaw);
    out.LateralDragN = drag * std::sin(yaw);
    out.EffectiveDragCoefficient = effectiveCd;
    out.EffectiveLiftCoefficient =
        params.LiftCoefficient * groundEffect * activeLift;
    out.GroundEffectMultiplier = groundEffect;
    out.ActiveAeroFraction = activeAeroState;
    out.RelativeWindCmS = speedCmS;
    out.RelativeYawRadians = yaw;
    out.DynamicPressurePa = pressure;
    out.AerodynamicPowerLossKw =
        drag * speedMS / 1000.0;
    out.BrakeCoolingCoefficient =
        params.BrakeCoolingCoefficient * speedMS;
    out.EngineCoolingCoefficient =
        params.EngineCoolingCoefficient * speedMS;
    out.TireCoolingCoefficient =
        params.TireCoolingCoefficient * speedMS;
    out.AeroBalanceFrontFraction = SafeDivision(
        out.FrontDownforceN, out.TotalDownforceN);
    out.DownforceVariationFromDamageN =
        baseDownforce * (1.0 -
            (params.FrontDownforceFraction * frontDamage +
             (1.0 - params.FrontDownforceFraction) * rearDamage));
    out.RideHeightUnderfloorRisk =
        sample.RideHeightCm <=
            0.5 * params.GroundEffectOptimumRideHeightCm;
    out.FlowSeparated = sideRatio > 0.65;
    out.AuthoritativeChaosForce = false;
    out.Valid = Finite(out.DragForceWorldN) &&
        Finite(out.TotalDownforceN) &&
        Finite(out.AerodynamicPowerLossKw);
    return out;
}
class ActiveAerodynamicController final
{
public:
    bool Configure(AeroParameters p) noexcept
    {
        if (!ValidAeroParameters(p)) return false;
        Parameters = p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        ActiveFraction = 0.0;
        Last = {};
        Frames = 0;
        Rejected = 0;
        HighDownforceSeconds = 0.0;
        PeakDragN = 0.0;
        PeakDownforceN = 0.0;
    }
    AeroForces Step(AeroObservation input) noexcept
    {
        if (!ValidAeroObservation(input, Parameters))
        {
            ++Rejected;
            Last = {};
            return {};
        }
        const double rate =
            input.ActiveAeroDemand > ActiveFraction ?
            Parameters.ActiveAeroDeploySeconds :
            Parameters.ActiveAeroRetractSeconds;
        ActiveFraction = ExponentialResponse(
            ActiveFraction, input.ActiveAeroDemand,
            rate, input.DeltaSeconds);
        Last = CalculateAerodynamicForces(
            input, Parameters, ActiveFraction);
        if (!Last.Valid)
        {
            ++Rejected;
            return Last;
        }
        ++Frames;
        if (Last.ActiveAeroFraction >= 0.5)
            HighDownforceSeconds += input.DeltaSeconds;
        PeakDragN = std::max(
            PeakDragN, Length(Last.DragForceWorldN));
        PeakDownforceN = std::max(
            PeakDownforceN, Last.TotalDownforceN);
        return Last;
    }
    AeroForces Snapshot() const noexcept { return Last; }
    std::uint64_t QualifiedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    double PeakDownforceNewtons() const noexcept
    {
        return PeakDownforceN;
    }
    double PeakDragNewtons() const noexcept { return PeakDragN; }
private:
    AeroParameters Parameters{};
    AeroForces Last{};
    double ActiveFraction = 0.0;
    double HighDownforceSeconds = 0.0;
    double PeakDragN = 0.0;
    double PeakDownforceN = 0.0;
    std::uint64_t Frames = 0;
    std::uint64_t Rejected = 0;
};
} // namespace latebrake::racecraft
