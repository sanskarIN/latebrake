#pragma once

#include "Vehicles/WheelBrakeProfile.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
struct WheelBrakeStep final
{
    double DeltaSeconds = 0.0;
    double RoadMetersPerSecond = 0.0;
    double NormalLoadNewtons = 0.0;
    double GripScale = 1.0;
    // Signed existing physical brake-force ceiling; already contact scaled.
    // The wheel model may only reduce this request, never add propulsion.
    double AuthorizedForceNewtons = 0.0;
    double BrakePressure = 0.0;
};

struct WheelBrakeStepOutput final
{
    bool Valid = false;
    double ForceNewtons = 0.0;
    double ModeledRadiansPerSecond = 0.0;
    double ModeledSlip = 0.0;
    double RotorEnergyJoules = 0.0;
    double RotorEnergyChangeJoules = 0.0;
    double ContactWorkJoules = 0.0; // force * prescribed road-point velocity * dt
    double BrakeWorkJoules = 0.0;
    double TireLossJoules = 0.0;
    double NumericalLossJoules = 0.0;
    unsigned SolverIterations = 0;
    bool StickingRotor = false;
};

// Contact-phase wheel-brake model driven by the actual chassis point speed.
// Its rotor is modeled, not a Chaos wheel joint or a measured wheel sensor.
// Non-braking/unsupported lifecycle transitions are handled by the caller.
class WheelBrakeDynamics final
{
public:
    static constexpr double MinimumStepSeconds = 1.0e-6;
    static constexpr double MaximumStepSeconds = 1.0 / 600.0;
    static constexpr unsigned RootIterations = 36;

    void Reset() noexcept
    {
        Omega = 0.0;
        Seeded = false;
    }

    bool SeedRolling(double roadMetersPerSecond,
                     const WheelBrakeProfile& p) noexcept
    {
        if (!IsValidWheelBrakeProfile(p) ||
            !std::isfinite(roadMetersPerSecond) ||
            std::abs(roadMetersPerSecond) > 1000.0)
        {
            Reset();
            return false;
        }
        // Explicit kinematic initialization of a NEW contact-phase model.
        // This is not newly measured rotor energy and creates no body force.
        Omega = roadMetersPerSecond / p.RadiusMeters;
        Seeded = true;
        return true;
    }

    bool HasState() const noexcept { return Seeded; }
    double ModeledAngularSpeed() const noexcept { return Omega; }

    static bool IsValidSample(const WheelBrakeStep& s) noexcept
    {
        return std::isfinite(s.DeltaSeconds) && s.DeltaSeconds >= MinimumStepSeconds &&
            s.DeltaSeconds <= MaximumStepSeconds * (1.0 + 1.0e-12) &&
            std::isfinite(s.RoadMetersPerSecond) &&
            std::abs(s.RoadMetersPerSecond) <= 1000.0 &&
            std::isfinite(s.NormalLoadNewtons) &&
            s.NormalLoadNewtons >= 0.0 && s.NormalLoadNewtons <= 1000000.0 &&
            std::isfinite(s.GripScale) && s.GripScale >= 0.05 && s.GripScale <= 4.0 &&
            std::isfinite(s.AuthorizedForceNewtons) &&
            std::abs(s.AuthorizedForceNewtons) <= 1000000.0 &&
            s.AuthorizedForceNewtons * s.RoadMetersPerSecond <= 0.0 &&
            (s.RoadMetersPerSecond != 0.0 || s.AuthorizedForceNewtons == 0.0) &&
            std::isfinite(s.BrakePressure) && s.BrakePressure >= 0.0 &&
            s.BrakePressure <= 1.0;
    }

