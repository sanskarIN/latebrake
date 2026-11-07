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
        const auto candidate =
            boost.Step(0.05, eligible, braking, throttle, speed);
        if (candidate.Active) boost.CommitDeliveredFrame();
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
    // Nitro is only provisionally ACTIVE until the real Chaos motor
    // dispatch confirms positive bonus thrust. A saturated grip circle,
    // gear interruption or engine derate must not consume finite charge.
    boost.Reset();
    boost.Press(true);
    const double startingCharge = boost.ChargeFraction();
    const double startingHeat = boost.HeatFraction();
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active &&
          boost.RejectUndeliveredFrame() &&
          !boost.IsActive() && boost.IsHeld() &&
          std::abs(boost.ChargeFraction() - startingCharge) < 1.0e-12 &&
          std::abs(boost.HeatFraction() - startingHeat) < 1.0e-12 &&
          boost.CooldownRemainingSeconds() == 0.0,
          "undelivered nitro cannot spend energy or accumulate meter heat");
    Check(!boost.RejectUndeliveredFrame() &&
          boost.ChargeFraction() == startingCharge &&
          boost.HeatFraction() == startingHeat,
          "rejected boost cannot refund the same frame twice");
    for (int i = 0; i < 250; ++i)
    {
        const auto candidate = boost.Step(0.05, true, false, 1.0, 2200.0);
        Check(candidate.Active && boost.RejectUndeliveredFrame() &&
              boost.IsHeld() && !boost.IsActive(),
              "held key stays eligible when temporary traction blocks boost");
    }
    Check(std::abs(boost.ChargeFraction() - startingCharge) < 1.0e-12 &&
          std::abs(boost.HeatFraction() - startingHeat) < 1.0e-12 &&
          boost.CooldownRemainingSeconds() == 0.0,
          "250 consecutive undelivered frames cannot drain/overheat nitro");
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active &&
          boost.ChargeFraction() < startingCharge &&
          boost.HeatFraction() > startingHeat,
          "next physically delivered nitro frame charges normally");
    const double deliveredCharge = boost.ChargeFraction();
    Check(boost.CommitDeliveredFrame() &&
          boost.IsActive() && boost.IsHeld() &&
          !boost.CommitDeliveredFrame() &&
          boost.ChargeFraction() == deliveredCharge,
          "delivered nitro settles exactly once without double debit");
    boost.Release();
    Check(!boost.RejectUndeliveredFrame() &&
          boost.ChargeFraction() == deliveredCharge,
          "released genuinely spent nitro cannot be retroactively refunded");
    // Unsettled engine-frame receipts must NEVER become an implicit
    // delivery when another Tick or input lifecycle event arrives.
    boost.Reset();
    boost.Press(true);
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "orphan candidate fixture starts with legitimate press");
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active &&
          std::abs(boost.ChargeFraction() -
              (1.0 - 0.05 * ArcadeNitroBoost::DrainPerSecond)) < 1e-12,
          "next Tick refunds missing settlement before its own new debit");
    Check(boost.RejectUndeliveredFrame() &&
          std::abs(boost.ChargeFraction() - 1.0) < 1e-12 &&
          std::abs(boost.HeatFraction()) < 1e-12 &&
          boost.IsHeld(), "new Tick retains held input without phantom cost");
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "release-interruption fixture creates a pending receipt");
    boost.Release();
    Check(!boost.IsHeld() && !boost.IsActive() &&
          std::abs(boost.ChargeFraction() - 1.0) < 1e-12 &&
          std::abs(boost.HeatFraction()) < 1e-12 &&
          !boost.CommitDeliveredFrame(),
          "release refunds undelivered bonus without leaving commit authority");
    boost.Press(true);
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "disallowed-press fixture has another pending receipt");
    boost.Press(false);
    Check(!boost.IsHeld() && !boost.IsActive() &&
          std::abs(boost.ChargeFraction() - 1.0) < 1e-12 &&
          !boost.RejectUndeliveredFrame(),
          "disallowed press refunds prior candidate and disarms correctly");
    boost.Press(true);
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "suspend-interruption fixture starts with pending candidate");
    boost.Suspend();
    Check(!boost.IsHeld() && !boost.IsActive() &&
          std::abs(boost.ChargeFraction() - 1.0) < 1e-12 &&
          std::abs(boost.HeatFraction()) < 1e-12 &&
          boost.CooldownRemainingSeconds() == 0.0,
          "pause or invalid tick cannot retain an undelivered debit");

    boost.Reset();
    boost.Press(true);
    Check(boost.Step(0.05, true, false, 1.0, 2200.0).Active,
          "test fixture resumes real qualifying held state");

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
        const auto candidate = boost.Step(0.05, true, false, 1.0, 2200.0);
        if (candidate.Active) boost.CommitDeliveredFrame();
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

    // An overheat-limit sample is not a physically delivered bonus
    // until the engine's actual AddForce. Keep eligible key/charge
    // through repeated rejected terminal candidates, then prove the
    // first true delivery is what latches the cooldown.
    ArcadeNitroBoost heatEdge;
    heatEdge.Press(true);
    for (int i = 0; i < 52; ++i)
    {
        const auto s = heatEdge.Step(0.05, true, false, 1.0, 2200.0);
        Check(s.Active && heatEdge.CommitDeliveredFrame(),
              "preheat setup must commit qualified physical bonus");
    }
    Check(heatEdge.HeatFraction() < ArcadeNitroBoost::HeatCutoff &&
          heatEdge.HeatFraction() + 0.05 *
              ArcadeNitroBoost::HeatPerSecond >= ArcadeNitroBoost::HeatCutoff,
          "thermal edge fixture is one committed bonus from cutoff");
    const double baselineCharge = heatEdge.ChargeFraction();
    const double baselineHeat = heatEdge.HeatFraction();
    Check(heatEdge.Step(0.05, true, false, 1.0, 2200.0).Active &&
          heatEdge.Step(0.05, true, false, 1.0, 2200.0).Active &&
          heatEdge.IsHeld() && heatEdge.CooldownRemainingSeconds() == 0.0 &&
          heatEdge.RejectUndeliveredFrame() &&
          std::abs(heatEdge.ChargeFraction() - baselineCharge) < 1e-10 &&
          std::abs(heatEdge.HeatFraction() - baselineHeat) < 1e-10,
          "unsettled thermal cutoff retries without phantom cooldown");
    for (int i = 0; i < 120; ++i)
    {
        const auto s = heatEdge.Step(0.05, true, false, 1.0, 2200.0);
        Check(s.Active &&
              heatEdge.HeatFraction() >= ArcadeNitroBoost::HeatCutoff &&
              heatEdge.RejectUndeliveredFrame() && heatEdge.IsHeld() &&
              heatEdge.CooldownRemainingSeconds() == 0.0 &&
              std::abs(heatEdge.ChargeFraction() - baselineCharge) < 1e-10 &&
              std::abs(heatEdge.HeatFraction() - baselineHeat) < 1e-10,
              "120 rejected overheat-boundary frames cannot start cooldown");
    }
    const auto lastHeat =
        heatEdge.Step(0.05, true, false, 1.0, 2200.0);
    Check(lastHeat.Active && heatEdge.CommitDeliveredFrame() &&
          !heatEdge.IsHeld() && !heatEdge.IsActive() &&
          heatEdge.CooldownRemainingSeconds() ==
              ArcadeNitroBoost::CooldownSeconds &&
          !heatEdge.CommitDeliveredFrame() &&
          !heatEdge.RejectUndeliveredFrame(),
          "only the final delivered hot bonus triggers cooldown");

    // Alternating qualified boost and ordinary cooling frames can
    // exhaust charge before the heat limit, exercising the OTHER
    // previously irrevocable cutoff boundary.
    ArcadeNitroBoost chargeEdge;
    chargeEdge.Press(true);
    for (int i = 0; i < 120; ++i)
    {
        const auto s = chargeEdge.Step(0.05, true, false, 1.0, 2200.0);
        Check(s.Active && chargeEdge.CommitDeliveredFrame(),
              "low-charge fixture actually delivers each motor bonus");
        chargeEdge.Release();
        Check(!chargeEdge.Step(0.05, true, false, 1.0, 2200.0).Active,
              "cooling/regen fixture has no stale held key");
        chargeEdge.Press(true);
    }
    Check(chargeEdge.ChargeFraction() > 0.0 &&
          chargeEdge.ChargeFraction() <
              0.05 * ArcadeNitroBoost::DrainPerSecond &&
          chargeEdge.HeatFraction() < ArcadeNitroBoost::HeatCutoff,
          "low-charge test sits one real bonus from depletion");
    const double remainingCharge = chargeEdge.ChargeFraction();
    const auto rejectedLast =
        chargeEdge.Step(0.05, true, false, 1.0, 2200.0);
    Check(rejectedLast.Active && chargeEdge.ChargeFraction() == 0.0 &&
          chargeEdge.RejectUndeliveredFrame() &&
          std::abs(chargeEdge.ChargeFraction() - remainingCharge) < 1e-10 &&
          chargeEdge.CooldownRemainingSeconds() == 0.0 &&
          chargeEdge.IsHeld(),
          "no-thrust charge depletion must not trigger phantom cooldown");
    const auto finalDelivered =
        chargeEdge.Step(0.05, true, false, 1.0, 2200.0);
    Check(finalDelivered.Active && chargeEdge.CommitDeliveredFrame() &&
          !chargeEdge.IsHeld() &&
          chargeEdge.CooldownRemainingSeconds() ==
              ArcadeNitroBoost::CooldownSeconds,
          "physically delivered last charge triggers cooldown exactly once");

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
