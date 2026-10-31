#include "Vehicles/GearAwareMotorPower.h"
#include "Vehicles/ArcadeBoostArbitration.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "GearAwareMotorPower FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a, double b, double tolerance = 1e-10)
{
    return std::abs(a - b) <= tolerance;
}
} // namespace

int main()
{
    using latebrake::GearAwareMotorPowerFactor;
    using latebrake::MaximumReverseSpeedCmPerSecond;
    using latebrake::AllocateArcadeBoost;

    for (const bool arcade : {true, false})
    {
        const double reverseCap = MaximumReverseSpeedCmPerSecond(arcade);
        const double forwardCap = arcade ? 6500.0 : 5400.0;

        Require(Near(GearAwareMotorPowerFactor(1.0, 0.0, forwardCap, arcade), 1.0) &&
                Near(GearAwareMotorPowerFactor(-1.0, 0.0, forwardCap, arcade), 1.0),
                "full standing-start authority is preserved");
        Require(Near(GearAwareMotorPowerFactor(1.0, 0.82 * forwardCap, forwardCap, arcade), 1.0),
                "full forward force stays available below taper threshold");
        Require(Near(GearAwareMotorPowerFactor(-1.0, -0.74 * reverseCap, forwardCap, arcade), 1.0),
                "full reverse force stays available below taper threshold");

        Require(Near(GearAwareMotorPowerFactor(1.0, forwardCap, forwardCap, arcade), 0.0) &&
                Near(GearAwareMotorPowerFactor(-1.0, -reverseCap, forwardCap, arcade), 0.0),
                "power goes to zero at both gear-specific limits");
        Require(Near(GearAwareMotorPowerFactor(1.0, forwardCap + 1200.0, forwardCap, arcade), 0.0) &&
                Near(GearAwareMotorPowerFactor(-1.0, -reverseCap - 1200.0, forwardCap, arcade), 0.0),
                "speed already above either cap cannot authorize more same-direction motor force");

        Require(Near(GearAwareMotorPowerFactor(1.0, -reverseCap, forwardCap, arcade), 1.0) &&
                Near(GearAwareMotorPowerFactor(-1.0, forwardCap, forwardCap, arcade), 1.0),
                "opposing throttle retains unchanged slowdown and direction reversal");
        Require(Near(GearAwareMotorPowerFactor(0.0, forwardCap, forwardCap, arcade), 1.0),
                "zero throttle cannot create motor force or brake demand");

        const double fwdHalf = forwardCap * (0.82 + (1.0 - 0.82) * 0.5);
        const double revHalf = reverseCap * (0.74 + (1.0 - 0.74) * 0.5);
        Require(Near(GearAwareMotorPowerFactor(1.0, fwdHalf, forwardCap, arcade), 0.5) &&
                Near(GearAwareMotorPowerFactor(-1.0, -revHalf, forwardCap, arcade), 0.5),
                "smoothstep gives a symmetric, half-power transition");
        // Higher garage top-speed should extend the forward envelope only.
        Require(GearAwareMotorPowerFactor(1.0, 5500.0, forwardCap * 1.6, arcade) >
                GearAwareMotorPowerFactor(1.0, 5500.0, forwardCap, arcade),
                "garage higher speed target extends only forward motor envelope");
        Require(Near(
                    GearAwareMotorPowerFactor(-1.0, -reverseCap * 0.88, forwardCap, arcade),
                    GearAwareMotorPowerFactor(-1.0, -reverseCap * 0.88, forwardCap * 1.6, arcade)),
                "garage top-speed tuning cannot increase reverse propulsion budget");

        // No sudden curve jumps as speeds cross the two C1 endpoints.
        double previousForward = 1.0;
        double previousReverse = 1.0;
        for (int sample = 0; sample <= 10000; ++sample)
        {
            const double fraction = static_cast<double>(sample) / 10000.0;
            const double nextForward = GearAwareMotorPowerFactor(
                1.0, forwardCap * fraction, forwardCap, arcade);
            const double nextReverse = GearAwareMotorPowerFactor(
                -1.0, -reverseCap * fraction, forwardCap, arcade);
            Require(std::isfinite(nextForward) &&
                    std::isfinite(nextReverse) &&
                    nextForward <= previousForward + 1e-12 &&
                    nextReverse <= previousReverse + 1e-12 &&
                    nextForward >= 0.0 && nextReverse >= 0.0,
                    "forward and reverse power curves decrease monotonically");
            previousForward = nextForward;
            previousReverse = nextReverse;
        }
    }

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, -inf})
    {
        Require(Near(GearAwareMotorPowerFactor(invalid, 0.0, 6500.0, true), 0.0) &&
                Near(GearAwareMotorPowerFactor(1.0, invalid, 6500.0, true), 0.0) &&
                Near(GearAwareMotorPowerFactor(1.0, 1000.0, invalid, true), 0.0),
                "invalid physics or driver demand must deny propulsion");
    }
    Require(Near(GearAwareMotorPowerFactor(1.0, 1000.0, 0.0, true), 0.0) &&
            Near(GearAwareMotorPowerFactor(1.0, 1000.0, 25000.0, true), 0.0),
            "implausible or corrupt forward speed cap fails closed");

    // Production nitro and drafting allocator remains the sole extra-force
    // authority. The new taper is shared equally by all three motor terms.
    const auto fullBoost = AllocateArcadeBoost(1.38, 0.06, true);
    Require(Near(fullBoost.TotalExtra(), 0.4), "shared boost allocation remains unchanged");

    for (int i = 0; i < 200000; ++i)
    {
        const bool arcade = i % 2 == 0;
        const double maxForward = 500.0 + static_cast<double>(i % 14500);
        const double velocity = static_cast<double>(i % 36001) - 18000.0;
        const double demand = static_cast<double>(i % 1999 - 999) / 999.0;
        const double factor = GearAwareMotorPowerFactor(
            demand, velocity, maxForward, arcade);
        Require(std::isfinite(factor) && factor >= 0.0 && factor <= 1.0,
                "200000 valid chassis input samples give bounded motor scaling");

        const double reverseMax = MaximumReverseSpeedCmPerSecond(arcade);
        if (demand > 0.0 && velocity >= maxForward)
        {
            Require(Near(factor, 0.0), "positive force denied past forward speed cap");
        }
        if (demand < 0.0 && velocity <= -reverseMax)
        {
            Require(Near(factor, 0.0), "reverse force denied past reverse gear cap");
        }
        if ((demand > 0.0 && velocity <= 0.0) ||
            (demand < 0.0 && velocity >= 0.0))
        {
            Require(Near(factor, 1.0), "driver direction reversal remains unimpeded");
        }
        const double boosted = std::abs(demand) * factor *
            (1.0 + fullBoost.TotalExtra());
        Require(boosted <= 1.4 + 1e-12,
                "gear taper cannot increase even fully boosted motor power");
    }

    std::cout << "GearAwareMotorPower: 200000 chassis input samples passed (no Unreal)\n";
    return EXIT_SUCCESS;
}
