#include "Vehicles/LateralGripBudget.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "LateralGripBudget FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-6)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    static_assert(ArcadeCombinedGripBudget > ArcadeLateralAccelerationBudget);
    static_assert(HandbrakeLateralGripFraction > 0.0 &&
                  HandbrakeLateralGripFraction < 1.0);

    auto sample = [](double sideways, double rate = 8.5,
                     double grip = 1.0, double brake = 0.0,
                     bool arcade = true, bool handbrake = false,
                     double dt = 1.0/60.0)
    {
        return CalculateLateralGripForce({
            dt, sideways, 1100.0, rate, grip, brake, arcade, handbrake});
    };

    const auto stationary = sample(0.0);
    Require(stationary.Valid && stationary.RightForce == 0.0 &&
            stationary.AppliedLateralDeceleration == 0.0 &&
            !stationary.GripLimited,
            "zero measured lateral movement cannot invent a force");
    const auto ordinary = sample(50.0);
    Require(ordinary.Valid && ordinary.RightForce < 0.0 &&
            Near(ordinary.AppliedLateralDeceleration, 425.0) &&
            !ordinary.GripLimited,
            "small lateral slip gets continuous bounded correction");
    const auto sliding = sample(3000.0);
    Require(sliding.Valid && sliding.GripLimited &&
            Near(sliding.AppliedLateralDeceleration,
                 ArcadeLateralAccelerationBudget) &&
            Near(sliding.RightForce, -1100.0 * ArcadeLateralAccelerationBudget),
            "hard drift cannot be erased by 8.5 times per second damping");
    Require(sliding.RequestedLateralDeceleration >
            sliding.AppliedLateralDeceleration * 10.0,
            "traction budget genuinely controls large slip correction");
    const auto reverseSide = sample(-3000.0);
    Require(reverseSide.Valid &&
            Near(reverseSide.RightForce, -sliding.RightForce) &&
            Near(reverseSide.AppliedLateralDeceleration,
                 sliding.AppliedLateralDeceleration),
            "opposite-direction slip must be corrected symmetrically");
    const auto wet = sample(3000.0, 8.5, 0.72);
    Require(wet.Valid && wet.GripLimited &&
            Near(wet.AppliedLateralDeceleration,
                 0.72 * ArcadeLateralAccelerationBudget),
            "confirmed offroad/worn tire/damage lower lateral force budget");
    const auto simulator = sample(3000.0, 4.25, 1.0, 0.0, false);
    Require(simulator.Valid && simulator.GripLimited &&
            Near(simulator.AppliedLateralDeceleration,
                 SimulationLateralAccelerationBudget),
            "simulation handling receives its own finite lateral budget");

    const auto handbrake = sample(3000.0, 1.8, 1.0, 470.0, true, true);
    Require(handbrake.Valid && handbrake.GripLimited &&
            handbrake.AppliedLateralDeceleration <
                sliding.AppliedLateralDeceleration * 0.5,
            "human handbrake unlocks prolonged physical slide");
    const auto braking = sample(3000.0, 8.5, 1.0, 900.0);
    Require(braking.Valid && braking.GripLimited &&
            braking.AppliedLateralDeceleration <=
                sliding.AppliedLateralDeceleration &&
            Near(braking.AppliedLateralDeceleration,
                 ArcadeLateralAccelerationBudget),
            "ordinary requested service braking shares available contact grip");
    const auto hardBraking = sample(3000.0, 8.5, 1.0, 1200.0);
    Require(hardBraking.Valid && hardBraking.GripLimited &&
            hardBraking.AppliedLateralDeceleration <
                sliding.AppliedLateralDeceleration &&
            Near(std::hypot(1200.0,
                hardBraking.AppliedLateralDeceleration),
                ArcadeCombinedGripBudget),
            "high brake demand leaves smaller friction-circle corner budget");
    const auto overBudget = sample(3000.0, 8.5, 1.0, 2000.0);
    Require(overBudget.Valid && overBudget.RightForce == 0.0 &&
            overBudget.GripLimited,
            "fully spent longitudinal allowance cannot create lateral grip");

    const auto creeping = sample(20.0, 8.5, 1.0, 0.0, true, false, 0.1);
    Require(creeping.Valid && Near(
        creeping.AppliedLateralDeceleration, 160.0) &&
        std::abs(creeping.RightForce) * 0.1 / 1100.0 < 20.0,
        "0.1s frame cannot flip sideways velocity through excess impulse");
    const auto nonfinite = std::numeric_limits<double>::quiet_NaN();
    Require(!sample(nonfinite).Valid &&
            !sample(1000.0, nonfinite).Valid &&
            !sample(1000.0, 8.5, nonfinite).Valid &&
            !sample(1000.0, 8.5, 1.0, nonfinite).Valid &&
            !sample(1000.0, 8.5, 1.0, 0.0, true, false, 0.5).Valid &&
            !sample(16000.0).Valid &&
            !CalculateLateralGripForce({
                0.016, 1000.0, 0.0, 8.5, 1.0, 0.0, true, false}).Valid,
            "corrupted speed/rate/mass/grip/hitch cannot authorize force");

    for (int i = 0; i < 20000; ++i)
    {
        const double side = (i % 29999) - 15000.0;
        const double grip = 0.25 + (i % 200) / 100.0;
        const double rate = (i % 450) / 10.0;
        const double brake = (i % 150) * 20.0;
        const double dt = 0.01 * (1 + i % 10);
        const auto q = sample(side, rate, grip, brake,
                              i % 2 == 0, i % 3 == 0, dt);
        Require(q.Valid && std::isfinite(q.RightForce) &&
                std::isfinite(q.AvailableLateralDeceleration) &&
                q.AppliedLateralDeceleration >= 0.0 &&
                q.AppliedLateralDeceleration <=
                    q.AvailableLateralDeceleration + 1.0e-8 &&
                q.AppliedLateralDeceleration * dt <=
                    0.8 * std::abs(side) + 1.0e-6,
                "20k variable frame/slip/grip/brake samples remain bounded");
        if (side > 0.0) Require(q.RightForce <= 0.0,
            "rightward slip never receives rightward fake propulsion");
        if (side < 0.0) Require(q.RightForce >= 0.0,
            "leftward slip never receives leftward fake propulsion");
    }
    std::cout << "Lateral friction-circle budget validated.\n";
    return EXIT_SUCCESS;
}
