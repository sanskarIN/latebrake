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
