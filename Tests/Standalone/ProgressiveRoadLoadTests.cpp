#include "Vehicles/ProgressiveRoadLoad.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "ProgressiveRoadLoad FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-7)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    static_assert(RoadLoadChassisLinearDamping < 0.5);
    static_assert(RoadLoadArcadeQuadraticDrag <
                  RoadLoadSimulationQuadraticDrag);

    auto sample = [](double vx, double vy = 0.0, bool arcade = true,
                     bool offRoad = false, double dt = 1.0 / 60.0)
    {
        return CalculateProgressiveRoadLoad(
            {dt, vx, vy, 1100.0, arcade, offRoad});
    };
    const auto idle = sample(0.0);
    Require(idle.Valid && idle.ForceX == 0.0 && idle.ForceY == 0.0 &&
            idle.DecelerationCmPerSecondSquared == 0.0,
            "resting chassis cannot receive phantom rolling force");

    const auto slow = sample(300.0);
    const auto medium = sample(3000.0);
    const auto high = sample(6000.0);
    Require(slow.Valid && medium.Valid && high.Valid,
            "all ordinary circuit speeds accepted");
    Require(slow.DecelerationCmPerSecondSquared > 0.0 &&
            medium.DecelerationCmPerSecondSquared >
                slow.DecelerationCmPerSecondSquared &&
            high.DecelerationCmPerSecondSquared >
                3.0 * medium.DecelerationCmPerSecondSquared,
            "aero load rises steeply rather than a fixed drag on every speed");
    Require(high.ForceX < medium.ForceX && medium.ForceX < slow.ForceX &&
            high.ForceY == 0.0, "higher forward speed means stronger drag");
    Require(Near(high.DecelerationCmPerSecondSquared,
                 6000.0 * 6000.0 * RoadLoadArcadeQuadraticDrag +
                     RoadLoadArcadeRollingDeceleration),
            "original arcade coefficient and rolling estimate are stable");

    const auto reverse = sample(-3000.0);
    Require(reverse.Valid && Near(reverse.ForceX, -medium.ForceX),
            "reverse travel is resisted, never pushed farther backward");
    const auto sideways = sample(0.0, 3000.0);
    Require(sideways.Valid && sideways.ForceY < 0.0 &&
            Near(sideways.ForceY, medium.ForceX),
            "sideways drift sees opposing planar resistance");
    const auto diagonal = sample(3000.0, 4000.0);
    Require(diagonal.Valid && diagonal.ForceX < 0.0 &&
            diagonal.ForceY < 0.0 &&
            Near(diagonal.PlanarSpeedCmPerSecond, 5000.0) &&
            Near(diagonal.ForceX / diagonal.ForceY, 0.75),
            "diagonal travel resisted without changing physical heading");

    const auto sim = sample(3000.0, 0.0, false);
    const auto grass = sample(3000.0, 0.0, true, true);
    Require(sim.Valid && grass.Valid &&
            sim.DecelerationCmPerSecondSquared >
                medium.DecelerationCmPerSecondSquared &&
            grass.DecelerationCmPerSecondSquared >
                medium.DecelerationCmPerSecondSquared,
            "simulation handling and off-pavement roll down more quickly");
    Require(Near(sample(1000.0, 0.0, true, false, 0.1)
                     .DecelerationCmPerSecondSquared,
                 sample(1000.0).DecelerationCmPerSecondSquared),
            "frame rate does not alter unsaturated nominal load");
    const auto creeping = sample(0.001);
    Require(creeping.Valid && creeping.DecelerationCmPerSecondSquared <
            0.001 * 0.8 / (1.0 / 60.0),
            "creeping movement receives continuous non-reversing road load");

    const auto highFrame = sample(10.0, 0.0, true, true, 0.1);
    Require(highFrame.Valid &&
            highFrame.DecelerationCmPerSecondSquared <= 80.0 + 1e-9,
            "0.1 second frame cap cannot reverse very slow planar motion");
    const auto hitch = sample(1000.0, 0.0, true, false, 0.5);
    Require(!hitch.Valid && hitch.ForceX == 0.0,
            "hitch and invalid frame must not authorize an impulse");
    Require(!sample(std::numeric_limits<double>::quiet_NaN()).Valid &&
            !sample(std::numeric_limits<double>::infinity()).Valid &&
            !sample(16000.0).Valid,
            "corrupted or physically implausible velocities are rejected");
    Require(!CalculateProgressiveRoadLoad({0.016, 1000.0, 0.0, 0.0,
            true, false}).Valid &&
            !CalculateProgressiveRoadLoad({0.016, 1000.0, 0.0,
            std::numeric_limits<double>::infinity(), true, false}).Valid,
            "invalid chassis mass is rejected");
    Require(!sample(100.0, 0.0, true, false, -0.1).Valid,
            "negative time never authorizes forces");

    // Existing top-speed hard cap is not replaced by this drag model.
    std::cout << "Progressive road-load physics policy validated.\n";
    return EXIT_SUCCESS;
}
