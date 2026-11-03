#include "Vehicles/SprungChassisDynamics.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

// This is an ISOLATED one-dimensional vertical-body integration fixture.
// It exercises the *actual portable production spring policy* at different
// sample rates; it does not simulate UE5 Chaos collision, tire friction,
// player input, articulated wheels, or an installed game on hardware.
namespace
{
void Require(bool success, const char* reason)
{
    if (!success)
    {
        std::cerr << "SprungChassisRideHeight FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct Result
{
    double FinalHeight = 0.0;
    double FinalVelocity = 0.0;
    double MinimumHeight = 1.0e12;
    double MaximumForce = 0.0;
    int ContactFrames = 0;
};
Result Integrate(double initialHeight, int fps,
                 double massKg, double gravity)
{
    using namespace latebrake;
    const double delta = 1.0 / static_cast<double>(fps);
    double height = initialHeight;
    double velocity = 0.0;
    Result result;

    for (int step = 0; step < fps * 6; ++step)
    {
        SuspensionFrame frame;
        frame.PhysicsReady = true;
        frame.DeltaSeconds = delta;
        frame.MassKg = massKg;
        frame.GravityCmPerSecondSquared = gravity;
        for (auto& c : frame.Corners)
        {
            // Infinite perfectly level road plane, an idealized observation
            // of the very same WorldStatic rays used by the Unreal adapter.
            c.Hit = height >= 0.0 && height <=
                SuspensionMaxTraceDistanceCm;
            c.DistanceCm = c.Hit ? height : 0.0;
            c.NormalZ = c.Hit ? 1.0 : 0.0;
            c.PointVerticalSpeedCmPerSecond = velocity;
        }
        const auto support = CalculateSprungChassis(frame);
        Require(support.Valid, "finite mass/gravity/frame must remain accepted");
        if (support.ContactCount > 0) ++result.ContactFrames;
        result.MaximumForce =
            std::max(result.MaximumForce, support.TotalUpForce);
        Require(std::isfinite(support.TotalUpForce),
                "no valid frame can inject infinite spring energy");
        const double acceleration = support.TotalUpForce / massKg - gravity;
        velocity += acceleration * delta;
        height += velocity * delta; // semi-implicit fixed-step Euler
        result.MinimumHeight = std::min(result.MinimumHeight, height);
        Require(std::isfinite(height) && std::isfinite(velocity),
                "height and velocity must stay finite throughout settling");
    }
    result.FinalHeight = height;
    result.FinalVelocity = velocity;
    return result;
}
}

int main()
{
    using namespace latebrake;
    constexpr double chassisHalfHeightCm = 32.5;
    // A typical 1.1-ton racing chassis, from the pawn's actual spawn height
    // (180 cm above the WorldStatic road), must not clip its physical body
    // into the flat ground in this isolated vertical-only reference model.
    for (const int fps : {30, 60, 120})
    {
        for (const double mass : {800.0, 1100.0, 1800.0})
        {
            for (const double height : {80.0, 180.0})
            {
                const auto run = Integrate(height, fps, mass, 980.0);
                Require(run.ContactFrames > fps,
                        "real support must be reached during the grid countdown");
                Require(std::abs(run.FinalHeight -
                    SuspensionTargetRideDistanceCm) < 0.75,
                        "6 seconds of spring loading settle to 59 cm at 30/60/120 FPS");
                Require(std::abs(run.FinalVelocity) < 5.0,
                        "springs dissipate landing speed without perpetual bounce");
                Require(run.MinimumHeight > chassisHalfHeightCm + 2.0,
                        "large default grid drop keeps the rigid chassis clear of ground");
                Require(run.MaximumForce <= mass * 980.0 *
                    SuspensionMaxPerCornerWeightFactor * 4.0 + 0.001,
                        "peak landing force remains within exactly four capped springs");
            }
        }
    }

    // Changes in actual world gravity must preserve the static ride height
    // for gently positioned bodies. This does NOT qualify high-impact
    // driving on arbitrary-gravity Unreal maps.
    for (const int fps : {30, 60, 120})
    {
        for (const double gravity : {490.0, 980.0, 1600.0})
        {
            const auto run = Integrate(80.0, fps, 1100.0, gravity);
            Require(std::abs(run.FinalHeight -
                SuspensionTargetRideDistanceCm) < 0.75,
                "spring equilibrium scales with actual Unreal world gravity");
            Require(std::abs(run.FinalVelocity) < 5.0,
                "alternate-gravity ride-height oscillation damps out");
        }
    }

    std::cout << "Idealized spring heave: fixed-step 30/60/120 FPS, "
                 "grid drop and three gravities passed.\n";
    return EXIT_SUCCESS;
}
