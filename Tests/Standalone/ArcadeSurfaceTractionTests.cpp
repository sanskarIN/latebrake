#include "Vehicles/ArcadeSurfaceTraction.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <initializer_list>

namespace
{
void Check(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "ArcadeSurfaceTraction FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::ArcadeSurfaceOutput Step(
    latebrake::ArcadeSurfaceTraction& state, double x, double y,
    double dt = 0.05)
{
    return state.Observe({dt, x, y, true});
}
void Road(latebrake::ArcadeSurfaceOutput x)
{
    Check(x.Valid && !x.OffPavement && x.MotorMultiplier == 1.0 &&
          x.LateralGripMultiplier == 1.0, "road must retain baseline forces");
}
void Dirt(latebrake::ArcadeSurfaceOutput x)
{
    Check(x.Valid && x.OffPavement &&
          x.MotorMultiplier ==
              latebrake::ArcadeSurfaceTraction::OffPavementMotorMultiplier &&
          x.LateralGripMultiplier ==
              latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier,
          "confirmed off-road must reduce both bounded force scales");
}
}
int main()
{
    using namespace latebrake;
    ArcadeSurfaceTraction surface;
    Road(Step(surface, PlayerStart.Position.X, PlayerStart.Position.Y));
    Road(Step(surface, RivalStart.Position.X, RivalStart.Position.Y));
    Road(Step(surface, EastX, 0.0));
    Road(Step(surface, WestX, 0.0));
    Road(Step(surface, 0.0, SouthY));
    Road(Step(surface, 0.0, NorthY));
    Road(Step(surface, EastX + RoadHalfWidth, 0.0));
    Road(Step(surface, EastX - RoadHalfWidth, 0.0));
    Road(Step(surface, 0.0, SouthY + RoadHalfWidth));
    Road(Step(surface, 0.0, SouthY - RoadHalfWidth));

    // A single excursion or boundary chatter cannot flicker forces.
    Road(Step(surface, 0.0, 0.0));
    Road(Step(surface, 0.0, 0.0));
    Road(Step(surface, EastX, 0.0));
    Road(Step(surface, 0.0, 0.0));
    Road(Step(surface, 0.0, 0.0));
    Dirt(Step(surface, 0.0, 0.0));
    Check(surface.IsOffPavement(), "sustained off-road state is observable");

    // Re-entry must remain stable long enough before force restoration.
    for (int i = 0; i < 3; ++i) Dirt(Step(surface, EastX, 0.0));
    Dirt(Step(surface, 0.0, 0.0));
    for (int i = 0; i < 3; ++i) Dirt(Step(surface, EastX, 0.0));
    Road(Step(surface, EastX, 0.0));
    Check(!surface.IsOffPavement(), "road re-entry releases held traction");
    surface.Reset();
    Road(Step(surface, EastX + RoadHalfWidth + 1.0, 0.0));
    Road(Step(surface, EastX + RoadHalfWidth + 1.0, 0.0));
    Dirt(Step(surface, EastX + RoadHalfWidth + 1.0, 0.0));
    surface.Reset();
    Road(Step(surface, EastX + RoadHalfWidth, 0.0));

    // No input from menus/interruptions, NaNs, large frame hitches,
    // or implausibly distant world coordinates can grant motor power.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (const auto bad : {
        ArcadeSurfaceSample{0.05, 0.0, 0.0, false},
        ArcadeSurfaceSample{0.0, 0.0, 0.0, true},
        ArcadeSurfaceSample{-1.0, 0.0, 0.0, true},
        ArcadeSurfaceSample{0.11, 0.0, 0.0, true},
        ArcadeSurfaceSample{nan, 0.0, 0.0, true},
        ArcadeSurfaceSample{inf, 0.0, 0.0, true},
        ArcadeSurfaceSample{0.05, nan, 0.0, true},
        ArcadeSurfaceSample{0.05, 0.0, -inf, true},
        ArcadeSurfaceSample{0.05, 25000.01, 0.0, true},
        ArcadeSurfaceSample{0.05, 0.0, -25000.01, true}
    })
    {
        surface.Reset();
        Step(surface, 0.0, 0.0);
        Step(surface, 0.0, 0.0);
        Dirt(Step(surface, 0.0, 0.0));
        const auto failed = surface.Observe(bad);
        Check(!failed.Valid && !failed.OffPavement &&
              failed.MotorMultiplier == 0.0 &&
              failed.LateralGripMultiplier == 0.0 &&
              !surface.HasValidSample() && !surface.IsOffPavement() &&
              surface.PendingConfirmationSeconds() == 0.0,
              "invalid samples must fail closed and clear hysteresis");
        Road(Step(surface, EastX, 0.0));
    }

    // Ordinary reset and recovery cannot preserve old off-road state.
    Step(surface, 0.0, 0.0);
    Step(surface, 0.0, 0.0);
    Dirt(Step(surface, 0.0, 0.0));
    surface.Suspend();
    Road(Step(surface, EastX, 0.0));

    // 250000 alternating on-/off-road frames across 1000 fresh races.
    // These fixtures do not run Unreal or simulate real tire contact.
    for (int session = 0; session < 1000; ++session)
    {
        surface.Reset();
        for (int frame = 0; frame < 250; ++frame)
        {
            const bool onRoad = (frame / 13) % 2 == 0;
            const auto output = Step(
                surface, onRoad ? EastX : 0.0,
                onRoad ? (frame % 2 ? 350.0 : -350.0) : 0.0, 0.016);
            Check(output.Valid && std::isfinite(output.MotorMultiplier) &&
                  std::isfinite(output.LateralGripMultiplier) &&
                  output.MotorMultiplier > 0.0 &&
                  output.MotorMultiplier <= 1.0 &&
                  output.LateralGripMultiplier > 0.0 &&
                  output.LateralGripMultiplier <= 1.0,
                  "road traction cannot amplify, invert, or corrupt forces");
        }
        surface.Suspend();
        Check(!surface.HasValidSample() &&
              !surface.IsOffPavement() &&
              surface.PendingConfirmationSeconds() == 0.0,
              "session boundary must clear pending hysteresis");
    }
    std::cout << "ArcadeSurfaceTraction passed 250000 source frames.\n";
    return EXIT_SUCCESS;
}
