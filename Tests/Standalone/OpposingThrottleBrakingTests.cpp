#include "Vehicles/OpposingThrottleBraking.h"
#include "Vehicles/AxleTractionBudget.h"
#include "Vehicles/SprungGripReattachment.h"
#include "Vehicles/LateralGripBudget.h"

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
        std::cerr << "OpposingThrottleBraking FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double epsilon = 1.0e-7)
{
    return std::isfinite(a) && std::isfinite(b) && std::abs(a - b) < epsilon;
}
latebrake::OpposingThrottleBrakeSample Sample(double forward = 6500.0)
{
    return {1.0 / 60.0, forward, std::abs(forward), 1100.0,
        forward < 0.0 ? 1.0 : -1.0, 1.0, 1.0, 1.0, false, true};
}

double Integrate(int fps)
{
    double speed = 3000.0;
    for (int i = 0; i < fps * 2; ++i)
    {
        auto s = Sample(speed);
        s.DeltaSeconds = 1.0 / fps;
        const auto out = latebrake::CalculateOpposingThrottleBraking(s);
        Require(out.Valid && out.OpposingThrottle, "direction request remains braking");
        const double next = speed + out.ForwardForce / s.MassKg * s.DeltaSeconds;
        Require(next >= 0.0 && next <= speed,
            "brake force alone cannot increase speed or reverse travel");
        speed = next;
    }
    return speed;
}
} // namespace

