#include "Vehicles/CornerTractionAssist.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* msg)
{
    if (!ok)
    {
        std::cerr << "Corner traction assist FAIL: " << msg << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 1e-9;
}
}

int main()
{
    using namespace latebrake;
    Require(Near(CornerThrottleFactor(0.0, 0.0, true), 1.0) &&
            Near(CornerThrottleFactor(899.0, 1000.0, true), 1.0),
            "launching and low-speed steering retain full motor authority");
    Require(Near(CornerThrottleFactor(3000.0, 249.0, true), 1.0),
            "small sideways drift keeps full throttle");
    Require(Near(CornerThrottleFactor(3000.0, 0.0, false), 1.0),
            "straight-line maximum speed is unchanged");

    const double cornerArcade = CornerThrottleFactor(1800.0, 950.0, true);
    const double cornerSimulation = CornerThrottleFactor(1800.0, 950.0, false);
    Require(cornerArcade < 1.0 && cornerArcade >= 0.58 &&
            cornerSimulation < 1.0 && cornerSimulation >= 0.45,
            "both handling modes smoothly moderate a fast sideways slide");
    Require(cornerSimulation < cornerArcade,
            "simulation-inspired setup is less permissive under same slip");
    Require(Near(CornerThrottleFactor(1500.0, 8000.0, true), 0.58) &&
            Near(CornerThrottleFactor(1500.0, 8000.0, false), 0.45),
            "minimum motor authority is bounded, never negative or above 1");
    for (bool arcade : {true, false})
    {
        Require(Near(CornerThrottleFactor(899.0, 2000.0, arcade), 1.0) &&
                Near(CornerThrottleFactor(900.0, 2000.0, arcade), 1.0) &&
                CornerThrottleFactor(901.0, 2000.0, arcade) > 0.99,
                "forward-speed threshold smoothly blends without throttle step");
        Require(Near(CornerThrottleFactor(1800.0, 249.0, arcade), 1.0) &&
                Near(CornerThrottleFactor(1800.0, 250.0, arcade), 1.0) &&
                CornerThrottleFactor(1800.0, 251.0, arcade) > 0.99,
                "sideways-speed threshold smoothly blends without throttle step");
    }

    for (bool arcade : {true, false})
    {
        double prior = 1.0;
        for (int lateral = 0; lateral < 15000; lateral += 25)
        {
            const double now = CornerThrottleFactor(
                1500.0, static_cast<double>(lateral), arcade);
            Require(now <= prior + 1e-12 && now > 0.0 && now <= 1.0,
                    "traction factor monotonic, positive and <= unity");
            prior = now;
        }
        Require(Near(CornerThrottleFactor(-2200.0, -900.0, arcade),
                     CornerThrottleFactor(2200.0, 900.0, arcade)),
                "traction depends on magnitude, not sign of measured motion");
    }

    Require(Near(StabilizeCornerThrottle(1.0, 1800.0, 950.0, true),
                 cornerArcade),
            "full positive demand is moderated according to actual lateral velocity");
    Require(Near(StabilizeCornerThrottle(0.5, 1800.0, 950.0, true),
                 cornerArcade * 0.5),
            "analog, touch and rival throttle share the same policy");
    Require(Near(StabilizeCornerThrottle(-0.75, 1800.0, 950.0, true), -0.75) &&
            Near(StabilizeCornerThrottle(0.0, 1800.0, 950.0, true), 0.0),
            "reverse and neutral demand remain intact");
    // Real signed motion matters, not merely the sign of the pedal:
    // positive throttle while physically reversing IS engine braking.
    // Old magnitude-only attenuation wrongly reduced its authority
    // during a sideways reverse slide.
    for (bool arcade : {true, false})
    {
        const double movingForward = StabilizeCornerThrottle(
            0.80, 2700.0, 2000.0, arcade);
        const double opposingReverse = StabilizeCornerThrottle(
            0.80, -2700.0, 2000.0, arcade);
        Require(movingForward > 0.0 && movingForward < 0.80 &&
                Near(opposingReverse, 0.80),
                "cornering assist must attenuate forward propulsion but never forward-pedal engine braking while rolling backwards");
        Require(Near(StabilizeCornerThrottle(-0.65, 2700.0, -2000.0, arcade), -0.65),
                "reverse pedal against forward motion retains the full engine braking command");
        Require(Near(StabilizeCornerThrottle(0.50, -2700.0, -2000.0, arcade), 0.50),
                "positive pedal against reverse drift preserves left/right symmetric deceleration");
        for (int i = 0; i < 10000; ++i)
        {
            const double speed = -900.0 - static_cast<double>(i % 5800);
            const double sideways = (static_cast<double>(i % 6400) - 3200.0);
            const double pedal = static_cast<double>(i % 101) / 100.0;
            const auto result = StabilizeCornerThrottle(
                pedal, speed, sideways, arcade);
            Require(std::isfinite(result) && Near(result, pedal),
                    "20k fast reversing, skidding and recovering samples preserve the complete opposite-direction motor braking demand");
        }
    }
    Require(Near(StabilizeCornerThrottle(4.0, 3000.0, 0.0, true), 1.0) &&
            Near(StabilizeCornerThrottle(-5.0, 3000.0, 0.0, true), -1.0),
            "out-of-range raw input is clamped");

    const double bad[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };
    for (double nonfinite : bad)
    {
        Require(Near(CornerThrottleFactor(nonfinite, 100.0, true), 0.0) &&
                Near(CornerThrottleFactor(500.0, nonfinite, false), 0.0),
                "malformed speed cannot produce a positive traction factor");
        Require(Near(StabilizeCornerThrottle(nonfinite, 2000.0, 700.0, true),
                     0.0) &&
                Near(StabilizeCornerThrottle(1.0, nonfinite, 100.0, true),
                     0.0) &&
                Near(StabilizeCornerThrottle(-1.0, 800.0, nonfinite, false),
                     0.0),
                "nonfinite demand or velocity can never authorize engine force");
    }
    std::cout << "Corner traction assist policy passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
