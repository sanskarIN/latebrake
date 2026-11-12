#pragma once

#include "Vehicles/AxleTractionBudget.h"
#include "Vehicles/GripLimitedBraking.h"
#include "Vehicles/WheelBrakeProfile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake
{
struct ServiceBrakeCapabilitySample final
{
    double MassKg = 0.0;
    double GripScale = 1.0;
    double HeatScale = 1.0;
    bool Arcade = true;
    bool ModeledRotors = true;
    bool RoadSamplesValid = false;
    double ForwardCmPerSecond = 0.0;
    std::array<double, 4> RoadForwardCmPerSecond{};
    std::array<double, 4> NormalZ{};
    std::array<double, 4> ForwardProjection{};
};

struct BrakeCornerCapability final
{
    bool Loaded = false;
    double NormalEstimateNewtons = 0.0;
    double AdmittedForceNewtons = 0.0;
    double TireTargetForceNewtons = 0.0;
    double TorqueForceNewtons = 0.0;
    double PlanningForceNewtons = 0.0;
};

struct ServiceBrakeCapability final
{
    bool Valid = false;
    bool Supported = false;
    bool UsesModeledRotors = false;
    unsigned LoadedMask = 0;
    double NominalDecelerationCmPerSecondSquared = 0.0;
    double PlanningDecelerationCmPerSecondSquared = 0.0;
    double GradeAllowanceCmPerSecondSquared = 0.0;
    double EquivalentRotorMassKg = 0.0;
    std::array<BrakeCornerCapability, 4> Corners{};
};

// An advisory estimate from ONE accepted spring/road frame, not measured
// deceleration or a guarantee about the next road patch. Never apply this
// estimate as a force: the real service path still admits each force itself.
inline ServiceBrakeCapability EstimateServiceBrakeCapability(
    const ServiceBrakeCapabilitySample& s,
    const ChassisGroundContactOutput& ground,
    const SuspensionResponse& springs,
    const FourCornerBrakeProfile& profile) noexcept
{
    constexpr double PlanningReserve = 0.65;
    if (!CalculateAxleTractionBudget(ground, springs).Valid ||
        !IsValidFourCornerBrakeProfile(profile) || !s.RoadSamplesValid ||
        !std::isfinite(s.MassKg) || s.MassKg < 100.0 || s.MassKg > 15000.0 ||
        !std::isfinite(s.GripScale) || s.GripScale < 0.05 || s.GripScale > 4.0 ||
        !std::isfinite(s.ForwardCmPerSecond) || std::abs(s.ForwardCmPerSecond) > 15000.0 ||
        !std::isfinite(s.HeatScale) || s.HeatScale < 0.05 || s.HeatScale > 1.0)
        return {};
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!springs.Corners[i].Contact) continue;
        if (!std::isfinite(s.NormalZ[i]) || s.NormalZ[i] < SuspensionMinNormalZ ||
            s.NormalZ[i] > 1.0 || !std::isfinite(s.ForwardProjection[i]) ||
            !std::isfinite(s.RoadForwardCmPerSecond[i]) ||
            std::abs(s.RoadForwardCmPerSecond[i]) > 100000.0 ||
            s.ForwardProjection[i] < SuspensionMinNormalZ - 1.0e-3 ||
            s.ForwardProjection[i] > 1.0)
            return {};
    }
    ServiceBrakeCapability out;
    out.Valid = true;
    out.UsesModeledRotors = s.ModeledRotors;
    out.Supported = ground.HasDriveContact && springs.TotalUpForce > 0.0;
    const double nominal = (s.Arcade ? ArcadeServiceBrakeDeceleration :
        SimulationServiceBrakeDeceleration) * std::clamp(s.GripScale, 0.10, 2.0) * s.HeatScale;
    out.NominalDecelerationCmPerSecondSquared = nominal;
    if (!out.Supported) return out;
    const double admittedNewtons = s.MassKg * nominal *
        ground.ContactTractionScale * 0.01;
    double totalPlanarForce = 0.0;
    for (unsigned i = 0; i < 4; ++i)
    {
        const auto& spring = springs.Corners[i];
        if (!spring.Contact || spring.UpForce <= 0.0) continue;
        const auto& p = profile.Wheels[i];
        auto& corner = out.Corners[i];
        corner.Loaded = true;
        out.LoadedMask |= 1u << i;
        const double share = spring.UpForce / springs.TotalUpForce;
        corner.NormalEstimateNewtons = spring.UpForce * s.NormalZ[i] * 0.01;
        corner.AdmittedForceNewtons = admittedNewtons * share;
        if (std::abs(s.ForwardCmPerSecond) > 25.0)
        {
            const double local = s.RoadForwardCmPerSecond[i];
            corner.AdmittedForceNewtons *= local * s.ForwardCmPerSecond > 0.0
                ? std::min(1.0, std::abs(local / s.ForwardCmPerSecond)) : 0.0;
        }
        const double targetSlip = 0.5 * (p.AbsReleaseSlip + p.AbsReapplySlip);
        corner.TireTargetForceNewtons = p.FrictionCoefficient * s.GripScale *
            corner.NormalEstimateNewtons * std::tanh(p.SlipStiffness * targetSlip);
        corner.TorqueForceNewtons = p.MaximumBrakeTorqueNm * s.HeatScale / p.RadiusMeters;
        corner.PlanningForceNewtons = s.ModeledRotors
            ? std::min({corner.AdmittedForceNewtons, corner.TireTargetForceNewtons,
                        corner.TorqueForceNewtons})
            : corner.AdmittedForceNewtons;
        totalPlanarForce += corner.PlanningForceNewtons * s.ForwardProjection[i];
        if (s.ModeledRotors)
            out.EquivalentRotorMassKg += p.InertiaKgMetersSquared /
                (p.RadiusMeters * p.RadiusMeters);
        // The tangent has no lateral component. Its measured horizontal
        // projection therefore bounds |vertical| without inventing a new
        // road normal. Reserve the downhill sign, even for an uphill sample.
        const double vertical = std::sqrt(std::max(0.0,
            1.0 - s.ForwardProjection[i] * s.ForwardProjection[i]));
        out.GradeAllowanceCmPerSecondSquared += share *
            SuspensionGravityCmPerSecondSquared * vertical * s.ForwardProjection[i];
    }
    const double rotorFactor = 1.0 + out.EquivalentRotorMassKg / s.MassKg;
    out.PlanningDecelerationCmPerSecondSquared = std::clamp(
        PlanningReserve * totalPlanarForce * 100.0 / s.MassKg / rotorFactor -
            out.GradeAllowanceCmPerSecondSquared, 0.0, nominal);
    if (!std::isfinite(out.PlanningDecelerationCmPerSecondSquared)) return {};
    return out;
}

