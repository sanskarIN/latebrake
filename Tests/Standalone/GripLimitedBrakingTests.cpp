#include "Vehicles/GripLimitedBraking.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass)
    {
        std::cerr << "GripLimitedBraking FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double eps = 1e-8)
{
    return std::abs(a - b) <= eps;
}
}

int main()
{
    using namespace latebrake;
    static_assert(ArcadeServiceBrakeDeceleration <
                  SimulationServiceBrakeDeceleration);
    auto calc = [](double speed, bool service = true,
                   bool handbrake = false, bool arcade = true,
                   double surface = 1.0, double tire = 1.0,
                   double damage = 1.0, double heat = 1.0,
                   double dt = 1.0 / 60.0)
    {
        return CalculateGripLimitedBraking({
            dt, speed, 1100.0, service, handbrake, arcade,
            surface, tire, damage, heat});
    };
    const auto stopped = calc(0.0);
    Require(stopped.Valid && stopped.ForwardForce == 0.0 &&
            stopped.DecelerationCmPerSecondSquared == 0.0,
            "stationary brake produces no phantom motion");
    const auto unloaded = calc(4000.0, false, false);
    Require(unloaded.Valid && unloaded.ForwardForce == 0.0,
            "no brake request cannot create extra braking force");

    const auto paved = calc(3000.0);
    Require(paved.Valid && Near(paved.DecelerationCmPerSecondSquared, 900.0) &&
            Near(paved.ForwardForce, -990000.0),
            "normal arcade brake produces bounded approx 0.92g decel");
    const auto simulator = calc(3000.0, true, false, false);
    Require(simulator.Valid &&
            Near(simulator.DecelerationCmPerSecondSquared, 1050.0),
            "simulation profile uses distinct bounded service brake");
    const auto reverse = calc(-3000.0);
    Require(reverse.Valid && Near(reverse.ForwardForce, -paved.ForwardForce),
            "reverse braking force opposes current travel");
    const auto faded = calc(3000.0, true, false, true,
                            1.0, 1.0, 1.0, 0.74);
    Require(faded.Valid && faded.DecelerationCmPerSecondSquared <
            paved.DecelerationCmPerSecondSquared &&
            Near(faded.DecelerationCmPerSecondSquared, 666.0),
            "real heat fade reduces, rather than boosts, service braking");
    const auto gravel = calc(3000.0, true, false, true,
                             0.72, 0.80, 0.90);
    Require(gravel.Valid &&
            gravel.DecelerationCmPerSecondSquared <
            paved.DecelerationCmPerSecondSquared &&
            Near(gravel.DecelerationCmPerSecondSquared, 466.56),
            "confirmed off-road grip, worn tires and damage all reduce braking");
    // Regression: the lateral friction-circle policy uses the actual
    // product of road, tire and damage grip, WITHOUT a 10% floor.
    // Braking must not invent more traction than that shared contact
    // budget when severely degraded but still eligible (>= 5%).
    for (const double speed : {1200.0, 3000.0, -3000.0})
    {
        for (const bool arcade : {true, false})
        {
            const double signedDirection = speed > 0.0 ? -1.0 : 1.0;
            const double baseline = arcade
                ? ArcadeServiceBrakeDeceleration
                : SimulationServiceBrakeDeceleration;
            const auto lowGrip = calc(
                speed, true, false, arcade, 0.4, 0.4, 0.5);
            const double combinedGrip = 0.4 * 0.4 * 0.5;
            Require(lowGrip.Valid &&
                    Near(combinedGrip, 0.08) &&
                    Near(lowGrip.DecelerationCmPerSecondSquared,
                         baseline * combinedGrip) &&
                    Near(lowGrip.ForwardForce,
                         signedDirection * 1100.0 * baseline * combinedGrip),
                    "8 percent physical contact grip must yield exactly 8 percent longitudinal brake authority");
            const auto bothLowGrip = calc(
                speed, true, true, arcade, 0.4, 0.4, 0.5, 0.05);
            Require(bothLowGrip.Valid &&
                    Near(bothLowGrip.DecelerationCmPerSecondSquared,
                         HandbrakeDeceleration * combinedGrip) &&
                    Near(bothLowGrip.ForwardForce,
                         signedDirection * 1100.0 *
                             HandbrakeDeceleration * combinedGrip),
                    "service heat fade and handbrake preserve 8 percent road grip without a floor");
        }
    }
    // Real source can supply the lowest validated factor for each
    // independently measured surface/tire/damage component. Neither
    // smooth driving nor the brake may amplify a product smaller than
    // the friction-circle's admissible 5%-scale operating region.
    const auto extremeWear = calc(
        3000.0, true, false, true, 0.05, 0.05, 0.05);
    Require(extremeWear.Valid &&
            Near(extremeWear.DecelerationCmPerSecondSquared,
                 ArcadeServiceBrakeDeceleration * 0.000125) &&
            extremeWear.DecelerationCmPerSecondSquared > 0.0,
            "near-zero but finite validated grip cannot be promoted to a false 10 percent braking floor");
    // The pre-existing 2.0x upper bound still prevents excessive
    // acceleration authority from multiplying maximum garage/grip values.
    const auto upperBound = calc(
        3000.0, true, false, true, 2.0, 2.0, 2.0);
    Require(upperBound.Valid &&
            Near(upperBound.DecelerationCmPerSecondSquared,
                 ArcadeServiceBrakeDeceleration * 2.0),
            "existing maximum physical brake multiplier stays capped at two");

    // Vary ALL grip contributors. Effective braking must be monotonic
    // in the combined validated product across 30/60/120 FPS, with
    // forward and reverse parity. Tests use speeds far from frame cap.
    for (const double dt : {1.0 / 30.0, 1.0 / 60.0, 1.0 / 120.0})
    {
        double previous = 0.0;
        for (int n = 0; n <= 100; ++n)
        {
            const double surface = 0.2 + 0.008 * n;
            const double tire = 0.4;
            const double damage = 0.5;
            const double total = surface * tire * damage;
            const auto forward = calc(
                4500.0, true, false, true,
                surface, tire, damage, 1.0, dt);
            const auto reverseLowGrip = calc(
                -4500.0, true, false, true,
                surface, tire, damage, 1.0, dt);
            Require(forward.Valid && reverseLowGrip.Valid &&
                    Near(forward.DecelerationCmPerSecondSquared,
                         ArcadeServiceBrakeDeceleration * total) &&
                    Near(reverseLowGrip.DecelerationCmPerSecondSquared,
                         forward.DecelerationCmPerSecondSquared) &&
                    Near(reverseLowGrip.ForwardForce, -forward.ForwardForce) &&
                    forward.DecelerationCmPerSecondSquared + 1e-9 >= previous,
                    "low-grip braking remains proportional, frame-rate invariant and opposite real velocity");
            previous = forward.DecelerationCmPerSecondSquared;
        }
    }

    const auto parking = calc(3000.0, false, true);
    Require(parking.Valid && Near(parking.DecelerationCmPerSecondSquared, 470.0),
            "human handbrake can slow car while preserving drift grip policy");
    const auto both = calc(3000.0, true, true, true,
                           1.0, 1.0, 1.0, 0.30);
    Require(both.Valid && Near(both.DecelerationCmPerSecondSquared, 470.0),
            "parking brake retains minimum when service brakes fade");
    Require(calc(3000.0, true, true).DecelerationCmPerSecondSquared <=
            paved.DecelerationCmPerSecondSquared,
            "two brake inputs cannot double-count grip");

    const auto creep = calc(20.0, true, false, true,
                            1.0, 1.0, 1.0, 1.0, 0.1);
    Require(creep.Valid && creep.DecelerationCmPerSecondSquared <= 170.0 &&
            std::abs(creep.ForwardForce) * 0.1 / 1100.0 < 20.0,
            "large qualified frame cannot reverse slow car");
    const auto rightBeforeStop = calc(0.001);
    Require(rightBeforeStop.Valid && rightBeforeStop.ForwardForce < 0.0 &&
            std::abs(rightBeforeStop.ForwardForce) / 1100.0 *
                (1.0 / 60.0) < 0.001,
            "very low speed brake never requests a reversing impulse");
    const auto noHitch = calc(3000.0, true, false, true,
                              1.0, 1.0, 1.0, 1.0, 0.1);
    Require(Near(noHitch.DecelerationCmPerSecondSquared, 900.0),
            "brake deceleration remains frame-rate stable when uncapped");

    for (const double bad : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()})
    {
        Require(!calc(bad).Valid &&
                !calc(3000.0, true, false, true, bad).Valid &&
                !calc(3000.0, true, false, true, 1.0, bad).Valid &&
                !calc(3000.0, true, false, true, 1.0, 1.0, bad).Valid &&
                !calc(3000.0, true, false, true,
                      1.0, 1.0, 1.0, bad).Valid,
                "invalid physics and grip parameters fail closed");
    }
    Require(!calc(17000.0).Valid &&
            !calc(3000.0, true, false, true,
                  1.0, 1.0, 1.0, 1.0, 0.5).Valid &&
            !calc(3000.0, true, false, true,
                  0.0).Valid,
            "physically implausible velocity, hitch or zero grip rejected");
    Require(!CalculateGripLimitedBraking({
            0.016, 3000.0, 0.0, true, false, true,
            1.0, 1.0, 1.0, 1.0}).Valid,
            "invalid chassis mass fails closed");

    for (int i = 1; i <= 10000; ++i)
    {
        const double speed = i * 1.2;
        const double dt = (i % 10 + 1) * 0.01;
        const auto o = calc(speed, true, false, i % 2 == 0,
                            0.72, 0.8, 0.95, 0.8, dt);
        Require(o.Valid && std::isfinite(o.ForwardForce) &&
                o.ForwardForce < 0.0 &&
                o.DecelerationCmPerSecondSquared >= 0.0 &&
                o.DecelerationCmPerSecondSquared * dt <= speed * 0.85 + 1e-9,
                "10000 speed/cadence samples remain signed and non-reversing");
    }
    std::cout << "Grip-limited braking source policy validated.\n";
    return EXIT_SUCCESS;
}