    // Monotone brake-slip law. Denominator uses prescribed ROAD speed,
    // not wheel speed, so implicit rotor integration has a unique root even
    // when the rotor and body are travelling in opposite directions.
    static double TireForceAt(double omega, const WheelBrakeStep& s,
                              const WheelBrakeProfile& p) noexcept
    {
        const double slip = (omega * p.RadiusMeters - s.RoadMetersPerSecond) /
            std::max(std::abs(s.RoadMetersPerSecond), p.SlipSpeedFloorMetersPerSecond);
        const double raw = p.FrictionCoefficient * s.GripScale * s.NormalLoadNewtons *
            std::tanh(p.SlipStiffness * std::clamp(slip, -4.0, 4.0));
        return std::clamp(raw, std::min(0.0, s.AuthorizedForceNewtons),
                              std::max(0.0, s.AuthorizedForceNewtons));
    }

    WheelBrakeStepOutput Advance(const WheelBrakeStep& s,
                                 const WheelBrakeProfile& p) noexcept
    {
        if (!Seeded || !IsValidWheelBrakeProfile(p) || !IsValidSample(s))
        {
            Reset();
            return {};
        }
        const double old = Omega;
        const double inertiaRate = p.InertiaKgMetersSquared / s.DeltaSeconds;
        const double torque = p.MaximumBrakeTorqueNm * s.BrakePressure;
        const auto equationWithoutBrake = [&](double omega) noexcept
        {
            return inertiaRate * (omega - old) +
                p.RadiusMeters * TireForceAt(omega, s, p);
        };
        const double atRest = equationWithoutBrake(0.0);
        double next = 0.0;
        unsigned iterations = 0;
        bool sticking = std::abs(atRest) <= torque;
        if (!sticking)
        {
            // Static brake complementarity: either the rotor sticks at 0,
            // or solve I/dt*(w-new - w-old) + R*F(w-new) +/- B = 0.
            // Monotone F and positive I guarantee a unique bracketed root.
            const bool positive = atRest < -torque;
            double low = positive ? 0.0 :
                std::min({old, s.RoadMetersPerSecond / p.RadiusMeters, 0.0});
            double high = positive ?
                std::max({old, s.RoadMetersPerSecond / p.RadiusMeters, 0.0}) : 0.0;
            const double signedBrake = positive ? torque : -torque;
            for (; iterations < RootIterations; ++iterations)
            {
                const double middle = low + (high - low) * 0.5;
                const double residual = equationWithoutBrake(middle) + signedBrake;
                if (residual > 0.0) high = middle;
                else low = middle;
            }
            next = low + (high - low) * 0.5;
        }
        const double force = TireForceAt(next, s, p);
        const double slip = (next * p.RadiusMeters - s.RoadMetersPerSecond) /
            std::max(std::abs(s.RoadMetersPerSecond), p.SlipSpeedFloorMetersPerSecond);
        const double change = next - old;
        const double oldEnergy = 0.5 * p.InertiaKgMetersSquared * old * old;
        const double energy = 0.5 * p.InertiaKgMetersSquared * next * next;
        const double contactWork = s.DeltaSeconds * force * s.RoadMetersPerSecond;
        const double brakeWork = s.DeltaSeconds * torque * std::abs(next);
        const double tireLoss = s.DeltaSeconds * force *
            (next * p.RadiusMeters - s.RoadMetersPerSecond);
        const double numericalLoss = 0.5 * p.InertiaKgMetersSquared * change * change;
        // Root roundoff is bounded relative to physical work. Never accept
        // an energy-generating model step or return an over-budget force.
        const double tolerance = 1.0e-6 *
            (1.0 + oldEnergy + std::abs(contactWork) + brakeWork);
        if (!std::isfinite(next) || !std::isfinite(force) ||
            !std::isfinite(energy) || !std::isfinite(tireLoss) ||
            std::abs(next) > 10000.0 || tireLoss < -tolerance ||
            energy - oldEnergy + contactWork + brakeWork > tolerance ||
            std::abs(force) > std::abs(s.AuthorizedForceNewtons) + 1.0e-8)
        {
            Reset();
            return {};
        }
        Omega = next;
        return {true, force, next, std::clamp(slip, -4.0, 4.0), energy,
            energy - oldEnergy, contactWork, brakeWork,
            std::max(0.0, tireLoss), numericalLoss, iterations, sticking};
    }

private:
    double Omega = 0.0;
    bool Seeded = false;
};
} // namespace latebrake
