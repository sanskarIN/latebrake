#include "Vehicles/ArcadeReverseSpeedLimit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool valid, const char* description)
{
    if (!valid)
    {
        std::cerr << "ReverseSpeedLimit FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Close(double a, double b, double tolerance = 1e-8)
{
    return std::abs(a - b) <= tolerance;
}
} // namespace

int main()
{
    using latebrake::LimitPlanarReverseSpeed;
    using latebrake::MaximumReverseSpeedCmPerSecond;

    const double arcadeCap = MaximumReverseSpeedCmPerSecond(true);
    const double simulationCap = MaximumReverseSpeedCmPerSecond(false);
    Require(Close(arcadeCap, 1350.0) && Close(simulationCap, 1000.0),
            "reverse speed caps are fixed by handling mode");
    Require(arcadeCap < 6500.0 && simulationCap < 5400.0,
            "reverse speeds stay below their respective forward caps");

    const auto rest = LimitPlanarReverseSpeed(0.0, 0.0, 1.0, 0.0, true);
    Require(rest.Valid && !rest.Limited &&
            Close(rest.VelocityX, 0.0) && Close(rest.VelocityY, 0.0),
            "car at rest must stay at rest");

    const auto forward = LimitPlanarReverseSpeed(7900.0, 200.0, 1.0, 0.0, true);
    Require(forward.Valid && !forward.Limited &&
            Close(forward.VelocityX, 7900.0) && Close(forward.VelocityY, 200.0),
            "this policy cannot cap forward cruising or drifting");

    const auto withinReverse = LimitPlanarReverseSpeed(
        -1000.0, 300.0, 1.0, 0.0, true);
    Require(withinReverse.Valid && !withinReverse.Limited &&
            Close(withinReverse.VelocityX, -1000.0) &&
            Close(withinReverse.VelocityY, 300.0),
            "normal low-speed reversing stays unchanged");

    const auto backwards = LimitPlanarReverseSpeed(
        -3800.0, 750.0, 1.0, 0.0, true);
    Require(backwards.Valid && backwards.Limited &&
            Close(backwards.VelocityX, -arcadeCap) &&
            Close(backwards.VelocityY, 750.0),
            "reverse cap removes only excessive backward velocity");

    const auto sim = LimitPlanarReverseSpeed(
        -3800.0, 750.0, 1.0, 0.0, false);
    Require(sim.Valid && sim.Limited &&
            Close(sim.VelocityX, -simulationCap) &&
            Close(sim.VelocityY, 750.0),
            "simulation mode has the tighter reverse cap");

    const auto turned = LimitPlanarReverseSpeed(
        1000.0, -3000.0, 0.0, 2.0, true);
    Require(turned.Valid && turned.Limited &&
            Close(turned.VelocityX, 1000.0) &&
            Close(turned.VelocityY, -arcadeCap),
            "real yaw determines reverse direction, not world X");

    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto infinity = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, infinity, -infinity})
    {
        Require(!LimitPlanarReverseSpeed(invalid, 0.0, 1.0, 0.0, true).Valid &&
                !LimitPlanarReverseSpeed(0.0, invalid, 1.0, 0.0, true).Valid &&
                !LimitPlanarReverseSpeed(0.0, 0.0, invalid, 0.0, true).Valid &&
                !LimitPlanarReverseSpeed(0.0, 0.0, 1.0, invalid, true).Valid,
                "nonfinite physics and orientation must fail closed");
    }
    Require(!LimitPlanarReverseSpeed(100.0, 50.0, 0.0, 0.0, true).Valid &&
            !LimitPlanarReverseSpeed(100.0, 50.0, 0.001, 0.0, true).Valid,
            "degenerate planar heading must fail closed");

    // A broad deterministic range of headings/speeds. Real chassis yaw
    // may vary independently from world heading and is not always aligned
    // to X/Y; normalizing the forward XY input must preserve lateral slip.
    for (int i = 0; i < 200000; ++i)
    {
        const bool arcade = (i % 2) == 0;
        const double cap = MaximumReverseSpeedCmPerSecond(arcade);
        const double angle = 0.000131 * static_cast<double>(i);
        const double fx = std::cos(angle);
        const double fy = std::sin(angle);
        const double requestedLongitudinal =
            static_cast<double>(i % 12501) - 6000.0;
        const double lateral = static_cast<double>((i * 67) % 6001) - 3000.0;
        const double velocityX = requestedLongitudinal * fx - lateral * fy;
        const double velocityY = requestedLongitudinal * fy + lateral * fx;
        const auto result = LimitPlanarReverseSpeed(
            velocityX, velocityY, fx * 1.5, fy * 1.5, arcade);
        const double expectedLongitudinal =
            requestedLongitudinal < -cap ? -cap : requestedLongitudinal;
        const double outputLongitudinal =
            result.VelocityX * fx + result.VelocityY * fy;
        const double outputLateral =
            -result.VelocityX * fy + result.VelocityY * fx;
        Require(result.Valid &&
                result.Limited == (requestedLongitudinal < -cap) &&
                std::isfinite(result.VelocityX) &&
                std::isfinite(result.VelocityY) &&
                std::abs(outputLongitudinal - expectedLongitudinal) < 1e-7 &&
                std::abs(outputLateral - lateral) < 1e-7,
                "200000 frames preserve lateral motion while limiting reverse");
    }

    std::cout << "ReverseSpeedLimit: 200000 chassis samples passed (no UE)\n";
    return EXIT_SUCCESS;
}
