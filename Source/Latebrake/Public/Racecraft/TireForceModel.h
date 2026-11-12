#pragma once
#include "Racecraft/SurfaceMaterialModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace latebrake::racecraft
{
struct TireModelParameters final
{
    double CorneringStiffnessNPerRad = 48000.0;
    double LongitudinalStiffnessNPerSlip = 45000.0;
    double PeakLateralGrip = 1.20;
    double PeakLongitudinalGrip = 1.25;
    double SlipAngleAtPeakRadians = 0.18;
    double SlipRatioAtPeak = 0.12;
    double LoadSensitivityExponent = 0.92;
    double NominalNormalLoadN = 3400.0;
    double RelaxationLengthCm = 45.0;
    double CamberStiffnessNPerRad = 1500.0;
    double RollingResistanceFraction = 0.015;
    double AligningTorqueLeverCm = 6.0;
    double HeatGenerationPerJoule = 0.000015;
    double CoolingPerSecond = 0.028;
    double WearPerJoule = 0.000000001;
    double MinimumTractionSpeedCmS = 40.0;
    double MaximumWheelTorqueNm = 6000.0;
    double MaximumBrakeTorqueNm = 8000.0;
};
struct TireDynamicState final
{
    double FilteredSlipAngleRadians = 0.0;
    double FilteredSlipRatio = 0.0;
    double TemperatureC = 65.0;
    double WearFraction = 0.0;
    double LongitudinalForceN = 0.0;
    double LateralForceN = 0.0;
    double RelaxationSeconds = 0.0;
    double SlipPowerWatts = 0.0;
    double AccumulatedSlipEnergyJ = 0.0;
    double NormalizedTractionUse = 0.0;
    double ContactSeconds = 0.0;
    bool HasPhysicalContact = false;
    bool LockedWheel = false;
    bool WheelSpinning = false;
    bool Saturated = false;
};
struct TireForceInput final
{
    WheelObservation Wheel{};
    SurfaceContact Surface{};
    TireCompound Compound = TireCompound::Unknown;
    double WheelLongitudinalSpeedCmS = 0.0;
    double WheelLateralSpeedCmS = 0.0;
    double CamberRadians = 0.0;
    double DriveTorqueNm = 0.0;
    double BrakeTorqueNm = 0.0;
    double AmbientTemperatureC = 20.0;
    double DeltaSeconds = 0.0;
    bool OriginalWheelContactMeasured = false;
    bool IsBrakeApplying = false;
};
struct TireForceOutput final
{
    double LongitudinalForceN = 0.0;
    double LateralForceN = 0.0;
    double AligningMomentNm = 0.0;
    double RollingResistanceN = 0.0;
    double AvailableLongitudinalForceN = 0.0;
    double AvailableLateralForceN = 0.0;
    double CombinedTractionUse = 0.0;
    double RawSlipRatio = 0.0;
    double RawSlipAngleRadians = 0.0;
    double LongitudinalSlipPowerWatts = 0.0;
    double LateralSlipPowerWatts = 0.0;
    double WheelTorqueReactionNm = 0.0;
    double GeneratedHeatJ = 0.0;
    double ExpectedTreadWearDelta = 0.0;
    double ContactPatchLoadN = 0.0;
    double ForceDemandN = 0.0;
    double ContactConfidence = 0.0;
    double EquivalentFriction = 0.0;
    bool LockedWheel = false;
    bool WheelSpinning = false;
    bool TractionLimited = false;
    bool Grounded = false;
    bool OriginalPhysicsProven = false;
    bool Valid = false;
};
inline bool ValidTireParameters(TireModelParameters s) noexcept
{
    return Finite(s.CorneringStiffnessNPerRad) &&
        s.CorneringStiffnessNPerRad > 0.0 &&
        Finite(s.LongitudinalStiffnessNPerSlip) &&
        s.LongitudinalStiffnessNPerSlip > 0.0 &&
        Finite(s.PeakLateralGrip) && s.PeakLateralGrip > 0.0 &&
        Finite(s.PeakLongitudinalGrip) &&
        s.PeakLongitudinalGrip > 0.0 &&
        Finite(s.SlipAngleAtPeakRadians) &&
        s.SlipAngleAtPeakRadians > 0.0 &&
        Finite(s.SlipRatioAtPeak) && s.SlipRatioAtPeak > 0.0 &&
        Finite(s.LoadSensitivityExponent) &&
        s.LoadSensitivityExponent >= 0.5 &&
        s.LoadSensitivityExponent <= 1.5 &&
        Finite(s.NominalNormalLoadN) &&
        s.NominalNormalLoadN > 0.0 &&
        Finite(s.RelaxationLengthCm) &&
        s.RelaxationLengthCm > 0.0 &&
        Finite(s.CamberStiffnessNPerRad) &&
        s.CamberStiffnessNPerRad >= 0.0 &&
        Finite(s.RollingResistanceFraction) &&
        s.RollingResistanceFraction >= 0.0 &&
        Finite(s.AligningTorqueLeverCm) &&
        s.AligningTorqueLeverCm >= 0.0 &&
        Finite(s.HeatGenerationPerJoule) &&
        s.HeatGenerationPerJoule >= 0.0 &&
        Finite(s.CoolingPerSecond) &&
        s.CoolingPerSecond >= 0.0 &&
        Finite(s.WearPerJoule) &&
        s.WearPerJoule >= 0.0 &&
        Finite(s.MinimumTractionSpeedCmS) &&
        s.MinimumTractionSpeedCmS > 0.0 &&
        Finite(s.MaximumWheelTorqueNm) &&
        s.MaximumWheelTorqueNm > 0.0 &&
        Finite(s.MaximumBrakeTorqueNm) &&
        s.MaximumBrakeTorqueNm > 0.0;
}
inline double LongitudinalSlipRatio(
    double wheelAngularRadS, double radiusCm,
    double surfaceForwardCmS, double lowSpeedFloorCmS = 40.0) noexcept
{
    if (!Finite(wheelAngularRadS) || !Finite(radiusCm) ||
        !Finite(surfaceForwardCmS) || !Finite(lowSpeedFloorCmS) ||
        radiusCm <= 0.0 || lowSpeedFloorCmS <= 0.0)
        return 0.0;
    return std::clamp(
        (wheelAngularRadS * radiusCm -
         surfaceForwardCmS) /
        std::max({std::abs(surfaceForwardCmS),
                  std::abs(wheelAngularRadS * radiusCm),
                  lowSpeedFloorCmS}),
        -5.0, 5.0);
}
inline double TireSlipAngle(
    double longitudinalSpeedCmS,
    double lateralSpeedCmS,
    double steerAngleRadians) noexcept
{
    if (!Finite(longitudinalSpeedCmS) ||
        !Finite(lateralSpeedCmS) ||
        !Finite(steerAngleRadians)) return 0.0;
    if (std::hypot(longitudinalSpeedCmS,
                   lateralSpeedCmS) < 20.0) return 0.0;
    return WrappedAngle(std::atan2(
        lateralSpeedCmS,
        std::max(40.0, std::abs(longitudinalSpeedCmS))) -
        steerAngleRadians);
}
inline double TirePacejkaShape(
    double normalizedSlip, double stiffness = 8.0,
    double curvature = 0.85) noexcept
{
    if (!Finite(normalizedSlip) || !Finite(stiffness) ||
        !Finite(curvature) || stiffness <= 0.0 ||
        curvature < 0.0 || curvature > 1.0) return 0.0;
    const double bx = stiffness * normalizedSlip;
    return std::sin(1.9 * std::atan(
        bx - curvature * (bx - std::atan(bx))));
}
inline double ForceDropFromLoad(
    double currentLoadN, TireModelParameters s) noexcept
{
    if (!Finite(currentLoadN) || currentLoadN <= 0.0 ||
        !ValidTireParameters(s)) return 0.0;
    const double loadRatio =
        currentLoadN / s.NominalNormalLoadN;
    return std::clamp(std::pow(loadRatio,
        s.LoadSensitivityExponent - 1.0), 0.35, 1.5);
}
inline double TireEffectiveGrip(
    TireForceInput input, TireModelParameters parameters) noexcept
{
    if (!input.Surface.Valid || !ValidTireParameters(parameters))
        return 0.0;
    const double loadFactor =
        ForceDropFromLoad(input.Wheel.NormalLoadN, parameters);
    return std::clamp(
        input.Surface.Friction * loadFactor,
        0.01, 4.0);
}
inline TireForceOutput CalculateTireForce(
    TireForceInput input,
    TireDynamicState state,
    TireModelParameters parameters = {}) noexcept
{
    TireForceOutput out;
    if (!ValidTireParameters(parameters) ||
        !ValidWheelObservation(input.Wheel) ||
        !input.Surface.Valid ||
        !Finite(input.WheelLongitudinalSpeedCmS) ||
        !Finite(input.WheelLateralSpeedCmS) ||
        !Finite(input.CamberRadians) ||
        !Finite(input.DriveTorqueNm) ||
        !Finite(input.BrakeTorqueNm) ||
        !Finite(input.AmbientTemperatureC) ||
        !ValidFrameDelta(input.DeltaSeconds) ||
        !input.Wheel.ContactVerified ||
        !input.Wheel.PhysicalContact ||
        !input.OriginalWheelContactMeasured ||
        input.Wheel.NormalLoadN <= 0.0 ||
        input.Compound == TireCompound::Unknown ||
        std::abs(input.DriveTorqueNm) >
            parameters.MaximumWheelTorqueNm ||
        input.BrakeTorqueNm < 0.0 ||
        input.BrakeTorqueNm > parameters.MaximumBrakeTorqueNm)
        return out;
    const double slipRatio = LongitudinalSlipRatio(
        input.Wheel.AngularVelocityRadS,
        input.Wheel.RadiusCm,
        input.WheelLongitudinalSpeedCmS,
        parameters.MinimumTractionSpeedCmS);
    const double slipAngle = TireSlipAngle(
        input.WheelLongitudinalSpeedCmS,
        input.WheelLateralSpeedCmS,
        0.0);
    const double relaxationSeconds =
        parameters.RelaxationLengthCm /
        std::max(parameters.MinimumTractionSpeedCmS,
                 std::abs(input.WheelLongitudinalSpeedCmS));
    const double ratio = ExponentialResponse(
        state.FilteredSlipRatio, slipRatio,
        relaxationSeconds, input.DeltaSeconds);
    const double angle = ExponentialResponse(
        state.FilteredSlipAngleRadians, slipAngle,
        relaxationSeconds, input.DeltaSeconds);
    const double grip = TireEffectiveGrip(
        input, parameters);
    const double peakLong = grip *
        input.Wheel.NormalLoadN *
        parameters.PeakLongitudinalGrip;
    const double peakLat = grip *
        input.Wheel.NormalLoadN *
        parameters.PeakLateralGrip;
    const double normalizedRatio =
        ratio / parameters.SlipRatioAtPeak;
    const double normalizedAngle =
        angle / parameters.SlipAngleAtPeakRadians;
    const double desiredLong =
        peakLong * TirePacejkaShape(normalizedRatio);
    const double desiredLat = -
        peakLat * TirePacejkaShape(normalizedAngle) -
        parameters.CamberStiffnessNPerRad *
        input.CamberRadians;
    const double longDemand = SafeDivision(
        desiredLong, std::max(PhysicsEpsilon, peakLong));
    const double latDemand = SafeDivision(
        desiredLat, std::max(PhysicsEpsilon, peakLat));
    const double ellipse = std::hypot(longDemand, latDemand);
    const double scale = ellipse > 1.0 ?
        1.0 / ellipse : 1.0;
    out.LongitudinalForceN = desiredLong * scale;
    out.LateralForceN = desiredLat * scale;
    out.AligningMomentNm = -
        out.LateralForceN *
        parameters.AligningTorqueLeverCm /
        CentimetersPerMeter;
    out.RollingResistanceN =
        parameters.RollingResistanceFraction *
        input.Wheel.NormalLoadN *
        Sign(input.WheelLongitudinalSpeedCmS);
    out.AvailableLongitudinalForceN = peakLong;
    out.AvailableLateralForceN = peakLat;
    out.CombinedTractionUse = ellipse;
    out.RawSlipRatio = slipRatio;
    out.RawSlipAngleRadians = slipAngle;
    out.ContactPatchLoadN =
        input.Wheel.NormalLoadN;
    out.ForceDemandN =
        std::hypot(desiredLong, desiredLat);
    out.LongitudinalSlipPowerWatts =
        std::abs(out.LongitudinalForceN *
                 slipRatio *
                 input.WheelLongitudinalSpeedCmS /
                 CentimetersPerMeter);
    out.LateralSlipPowerWatts =
        std::abs(out.LateralForceN *
                 std::tan(std::clamp(
                     slipAngle, -1.0, 1.0)) *
                 input.WheelLongitudinalSpeedCmS /
                 CentimetersPerMeter);
    out.WheelTorqueReactionNm =
        out.LongitudinalForceN *
        input.Wheel.RadiusCm / CentimetersPerMeter;
    out.GeneratedHeatJ =
        (out.LongitudinalSlipPowerWatts +
         out.LateralSlipPowerWatts) * input.DeltaSeconds;
    out.ExpectedTreadWearDelta =
        parameters.WearPerJoule * out.GeneratedHeatJ;
    out.EquivalentFriction = grip;
    out.LockedWheel = ratio < -0.75 &&
        input.IsBrakeApplying;
    out.WheelSpinning = ratio > 0.30 &&
        input.DriveTorqueNm > 0.0;
    out.TractionLimited = ellipse >= 1.0;
    out.Grounded = true;
    out.OriginalPhysicsProven = true;
    out.ContactConfidence = 1.0;
    out.Valid = Finite(out.LongitudinalForceN) &&
        Finite(out.LateralForceN) &&
        Finite(out.AligningMomentNm) &&
        Finite(out.GeneratedHeatJ);
    return out;
}
class TireForceIntegrator final
{
public:
    bool Configure(TireModelParameters settings) noexcept
    {
        if (!ValidTireParameters(settings)) return false;
        Parameters = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        State = {};
        Last = {};
        Frames = 0;
        Rejected = 0;
        PeakGripUsage = 0.0;
        SlipEnergyJ = 0.0;
        BottomingEvents = 0;
    }
    TireForceOutput Step(TireForceInput input) noexcept
    {
        Last = CalculateTireForce(input, State, Parameters);
        if (!Last.Valid)
        {
            ++Rejected;
            State.HasPhysicalContact = false;
            return Last;
        }
        State.FilteredSlipRatio = ExponentialResponse(
            State.FilteredSlipRatio,
            Last.RawSlipRatio,
            Parameters.RelaxationLengthCm /
                std::max(Parameters.MinimumTractionSpeedCmS,
                         std::abs(input.WheelLongitudinalSpeedCmS)),
            input.DeltaSeconds);
        State.FilteredSlipAngleRadians = ExponentialResponse(
            State.FilteredSlipAngleRadians,
            Last.RawSlipAngleRadians,
            Parameters.RelaxationLengthCm /
                std::max(Parameters.MinimumTractionSpeedCmS,
                         std::abs(input.WheelLongitudinalSpeedCmS)),
            input.DeltaSeconds);
        const double energy = Last.GeneratedHeatJ;
        State.AccumulatedSlipEnergyJ += energy;
        State.TemperatureC = ExponentialResponse(
            State.TemperatureC + energy *
                Parameters.HeatGenerationPerJoule,
            input.AmbientTemperatureC,
            SafeDivision(1.0, Parameters.CoolingPerSecond,
                         1.0e9),
            input.DeltaSeconds);
        State.WearFraction = Clamp01(
            State.WearFraction +
            Last.ExpectedTreadWearDelta);
        State.LongitudinalForceN = Last.LongitudinalForceN;
        State.LateralForceN = Last.LateralForceN;
        State.RelaxationSeconds = Parameters.RelaxationLengthCm /
            std::max(Parameters.MinimumTractionSpeedCmS,
                     std::abs(input.WheelLongitudinalSpeedCmS));
        State.SlipPowerWatts =
            Last.LongitudinalSlipPowerWatts +
            Last.LateralSlipPowerWatts;
        State.NormalizedTractionUse =
            Last.CombinedTractionUse;
        State.ContactSeconds += input.DeltaSeconds;
        State.LockedWheel = Last.LockedWheel;
        State.WheelSpinning = Last.WheelSpinning;
        State.Saturated = Last.TractionLimited;
        State.HasPhysicalContact = true;
        PeakGripUsage = std::max(
            PeakGripUsage, Last.CombinedTractionUse);
        SlipEnergyJ += energy;
        ++Frames;
        return Last;
    }
    TireDynamicState Snapshot() const noexcept { return State; }
    TireForceOutput LastForces() const noexcept { return Last; }
    std::uint64_t QualifiedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    double PeakTractionDemand() const noexcept { return PeakGripUsage; }
    double TotalSlipEnergyJ() const noexcept { return SlipEnergyJ; }
private:
    TireModelParameters Parameters{};
    TireDynamicState State{};
    TireForceOutput Last{};
    std::uint64_t Frames = 0;
    std::uint64_t Rejected = 0;
    double PeakGripUsage = 0.0;
    double SlipEnergyJ = 0.0;
    std::uint64_t BottomingEvents = 0;
};
struct FourTireForces final
{
    std::array<TireForceOutput, PhysicalWheelCount> Wheels{};
    Vec2 TotalForceN{};
    double TotalYawMomentNm = 0.0;
    double TotalTractionDemand = 0.0;
    double TotalNormalLoadN = 0.0;
    double TotalSlipPowerWatts = 0.0;
    double GroundedFraction = 0.0;
    int ValidWheels = 0;
    bool CompletePhysicalContact = false;
    bool Valid = false;
};
inline FourTireForces SumFourTireForces(
    const std::array<TireForceOutput, PhysicalWheelCount>& forces,
    const ChassisObservation& chassis) noexcept
{
    FourTireForces out;
    out.Wheels = forces;
    if (!ValidChassisGeometry(chassis)) return out;
    for (std::size_t i = 0; i < forces.size(); ++i)
    {
        const TireForceOutput& tire = forces[i];
        if (!tire.Valid) continue;
        ++out.ValidWheels;
        const double xCm = i < 2 ?
            chassis.WheelbaseCm *
            (1.0 - chassis.FrontStaticLoadFraction) :
            -chassis.WheelbaseCm *
                chassis.FrontStaticLoadFraction;
        const double yCm = i % 2 == 0 ?
            -chassis.FrontTrackCm * 0.5 :
            chassis.FrontTrackCm * 0.5;
        out.TotalForceN = out.TotalForceN +
            Vec2{tire.LongitudinalForceN -
                     tire.RollingResistanceN,
                 tire.LateralForceN};
        out.TotalYawMomentNm +=
            (xCm * tire.LateralForceN -
             yCm * tire.LongitudinalForceN) /
            CentimetersPerMeter;
        out.TotalTractionDemand +=
            tire.CombinedTractionUse;
        out.TotalNormalLoadN += tire.ContactPatchLoadN;
        out.TotalSlipPowerWatts +=
            tire.LongitudinalSlipPowerWatts +
            tire.LateralSlipPowerWatts;
    }
    out.GroundedFraction = out.ValidWheels / 4.0;
    out.CompletePhysicalContact = out.ValidWheels == 4;
    out.Valid = out.ValidWheels > 0 &&
        Finite(out.TotalForceN) &&
        Finite(out.TotalYawMomentNm);
    return out;
}
} // namespace latebrake::racecraft
