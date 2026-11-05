#include "Vehicles/ProgressiveRoadLoad.h"
#include "Vehicles/SprungChassisDynamics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "Airborne road load: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double eps = 0.002)
{
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a - b) <= eps;
}
latebrake::ProgressiveRoadLoadOutput Drag(
    double vx, double vy, double springLoad,
    bool offRoad = false, bool arcade = true, double delta = 1.0 / 60.0)
{
    return latebrake::CalculateProgressiveRoadLoad(
        {delta, vx, vy, 1100.0, arcade, offRoad, springLoad});
}
latebrake::SuspensionFrame Springs()
{
    latebrake::SuspensionFrame frame{};
    frame.PhysicsReady = true;
    frame.DeltaSeconds = 1.0 / 60.0;
    frame.MassKg = 1100.0;
    frame.ChassisUpZ = 1.0;
    for (auto& corner : frame.Corners)
        corner = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    return frame;
}
}

int main()
{
    using namespace latebrake;
    const auto full = CalculateSprungChassis(Springs());
    Check(full.Valid && full.ContactCount == 4 &&
          Near(full.TotalUpForce / full.WeightForce, 1.0),
          "real level springs must provide one unit of supported weight");

    const double speed = 3000.0;
    const double aero = RoadLoadArcadeQuadraticDrag * speed * speed;
    const auto paved = Drag(speed, 0.0, 1.0);
    const auto grass = Drag(speed, 0.0, 1.0, true);
    Check(paved.Valid && grass.Valid &&
          Near(paved.DecelerationCmPerSecondSquared,
               aero + RoadLoadArcadeRollingDeceleration) &&
          Near(grass.DecelerationCmPerSecondSquared,
               paved.DecelerationCmPerSecondSquared *
                   RoadLoadOffPavementResistance),
          "full wheel support retains exact existing road/grass tuning");

    for (double ratio : {0.0, 0.25, 0.50, 0.75, 1.0})
    {
        const auto on = Drag(speed, 0.0, ratio);
        const auto off = Drag(speed, 0.0, ratio, true);
        const double expected = aero + RoadLoadArcadeRollingDeceleration * ratio;
        const double surface = 1.0 +
            (RoadLoadOffPavementResistance - 1.0) * ratio;
        Check(on.Valid && off.Valid &&
              Near(on.DecelerationCmPerSecondSquared, expected) &&
              Near(off.DecelerationCmPerSecondSquared, expected * surface),
              "rolling force must depend on physical spring load continuously");
        Check(on.ForceX <= 0.0 && off.ForceX <= 0.0,
              "aero and rolling resistance never accelerate the car");
    }

    auto springs = Springs();
    for (auto& wheel : springs.Corners) wheel.Hit = false;
    const auto airborne = CalculateSprungChassis(springs);
    Check(airborne.Valid && airborne.TotalUpForce == 0.0 &&
          Near(Drag(speed, 0.0,
                    airborne.TotalUpForce / airborne.WeightForce, true)
                   .DecelerationCmPerSecondSquared, aero),
          "fully airborne car has no rolling or grass friction");

    springs = Springs();
    springs.ChassisUpZ = -1.0;
    const auto inverted = CalculateSprungChassis(springs);
    Check(inverted.Valid && inverted.ContactCount == 0 &&
          Near(Drag(speed, 0.0,
                    inverted.TotalUpForce / inverted.WeightForce, true)
                   .DecelerationCmPerSecondSquared, aero),
          "roof-down body with geometric ground hits cannot roll on fake tires");

    springs = Springs();
    for (auto& wheel : springs.Corners)
        wheel.PointVerticalSpeedCmPerSecond = 700.0;
    const auto rebound = CalculateSprungChassis(springs);
    Check(rebound.Valid && rebound.TotalUpForce == 0.0 &&
          Near(Drag(speed, 0.0,
                    rebound.TotalUpForce / rebound.WeightForce)
                   .DecelerationCmPerSecondSquared, aero),
          "positive rebound cannot retain old-tick rolling resistance");

    const auto reverse = Drag(-speed, 0.0, 0.0, true);
    const auto sideways = Drag(0.0, speed, 0.0, true);
    Check(reverse.Valid && sideways.Valid &&
          Near(reverse.DecelerationCmPerSecondSquared, aero) &&
          Near(sideways.DecelerationCmPerSecondSquared, aero) &&
          reverse.ForceX > 0.0 && sideways.ForceY < 0.0,
          "airborne reverse and side-slips keep only opposing aero drag");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Check(!Drag(speed,0.0,-0.01).Valid &&
          !Drag(speed,0.0,1.01).Valid &&
          !Drag(speed,0.0,nan).Valid &&
          !Drag(speed,0.0,inf).Valid,
          "invalid spring support must never silently become wheel traction");

    for (int fps : {30,60,120})
    {
        const auto out = Drag(speed,0.0,0.45,true,true,1.0/fps);
        Check(out.Valid &&
              Near(out.DecelerationCmPerSecondSquared,
                   (aero + RoadLoadArcadeRollingDeceleration * 0.45) *
                   (1.0 + (RoadLoadOffPavementResistance - 1.0) * 0.45)),
              "unsaturated road load should match at 30/60/120 FPS");
    }

    for (int i = 0; i < 120000; ++i)
    {
        const double support = (i % 1001) / 1000.0;
        const double vx = ((i * 31) % 22001) - 11000.0;
        const double vy = ((i * 17) % 12001) - 6000.0;
        const double dt = 1.0 / (30.0 + (i % 91));
        const bool grassPosition = i % 3 == 0;
        const bool arcade = i % 2 == 0;
        const auto result = Drag(vx,vy,support,grassPosition,arcade,dt);
        const double planarSpeed = std::hypot(vx,vy);
        Check(result.Valid &&
              std::isfinite(result.DecelerationCmPerSecondSquared) &&
              result.DecelerationCmPerSecondSquared >= 0.0 &&
              result.DecelerationCmPerSecondSquared <=
                  0.8 * planarSpeed / dt + 1e-9 &&
              std::isfinite(result.ForceX) &&
              std::isfinite(result.ForceY) &&
              result.ForceX * vx + result.ForceY * vy <= 0.001,
              "120k varying spring loads obey force and no-reversal bounds");
        if (support == 0.0)
        {
            const auto flippedSurface =
                Drag(vx,vy,0.0,!grassPosition,arcade,dt);
            Check(flippedSurface.Valid &&
                  Near(result.DecelerationCmPerSecondSquared,
                       flippedSurface.DecelerationCmPerSecondSquared),
                  "pavement classification cannot affect airborne aero drag");
        }
    }
    std::cout << "Airborne road load passed (120k spring-load frames).\n";
    return EXIT_SUCCESS;
}
