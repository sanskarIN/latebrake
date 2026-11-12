#pragma once

#include "Vehicles/SprungContactBraking.h"
#include "Vehicles/WheelAbsController.h"
#include "Vehicles/WheelBrakeDynamics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
struct FourCornerWheelBrakeSample final
{
    double DeltaSeconds = 0.0;
    double MassKg = 0.0;
    double GripScale = 1.0;
    double ServiceHeatScale = 1.0;
    bool ServiceBrake = false;
    bool MechanicalHandbrake = false;
    bool PhysicsReady = false;
    std::array<double, 4> RoadSpeedCmPerSecond{};
    std::array<double, 4> RoadNormalZ{};
    std::array<double, 4> PlanarForwardScales{1.0, 1.0, 1.0, 1.0};
};

struct WheelBrakeReadout final
{
    bool Available = false;
    bool NewlySeeded = false;
    bool Modulating = false;
    bool StickingRotor = false;
    double ModeledRadiansPerSecond = 0.0;
    double ModeledRotationRadians = 0.0;
    double ModeledSlip = 0.0;
    double Pressure = 0.0;
    double EffectiveTorquePressure = 0.0;
    double NormalLoadNewtons = 0.0;
    double AverageForceNewtons = 0.0;
    double BrakeWorkJoules = 0.0;
    double TireLossJoules = 0.0;
    double NumericalLossJoules = 0.0;
    double RotorEnergyJoules = 0.0;
    WheelAbsPhase Phase = WheelAbsPhase::Inactive;
};

struct FourCornerWheelBrakeOutput final
{
    bool Valid = false;
    std::uint64_t Receipt = 0;
    SprungContactBrakeOutput Brakes{};
    std::array<WheelBrakeReadout, 4> Wheels{};
    unsigned InternalSteps = 0;
    unsigned SolverIterations = 0;
};

// Four independent contact-phase rotors, all bounded by the existing
// admitted chassis brake force. This is not a complete drive/wheel rig:
// motor coasting and airborne motion do not create inferred tire forces.
// Actual road speed/load are measured by the caller; rotor speed and ABS
// pressure are explicitly MODELED and must be identified that way in UI/QA.
class FourCornerWheelBrakes final
{
public:
    static constexpr double MaximumFrameSeconds = 0.10;
    static constexpr unsigned MaximumInternalSteps = 61;

    void Reset() noexcept
    {
        Committed = {};
        Pending = {};
        PendingOutput = {};
        Last = {};
        HasPending = false;
        // Never reuse a receipt after restart, pause or teleport.
    }

    const FourCornerWheelBrakeOutput& LastCommitted() const noexcept { return Last; }
    bool HasUnsettledProposal() const noexcept { return HasPending; }

    bool CommitDeliveredFrame(std::uint64_t receipt) noexcept
    {
        if (!HasPending || receipt == 0 || receipt != PendingOutput.Receipt)
            return false;
        Committed = Pending;
        Last = PendingOutput;
        HasPending = false;
        PendingOutput = {};
        return true;
    }

    bool RejectUndeliveredFrame(std::uint64_t receipt) noexcept
    {
        if (!HasPending || receipt == 0 || receipt != PendingOutput.Receipt)
            return false;
        HasPending = false;
        PendingOutput = {};
        Last = {};
        return true;
    }

