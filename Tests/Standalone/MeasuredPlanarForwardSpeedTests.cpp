#include "Vehicles/MeasuredPlanarForwardSpeed.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "MeasuredPlanarForwardSpeed FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double actual, double expected, double epsilon = 1.0e-9)
{
    return std::isfinite(actual) &&
        std::abs(actual - expected) <= epsilon;
}
}

int main()
{
    using latebrake::MeasuredPlanarForwardSpeedKmh;
    Require(Near(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, 1.0, 0.0), 3.6),
        "flat forward road speed is in signed km/h");
    Require(Near(MeasuredPlanarForwardSpeedKmh(-100.0, 0.0, 1.0, 0.0), -3.6),
        "reverse motion remains negative");
    Require(Near(MeasuredPlanarForwardSpeedKmh(0.0, 125.0, 0.0, 1.0), 4.5),
        "yaw-rotated road speed is measured forward");
    Require(Near(MeasuredPlanarForwardSpeedKmh(500.0, 100.0, 0.0, 1.0), 3.6),
        "pure lateral sideslip never counts as forward launch");
    Require(Near(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, 0.6, 0.0), 3.6),
        "pitched car uses normalized horizontal heading, not cos(pitch)");
    Require(Near(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, -0.6, 0.0), -3.6),
        "inverted planar heading preserves sign rather than speed magnitude");
    Require(Near(MeasuredPlanarForwardSpeedKmh(0.0, 0.0, 0.6, 0.0), 0.0),
        "vertical-only descent has zero road speed, not a false launch");

    const double bad = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    for (const auto value : {bad, infinity, -infinity})
    {
        Require(std::isnan(MeasuredPlanarForwardSpeedKmh(value, 0.0, 1.0, 0.0)),
            "nonfinite horizontal velocity rejects the frame");
        Require(std::isnan(MeasuredPlanarForwardSpeedKmh(0.0, value, 1.0, 0.0)),
            "nonfinite lateral velocity rejects the frame");
        Require(std::isnan(MeasuredPlanarForwardSpeedKmh(1.0, 2.0, value, 0.0)),
            "nonfinite heading X rejects the frame");
        Require(std::isnan(MeasuredPlanarForwardSpeedKmh(1.0, 2.0, 1.0, value)),
            "nonfinite heading Y rejects the frame");
    }
    Require(std::isnan(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, 0.0, 0.0)),
        "fully vertical body has no well-defined road heading");
    Require(std::isnan(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, 0.00001, 0.0)),
        "almost vertical body must not invent a horizontal launch direction");
    Require(std::isnan(MeasuredPlanarForwardSpeedKmh(100.0, 0.0, 1.2, 0.0)),
        "corrupt nonunit physics heading rejects rather than boosting speed");

    // 120k deterministic yaw/pitch/sideslip frames, with alternating
    // forward/reverse movement. The apparent vertical chassis velocity
    // never enters this road-speed-only policy.
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < 120000; ++i)
    {
        const double yaw = (i % 720) * pi / 360.0;
        const double projection = 0.25 + 0.75 * (i % 13) / 12.0;
        const double fx = std::cos(yaw);
        const double fy = std::sin(yaw);
        const double longitudinal = (i % 2 ? -1.0 : 1.0) * (i % 380);
        const double lateral = (i % 41) - 20.0;
        const double vx = longitudinal * fx - lateral * fy;
        const double vy = longitudinal * fy + lateral * fx;
        const double observed = MeasuredPlanarForwardSpeedKmh(
            vx, vy, projection * fx, projection * fy);
        Require(Near(observed, longitudinal * 0.036, 1.0e-8),
            "yaw, pitch and lateral slip must not contaminate launch speed");
    }
    std::cout << "Measured planar forward speed passed 120k frames.\n";
    return EXIT_SUCCESS;
}