struct AcceptedBrakeCapability final
{
    bool Valid = false;
    std::uint64_t Sequence = 0;
    std::uint64_t TeleportRevision = 0;
    double FrameSeconds = 0.0;
    ServiceBrakeCapability Estimate{};
};

// Publish only after the shared pawn has accepted its complete drive frame.
// Invalidating a sample never reuses an identity after pause or teleport.
class ServiceBrakeCapabilityLedger final
{
public:
    void Invalidate() noexcept { Last = {}; }
    const AcceptedBrakeCapability& Read() const noexcept { return Last; }

    bool Publish(const ServiceBrakeCapability& estimate, double seconds,
                 std::uint64_t teleportRevision) noexcept
    {
        Invalidate();
        if (!IsConsistent(estimate) || !std::isfinite(seconds) || seconds <= 0.0 ||
            seconds > SuspensionMaximumFrameSeconds ||
            Sequence == std::numeric_limits<std::uint64_t>::max()) return false;
        Last = {true, ++Sequence, teleportRevision, seconds, estimate};
        return true;
    }

    static bool IsConsistent(const ServiceBrakeCapability& e) noexcept
    {
        if (!e.Valid || e.LoadedMask > 15u ||
            !std::isfinite(e.NominalDecelerationCmPerSecondSquared) ||
            e.NominalDecelerationCmPerSecondSquared < 0.0 ||
            e.NominalDecelerationCmPerSecondSquared > 5000.0 ||
            !std::isfinite(e.PlanningDecelerationCmPerSecondSquared) ||
            e.PlanningDecelerationCmPerSecondSquared < 0.0 ||
            e.PlanningDecelerationCmPerSecondSquared > e.NominalDecelerationCmPerSecondSquared ||
            !std::isfinite(e.GradeAllowanceCmPerSecondSquared) ||
            e.GradeAllowanceCmPerSecondSquared < 0.0 ||
            e.GradeAllowanceCmPerSecondSquared > SuspensionGravityCmPerSecondSquared ||
            !std::isfinite(e.EquivalentRotorMassKg) ||
            e.EquivalentRotorMassKg < 0.0 || e.EquivalentRotorMassKg > 8000.0)
            return false;
        unsigned mask = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const auto& c = e.Corners[i];
            for (double value : {c.NormalEstimateNewtons, c.AdmittedForceNewtons,
                    c.TorqueForceNewtons, c.PlanningForceNewtons})
                if (!std::isfinite(value) || value < 0.0 || value > 1000000.0) return false;
            if (!std::isfinite(c.TireTargetForceNewtons) || c.TireTargetForceNewtons < 0.0 ||
                c.TireTargetForceNewtons > 12000000.0) return false;
            if (c.Loaded) mask |= 1u << i;
            else if (c.NormalEstimateNewtons != 0.0 || c.AdmittedForceNewtons != 0.0 ||
                c.TireTargetForceNewtons != 0.0 || c.TorqueForceNewtons != 0.0 ||
                c.PlanningForceNewtons != 0.0) return false;
            if (c.PlanningForceNewtons > c.AdmittedForceNewtons + 1.0e-7 ||
                (e.UsesModeledRotors &&
                    (c.PlanningForceNewtons > c.TireTargetForceNewtons + 1.0e-7 ||
                     c.PlanningForceNewtons > c.TorqueForceNewtons + 1.0e-7))) return false;
        }
        if (mask != e.LoadedMask || (!e.Supported && (mask != 0u ||
            e.PlanningDecelerationCmPerSecondSquared != 0.0))) return false;
        return !e.Supported || mask != 0u;
    }

private:
    AcceptedBrakeCapability Last{};
    std::uint64_t Sequence = 0;
};
} // namespace latebrake
