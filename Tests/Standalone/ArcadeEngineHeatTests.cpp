#include "Vehicles/ArcadeEngineHeat.h"

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
        std::cerr << "ArcadeEngineHeat FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1e-10; }
} // namespace

int main()
{
    using latebrake::ArcadeEngineHeat;
    using latebrake::EngineHeatSample;
    ArcadeEngineHeat heat;
    Require(!heat.HasValidSample() &&
            Near(heat.HeatFraction(), ArcadeEngineHeat::IdleHeat) &&
            Near(heat.MotorMultiplier(), 1.0), "new state is safe");

    auto output = heat.Observe({0.05, 3000.0, 1.0, false, true});
    Require(output.Valid && output.HeatFraction > ArcadeEngineHeat::IdleHeat &&
            !output.Derated, "one throttle tick is not overheating");
    for (int i = 0; i < 1200; ++i)
    {
        output = heat.Observe({0.05, 3000.0, 1.0, false, true});
        Require(output.Valid && std::isfinite(output.HeatFraction) &&
                output.HeatFraction >= ArcadeEngineHeat::IdleHeat &&
                output.HeatFraction <= 1.0 &&
                output.MotorMultiplier >= ArcadeEngineHeat::MinimumMotorMultiplier &&
                output.MotorMultiplier <= 1.0, "bounded hot frame");
    }
    Require(output.Derated && output.HeatFraction >
            ArcadeEngineHeat::DerateThreshold &&
            output.MotorMultiplier < 1.0, "sustained demand derates motor");
    const double peak = output.HeatFraction;
    const double power = output.MotorMultiplier;
    for (int i = 0; i < 1200; ++i)
    {
        output = heat.Observe({0.05, 5000.0, 0.0, false, true});
    }
    Require(output.HeatFraction < peak && output.MotorMultiplier > power,
            "coasting returns motor power");

    heat.Reset();
    for (int i = 0; i < 450; ++i)
    {
        heat.Observe({0.05, 3200.0, 1.0, true, true});
    }
    const double nitroHot = heat.HeatFraction();
    heat.Reset();
    for (int i = 0; i < 450; ++i)
    {
        heat.Observe({0.05, -3200.0, -1.0, false, true});
    }
    Require(nitroHot > heat.HeatFraction(), "nitro adds load heat");
    const double remembered = heat.HeatFraction();
    const EngineHeatSample invalid[] = {
        {0.0, 2000.0, 1.0, false, true},
        {-0.01, 2000.0, 1.0, false, true},
        {0.101, 2000.0, 1.0, false, true},
        {std::numeric_limits<double>::infinity(), 2000.0, 1.0, false, true},
        {0.05, std::numeric_limits<double>::quiet_NaN(), 1.0, false, true},
        {0.05, 10001.0, 1.0, false, true},
        {0.05, 2000.0, std::numeric_limits<double>::quiet_NaN(), false, true},
        {0.05, 2000.0, -1.01, false, true},
        {0.05, 2000.0, 1.01, false, true},
        {0.05, 2000.0, 1.0, false, false},
    };
    for (const auto& frame : invalid)
    {
        output = heat.Observe(frame);
        Require(!output.Valid && !output.Derated &&
                Near(output.HeatFraction, remembered) &&
                Near(output.MotorMultiplier, 1.0) &&
                Near(heat.HeatFraction(), remembered) &&
                Near(heat.MotorMultiplier(), 1.0),
                "invalid frame cannot apply stale penalty or alter heat");
    }
    output = heat.Observe({0.05, 1000.0, 0.0, false, true});
    Require(output.Valid && output.HeatFraction < remembered,
            "valid cooldown resumes after invalid frames");
    heat.Suspend();
    Require(!heat.HasValidSample() && Near(heat.MotorMultiplier(), 1.0),
            "suspended state has no active penalty");
    heat.Reset();
    Require(!heat.HasValidSample() &&
            Near(heat.HeatFraction(), ArcadeEngineHeat::IdleHeat),
            "reset is race-local");
    for (int i = 0; i < 200000; ++i)
    {
        output = heat.Observe({0.025, 5800.0,
            i % 2 ? -1.0 : 1.0, i % 7 == 0, true});
        Require(output.Valid && std::isfinite(output.HeatFraction) &&
                std::isfinite(output.MotorMultiplier) &&
                output.HeatFraction >= ArcadeEngineHeat::IdleHeat &&
                output.HeatFraction <= 1.0 &&
                output.MotorMultiplier >= ArcadeEngineHeat::MinimumMotorMultiplier &&
                output.MotorMultiplier <= 1.0,
                "200k frames remain finite and bounded");
    }
    std::cout << "ArcadeEngineHeat: all tests passed\n";
    return EXIT_SUCCESS;
}
