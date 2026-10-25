#include "Vehicles/SpeedSensitiveSteering.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool passed, const char* detail)
{
    if (!passed)
    {
        std::cerr << "Speed-sensitive steering FAIL: " << detail << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1e-12;
}
}

int main()
{
    using latebrake::LimitSteeringAtSpeed;
    using latebrake::SteeringSpeedFactor;

    for (bool arcade : {false, true})
    {
        Require(Close(SteeringSpeedFactor(0.0, arcade), 1.0),
                "full steering authority at rest");
        Require(Close(SteeringSpeedFactor(900.0, arcade), 1.0),
                "full authority at low-speed threshold");
        Require(Close(LimitSteeringAtSpeed(0.5, 100.0, arcade), 0.5),
                "low-speed proportional steering");
        Require(Close(LimitSteeringAtSpeed(-0.7, 100.0, arcade), -0.7),
                "negative steering sign preserved");
        Require(Close(LimitSteeringAtSpeed(9.0, 0.0, arcade), 1.0),
                "oversized controls clamped before physics");
        Require(Close(LimitSteeringAtSpeed(-9.0, 0.0, arcade), -1.0),
                "negative oversized controls clamped");
        Require(Close(LimitSteeringAtSpeed(0.0, 5400.0, arcade), 0.0),
                "released steering stays neutral at speed");
        const double minimum = arcade ? 0.55 : 0.40;
        Require(Close(SteeringSpeedFactor(5400.0, arcade), minimum),
                "high-speed authority matches handling-specific floor");
        Require(Close(SteeringSpeedFactor(9000.0, arcade), minimum),
                "factor never decreases below the defined floor");
        Require(Close(SteeringSpeedFactor(
                          std::numeric_limits<double>::max(), arcade), minimum),
                "enormous but finite speed does not overflow or increase torque");
        Require(Close(SteeringSpeedFactor(3150.0, arcade),
                      (1.0 + minimum) / 2.0),
                "midpoint uses continuous linear interpolation");
        Require(Close(LimitSteeringAtSpeed(-1.0, 5400.0, arcade), -minimum),
                "negative high-speed input preserves direction and cap");

        // The curve cannot gain steering authority when speed increases.
        double previous = SteeringSpeedFactor(0.0, arcade);
        for (int speed = 1; speed <= 10000; ++speed)
        {
            const double next = SteeringSpeedFactor(
                static_cast<double>(speed), arcade);
            Require(std::isfinite(next) && next > 0.0 &&
                        next <= previous + 1e-14,
                    "curve stays finite, positive and monotonic");
            previous = next;
        }
    }
    Require(LimitSteeringAtSpeed(1.0, 4000.0, true) >
                LimitSteeringAtSpeed(1.0, 4000.0, false),
            "Arcade retains greater authority than Simulation Inspired");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (const double bad : {nan, inf, -inf, -1.0})
    {
        Require(Close(SteeringSpeedFactor(bad, true), 0.0) &&
                    Close(SteeringSpeedFactor(bad, false), 0.0),
                "invalid speed fails closed");
        Require(Close(LimitSteeringAtSpeed(1.0, bad, true), 0.0),
                "bad speed cannot produce steering command");
    }
    for (const double bad : {nan, inf, -inf})
    {
        Require(Close(LimitSteeringAtSpeed(bad, 1200.0, true), 0.0) &&
                    Close(LimitSteeringAtSpeed(bad, 1200.0, false), 0.0),
                "invalid steering controls cannot generate torque");
    }

    std::cout << "Speed-sensitive steering tests passed (no Unreal).\n";
    return EXIT_SUCCESS;
}
