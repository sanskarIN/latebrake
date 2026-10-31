#include "Vehicles/HumanSpinTractionControl.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool valid, const char* message)
{
    if (!valid)
    {
        std::cerr << "Human spin traction FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double lhs, double rhs, double epsilon = 1e-9)
{
    return std::abs(lhs - rhs) <= epsilon;
}
} // namespace

int main()
{
    using latebrake::ModerateHumanSpinThrottle;

    const auto output = [](double throttle, double forward, double lateral,
                           double yaw, bool arcade = true,
                           bool human = true, bool handbrake = false) {
        return ModerateHumanSpinThrottle(throttle, forward, lateral, yaw,
                                         arcade, human, handbrake);
    };

    Require(Near(output(0.85, 3000, 0, 3.0), 0.85),
            "straight-line fast yaw without lateral slip keeps throttle");
    Require(Near(output(0.80, 3000, 1800, 0.1), 0.80),
            "large ordinary sideways movement without a spin is unchanged");
    Require(Near(output(0.75, 1000, 800, 3.0), 0.75),
            "low-speed driving keeps existing positive motor authority");
    Require(Near(output(0.60, 3000, 1800, 3.0, false), 0.60),
            "Simulation Inspired keeps unassisted driving behavior");
    Require(Near(output(0.60, 3000, 1800, 3.0, true, false), 0.60),
            "AI rival is untouched by human-only traction policy");
    Require(Near(output(0.60, 3000, 1800, 3.0, true, true, true), 0.60),
            "held human handbrake still permits intentional slides");
    Require(Near(output(-0.75, 3000, 1800, 3.0), -0.75) &&
            Near(output(0.0, 3000, 1800, 3.0), 0.0),
            "reverse, engine braking and neutral demand are preserved");
    Require(Near(output(2.0, 3000, 0, 0.0), 1.0) &&
            Near(output(-2.0, 3000, 0, 0.0), -1.0),
            "unexpected input axes stay within existing unit bounds");

    const double severe = output(1.0, 3000.0, 1800.0, 3.0);
    Require(Near(severe, 0.30),
            "maximum qualifying spin keeps bounded 30-percent positive throttle");
    Require(output(0.5, 3000.0, 1800.0, 3.0) > 0.0 &&
            Near(output(0.5, 3000.0, 1800.0, 3.0), severe * 0.5),
            "human analog half-throttle is not treated as a full-throttle switch");
    Require(Near(output(1.0, 3000.0, 1800.0, -3.0), severe) &&
            Near(output(1.0, 3000.0, -1800.0, 3.0), severe),
            "left/right spins receive symmetrical measured-physics treatment");

    // Exactly-on-threshold no-slip, no-yaw, or not-yet-fast conditions
    // must produce no abrupt throttle step. The response is continuous.
    for (double delta : {0.0, 1e-6, 0.0001, 0.01})
    {
        const double atSpeed = output(1.0, 1400.0 + delta, 1800, 3.0);
        const double atYaw = output(1.0, 3000, 1800, 1.10 + delta);
        const double atSlip = output(1.0, 3000, 1000 + delta, 3.0);
        Require(atSpeed >= 1.0 - delta * 0.002 &&
                atYaw >= 1.0 - delta * 0.001 &&
                atSlip > 0.30 && atSlip <= 1.0,
                "threshold blends must avoid abrupt reversals");
    }

    // Higher measured yaw/slip/speed cannot give MORE propulsion in the
    // active intervention range. Never boost beyond the original request.
    double prior = 1.0;
    for (int i = 0; i <= 1000; ++i)
    {
        const double yaw = 1.10 + 0.005 * i;
        const double now = output(1.0, 3000, 1800, yaw);
        Require(now <= prior + 1e-12 && now >= 0.30 - 1e-12,
                "increasing yaw cannot increase throttle");
        prior = now;
    }
    prior = 1.0;
    for (int i = 0; i <= 1000; ++i)
    {
        const double lateral = 1000.0 + i * 10.0;
        const double now = output(1.0, 3000, lateral, 3.0);
        Require(now <= prior + 1e-12 && now >= 0.30 - 1e-12,
                "increasing lateral slip cannot increase throttle");
        prior = now;
    }

    const double bad[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()
    };
    for (double value : bad)
    {
        Require(Near(output(value, 3000, 1800, 3.0), 0.0) &&
                Near(output(1.0, value, 1800, 3.0), 0.0) &&
                Near(output(1.0, 3000, value, 3.0), 0.0) &&
                Near(output(1.0, 3000, 1800, value), 0.0),
                "corrupt throttle or angular/linear physics fails closed");
    }

    // Adversarial large-finite speed arithmetic must never emit NAN/INF
    // or unauthorized extra propulsion if hypot overflows.
    Require(Near(output(1.0, 1e308, 1e308, 3.0), 0.0),
            "unrepresentable planar speed produces neutral propulsion");

    // Exercise a long deterministic variable analog/physics stream without
    // allocations or hidden controller history, including steering reversals.
    double checksum = 0.0;
    for (int i = 0; i < 250000; ++i)
    {
        const double forward = 1400.0 + static_cast<double>(i % 3200);
        const double lateral = static_cast<double>((i * 7) % 5000 - 2500);
        const double yaw = static_cast<double>((i * 13) % 701 - 350) / 100.0;
        const double throttle = static_cast<double>(i % 101) / 100.0;
        const double result = output(throttle, forward, lateral, yaw);
        Require(std::isfinite(result) && result >= 0.0 && result <= throttle,
                "250k measured-physics samples remain finite and never accelerate");
        checksum += result;
    }
    Require(std::isfinite(checksum) && checksum > 0.0,
            "deterministic stream completes with finite result");

    std::cout << "Human physical spin throttle policy passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
