#include "Vehicles/ArcadeBrakeHeat.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Check(bool ok, const char* description)
{
    if (!ok)
    {
        throw std::runtime_error(description);
    }
}

bool Nearly(double lhs, double rhs, double epsilon = 1e-10)
{
    return std::abs(lhs - rhs) <= epsilon;
}
} // namespace

int main()
{
    using latebrake::ArcadeBrakeHeat;
    using latebrake::BrakeHeatSample;

    ArcadeBrakeHeat brakes;
    Check(!brakes.HasValidSample(), "new thermal history must be invalid");
    Check(Nearly(brakes.HeatFraction(), 0.20), "initial heat");
    Check(Nearly(brakes.ServiceBrakeMultiplier(), 1.0), "cold brake");

    const auto cold = brakes.Observe({0.05, 5000.0, true, true});
    Check(cold.Valid && !cold.Faded, "single valid frame must not fade");
    Check(cold.HeatFraction > 0.20, "service braking heats");
    Check(Nearly(cold.ServiceBrakeMultiplier, 1.0), "no cold fade");

    latebrake::BrakeHeatOutput hot;
    for (int frame = 0; frame < 400; ++frame)
    {
        hot = brakes.Observe({0.05, 6000.0, true, true});
        Check(hot.Valid, "valid hot frame");
        Check(hot.HeatFraction >= ArcadeBrakeHeat::IdleHeat &&
              hot.HeatFraction <= 1.0, "bounded heat");
        Check(hot.ServiceBrakeMultiplier >=
              ArcadeBrakeHeat::MinimumBrakeMultiplier &&
              hot.ServiceBrakeMultiplier <= 1.0, "bounded brake force");
    }
    Check(hot.HeatFraction > ArcadeBrakeHeat::FadeThreshold,
          "sustained high-speed braking heats enough to fade");
    Check(hot.Faded && hot.ServiceBrakeMultiplier < 1.0,
          "sustained braking reduces service brake");
    const double beforeCoolingHeat = hot.HeatFraction;
    const double beforeCoolingMultiplier = hot.ServiceBrakeMultiplier;

    for (int frame = 0; frame < 300; ++frame)
    {
        hot = brakes.Observe({0.05, 6000.0, false, true});
    }
    Check(hot.Valid && hot.HeatFraction < beforeCoolingHeat,
          "release must cool brake history");
    Check(hot.ServiceBrakeMultiplier >= beforeCoolingMultiplier,
          "cooling restores service brake force");
    const double retainedHeat = hot.HeatFraction;

    const BrakeHeatSample invalid[] = {
        {0.0, 3000.0, true, true},
        {-0.05, 3000.0, true, true},
        {0.101, 3000.0, true, true},
        {std::numeric_limits<double>::infinity(), 3000.0, true, true},
        {0.05, std::numeric_limits<double>::quiet_NaN(), true, true},
        {0.05, -std::numeric_limits<double>::infinity(), true, true},
        {0.05, 10001.0, true, true},
        {0.05, 2000.0, true, false}
    };
    for (const auto& sample : invalid)
    {
        const auto output = brakes.Observe(sample);
        Check(!output.Valid && !output.Faded,
              "invalid frame cannot report fading");
        Check(Nearly(output.ServiceBrakeMultiplier, 1.0),
              "invalid frame returns neutral brake multiplier");
        Check(Nearly(brakes.ServiceBrakeMultiplier(), 1.0),
              "invalid state has neutral getter");
        Check(Nearly(brakes.HeatFraction(), retainedHeat),
              "invalid frame freezes heat history");
    }

    const auto recovered = brakes.Observe({0.05, -5000.0, false, true});
    Check(recovered.Valid && recovered.HeatFraction <= retainedHeat,
          "valid reverse rolling cools preserved history");

    brakes.Reset();
    Check(!brakes.HasValidSample(), "reset discards validity");
    Check(Nearly(brakes.HeatFraction(), 0.20), "reset discards old heat");
    Check(Nearly(brakes.ServiceBrakeMultiplier(), 1.0), "reset cold brake");

    // Handbrake-only input is represented by ServiceBrakeHeld=false.
    for (int i = 0; i < 200; ++i)
    {
        const auto output = brakes.Observe({0.05, 5000.0, false, true});
        Check(!output.Faded, "handbrake-only cannot trigger service fade");
    }
    Check(Nearly(brakes.HeatFraction(), ArcadeBrakeHeat::IdleHeat),
          "handbrake-only does not generate service heat");

    // Long, bounded histories must not produce NaN, overflow or
    // a service-brake multiplier above the ordinary 100% resistance.
    for (int i = 0; i < 250000; ++i)
    {
        const auto output = brakes.Observe(
            {0.05, (i % 2 == 0) ? 9800.0 : 1000.0, i % 3 != 0, true});
        Check(output.Valid && std::isfinite(output.HeatFraction) &&
              std::isfinite(output.ServiceBrakeMultiplier),
              "long-run finite thermal state");
        Check(output.ServiceBrakeMultiplier >=
              ArcadeBrakeHeat::MinimumBrakeMultiplier &&
              output.ServiceBrakeMultiplier <= 1.0,
              "long-run bounded resistance");
    }

    std::cout << "ArcadeBrakeHeat: all scenarios passed\n";
    return 0;
}