int main()
{
    using namespace latebrake;
    // Regression: old reverse-pedal branch demanded 19500 cm/s^2 (~20g)
    // at 6500 cm/s. A normal-road full rear axle is now capped at 650.
    const auto normal = CalculateOpposingThrottleBraking(Sample());
    Require(normal.Valid && normal.OpposingThrottle &&
        Near(normal.ForwardForce, -715000.0) &&
        Near(normal.DecelerationCmPerSecondSquared, 650.0) &&
        Near(normal.WholeBodyBudgetDeceleration, 650.0),
        "racing-speed reverse demand cannot bypass the rear brake ceiling");
    Require(6500.0 * 3.0 > normal.DecelerationCmPerSecondSquared * 25.0,
        "fixture exercises the former extreme damping demand");

    auto s = Sample(-2500.0);
    auto out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && out.OpposingThrottle && out.ForwardForce > 0.0,
        "forward pedal slows actual reverse travel with opposite signed force");
    s.ArcadeHandling = false;
    Require(Near(CalculateOpposingThrottleBraking(s).DecelerationCmPerSecondSquared,
        SimulationOpposingThrottleDeceleration), "separate simulation-inspired ceiling");
    for (double demand : {0.01, 0.1, 0.5, 1.0})
    {
        s = Sample();
        s.EffectiveThrottle = -demand;
        out = CalculateOpposingThrottleBraking(s);
        Require(out.Valid && Near(out.DecelerationCmPerSecondSquared, 650.0 * demand),
            "analog pedal strength scales the bounded demand");
    }
    for (double grip : {0.05, 0.2, 0.58, 1.0, 2.0, 4.0})
    {
        s = Sample();
        s.GripScale = grip;
        out = CalculateOpposingThrottleBraking(s);
        Require(out.Valid && Near(out.DecelerationCmPerSecondSquared,
            650.0 * std::min(grip, 2.0)),
            "surface, wear, damage and garage grip apply once without a fake grip floor");
    }
    s = Sample();
    s.ContactTractionScale = 0.45;
    s.RearTractionScale = 0.18;
    out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && Near(out.DecelerationCmPerSecondSquared, 650.0 * 0.18) &&
        Near(out.WholeBodyBudgetDeceleration, 650.0 * 0.18 / 0.45),
        "partial rear loading is normalized to the actual whole-body lateral ceiling");
    s.RearTractionScale = 0.0;
    Require(CalculateOpposingThrottleBraking(s).ForwardForce == 0.0,
        "front-only support never grants engine braking");
    s.ContactTractionScale = 0.0;
    out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && out.ForwardForce == 0.0 &&
        out.WholeBodyBudgetDeceleration == 0.0,
        "airborne zero contact cannot divide by zero or create brake grip");

    for (double demand : {-0.009, 0.0, 0.25, 1.0})
    {
        s = Sample();
        s.EffectiveThrottle = demand;
        out = CalculateOpposingThrottleBraking(s);
        Require(out.Valid && !out.OpposingThrottle && out.ForwardForce == 0.0,
            "neutral or matching-direction demand is not an implicit brake");
    }
    s = Sample(99.9);
    out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && !out.OpposingThrottle && out.ForwardForce == 0.0,
        "preserve the original near-stop direction-change allowance");
    // Total planar speed can remain high during a spin even when the car
    // has no meaningful signed drive-axis movement. Neither direction
    // should be mistaken for opposing engine braking in this condition.
    s = Sample(0.0);
    s.PlanarCmPerSecond = 1500.0;
    out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && !out.OpposingThrottle && out.ForwardForce == 0.0,
        "pure sideslip does not steal throttle with a zero-force brake");
    for (int fps : {30, 60, 120})
    {
        for (double forward : {-99.9, -0.001, 0.0, 0.001, 99.9})
        {
            s = Sample(forward);
            s.DeltaSeconds = 1.0 / fps;
            s.PlanarCmPerSecond = 2400.0;
            out = CalculateOpposingThrottleBraking(s);
            Require(out.Valid && !out.OpposingThrottle &&
                out.ForwardForce == 0.0 &&
                out.WholeBodyBudgetDeceleration == 0.0,
                "sideways speed does not override signed-axis near-stop allowance");
        }
        for (double forward : {-100.0, 100.0, -1500.0, 1500.0})
        {
            s = Sample(forward);
            s.DeltaSeconds = 1.0 / fps;
            s.PlanarCmPerSecond = 2400.0;
            out = CalculateOpposingThrottleBraking(s);
            Require(out.Valid && out.OpposingThrottle &&
                out.ForwardForce * forward < 0.0 &&
                out.DecelerationCmPerSecondSquared > 0.0,
                "real opposing signed travel still brakes at all frame rates");
        }
    }
    s = Sample();
    s.EffectiveBrakeHeld = true;
    out = CalculateOpposingThrottleBraking(s);
    Require(out.Valid && !out.OpposingThrottle && out.ForwardForce == 0.0,
        "service or handbrake overrides engine brake without stacking forces");

    for (int fps : {30, 60, 120})
        Require(Near(Integrate(fps), 1700.0, 1.0e-6),
            "constant bounded force gives matching two-second speed loss at 30/60/120 FPS");

    // Every numeric input is validated even when brake precedence would
    // suppress output; invalid samples never masquerade as ordinary coasting.
    for (auto member : {&OpposingThrottleBrakeSample::DeltaSeconds,
             &OpposingThrottleBrakeSample::ForwardCmPerSecond,
             &OpposingThrottleBrakeSample::PlanarCmPerSecond,
             &OpposingThrottleBrakeSample::MassKg,
             &OpposingThrottleBrakeSample::EffectiveThrottle,
             &OpposingThrottleBrakeSample::GripScale,
             &OpposingThrottleBrakeSample::RearTractionScale,
             &OpposingThrottleBrakeSample::ContactTractionScale})
    {
        for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::infinity(),
                 -std::numeric_limits<double>::infinity()})
        {
            s = Sample();
            s.*member = invalid;
            out = CalculateOpposingThrottleBraking(s);
            Require(!out.Valid && !out.OpposingThrottle && out.ForwardForce == 0.0 &&
                out.WholeBodyBudgetDeceleration == 0.0,
                "nonfinite input withdraws all motor/brake classification authority");
        }
    }
    for (double dt : {0.0, -0.01, 0.100001})
    {
        s = Sample(); s.DeltaSeconds = dt;
        Require(!CalculateOpposingThrottleBraking(s).Valid, "invalid frame fails closed");
    }
    for (double mass : {99.99, 15000.01})
    {
        s = Sample(); s.MassKg = mass;
        Require(!CalculateOpposingThrottleBraking(s).Valid, "invalid mass fails closed");
    }
    for (double grip : {0.049, 4.001})
    {
        s = Sample(); s.GripScale = grip;
        Require(!CalculateOpposingThrottleBraking(s).Valid, "invalid grip fails closed");
    }
    for (double scale : {-0.01, 1.01})
    {
        s = Sample(); s.ContactTractionScale = scale;
        Require(!CalculateOpposingThrottleBraking(s).Valid, "invalid whole-body load fails closed");
        s = Sample(); s.RearTractionScale = scale;
        Require(!CalculateOpposingThrottleBraking(s).Valid, "invalid rear load fails closed");
    }
    s = Sample(); s.RearTractionScale = 0.5; s.ContactTractionScale = 0.45;
    Require(!CalculateOpposingThrottleBraking(s).Valid, "rear load cannot exceed body grip");
    s = Sample(); s.PlanarCmPerSecond = 15000.1;
    Require(!CalculateOpposingThrottleBraking(s).Valid, "absurd speed fails closed");
    s = Sample(); s.PlanarCmPerSecond = -1.0;
    Require(!CalculateOpposingThrottleBraking(s).Valid, "negative planar speed fails closed");
    s = Sample(); s.PlanarCmPerSecond = 5000.0;
    Require(!CalculateOpposingThrottleBraking(s).Valid, "impossible signed projection fails closed");
    s = Sample(); s.EffectiveThrottle = -1.001;
    Require(!CalculateOpposingThrottleBraking(s).Valid, "out-of-range pedal fails closed");

    // Composition with the actual four-spring, coarse contact, axle and
    // reattachment policies (not invented rear-load multipliers). Changing
    // contact, rebound and upside-down frames immediately withdraw braking.
    SprungGripReattachment adhesion;
    for (unsigned i = 0; i < 120000; ++i)
    {
        const int fps = i % 3 == 0 ? 30 : i % 3 == 1 ? 60 : 120;
        const double mass = 100.0 + static_cast<double>(i % 14901);
        SuspensionFrame spring;
        spring.DeltaSeconds = 1.0 / fps;
        spring.MassKg = mass;
        spring.PhysicsReady = true;
        spring.ChassisUpZ = i % 19 == 0 ? -1.0 : 1.0;
        ChassisGroundContactSample coarse;
        coarse.DeltaSeconds = spring.DeltaSeconds;
        coarse.PhysicsReady = true;
        const unsigned mask = (i / 9) % 16;
        for (unsigned c = 0; c < 4; ++c)
        {
            const bool hit = (mask & (1u << c)) != 0;
            const double height = 50.0 + static_cast<double>((i + c * 7) % 33);
            spring.Corners[c] = {hit, height, 1.0,
                i % 17 == 0 ? 900.0 : 0.0};
            coarse.Probes[c] = {hit, height, 1.0};
        }
        const auto sprung = CalculateSprungChassis(spring);
        const auto ground = ConstrainGroundContactToSuspension(
            EvaluateChassisGroundContact(coarse), sprung);
        const auto axles = CalculateAxleTractionBudget(ground, sprung);
        const auto attached = adhesion.Advance(spring.DeltaSeconds, ground, axles);
        Require(sprung.Valid && ground.Valid && axles.Valid && attached.Valid,
            "real four-corner support chain accepts physical fixtures");
        const double forward = (i % 2 == 0 ? 1.0 : -1.0) *
            (0.1 + static_cast<double>(i % 6500));
        const double lateral = static_cast<double>(i % 3000);
        s = Sample(forward);
        s.DeltaSeconds = spring.DeltaSeconds;
        s.PlanarCmPerSecond = std::hypot(forward, lateral);
        s.MassKg = mass;
        s.EffectiveThrottle *= 0.01 + static_cast<double>(i % 100) / 100.0;
        s.GripScale = 0.05 + static_cast<double>(i % 396) / 100.0;
        s.RearTractionScale = attached.Axles.RearDriveReady
            ? attached.Axles.RearDriveTractionScale : 0.0;
        s.ContactTractionScale = attached.Ground.ContactTractionScale;
        s.ArcadeHandling = i % 5 != 0;
        out = CalculateOpposingThrottleBraking(s);
        const auto sideways = CalculateLateralGripForce({
            s.DeltaSeconds, lateral, mass, 8.5, s.GripScale,
            out.WholeBodyBudgetDeceleration, s.ArcadeHandling, false});
        Require(out.Valid && sideways.Valid && std::isfinite(out.ForwardForce) &&
            out.ForwardForce * forward <= 0.0,
            "accepted rear brake always opposes true signed travel");
        const double cap = s.ArcadeHandling ? 650.0 : 750.0;
        Require(out.DecelerationCmPerSecondSquared <= cap *
                std::min(s.GripScale, 2.0) * s.RearTractionScale + 1.0e-7,
            "rear load, grip and profile bound every frame");
        Require(out.DecelerationCmPerSecondSquared * s.DeltaSeconds <=
                0.8 * std::abs(forward) + 1.0e-7,
            "no frame can reverse the sampled forward axis");
        const double combinedBudget = (s.ArcadeHandling
            ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) *
                s.GripScale * s.ContactTractionScale;
        Require(std::hypot(out.DecelerationCmPerSecondSquared,
            sideways.AppliedLateralDeceleration * s.ContactTractionScale) <=
                combinedBudget + 1.0e-7,
            "actual rear brake and actual whole-body lateral force share one grip circle");
        if (!attached.Axles.RearDriveReady)
            Require(out.ForwardForce == 0.0,
                "front-only, airborne, rebound and inverted frames cannot engine-brake");
    }
    std::cout << "OpposingThrottleBraking: all checks passed\n";
}