    FourCornerWheelBrakeOutput Propose(
        const FourCornerWheelBrakeSample& s,
        const FourCornerBrakeProfile& profile,
        const ChassisGroundContactOutput& ground,
        const SuspensionResponse& springs,
        const SprungContactBrakeOutput& authorized) noexcept
    {
        // An orphan proposal never becomes next frame's rotor state.
        HasPending = false;
        PendingOutput = {};
        Last = {};
        if (!ValidateFrame(s, profile, ground, springs, authorized) ||
            Sequence == std::numeric_limits<std::uint64_t>::max())
        {
            Reset();
            return {};
        }
        FourCornerWheelBrakeOutput result;
        result.Valid = true;
        result.Brakes.Valid = true;
        if ((!s.ServiceBrake && !s.MechanicalHandbrake) || !ground.HasDriveContact)
        {
            Reset();
            return result;
        }
        Pending = Committed;
        if (!Pending.HasProfile || !(Pending.Profile.Wheels == profile.Wheels))
        {
            Pending = {};
            Pending.Profile = profile;
            Pending.HasProfile = true;
        }
        const unsigned steps = static_cast<unsigned>(std::ceil(
            s.DeltaSeconds / WheelBrakeDynamics::MaximumStepSeconds));
        if (steps == 0 || steps > MaximumInternalSteps)
        {
            Reset();
            return {};
        }
        result.InternalSteps = steps;
        const double dt = s.DeltaSeconds / static_cast<double>(steps);
        for (unsigned corner = 0; corner < 4; ++corner)
        {
            auto& rotor = Pending.Rotors[corner];
            auto& abs = Pending.Abs[corner];
            auto& readout = result.Wheels[corner];
            const double capNewtons = authorized.TangentForces[corner] * 0.01;
            const bool supported = springs.Corners[corner].Contact &&
                springs.Corners[corner].UpForce > 0.0;
            if (!supported || capNewtons == 0.0)
            {
                rotor.Reset();
                abs.Reset();
                continue;
            }
            const double speed = s.RoadSpeedCmPerSecond[corner] * 0.01;
            // Projection of the accepted world-up spring force onto the
            // measured road normal. Conservative normal-load estimate;
            // never divide by Nz to manufacture extra grip on a slope.
            const double normalLoad = springs.Corners[corner].UpForce *
                s.RoadNormalZ[corner] * 0.01;
            auto wheelProfile = profile.Wheels[corner];
            const bool parking = s.MechanicalHandbrake && corner >= 2;
            if (!rotor.HasState())
            {
                if (!rotor.SeedRolling(speed, wheelProfile))
                {
                    Reset();
                    return {};
                }
                readout.NewlySeeded = true;
            }
            const double targetSlip = 0.5 *
                (wheelProfile.AbsReleaseSlip + wheelProfile.AbsReapplySlip);
            const double targetForce = std::min(std::abs(capNewtons),
                wheelProfile.FrictionCoefficient * s.GripScale * normalLoad *
                    std::tanh(wheelProfile.SlipStiffness * targetSlip));
            const double requestedDeceleration =
                authorized.WholeBodyBudgetDeceleration * ground.ContactTractionScale * 0.01;
            const double heat = parking ? 1.0 : s.ServiceHeatScale;
            const double targetTorque = targetForce * wheelProfile.RadiusMeters +
                wheelProfile.InertiaKgMetersSquared * requestedDeceleration *
                    (1.0 - targetSlip) / wheelProfile.RadiusMeters;
            const double availablePressure = std::clamp(targetTorque /
                (wheelProfile.MaximumBrakeTorqueNm * heat), 0.0, 1.0);
            double impulse = 0.0;
            for (unsigned step = 0; step < steps; ++step)
            {
                const auto pressure = abs.Advance({dt, speed,
                    rotor.ModeledAngularSpeed(), true, s.ServiceBrake, parking, availablePressure}, wheelProfile);
                if (!pressure.Valid)
                {
                    Reset();
                    return {};
                }
                const auto dynamics = rotor.Advance({dt, speed, normalLoad,
                    s.GripScale, capNewtons,
                    pressure.Pressure * (parking ? 1.0 : s.ServiceHeatScale)}, wheelProfile);
                if (!dynamics.Valid)
                {
                    Reset();
                    return {};
                }
                impulse += dynamics.ForceNewtons * dt;
                readout.Available = true;
                readout.Modulating = readout.Modulating || pressure.Modulating;
                readout.StickingRotor = dynamics.StickingRotor;
                readout.ModeledRadiansPerSecond = dynamics.ModeledRadiansPerSecond;
                readout.ModeledRotationRadians += dynamics.ModeledRadiansPerSecond * dt;
                readout.ModeledSlip = dynamics.ModeledSlip;
                readout.Pressure = pressure.Pressure;
                readout.EffectiveTorquePressure =
                    pressure.Pressure * (parking ? 1.0 : s.ServiceHeatScale);
                readout.NormalLoadNewtons = normalLoad;
                readout.BrakeWorkJoules += dynamics.BrakeWorkJoules;
                readout.TireLossJoules += dynamics.TireLossJoules;
                readout.NumericalLossJoules += dynamics.NumericalLossJoules;
                readout.RotorEnergyJoules = dynamics.RotorEnergyJoules;
                readout.Phase = pressure.Phase;
                result.SolverIterations += dynamics.SolverIterations;
            }
            readout.AverageForceNewtons = std::clamp(impulse / s.DeltaSeconds,
                std::min(0.0, capNewtons), std::max(0.0, capNewtons));
            const double force = readout.AverageForceNewtons * 100.0;
            // The old admitted road-tangent response is a HARD upper bound.
            // Modeling slip can reduce it, never increase it or change sign.
            if (!std::isfinite(force) ||
                force * authorized.TangentForces[corner] < 0.0 ||
                std::abs(force) > std::abs(authorized.TangentForces[corner]) + 1.0e-6)
            {
                Reset();
                return {};
            }
            result.Brakes.TangentForces[corner] = force;
            result.Brakes.TotalBrakeForceMagnitude += std::abs(force);
            result.Brakes.TotalForwardForce += force * s.PlanarForwardScales[corner];
        }
        result.Brakes.WholeBodyBudgetDeceleration =
            result.Brakes.TotalBrakeForceMagnitude / s.MassKg / ground.ContactTractionScale;
        result.Receipt = ++Sequence;
        PendingOutput = result;
        HasPending = true;
        return result;
    }

private:
    struct State final
    {
        std::array<WheelBrakeDynamics, 4> Rotors{};
        std::array<WheelAbsController, 4> Abs{};
        FourCornerBrakeProfile Profile{};
        bool HasProfile = false;
    };

