#include "Vehicles/TractionLimitedPropulsion.h"
#include "Vehicles/ArcadeBoostArbitration.h"
#include "Vehicles/ArcadeSurfaceTraction.h"
#include "Vehicles/CornerTractionAssist.h"
#include "Vehicles/GearAwareMotorPower.h"
#include "Vehicles/SprungGripReattachment.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "TractionLimitedPropulsion FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-7)
{
    return std::isfinite(a) && std::isfinite(b) && std::abs(a - b) < tolerance;
}
latebrake::TractionLimitedPropulsionSample Sample()
{
    return {1.0 / 60.0, 1100.0, 1500000.0, 0.0, 1.0, 1.0, 1.0, true};
}
} // namespace

int main()
{
    using namespace latebrake;
    // Actual pre-fix composition: mild sideslip with no corner-throttle
    // reduction and full gear power could demand more than configured grip.
    const double throttle = StabilizeCornerThrottle(1.0, 2500.0, 200.0, true);
    const auto side = CalculateLateralGripForce({
        1.0 / 60.0, 200.0, 1100.0, 8.5, 1.0, 0.0, true, false});
    const double power = GearAwareMotorPowerFactor(throttle, 2500.0, 6500.0, true);
    auto s = Sample();
    s.RequestedForwardForce *= throttle * power;
    s.AppliedRightForce = side.RightForce;
    Require(side.Valid && throttle == 1.0 && power == 1.0 &&
        std::hypot(s.RequestedForwardForce, s.AppliedRightForce) >
            ArcadeCombinedGripBudget * s.MassKg,
        "regression reproduces the real uncapped motor/cornering composition");
    auto out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && out.GripLimited && out.ForceScale > 0.0 && out.ForceScale < 1.0 &&
        Near(std::hypot(out.ForwardForce, s.AppliedRightForce),
            ArcadeCombinedGripBudget * s.MassKg, 1.0e-6),
        "motor yields exactly the remaining cornering allowance");
    const auto boosts = AllocateArcadeBoost(1.38, 0.06, true);
    s.RequestedForwardForce *= 1.0 + boosts.TotalExtra();
    const auto boosted = CalculateTractionLimitedPropulsion(s);
    Require(boosted.Valid && boosted.ForceScale < out.ForceScale &&
        Near(boosted.ForwardForce, out.ForwardForce, 1.0e-6),
        "nitro and drafting cannot escape already exhausted tire grip");

    s = Sample();
    out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && !out.GripLimited && out.ForceScale == 1.0 &&
        out.ForwardForce == s.RequestedForwardForce,
        "ordinary straight launch keeps full existing motor thrust");
    s.RequestedForwardForce = -s.RequestedForwardForce;
    out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && out.ForwardForce == s.RequestedForwardForce,
        "reverse propulsion retains its sign and available straight-line force");
    s.ArcadeHandling = false;
    out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && out.ForwardForce < 0.0 &&
        Near(std::abs(out.ForwardForce), s.MassKg * SimulationCombinedGripBudget),
        "simulation-inspired handling uses the existing separate combined ceiling");
    s = Sample(); s.ContactTractionScale = 0.75; s.RearTractionScale = 0.2;
    s.RequestedForwardForce = 1500000.0 * s.RearTractionScale;
    out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && out.ForceScale == 1.0 && out.ForwardForce == 300000.0,
        "an already rear-scaled request does not receive the rear scale twice");
    s.RequestedForwardForce = 10000000.0;
    out = CalculateTractionLimitedPropulsion(s);
    Require(out.Valid && Near(out.ForwardForce,
        s.MassKg * ArcadeCombinedGripBudget * s.RearTractionScale),
        "excess requested power respects rear support as an independent ceiling");
    s.RearTractionScale = 0.0;
    Require(CalculateTractionLimitedPropulsion(s).ForwardForce == 0.0,
        "front-only contact cannot grant propulsion");
    s.ContactTractionScale = 0.0;
    Require(CalculateTractionLimitedPropulsion(s).ForwardForce == 0.0,
        "airborne contact gives zero motor force");
    s = Sample(); s.AppliedRightForce = s.MassKg * ArcadeCombinedGripBudget;
    Require(CalculateTractionLimitedPropulsion(s).ForwardForce == 0.0,
        "an exhausted lateral budget has no remaining longitudinal allowance");
    s.AppliedRightForce *= 1.01;
    Require(CalculateTractionLimitedPropulsion(s).ForwardForce == 0.0,
        "over-budget lateral observation cannot produce imaginary square-root grip");
    s = Sample(); s.RequestedForwardForce = 0.0;
    Require(CalculateTractionLimitedPropulsion(s).ForwardForce == 0.0,
        "zero input never invents motor demand");
    for (int fps : {30, 60, 120})
    {
        s = Sample(); s.DeltaSeconds = 1.0 / fps; s.RequestedForwardForce *= 1.4;
        out = CalculateTractionLimitedPropulsion(s);
        double speed = 0.0;
        for (int step = 0; step < 2 * fps; ++step)
            speed += out.ForwardForce / s.MassKg * s.DeltaSeconds;
        Require(out.Valid && Near(speed, 2.0 * ArcadeCombinedGripBudget, 1.0e-6),
            "constant-force reference integrates consistently at 30/60/120 FPS");
    }

    for (auto member : {&TractionLimitedPropulsionSample::DeltaSeconds,
             &TractionLimitedPropulsionSample::MassKg,
             &TractionLimitedPropulsionSample::RequestedForwardForce,
             &TractionLimitedPropulsionSample::AppliedRightForce,
             &TractionLimitedPropulsionSample::GripScale,
             &TractionLimitedPropulsionSample::ContactTractionScale,
             &TractionLimitedPropulsionSample::RearTractionScale})
    {
        for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::infinity(),
                 -std::numeric_limits<double>::infinity()})
        {
            s = Sample(); s.*member = bad;
            out = CalculateTractionLimitedPropulsion(s);
            Require(!out.Valid && out.ForwardForce == 0.0 && out.ForceScale == 0.0,
                "all nonfinite inputs fail closed without retained thrust");
        }
    }
    for (double bad : {0.0, -0.01, 0.10001})
    {
        s = Sample(); s.DeltaSeconds = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "invalid time fails closed");
    }
    for (double bad : {99.9, 15000.1})
    {
        s = Sample(); s.MassKg = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "invalid mass fails closed");
    }
    for (double bad : {0.049, 4.001})
    {
        s = Sample(); s.GripScale = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "invalid grip fails closed");
    }
    for (double bad : {-0.01, 1.01})
    {
        s = Sample(); s.ContactTractionScale = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "invalid whole-body grip fails closed");
        s = Sample(); s.RearTractionScale = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "invalid rear grip fails closed");
    }
    s = Sample(); s.ContactTractionScale = 0.45; s.RearTractionScale = 0.5;
    Require(!CalculateTractionLimitedPropulsion(s).Valid, "rear load cannot exceed whole-body ceiling");
    for (double bad : {-1.0e8 - 1.0, 1.0e8 + 1.0})
    {
        s = Sample(); s.RequestedForwardForce = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "absurd motor force rejected");
        s = Sample(); s.AppliedRightForce = bad;
        Require(!CalculateTractionLimitedPropulsion(s).Valid, "absurd lateral force rejected");
    }

    SprungGripReattachment adhesion;
    ArcadeSurfaceTraction surfaces;
    for (unsigned i = 0; i < 120000; ++i)
    {
        const double dt = 1.0 / (i % 3 == 0 ? 30.0 : i % 3 == 1 ? 60.0 : 120.0);
        const double mass = 100.0 + static_cast<double>(i % 14901);
        SuspensionFrame spring;
        spring.DeltaSeconds = dt; spring.MassKg = mass; spring.PhysicsReady = true;
        spring.ChassisUpZ = i % 29 == 0 ? -1.0 : 1.0;
        ChassisGroundContactSample coarse;
        coarse.DeltaSeconds = dt; coarse.PhysicsReady = true;
        const unsigned mask = (i / 11) % 16;
        for (unsigned c = 0; c < 4; ++c)
        {
            const bool hit = (mask & (1u << c)) != 0;
            const double distance = 50.0 + static_cast<double>((i + 7 * c) % 33);
            spring.Corners[c] = {hit, distance, 1.0, i % 23 == 0 ? 1000.0 : 0.0};
            coarse.Probes[c] = {hit, distance, 1.0};
        }
        const auto springs = CalculateSprungChassis(spring);
        const auto ground = ConstrainGroundContactToSuspension(
            EvaluateChassisGroundContact(coarse), springs);
        const auto attached = adhesion.Advance(dt, ground,
            CalculateAxleTractionBudget(ground, springs));
        const auto road = surfaces.Observe({dt, i % 80 < 40 ? EastX : 0.0, 0.0, true});
        Require(springs.Valid && ground.Valid && attached.Valid && road.Valid,
            "actual spring/contact/axle/reattachment/surface chain accepts fixtures");
        const bool arcade = i % 5 != 0;
        const double forward = (i % 2 == 0 ? 1.0 : -1.0) * (100.0 + i % 5500);
        const double lateral = static_cast<double>(i % 1800);
        const double command = forward < 0.0 ? -1.0 : 1.0;
        const double grip = (0.5 + static_cast<double>(i % 151) / 100.0) *
            road.LateralGripMultiplier;
        const auto lateralForce = CalculateLateralGripForce({
            dt, lateral, mass, 8.5, grip, 0.0, arcade, false});
        const auto allocation = AllocateArcadeBoost(i % 3 == 0 ? 1.38 : 1.0,
            static_cast<double>(i % 61) / 1000.0, i % 3 == 0);
        const double axle = attached.Axles.RearDriveReady
            ? attached.Axles.RearDriveTractionScale : 0.0;
        const float axleGear = static_cast<float>(
            GearAwareMotorPowerFactor(command, forward, 6500.0, arcade) * axle);
        const double base = 1500000.0 * command * road.MotorMultiplier;
        s = {dt, mass, base * axleGear * (1.0 + allocation.TotalExtra()),
            lateralForce.RightForce * attached.Ground.ContactTractionScale,
            grip, attached.Ground.ContactTractionScale, axle, arcade};
        out = CalculateTractionLimitedPropulsion(s);
        const double budget = (arcade ? ArcadeCombinedGripBudget
            : SimulationCombinedGripBudget) * grip * s.ContactTractionScale;
        Require(lateralForce.Valid && out.Valid && out.ForceScale >= 0.0 &&
            out.ForceScale <= 1.0 && std::isfinite(out.ForwardForce),
            "all changing-contact outputs remain finite reductions");
        Require(out.ForwardForce * command >= 0.0 &&
            std::abs(out.ForwardForce) <= std::abs(s.RequestedForwardForce) + 1.0e-7 &&
            std::hypot(out.ForwardForce, s.AppliedRightForce) / mass <= budget + 1.0e-7,
            "actual motor plus actual lateral force obey the same contact grip circle");
        // Exercise the real float coefficient shared by all three Unreal
        // AddForce calls, rather than only the ideal double return value.
        const float finalFactor = axleGear * static_cast<float>(out.ForceScale);
        const double actualBase = base * finalFactor;
        const double actualTotal = actualBase +
            actualBase * static_cast<float>(allocation.NitroExtra) +
            actualBase * static_cast<float>(allocation.DraftExtra);
        Require(std::hypot(actualTotal, s.AppliedRightForce) / mass <=
            budget + 1.0e-6 * std::max(1.0, budget),
            "three float-scaled propulsion paths remain bounded within float precision");
        if (!attached.Axles.RearDriveReady)
            Require(out.ForwardForce == 0.0 && actualTotal == 0.0,
                "front-only, airborne, rebound and inverted poses get no propulsion");
    }
    std::cout << "TractionLimitedPropulsion: all checks passed\n";
}
