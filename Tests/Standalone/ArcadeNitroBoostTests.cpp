#include "Vehicles/ArcadeNitroBoost.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* reason)
{
    if (!value)
    {
        std::cerr << "ArcadeNitroBoost FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void Drive(latebrake::ArcadeNitroBoost& boost, int count,
           bool eligible = true, bool braking = false, double throttle = 1.0,
           double speed = 2200.0)
{
    for (int i = 0; i < count; ++i)
    {
        boost.Step(0.05, eligible, braking, throttle, speed);
    }
}
}

int main()
{
    using latebrake::ArcadeNitroBoost;
    ArcadeNitroBoost boost;
    Check(boost.ChargeFraction() == 1.0 && !boost.IsActive(), "fresh full charge");
    boost.Press(false);
    Check(!boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "AI or disallowed press cannot be replayed later");
    boost.Press(true);
    const auto active = boost.Step(0.05, true, false, 1.0, 2200.0);
    Check(active.Active && boost.IsActive() &&
          active.ForceMultiplier == ArcadeNitroBoost::ForceMultiplier &&
          active.SpeedCapMultiplier == ArcadeNitroBoost::SpeedCapMultiplier,
          "human forward-drive press activates both bounded effects");
    Check(boost.ChargeFraction() < 1.0 && boost.HeatFraction() > 0.0,
          "boost consumes energy and creates heat");
    const double chargeBeforeBlocked = boost.ChargeFraction();
    const double heatBeforeBlocked = boost.HeatFraction();
    const auto blocked = boost.Step(0.05, false, false, 1.0, 2200.0);
    Check(!blocked.Active && !boost.IsHeld() && !boost.IsActive() &&
          boost.ChargeFraction() == chargeBeforeBlocked &&
          boost.HeatFraction() == heatBeforeBlocked,
          "pause/focus preserves finite meter and clears held input");
    Check(!boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "resuming without a new key press never fires boost");

    for (double speed : {-100.0, 0.0, 699.0, 10000.1})
    {
        boost.Reset();
        boost.Press(true);
        Check(!boost.Step(0.05, true, false, 1.0, speed).Active,
              "boost unavailable in reverse, standstill or unsafe velocity");
    }
    boost.Reset();
    boost.Press(true);
    Check(!boost.Step(0.05, true, true, 1.0, 2200.0).Active,
          "brake takes priority over boost");
    Check(!boost.Step(0.05, true, false, 0.49, 2200.0).Active,
          "requires real positive forward throttle");
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "genuine press can start after brake released");
    boost.Release();
    Check(!boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "physical key release stops force immediately");

    boost.Reset();
    boost.Press(true);
    for (int i = 0; i < 150 && boost.IsHeld(); ++i)
    {
        boost.Step(0.05, true, false, 1.0, 2200.0);
    }
    Check(!boost.IsActive() && !boost.IsHeld() &&
          boost.CooldownRemainingSeconds() > 0.0 &&
          boost.HeatFraction() <= 1.0 && boost.ChargeFraction() >= 0.0,
          "thermal cutoff requires a fresh press and starts cooldown");
    boost.Press(true);
    Check(!boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "cooldown denies immediate restart");
    boost.Release();
    Drive(boost, 200);
    Check(boost.CooldownRemainingSeconds() == 0.0 &&
          boost.HeatFraction() == 0.0 && boost.ChargeFraction() > 0.0,
          "timer cools and charge regenerates without boost");
    boost.Press(true);
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "cooldown completed and new key press reactivates");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double dt : {nan, inf, -1.0, 0.0, 0.11})
    {
        boost.Reset();
        boost.Press(true);
        Check(!boost.Step(dt, true, false, 1.0, 2200.0).Active &&
              !boost.IsHeld() && boost.ChargeFraction() == 1.0,
              "malformed/hitch frame clears held state without recharge");
    }
    for (double speed : {nan, inf, -inf})
    {
        boost.Reset();
        boost.Press(true);
        Check(!boost.Step(0.05, true, false, 1.0, speed).Active &&
              !boost.IsHeld(), "malformed physics fails closed");
    }
    for (double throttle : {nan, inf, 1.01, -1.01})
    {
        boost.Reset();
        boost.Press(true);
        Check(!boost.Step(0.05, true, false, throttle, 2200.0).Active &&
              !boost.IsHeld(), "malformed throttle fails closed");
    }
    for (int run = 0; run < 5000; ++run)
    {
        boost.Reset();
        boost.Press(true);
        Drive(boost, 100);
        Check(std::isfinite(boost.ChargeFraction()) &&
              boost.ChargeFraction() >= 0.0 && boost.ChargeFraction() <= 1.0 &&
              std::isfinite(boost.HeatFraction()) &&
              boost.HeatFraction() >= 0.0 && boost.HeatFraction() <= 1.0,
              "stress run never emits nonfinite or unbounded meters");
    }
    boost.Reset();
    Check(boost.ChargeFraction() == 1.0 && boost.HeatFraction() == 0.0 &&
          boost.CooldownRemainingSeconds() == 0.0 && !boost.IsHeld(),
          "race restart resets run-local meter");
    std::cout << "ArcadeNitroBoost passed\n";
}
