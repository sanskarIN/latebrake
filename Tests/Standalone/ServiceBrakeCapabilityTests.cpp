#include "Vehicles/ServiceBrakeCapability.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool value, const char* why)
{
    if (!value) { std::cerr << "ServiceBrakeCapability FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
latebrake::SuspensionResponse Springs(double mass, std::array<double, 4> loads)
{
    latebrake::SuspensionResponse r;
    r.Valid = true;
    r.WeightForce = mass * 980.0;
    for (unsigned i = 0; i < 4; ++i)
    {
        auto& c = r.Corners[i];
        c.Contact = loads[i] > 0.0;
        c.UpForce = loads[i] * r.WeightForce;
        r.TotalUpForce += c.UpForce;
        if (c.Contact) ++r.ContactCount;
    }
    return r;
}
latebrake::ChassisGroundContactOutput Ground(const latebrake::SuspensionResponse& r)
{
    latebrake::ChassisGroundContactOutput g;
    g.Valid = true; g.SupportingProbes = r.ContactCount;
    return latebrake::ConstrainGroundContactToSuspension(g, r);
}
latebrake::ServiceBrakeCapabilitySample Sample(double mass = 1100.0)
{
    latebrake::ServiceBrakeCapabilitySample s;
    s.MassKg = mass; s.RoadSamplesValid = true;
    s.ForwardCmPerSecond = 3000.0; s.RoadForwardCmPerSecond.fill(3000.0);
    s.NormalZ.fill(1.0); s.ForwardProjection.fill(1.0);
    return s;
}
}
int main()
{
    using namespace latebrake;
    auto springs = Springs(1100.0, {0.25, 0.25, 0.25, 0.25});
    auto ground = Ground(springs);
    auto s = Sample();
    auto profile = BrakeProfileForVehicle(0, true);
    auto full = EstimateServiceBrakeCapability(s, ground, springs, profile);
    Require(full.Valid && full.Supported && full.LoadedMask == 15u &&
            ServiceBrakeCapabilityLedger::IsConsistent(full), "accepted flat contact produces a coherent estimate");
    const double equivalent = 4.0 * 1.8 / (0.29 * 0.29);
    Require(std::abs(full.EquivalentRotorMassKg - equivalent) < 1.0e-8 &&
            std::abs(full.PlanningDecelerationCmPerSecondSquared -
                0.65 * 900.0 / (1.0 + equivalent / 1100.0)) < 1.0e-6,
            "flat admitted-limited forecast obeys independent equivalent-mass reference");
    s.ModeledRotors = false;
    const auto legacy = EstimateServiceBrakeCapability(s, ground, springs, profile);
    Require(legacy.Valid && legacy.EquivalentRotorMassKg == 0.0 &&
            std::abs(legacy.PlanningDecelerationCmPerSecondSquared - 585.0) < 1.0e-8,
            "legacy A/B path retains an explicitly reserved original brake estimate");
    s.ModeledRotors = true;
    auto light = Springs(1100.0, {0.03, 0.03, 0.03, 0.03});
    const auto unloaded = EstimateServiceBrakeCapability(s, Ground(light), light, profile);
    Require(unloaded.Valid && unloaded.PlanningDecelerationCmPerSecondSquared <
            full.PlanningDecelerationCmPerSecondSquared,
            "small measured spring loads cannot retain a full dry-road forecast");
    auto two = Springs(1100.0, {0.25, 0.25, 0.0, 0.0});
    const auto partial = EstimateServiceBrakeCapability(s, Ground(two), two, profile);
    Require(partial.Valid && partial.LoadedMask == 3u && partial.Supported &&
            partial.PlanningDecelerationCmPerSecondSquared < full.PlanningDecelerationCmPerSecondSquared,
            "front-only two-corner support reduces capacity without inventing rear contact");
    auto air = Springs(1100.0, {0.0, 0.0, 0.0, 0.0});
    auto absent = EstimateServiceBrakeCapability(s, Ground(air), air, profile);
    Require(absent.Valid && !absent.Supported && absent.LoadedMask == 0u &&
            absent.PlanningDecelerationCmPerSecondSquared == 0.0,
            "valid unsupported chassis has zero available braking");
    s.RoadForwardCmPerSecond = {-3000.0, -3000.0, -3000.0, -3000.0};
    const auto spin = EstimateServiceBrakeCapability(s, ground, springs, profile);
    Require(spin.Valid && spin.PlanningDecelerationCmPerSecondSquared == 0.0,
            "opposing road-point motion cannot be counted as useful service braking");
    s = Sample(); s.ForwardCmPerSecond = 0.0; s.RoadForwardCmPerSecond.fill(0.0);
    Require(EstimateServiceBrakeCapability(s, ground, springs, profile).PlanningDecelerationCmPerSecondSquared > 0.0,
            "a stationary launch can plan future service braking without claiming delivered force");
    s = Sample(); s.NormalZ.fill(0.8); s.ForwardProjection.fill(0.8);
    const auto grade = EstimateServiceBrakeCapability(s, ground, springs, profile);
    Require(grade.Valid && std::abs(grade.GradeAllowanceCmPerSecondSquared - 470.4) < 1.0e-7 &&
            grade.PlanningDecelerationCmPerSecondSquared < full.PlanningDecelerationCmPerSecondSquared,
            "measured tangent inclination reserves worst-direction downhill acceleration");
    ServiceBrakeCapabilityLedger ledger;
    Require(ledger.Publish(full, 1.0/60.0, 7) && ledger.Read().Valid,
            "only accepted complete frames publish a sequenced advisory capability");
    const auto seq = ledger.Read().Sequence;
    ledger.Invalidate();
    Require(!ledger.Read().Valid && ledger.Publish(full, 1.0/60.0, 8) &&
            ledger.Read().Sequence > seq && ledger.Read().TeleportRevision == 8,
            "pause/teleport invalidation cannot reuse an old sample identity");
    auto corrupt = full; corrupt.Corners[3].PlanningForceNewtons *= 100.0;
    Require(!ledger.Publish(corrupt, 1.0/60.0, 8) && !ledger.Read().Valid,
            "an over-admitted late corner invalidates the whole published estimate");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(), -1.0})
    {
        s = Sample(); s.NormalZ[3] = bad;
        Require(!EstimateServiceBrakeCapability(s, ground, springs, profile).Valid,
                "malformed measured road data cannot produce a capacity estimate");
        Require(!ledger.Publish(full, bad, 8) && !ledger.Read().Valid,
                "invalid frame duration removes the last accepted advisory value");
    }
    std::mt19937 rng(0x5b9809);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (unsigned n = 0; n < 25000; ++n)
    {
        const double mass = 100.0 + 14900.0 * u(rng);
        std::array<double, 4> loads{};
        for (double& load : loads) load = u(rng) < 0.18 ? 0.0 : u(rng) * 0.80;
        springs = Springs(mass, loads); ground = Ground(springs); s = Sample(mass);
        s.GripScale = 0.05 + 3.95 * u(rng); s.HeatScale = 0.05 + 0.95 * u(rng);
        s.ModeledRotors = n%2 == 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            s.NormalZ[i] = 0.68 + 0.32 * u(rng);
            s.ForwardProjection[i] = s.NormalZ[i];
            s.RoadForwardCmPerSecond[i] = -500.0 + 5000.0 * u(rng);
        }
        const auto e = EstimateServiceBrakeCapability(s, ground, springs,
            BrakeProfileForVehicle(static_cast<int>(n%6), n%3 == 0));
        Require(e.Valid && ServiceBrakeCapabilityLedger::IsConsistent(e) &&
                e.PlanningDecelerationCmPerSecondSquared >= 0.0 &&
                e.PlanningDecelerationCmPerSecondSquared <= e.NominalDecelerationCmPerSecondSquared,
                "25k independently varied mass/contact/road frames keep forecasts finite and bounded");
    }
    std::cout << "Service brake capability: load, SI, grade, spin, receipts and 25000 mixed frames passed\n";
}