    static bool ValidateFrame(const FourCornerWheelBrakeSample& s,
                              const FourCornerBrakeProfile& profile,
                              const ChassisGroundContactOutput& ground,
                              const SuspensionResponse& springs,
                              const SprungContactBrakeOutput& authorized) noexcept
    {
        if (!s.PhysicsReady || !authorized.Valid ||
            !IsValidFourCornerBrakeProfile(profile) ||
            !CalculateAxleTractionBudget(ground, springs).Valid ||
            !std::isfinite(s.DeltaSeconds) ||
            s.DeltaSeconds < WheelBrakeDynamics::MinimumStepSeconds ||
            s.DeltaSeconds > MaximumFrameSeconds ||
            !std::isfinite(s.MassKg) || s.MassKg < 100.0 || s.MassKg > 15000.0 ||
            !std::isfinite(s.GripScale) || s.GripScale < 0.05 || s.GripScale > 4.0 ||
            !std::isfinite(s.ServiceHeatScale) ||
            s.ServiceHeatScale < 0.05 || s.ServiceHeatScale > 1.0 ||
            !std::isfinite(authorized.TotalForwardForce) ||
            !std::isfinite(authorized.TotalBrakeForceMagnitude) ||
            authorized.TotalBrakeForceMagnitude < 0.0 ||
            !std::isfinite(authorized.WholeBodyBudgetDeceleration) ||
            authorized.WholeBodyBudgetDeceleration < 0.0 ||
            authorized.WholeBodyBudgetDeceleration > 5000.0)
        {
            return false;
        }
        double magnitude = 0.0;
        double projection = 0.0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const double force = authorized.TangentForces[i];
            if (!std::isfinite(force)) return false;
            magnitude += std::abs(force);
            if (force == 0.0) continue;
            if (!springs.Corners[i].Contact || springs.Corners[i].UpForce <= 0.0 ||
                !std::isfinite(s.RoadSpeedCmPerSecond[i]) ||
                std::abs(s.RoadSpeedCmPerSecond[i]) > 100000.0 ||
                force * s.RoadSpeedCmPerSecond[i] >= 0.0 ||
                !std::isfinite(s.RoadNormalZ[i]) ||
                s.RoadNormalZ[i] < SuspensionMinNormalZ || s.RoadNormalZ[i] > 1.0 ||
                !std::isfinite(s.PlanarForwardScales[i]) ||
                s.PlanarForwardScales[i] < SuspensionMinNormalZ - 1.0e-3 ||
                s.PlanarForwardScales[i] > 1.0 ||
                (s.MechanicalHandbrake && !s.ServiceBrake && i < 2))
            {
                return false;
            }
            projection += force * s.PlanarForwardScales[i];
        }
        const double tolerance = 1.0e-6 + magnitude * 1.0e-10;
        if (std::abs(magnitude - authorized.TotalBrakeForceMagnitude) > tolerance ||
            std::abs(projection - authorized.TotalForwardForce) > tolerance ||
            magnitude > s.MassKg * 5000.0 * ground.ContactTractionScale + tolerance ||
            (!ground.HasDriveContact && magnitude > 0.0)) return false;
        const double expectedBudget = ground.ContactTractionScale > 0.0
            ? magnitude / s.MassKg / ground.ContactTractionScale : 0.0;
        return std::abs(expectedBudget - authorized.WholeBodyBudgetDeceleration) <= 1.0e-6;
    }

    State Committed{};
    State Pending{};
    FourCornerWheelBrakeOutput PendingOutput{};
    FourCornerWheelBrakeOutput Last{};
    bool HasPending = false;
    std::uint64_t Sequence = 0;
};
} // namespace latebrake
