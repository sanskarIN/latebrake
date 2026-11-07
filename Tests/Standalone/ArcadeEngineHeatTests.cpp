#include "Vehicles/ArcadeEngineHeat.h"
#include "Vehicles/OpposingThrottleBraking.h"

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
        Require(heat.SettleNitroDelivery(true),
                "hot nitro fixture must deliver and commit actual bonus");
    }
    const double nitroHot = heat.HeatFraction();
    heat.Reset();
    for (int i = 0; i < 450; ++i)
    {
        heat.Observe({0.05, -3200.0, -1.0, false, true});
    }
    Require(nitroHot > heat.HeatFraction(), "nitro adds load heat");

    // Cross-policy regression: motor force is withheld while the opposite
    // accelerator/reverse pedal brakes actual signed chassis travel. Its
    // input must not masquerade as propulsion heat for either direction.
    // This uses the production braking classifier, not a test-only sign
    // approximation. Exact pawn admission wiring is separately bound by
    // ArcadeEngineHeatBindingTests.py.
    for (int fps : {30, 60, 120})
    {
        const double dt = 1.0 / fps;
        for (double direction : {-1.0, 1.0})
        {
            const double forward = 3000.0 * direction;
            const double opposingPedal = -direction;
            const auto opposite = latebrake::CalculateOpposingThrottleBraking({
                dt, forward, 3400.0, 1100.0, opposingPedal,
                1.0, 1.0, 1.0, false, true});
            Require(opposite.Valid && opposite.OpposingThrottle &&
                    opposite.ForwardForce * forward < 0.0,
                "real brake classification for both travel directions");
            ArcadeEngineHeat brakingHeat;
            ArcadeEngineHeat motorHeat;
            for (int frame = 0; frame < fps * 40; ++frame)
            {
                // Mirrors the real pawn's propulsion authorization.
                const double admittedDemand = opposite.OpposingThrottle
                    ? 0.0 : opposingPedal;
                const auto braking = brakingHeat.Observe({
                    dt, forward, admittedDemand, false, true});
                const auto powered = motorHeat.Observe({
                    dt, forward, direction, false, true});
                Require(braking.Valid && powered.Valid,
                    "opposing and powered samples remain valid");
            }
            Require(Near(brakingHeat.HeatFraction(),
                         ArcadeEngineHeat::IdleHeat) &&
                    Near(brakingHeat.MotorMultiplier(), 1.0),
                "long opposite-pedal braking cannot heat/derate a motor that is off");
            Require(motorHeat.HeatFraction() >
                    ArcadeEngineHeat::DerateThreshold &&
                    motorHeat.MotorMultiplier() < 1.0,
                "same-direction reverse/forward propulsion still builds heat");

            const auto nearStop = latebrake::CalculateOpposingThrottleBraking({
                dt, 75.0 * direction, 2400.0, 1100.0,
                opposingPedal, 1.0, 1.0, 1.0, false, true});
            Require(nearStop.Valid && !nearStop.OpposingThrottle &&
                    nearStop.ForwardForce == 0.0,
                "large sideways slip must not block near-stop direction changes");
            ArcadeEngineHeat nearStopHeat;
            for (int frame = 0; frame < fps * 2; ++frame)
            {
                const auto warmed = nearStopHeat.Observe({
                    dt, 75.0 * direction,
                    nearStop.OpposingThrottle ? 0.0 : opposingPedal,
                    false, true});
                Require(warmed.Valid, "eligible near-stop propulsion sample");
            }
            Require(nearStopHeat.HeatFraction() >
                    ArcadeEngineHeat::IdleHeat,
                "near-stop direction-change motor demand may heat normally");
        }
    }

    // Undelivered nitro must add ZERO EXTRA engine heat, but still
    // retain the physical throttle/airflow baseline in every frame.
    ArcadeEngineHeat denied;
    ArcadeEngineHeat ordinary;
    for (int i = 0; i < 400; ++i)
    {
        const auto candidate = denied.Observe({
            0.05, 3000.0, 1.0, true, true});
        const auto normal = ordinary.Observe({
            0.05, 3000.0, 1.0, false, true});
        Require(candidate.Valid && normal.Valid &&
                Near(candidate.HeatFraction, normal.HeatFraction) &&
                Near(candidate.MotorMultiplier, normal.MotorMultiplier) &&
                Near(denied.HeatFraction(), ordinary.HeatFraction()) &&
                denied.SettleNitroDelivery(false) &&
                Near(denied.HeatFraction(), ordinary.HeatFraction()) &&
                !denied.SettleNitroDelivery(false),
                "400 denied nitro samples never enter heat before settlement");
    }
    // Before settlement all public heat readouts and power decisions
    // must represent only applied, ordinary load, not a speculative
    // nitro extra. An accepted force then commits that extra once.
    ArcadeEngineHeat proposed;
    ArcadeEngineHeat baseline;
    const auto pendingOutput = proposed.Observe({
        0.05, 3000.0, 1.0, true, true});
    const auto baselineOutput = baseline.Observe({
        0.05, 3000.0, 1.0, false, true});
    Require(pendingOutput.Valid && baselineOutput.Valid &&
            Near(pendingOutput.HeatFraction, baselineOutput.HeatFraction) &&
            Near(proposed.HeatFraction(), baseline.HeatFraction()) &&
            Near(pendingOutput.MotorMultiplier, baselineOutput.MotorMultiplier) &&
            Near(proposed.MotorMultiplier(), baseline.MotorMultiplier()),
            "uncommitted nitro must not expose speculative heat or power");
    const double beforeCommit = proposed.HeatFraction();
    Require(proposed.SettleNitroDelivery(true) &&
            proposed.HeatFraction() > beforeCommit &&
            !proposed.SettleNitroDelivery(true),
            "real bonus commits heat once, never twice");
    const double committedHeat = proposed.HeatFraction();
    proposed.Suspend();
    Require(Near(proposed.HeatFraction(), committedHeat) &&
            !proposed.SettleNitroDelivery(false),
            "suspend cannot refund already committed extra heat");

    ArcadeEngineHeat delivered;
    ArcadeEngineHeat ordinaryCompare;
    for (int i = 0; i < 400; ++i)
    {
        delivered.Observe({0.05, 3000.0, 1.0, true, true});
        ordinaryCompare.Observe({0.05, 3000.0, 1.0, false, true});
        Require(delivered.SettleNitroDelivery(true),
                "positive real nitro force commits extra engine heating");
    }
    Require(delivered.HeatFraction() > ordinaryCompare.HeatFraction() &&
            !delivered.SettleNitroDelivery(true),
            "only delivered bonus adds incremental engine heat");
    delivered.Reset();
    const auto rejected = delivered.Observe({
        0.0, 3000.0, 1.0, true, true});
    Require(!rejected.Valid && !delivered.SettleNitroDelivery(false),
            "invalid engine sampling cannot leave a refundable receipt");

    // A lost delivery verdict (early return, pause or next-frame race
    // transition) must never silently turn a proposed heat increment
    // into real history. Provisional nitro heat must also not derate the
    // *same* motor dispatch before the real extra force was accepted.
    ArcadeEngineHeat orphanNitroHeat;
    ArcadeEngineHeat throttleOnlyHeat;
    bool baselineDerated = false;
    for (int i = 0; i < 600; ++i)
    {
        const auto pending = orphanNitroHeat.Observe({
            0.05, 3000.0, 1.0, true, true});
        const auto ordinaryFrame = throttleOnlyHeat.Observe({
            0.05, 3000.0, 1.0, false, true});
        Require(pending.Valid && ordinaryFrame.Valid &&
                Near(pending.HeatFraction, ordinaryFrame.HeatFraction) &&
                Near(orphanNitroHeat.HeatFraction(),
                     throttleOnlyHeat.HeatFraction()) &&
                Near(pending.MotorMultiplier, ordinaryFrame.MotorMultiplier),
                "orphan proposal never pre-heats or pre-derates same-frame motor");
        baselineDerated = baselineDerated || ordinaryFrame.Derated;
        if (i % 17 == 0)
        {
            orphanNitroHeat.Suspend();
            throttleOnlyHeat.Suspend();
            Require(Near(orphanNitroHeat.HeatFraction(),
                         throttleOnlyHeat.HeatFraction()) &&
                    !orphanNitroHeat.SettleNitroDelivery(true),
                    "suspend must refund unanswered nitro receipt exactly once");
        }
    }
    Require(baselineDerated, "orphan sample fixture reaches engine derating");
    Require(orphanNitroHeat.SettleNitroDelivery(false) &&
            Near(orphanNitroHeat.HeatFraction(),
                 throttleOnlyHeat.HeatFraction()) &&
            !orphanNitroHeat.SettleNitroDelivery(false),
            "next Observe refunds absent verdict; final pending refund is idempotent");

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
