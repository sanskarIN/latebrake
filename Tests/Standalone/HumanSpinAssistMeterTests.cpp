#include "Vehicles/HumanSpinAssistMeter.h"

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
        std::cerr << "Human spin-assist meter FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double actual, double expected, double error = 1e-10)
{
    return std::abs(actual - expected) <= error;
}
} // namespace

int main()
{
    using latebrake::HumanSpinAssistMeter;
    using latebrake::HumanSpinAssistSample;
    HumanSpinAssistMeter meter;
    Require(!meter.IsActive() && Near(meter.CurrentReductionFraction(), 0.0) &&
            Near(meter.PeakReductionFraction(), 0.0) &&
            meter.InterventionCount() == 0u,
            "fresh race starts with neutral driver feedback");

    Require(meter.Observe({0.016, 0.8, 0.4, true}) &&
            meter.IsActive() &&
            Near(meter.CurrentReductionFraction(), 0.5) &&
            Near(meter.PeakReductionFraction(), 0.5) &&
            meter.InterventionCount() == 1u,
            "valid actual final throttle produces correct live percentage");
    Require(meter.Observe({0.016, 0.8, 0.4, true}) &&
            meter.InterventionCount() == 1u,
            "sustained assist is one intervention, not one per frame");
    Require(meter.Observe({0.016, 0.8, 0.79, true}) &&
            !meter.IsActive() &&
            Near(meter.CurrentReductionFraction(), 0.0) &&
            meter.InterventionCount() == 1u &&
            Near(meter.PeakReductionFraction(), 0.5),
            "invisible small trim clears live display but preserves peak");
    Require(meter.Observe({0.016, 0.8, 0.24, true}) &&
            meter.IsActive() && Near(meter.CurrentReductionFraction(), 0.70) &&
            meter.InterventionCount() == 2u,
            "new strong intervention caps at expected policy maximum");
    Require(meter.Observe({0.016, 0.8, 0.76, true}) &&
            !meter.IsActive() && meter.InterventionCount() == 2u,
            "sub-five-percent motor trim remains below visual threshold");
    Require(meter.Observe({0.016, 0.8, 0.7599, true}) &&
            meter.IsActive() && meter.InterventionCount() == 3u,
            "crossing five-percent threshold activates bounded display");

    // Losing driving authority, a game pause or focus loss must immediately
    // erase the current highlight without inventing a new racing record.
    Require(!meter.Observe({0.016, 0.8, 0.4, false}) &&
            !meter.IsActive() &&
            Near(meter.CurrentReductionFraction(), 0.0) &&
            Near(meter.PeakReductionFraction(), 0.70) &&
            meter.InterventionCount() == 3u,
            "menu/focus interruption suspends live reading only");
    meter.Observe({0.016, 0.8, 0.4, true});
    meter.Suspend();
    Require(!meter.IsActive() && meter.InterventionCount() == 4u,
            "explicit interrupted input clears live indicator");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, -inf})
    {
        for (const HumanSpinAssistSample sample : {
                 HumanSpinAssistSample{invalid, 0.8, 0.4, true},
                 HumanSpinAssistSample{0.016, invalid, 0.4, true},
                 HumanSpinAssistSample{0.016, 0.8, invalid, true}})
        {
            Require(!meter.Observe(sample) && !meter.IsActive() &&
                    Near(meter.CurrentReductionFraction(), 0.0),
                    "invalid finite frame and throttle measurements fail closed");
        }
    }
    for (const HumanSpinAssistSample rejected : {
             HumanSpinAssistSample{-0.1, 0.8, 0.4, true},
             HumanSpinAssistSample{0.0, 0.8, 0.4, true},
             HumanSpinAssistSample{0.10001, 0.8, 0.4, true},
             HumanSpinAssistSample{0.016, 0.0, 0.0, true},
             HumanSpinAssistSample{0.016, 0.01, 0.005, true},
             HumanSpinAssistSample{0.016, 1.01, 0.5, true},
             HumanSpinAssistSample{0.016, 0.8, -0.1, true},
             HumanSpinAssistSample{0.016, 0.8, 0.9, true},
             HumanSpinAssistSample{0.016, 0.8, 0.1, true}})
    {
        Require(!meter.Observe(rejected) && !meter.IsActive(),
                "invalid/out-of-policy throttle samples never display a false assist");
    }
    Require(meter.Observe({0.10, 0.8, 0.4, true}) &&
            meter.IsActive(), "maximum physical frame step is inclusive");
    Require(!meter.Observe({0.101, 0.8, 0.4, true}) &&
            !meter.IsActive(), "unqualified hitch never advertises control action");

    // Exhaust activation counter and numeric boundary with deterministic
    // rapidly alternating live frames. No allocation, wall-clock or engine.
    meter.Reset();
    for (int frame = 0; frame < 200000; ++frame)
    {
        const double throttle = (frame % 100 + 1) / 100.0;
        const double reduction = (frame % 71) / 100.0;
        if (throttle <= 0.01)
        {
            Require(!meter.Observe({0.016, throttle, throttle * (1.0 - reduction), true}),
                    "near-zero throttle is not an observable motor trim");
        }
        else
        {
            Require(meter.Observe({
                    0.016, throttle, throttle * (1.0 - reduction), true}),
                    "200k validated source samples remain accepted");
        }
        Require(std::isfinite(meter.CurrentReductionFraction()) &&
                meter.CurrentReductionFraction() >= 0.0 &&
                meter.CurrentReductionFraction() <= 0.70 + 1e-10 &&
                meter.InterventionCount() <= HumanSpinAssistMeter::MaxInterventions,
                "long race display remains bounded and finite");
    }
    Require(meter.InterventionCount() > 0u &&
            Near(meter.PeakReductionFraction(), 0.70),
            "long source telemetry stream retains valid session peak");

    meter.Reset();
    Require(!meter.IsActive() && meter.InterventionCount() == 0u &&
            Near(meter.PeakReductionFraction(), 0.0) &&
            Near(meter.CurrentReductionFraction(), 0.0),
            "race restart discards old session counters and last frame");
    std::cout << "Human spin-assist display telemetry passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
