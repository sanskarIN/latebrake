#pragma once
#include "Racecraft/TireForceModel.h"
#include "Racecraft/VehicleAerodynamics.h"
#include "Racecraft/ChassisLoadTransfer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct PlanarDynamicsParameters final
{
    double MassKg = 1320.0;
    double YawInertiaKgM2 = 1980.0;
    double WheelbaseCm = 268.0;
    double CenterOfMassFrontFraction = 0.53;
    double FrontTrackCm = 160.0;
    double RearTrackCm = 158.0;
    double MaximumPlanarAccelerationG = 4.0;
    double MaximumYawAccelerationRadS2 = 40.0;
    double MaximumYawRateRadS = 8.0;
    double MaximumSimulationSpeedCmS = MaximumPhysicalSpeedCmS;
    double LinearDragCoefficient = 0.015;
    double RotationalDragCoefficient = 0.24;
    double RoadGradientSupportRadians = 0.45;
    double MaximumAllowedStepSeconds = 0.03;
    int MaximumIntegrationSubsteps = 16;
    double SteeringReactionSeconds = 0.04;
    double MaximumCrossTrackSpeedCmS = 8000.0;
    double LowSpeedLateralDamping = 0.1;
    bool RequireFourContactWheels = true;
    bool RejectOffPavement = true;
    bool UseSemiImplicitIntegration = true;
};
struct PlanarDynamicsState final
{
    Vec2 WorldPositionCm{};
    Vec2 WorldVelocityCmS{};
    double HeadingRadians = 0.0;
    double YawRateRadS = 0.0;
    double LongitudinalSpeedCmS = 0.0;
    double LateralSpeedCmS = 0.0;
    double LongitudinalAccelerationCmS2 = 0.0;
    double LateralAccelerationCmS2 = 0.0;
    double IntegratedDistanceCm = 0.0;
    double IntegratedYawRadians = 0.0;
    double KineticEnergyJ = 0.0;
    double TimeSeconds = 0.0;
    bool BodySupported = false;
    bool OriginalSourceState = false;
    bool Valid = false;
};
struct PlanarDynamicsInput final
{
    PhysicalVehicleFrame Source{};
    std::array<TireForceOutput, 4> Tires{};
    AeroForces Aero{};
    WheelLoadDistribution Loads{};
    double EngineBrakeForceN = 0.0;
    double AdditionalLongitudinalForceN = 0.0;
    double AdditionalYawMomentNm = 0.0;
    double MeasuredGradeRadians = 0.0;
    double DeltaSeconds = 0.0;
    bool OriginalWheelForcesQualified = false;
    bool OriginalChaosBodyQualified = false;
};
struct PlanarDynamicsOutput final
{
    PlanarDynamicsState State{};
    Vec2 NetWorldForceN{};
    Vec2 NetBodyForceN{};
    double NetYawMomentNm = 0.0;
    double LongitudinalForceN = 0.0;
    double LateralForceN = 0.0;
    double ForceMagnitudeN = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double LateralAccelerationG = 0.0;
    double YawAccelerationRadS2 = 0.0;
    double EffectiveDragForceN = 0.0;
    double GroundFrictionDemand = 0.0;
    double IntegrationErrorCm = 0.0;
    double IntegrationErrorRadians = 0.0;
    double SourcePositionDifferenceCm = 0.0;
    double SourceHeadingDifferenceRadians = 0.0;
    int IntegrationSubsteps = 0;
    bool SpeedCapApplied = false;
    bool YawCapApplied = false;
    bool BodyContactsQualified = false;
    bool AppliedForceToChaos = false;
    bool Valid = false;
};
inline bool ValidPlanarDynamicsParameters(
    PlanarDynamicsParameters s) noexcept
{
    return Finite(s.MassKg) && s.MassKg >= 200.0 &&
        s.MassKg <= 15000.0 &&
        Finite(s.YawInertiaKgM2) &&
        s.YawInertiaKgM2 > 0.0 &&
        Finite(s.WheelbaseCm) &&
        s.WheelbaseCm > 100.0 &&
        Finite(s.CenterOfMassFrontFraction) &&
        s.CenterOfMassFrontFraction > 0.1 &&
        s.CenterOfMassFrontFraction < 0.9 &&
        Finite(s.FrontTrackCm) &&
        s.FrontTrackCm > 70.0 &&
        Finite(s.RearTrackCm) &&
        s.RearTrackCm > 70.0 &&
        Finite(s.MaximumPlanarAccelerationG) &&
        s.MaximumPlanarAccelerationG > 0.0 &&
        Finite(s.MaximumYawAccelerationRadS2) &&
        s.MaximumYawAccelerationRadS2 > 0.0 &&
        Finite(s.MaximumYawRateRadS) &&
        s.MaximumYawRateRadS > 0.0 &&
        Finite(s.MaximumSimulationSpeedCmS) &&
        s.MaximumSimulationSpeedCmS > 0.0 &&
        Finite(s.LinearDragCoefficient) &&
        s.LinearDragCoefficient >= 0.0 &&
        Finite(s.RotationalDragCoefficient) &&
        s.RotationalDragCoefficient >= 0.0 &&
        Finite(s.MaximumAllowedStepSeconds) &&
        s.MaximumAllowedStepSeconds > 0.0 &&
        s.MaximumAllowedStepSeconds <=
            MaxSimulationFrameSeconds &&
        s.MaximumIntegrationSubsteps >= 1 &&
        s.MaximumIntegrationSubsteps <= 64;
}
inline bool ValidPlanarDynamicsInput(
    const PlanarDynamicsInput& s,
    PlanarDynamicsParameters settings) noexcept
{
    if (ValidateVehicleFrame(
            s.Source, settings.RequireFourContactWheels) !=
            SnapshotFault::None ||
        !s.OriginalWheelForcesQualified ||
        !s.OriginalChaosBodyQualified ||
        !s.Loads.Valid ||
        !s.Aero.Valid ||
        !ValidFrameDelta(s.DeltaSeconds) ||
        !Finite(s.EngineBrakeForceN) ||
        !Finite(s.AdditionalLongitudinalForceN) ||
        !Finite(s.AdditionalYawMomentNm) ||
        !Finite(s.MeasuredGradeRadians) ||
        std::abs(s.MeasuredGradeRadians) >
            settings.RoadGradientSupportRadians ||
        (settings.RejectOffPavement &&
         !s.Source.Track.OnOriginalPavement))
        return false;
    for (TireForceOutput tire : s.Tires)
    {
        if (!tire.Valid ||
            !Finite(tire.LongitudinalForceN) ||
            !Finite(tire.LateralForceN))
            return false;
    }
    return true;
}
inline Vec2 SumTireBodyForces(
    const std::array<TireForceOutput, 4>& wheels) noexcept
{
    Vec2 total{};
    for (TireForceOutput wheel : wheels)
    {
        total = total + Vec2{
            wheel.LongitudinalForceN -
                wheel.RollingResistanceN,
            wheel.LateralForceN
        };
    }
    return total;
}
inline double WheelMomentAboutYaw(
    const std::array<TireForceOutput, 4>& tires,
    PlanarDynamicsParameters settings) noexcept
{
    double moment = 0.0;
    for (std::size_t i = 0; i < 4; ++i)
    {
        const double axle = i < 2 ?
            settings.WheelbaseCm *
                (1.0 - settings.CenterOfMassFrontFraction) :
            -settings.WheelbaseCm *
                settings.CenterOfMassFrontFraction;
        const double halfTrack = i < 2 ?
            settings.FrontTrackCm * 0.5 :
            settings.RearTrackCm * 0.5;
        const double offsetY = i % 2 == 0 ?
            -halfTrack : halfTrack;
        moment += (
            axle * tires[i].LateralForceN -
            offsetY * tires[i].LongitudinalForceN) /
            CentimetersPerMeter;
    }
    return moment;
}
inline PlanarDynamicsState BeginFromObservedChassis(
    PhysicalVehicleFrame source,
    double timeSeconds) noexcept
{
    PlanarDynamicsState state;
    if (ValidateVehicleFrame(source) != SnapshotFault::None ||
        !Finite(timeSeconds) || timeSeconds < 0.0)
        return state;
    state.WorldPositionCm =
        source.Chassis.PlanarPose.Position;
    state.WorldVelocityCmS = {
        source.Chassis.VelocityCmS.X,
        source.Chassis.VelocityCmS.Y
    };
    state.HeadingRadians =
        source.Chassis.PlanarPose.HeadingRadians;
    state.YawRateRadS =
        source.Chassis.AngularVelocityRadS.Z;
    state.LongitudinalSpeedCmS =
        ForwardSpeed(state.WorldVelocityCmS,
                     state.HeadingRadians);
    state.LateralSpeedCmS =
        SideSpeed(state.WorldVelocityCmS,
                  state.HeadingRadians);
    state.KineticEnergyJ = KineticEnergyJ(
        source.Chassis.MassKg,
        Length(state.WorldVelocityCmS));
    state.TimeSeconds = timeSeconds;
    state.BodySupported = source.ChassisOnWorldStatic;
    state.OriginalSourceState = true;
    state.Valid = true;
    return state;
}
inline PlanarDynamicsOutput IntegratePlanarMotion(
    PlanarDynamicsState initial,
    const PlanarDynamicsInput& input,
    PlanarDynamicsParameters parameters) noexcept
{
    PlanarDynamicsOutput out;
    if (!ValidPlanarDynamicsParameters(parameters) ||
        !initial.Valid ||
        !ValidPlanarDynamicsInput(input, parameters))
        return out;
    out.NetBodyForceN = SumTireBodyForces(input.Tires);
    out.NetBodyForceN.X +=
        input.EngineBrakeForceN +
        input.AdditionalLongitudinalForceN -
        parameters.MassKg *
        GravityCmS2 / CentimetersPerMeter *
        std::sin(input.MeasuredGradeRadians);
    out.NetWorldForceN =
        Rotate(out.NetBodyForceN,
               initial.HeadingRadians) +
        input.Aero.DragForceWorldN +
        input.Aero.SideForceWorldN -
        initial.WorldVelocityCmS *
            parameters.LinearDragCoefficient;
    out.NetYawMomentNm =
        WheelMomentAboutYaw(input.Tires, parameters) +
        input.AdditionalYawMomentNm -
        initial.YawRateRadS *
            parameters.RotationalDragCoefficient;
    out.LongitudinalForceN = out.NetBodyForceN.X;
    out.LateralForceN = out.NetBodyForceN.Y;
    out.ForceMagnitudeN = Length(out.NetWorldForceN);
    const double maximumForceN =
        parameters.MassKg *
        GravityCmS2 / CentimetersPerMeter *
        parameters.MaximumPlanarAccelerationG;
    if (out.ForceMagnitudeN > maximumForceN)
    {
        out.NetWorldForceN = out.NetWorldForceN *
            (maximumForceN / out.ForceMagnitudeN);
        out.ForceMagnitudeN = maximumForceN;
    }
    out.YawAccelerationRadS2 = std::clamp(
        SafeDivision(out.NetYawMomentNm,
                     parameters.YawInertiaKgM2),
        -parameters.MaximumYawAccelerationRadS2,
        parameters.MaximumYawAccelerationRadS2);
    const Vec2 accelerationCmS2 =
        out.NetWorldForceN *
        (CentimetersPerMeter / parameters.MassKg);
    out.LongitudinalAccelerationG = SafeDivision(
        Dot(accelerationCmS2,
            HeadingVector(initial.HeadingRadians)),
        GravityCmS2);
    out.LateralAccelerationG = SafeDivision(
        Cross(HeadingVector(initial.HeadingRadians),
              accelerationCmS2),
        GravityCmS2);
    const int steps = std::clamp(
        static_cast<int>(std::ceil(
            input.DeltaSeconds /
            parameters.MaximumAllowedStepSeconds)),
        1, parameters.MaximumIntegrationSubsteps);
    const double dt =
        input.DeltaSeconds / static_cast<double>(steps);
    PlanarDynamicsState state = initial;
    for (int i = 0; i < steps; ++i)
    {
        const Vec2 priorVelocity =
            state.WorldVelocityCmS;
        state.WorldVelocityCmS =
            state.WorldVelocityCmS +
            accelerationCmS2 * dt;
        const double speed =
            Length(state.WorldVelocityCmS);
        if (speed > parameters.MaximumSimulationSpeedCmS)
        {
            state.WorldVelocityCmS =
                state.WorldVelocityCmS *
                (parameters.MaximumSimulationSpeedCmS / speed);
            out.SpeedCapApplied = true;
        }
        state.YawRateRadS +=
            out.YawAccelerationRadS2 * dt;
        if (std::abs(state.YawRateRadS) >
            parameters.MaximumYawRateRadS)
        {
            state.YawRateRadS = std::copysign(
                parameters.MaximumYawRateRadS,
                state.YawRateRadS);
            out.YawCapApplied = true;
        }
        const Vec2 effectiveVelocity =
            parameters.UseSemiImplicitIntegration ?
            state.WorldVelocityCmS :
            (priorVelocity + state.WorldVelocityCmS) * 0.5;
        state.WorldPositionCm =
            state.WorldPositionCm +
            effectiveVelocity * dt;
        state.HeadingRadians = WrappedAngle(
            state.HeadingRadians + state.YawRateRadS * dt);
        state.IntegratedDistanceCm +=
            Length(effectiveVelocity) * dt;
        state.IntegratedYawRadians +=
            state.YawRateRadS * dt;
        state.TimeSeconds += dt;
    }
    state.LongitudinalSpeedCmS =
        ForwardSpeed(state.WorldVelocityCmS,
                     state.HeadingRadians);
    state.LateralSpeedCmS =
        SideSpeed(state.WorldVelocityCmS,
                  state.HeadingRadians);
    state.LongitudinalAccelerationCmS2 =
        Dot(accelerationCmS2,
            HeadingVector(state.HeadingRadians));
    state.LateralAccelerationCmS2 =
        Cross(HeadingVector(state.HeadingRadians),
              accelerationCmS2);
    state.KineticEnergyJ =
        KineticEnergyJ(parameters.MassKg,
                       Length(state.WorldVelocityCmS));
    state.OriginalSourceState = false;
    state.Valid = Finite(state.WorldPositionCm) &&
        Finite(state.WorldVelocityCmS) &&
        Finite(state.HeadingRadians) &&
        Finite(state.YawRateRadS) &&
        Finite(state.KineticEnergyJ);
    out.State = state;
    out.IntegrationSubsteps = steps;
    out.EffectiveDragForceN =
        Length(input.Aero.DragForceWorldN);
    out.GroundFrictionDemand =
        SafeDivision(out.ForceMagnitudeN,
                     input.Loads.TotalContactLoadN);
    out.SourcePositionDifferenceCm = Distance(
        state.WorldPositionCm,
        input.Source.Chassis.PlanarPose.Position);
    out.SourceHeadingDifferenceRadians = std::abs(
        AngleDifference(state.HeadingRadians,
                        input.Source.Chassis.PlanarPose.HeadingRadians));
    out.IntegrationErrorCm = out.SourcePositionDifferenceCm;
    out.IntegrationErrorRadians =
        out.SourceHeadingDifferenceRadians;
    out.BodyContactsQualified = true;
    out.AppliedForceToChaos = false;
    out.Valid = state.Valid &&
        Finite(out.GroundFrictionDemand);
    return out;
}
class PlanarVehiclePredictor final
{
public:
    bool Configure(PlanarDynamicsParameters settings) noexcept
    {
        if (!ValidPlanarDynamicsParameters(settings))
            return false;
        Settings = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        State = {};
        Last = {};
        HasState = false;
        Accepted = 0;
        Rejected = 0;
        TotalPredictedDistanceCm = 0.0;
        MaximumIntegrationErrorCm = 0.0;
    }
    bool Begin(const PhysicalVehicleFrame& actor) noexcept
    {
        State = BeginFromObservedChassis(actor, actor.TimeSeconds);
        HasState = State.Valid;
        return HasState;
    }
    PlanarDynamicsOutput Step(const PlanarDynamicsInput& input) noexcept
    {
        if (!HasState)
        {
            ++Rejected;
            return {};
        }
        const auto next =
            IntegratePlanarMotion(State, input, Settings);
        if (!next.Valid)
        {
            ++Rejected;
            return {};
        }
        Last = next;
        State = next.State;
        ++Accepted;
        TotalPredictedDistanceCm =
            State.IntegratedDistanceCm;
        MaximumIntegrationErrorCm = std::max(
            MaximumIntegrationErrorCm,
            next.IntegrationErrorCm);
        return Last;
    }
    PlanarDynamicsState Snapshot() const noexcept { return State; }
    PlanarDynamicsOutput LastPrediction() const noexcept { return Last; }
    std::uint64_t SuccessfulSteps() const noexcept { return Accepted; }
    std::uint64_t RejectedSteps() const noexcept { return Rejected; }
    double PredictedDistanceCm() const noexcept
    {
        return TotalPredictedDistanceCm;
    }
private:
    PlanarDynamicsParameters Settings{};
    PlanarDynamicsState State{};
    PlanarDynamicsOutput Last{};
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    double TotalPredictedDistanceCm = 0.0;
    double MaximumIntegrationErrorCm = 0.0;
    bool HasState = false;
};
} // namespace latebrake::racecraft
