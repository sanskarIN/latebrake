#include "Vehicles/ArcadeCollisionDamage.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Check(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}
bool Near(double a, double b, double tolerance = 1e-10)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using latebrake::ArcadeCollisionDamage;
    using latebrake::ArcadeImpactSample;

    ArcadeCollisionDamage damage;
    const ArcadeImpactSample hardWall{1100000.0, 1100.0, 3800.0, 0.1, true};
    Check(!damage.HasEligibleFrame(), "no unverified pre-tick collision");
    Check(!damage.ObserveImpact(hardWall), "not valid before first race tick");
    Check(Near(damage.DamageFraction(), 0.0), "fresh chassis is undamaged");
    Check(Near(damage.MotorMultiplier(), 1.0), "healthy full power");
    Check(Near(damage.LateralGripMultiplier(), 1.0), "healthy full grip");
    Check(damage.Advance(0.05, true), "finite eligible frame accepted");

    // Ground, kerb, gentle sideswipe, and nonphysical hit must not damage.
    Check(!damage.ObserveImpact({1100000.0, 1100.0, 3800.0, 0.85, true}),
          "ground contact excluded");
    Check(!damage.ObserveImpact({1100000.0, 1100.0, 700.0, 0.0, true}),
          "low-speed shunt excluded");
    Check(!damage.ObserveImpact({300000.0, 1100.0, 3800.0, 0.0, true}),
          "small collision impulse excluded");
    Check(!damage.ObserveImpact({0.0, 1100.0, 3800.0, 0.0, true}),
          "non-impulsive hit excluded");

    Check(damage.ObserveImpact(hardWall), "genuine high-energy impact accepted");
    const double initialDamage = damage.DamageFraction();
    Check(initialDamage > 0.03 && initialDamage < 0.13,
          "one impact applies bounded finite damage");
    Check(damage.ImpactEventCount() == 1, "one recorded impact");
    Check(damage.MotorMultiplier() < 1.0, "damage lowers drive");
    Check(damage.LateralGripMultiplier() < 1.0, "damage lowers grip");
    Check(!damage.ObserveImpact(hardWall), "same-frame repeat refused");
    for (int i = 0; i < 7; ++i)
    {
        Check(damage.Advance(0.05, true), "cooldown physics frame");
        Check(!damage.ObserveImpact(hardWall), "burst contact throttled");
    }
    Check(damage.Advance(0.05, true), "cooldown expires");
    Check(damage.ObserveImpact(hardWall), "later independent contact accepted");

    const auto infinity = std::numeric_limits<double>::infinity();
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const ArcadeImpactSample invalid[] = {
        {nan, 1100, 4000, 0, true},
        {infinity, 1100, 4000, 0, true},
        {-1, 1100, 4000, 0, true},
        {1.0e10, 1100, 4000, 0, true},
        {900000, 0, 4000, 0, true},
        {900000, nan, 4000, 0, true},
        {900000, infinity, 4000, 0, true},
        {900000, 49, 4000, 0, true},
        {900000, 11000, 4000, 0, true},
        {900000, 1100, nan, 0, true},
        {900000, 1100, infinity, 0, true},
        {900000, 1100, -200, 0, true},
        {900000, 1100, 11000, 0, true},
        {900000, 1100, 4000, nan, true},
        {900000, 1100, 4000, infinity, true},
        {900000, 1100, 4000, 1.1, true},
        {900000, 1100, 4000, 0, false}
    };
    for (const auto& sample : invalid)
    {
        // Start with a fully armed, cooldown-free controller on EACH
        // negative sample. Otherwise the previous hit cooldown could
        // conceal a broken input validator and produce a false pass.
        damage.Reset();
        Check(damage.Advance(0.05, true), "invalid fixture is armed");
        Check(damage.CanRecordImpact(), "invalid fixture not cooling down");
        const double before = damage.DamageFraction();
        Check(!damage.ObserveImpact(sample), "invalid impact rejected");
        Check(Near(damage.DamageFraction(), before),
              "invalid impact cannot mutate damage");
        Check(damage.ImpactEventCount() == 0, "invalid cannot count impact");
    }

    Check(!damage.Advance(0.101, true), "long tick invalidates impact authority");
    Check(!damage.ObserveImpact(hardWall), "late physical callback rejected");
    Check(!damage.Advance(nan, true), "NaN tick rejected");
    Check(!damage.Advance(infinity, true), "infinite tick rejected");
    Check(!damage.Advance(0.05, false), "focus/pause tick rejected");
    Check(!damage.ObserveImpact(hardWall), "paused impact rejected");
    Check(damage.Advance(0.05, true), "valid race resumes after interruption");

    // Real collision callback spam over the entire race cannot push
    // physics response beyond the published conservative source limits.
    for (int i = 0; i < 2000; ++i)
    {
        damage.Advance(0.05, true);
        damage.ObserveImpact({10000000.0, 1100.0, 6000.0, -0.05, true});
        Check(std::isfinite(damage.DamageFraction()) &&
              damage.DamageFraction() <= ArcadeCollisionDamage::MaxDamageFraction,
              "bounded damage");
        Check(std::isfinite(damage.MotorMultiplier()) &&
              damage.MotorMultiplier() >= ArcadeCollisionDamage::MinMotorMultiplier &&
              damage.MotorMultiplier() <= 1.0, "bounded power");
        Check(std::isfinite(damage.LateralGripMultiplier()) &&
              damage.LateralGripMultiplier() >=
                  ArcadeCollisionDamage::MinLateralGripMultiplier &&
              damage.LateralGripMultiplier() <= 1.0, "bounded lateral grip");
    }
    Check(Near(damage.DamageFraction(), ArcadeCollisionDamage::MaxDamageFraction),
          "damage cap reached");
    Check(!damage.ObserveImpact(hardWall), "no overflow damage past cap");

    const double retainedDamage = damage.DamageFraction();
    damage.Suspend();
    Check(!damage.HasEligibleFrame(), "interruption disables callbacks");
    Check(!damage.ObserveImpact(hardWall), "cannot damage during interruption");
    Check(Near(damage.DamageFraction(), retainedDamage),
          "interruption retains prior race damage");
    damage.Reset();
    Check(damage.ImpactEventCount() == 0, "new event clears counters");
    Check(Near(damage.DamageFraction(), 0.0), "fresh event repairs vehicle");
    Check(Near(damage.MotorMultiplier(), 1.0), "new event restores full power");
    Check(Near(damage.LateralGripMultiplier(), 1.0),
          "new event restores baseline grip");

    // Deterministic long-run frame and callback fuzz without any
    // reallocations, timer dependency or mutable global engine state.
    for (int i = 0; i < 250000; ++i)
    {
        const bool frame = damage.Advance(0.05, i % 101 != 0);
        if (frame && i % 17 == 0)
        {
            damage.ObserveImpact({1500000.0, 1100.0, 4000.0, 0.2, true});
        }
        Check(std::isfinite(damage.DamageFraction()) &&
              damage.DamageFraction() >= 0.0 &&
              damage.DamageFraction() <= ArcadeCollisionDamage::MaxDamageFraction,
              "long run finite");
    }
    std::cout << "ArcadeCollisionDamage: all scenarios passed\n";
    return 0;
}
