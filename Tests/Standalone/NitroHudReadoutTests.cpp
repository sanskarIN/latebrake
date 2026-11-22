#include "UI/NitroHudReadout.h"

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
        std::cerr << "Nitro HUD FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::NitroHudReadout Observe(
    latebrake::NitroHudSample sample) noexcept
{
    return latebrake::ClassifyNitroHud(sample);
}

bool Is(const latebrake::NitroHudReadout& value,
        latebrake::NitroHudPhase phase)
{
    return value.Valid && value.Phase == phase;
}
} // namespace

int main()
{
    using namespace latebrake;

    const NitroHudSample normal{0.75, 0.15, 0.0, 60.0, false, true};
    Require(Is(Observe(normal), NitroHudPhase::Stored),
        "positive charge is only stored, never guaranteed ready");
    auto sample = normal;
    sample.DrivingEligible = false;
    Require(Is(Observe(sample), NitroHudPhase::Unavailable),
        "interrupted/paused race never claims the booster is usable");
    sample = normal;
    sample.SignedForwardKmH = 0.0;
    Require(Is(Observe(sample), NitroHudPhase::BuildSpeed),
        "stationary booster needs physical minimum launch speed");
    sample.SignedForwardKmH = -45.0;
    Require(Is(Observe(sample), NitroHudPhase::BuildSpeed),
        "reversing never qualifies for forward nitro");
    sample.SignedForwardKmH =
        ArcadeNitroBoost::MinForwardCmPerSecond * 0.036;
    Require(Is(Observe(sample), NitroHudPhase::Stored),
        "exact activation speed is accepted");

    sample = normal;
    sample.ChargeFraction = 0.0;
    Require(Is(Observe(sample), NitroHudPhase::Empty),
        "depleted booster reports empty rather than ready");
    sample = normal;
    sample.HeatFraction = ArcadeNitroBoost::HeatCutoff;
    Require(Is(Observe(sample), NitroHudPhase::Overheated),
        "thermal lockout takes precedence over available charge");
    sample = normal;
    sample.CooldownSeconds = 1.7;
    Require(Is(Observe(sample), NitroHudPhase::Cooldown) &&
            std::abs(Observe(sample).CooldownSeconds - 1.7) < 1.e-9,
        "cooldown reports actual remaining time");
    sample.HeatFraction = 1.0;
    sample.ChargeFraction = 0.0;
    sample.SignedForwardKmH = 0.0;
    Require(Is(Observe(sample), NitroHudPhase::Cooldown),
        "finite cooldown takes precedence over hot/empty/low speed");

    sample = normal;
    sample.Active = true;
    Require(Is(Observe(sample), NitroHudPhase::Active),
        "confirmed nitro drive output displays active");
    sample.CooldownSeconds = 1.0;
    Require(!Observe(sample).Valid,
        "contradictory active+cooldown sample fails closed");
    sample = normal;
    sample.Active = true;
    sample.ChargeFraction = 0.0;
    Require(!Observe(sample).Valid,
        "contradictory active+empty sample fails closed");
    sample = normal;
    sample.Active = true;
    sample.HeatFraction = 1.0;
    Require(!Observe(sample).Valid,
        "contradictory active+hot sample fails closed");
    sample = normal;
    sample.Active = true;
    sample.SignedForwardKmH = 0.0;
    Require(!Observe(sample).Valid,
        "contradictory active+stopped sample fails closed");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double bad : {nan, inf, -1.0, 1.1})
    {
        sample = normal;
        sample.ChargeFraction = bad;
        Require(!Observe(sample).Valid, "invalid charge rejected");
        sample = normal;
        sample.HeatFraction = bad;
        Require(!Observe(sample).Valid, "invalid heat rejected");
    }
    for (double bad : {nan, inf, -0.1, 3.0})
    {
        sample = normal;
        sample.CooldownSeconds = bad;
        Require(!Observe(sample).Valid, "invalid cooldown rejected");
    }
    for (double bad : {nan, inf, -361.0, 361.0})
    {
        sample = normal;
        sample.SignedForwardKmH = bad;
        Require(!Observe(sample).Valid, "invalid chassis speed rejected");
    }
    // When driving is ineligible, a bad/nonexistent speed sample is
    // allowed but must never disclose any optimistic nitro status.
    sample = normal;
    sample.DrivingEligible = false;
    sample.SignedForwardKmH = nan;
    Require(Is(Observe(sample), NitroHudPhase::Unavailable),
        "paused race with missing velocity cannot show readiness");

    // Exercise the actual gameplay nitro component end to end without
    // changing its force, heat or recharge rules. UI consumes snapshots.
    ArcadeNitroBoost actual;
    Require(Is(ReadNitroHud(actual, 0.0, true),
        NitroHudPhase::BuildSpeed), "fresh booster is stored but too slow");
    actual.Press(true);
    const auto first = actual.Step(0.1, true, false, 1.0, 800.0);
    Require(first.Active &&
        Is(ReadNitroHud(actual, 28.8, true), NitroHudPhase::Active),
        "real engaged booster is observed as active");

    // Drive until the physical ability performs its own depletion or
    // heat-cutoff and starts its existing cooldown timer.
    bool foundCooldown = false;
    for (int i = 0; i < 70; ++i)
    {
        const auto output = actual.Step(0.1, true, false, 1.0, 800.0);
        const auto readout = ReadNitroHud(actual, 28.8, true);
        Require(!output.Active || Is(readout, NitroHudPhase::Active),
            "visible active always follows an active gameplay output");
        if (Is(readout, NitroHudPhase::Cooldown))
        {
            foundCooldown = true;
            Require(!output.Active && readout.CooldownSeconds > 0.0,
                "real ability cooldown is observed without reviving boost");
            break;
        }
    }
    Require(foundCooldown,
        "real ability eventually enters cooldown under sustained boost");
    const auto frozen = ReadNitroHud(actual, 28.8, false);
    Require(Is(frozen, NitroHudPhase::Unavailable),
        "guarded race masks even a charged ability and its cooldown");

    // The HUD is never allowed to mutate this component's timers.
    const double charged = actual.ChargeFraction();
    const double heat = actual.HeatFraction();
    const double remaining = actual.CooldownRemainingSeconds();
    for (int i = 0; i < 100; ++i)
    {
        const auto value = ReadNitroHud(actual, 28.8, true);
        Require(value.Valid, "deterministic read-only repeat observation");
    }
    Require(actual.ChargeFraction() == charged &&
            actual.HeatFraction() == heat &&
            actual.CooldownRemainingSeconds() == remaining,
            "100 HUD reads neither recharge nor advance the ability");

    // Pit refill is authoritative; the readout must follow, never cause it.
    actual.RefillAtPit();
    Require(Is(ReadNitroHud(actual, 28.8, true), NitroHudPhase::Stored),
        "real pit service restores stored energy status");
    actual.Suspend();
    Require(!actual.IsActive() &&
        Is(ReadNitroHud(actual, 28.8, false),
            NitroHudPhase::Unavailable),
        "suspended game never shows a stale active label");

    std::cout << "Observed nitro readiness and source integration passed.\n";
    return EXIT_SUCCESS;
}
