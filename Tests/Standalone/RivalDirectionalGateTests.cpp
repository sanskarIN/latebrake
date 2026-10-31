#include "AI/RivalRouteProgress.h"
#include "Racing/CheckpointMotionTrace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool passed, const char* explanation)
{
    if (!passed)
    {
        std::cerr << "RivalDirectionalGates FAIL: " << explanation << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::CircuitGateSample At(int gate, double along, double lateral = 0.0,
                                double height = 180.0)
{
    const auto center = latebrake::CircuitGate(gate);
    const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
    const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
    return {{center.X + dx * along + dy * lateral,
             center.Y + dy * along + dx * lateral}, height};
}

double Distance(int gate, latebrake::CircuitGateSample point)
{
    const auto center = latebrake::CircuitGate(gate);
    return std::hypot(point.XY.X - center.X, point.XY.Y - center.Y);
}

double ForwardX(int gate) { return gate == 0 ? 1700.0 : gate == 2 ? -1700.0 : 0.0; }
double ForwardY(int gate) { return gate == 1 ? 1700.0 : gate == 3 ? -1700.0 : 0.0; }

latebrake::RouteObservation Feed(latebrake::RivalRouteProgress& route,
                                latebrake::CheckpointMotionTrace& trace,
                                int gate, latebrake::CircuitGateSample sample,
                                double vx, double vy)
{
    const bool crossed = trace.Sample(gate, 0, sample, vx, vy);
    return route.ObserveVerifiedGate(
        Distance(gate, sample), std::hypot(vx, vy), 0.05, crossed);
}
}

int main()
{
    using latebrake::CheckpointMotionTrace;
    using latebrake::RivalRouteProgress;

    RivalRouteProgress route(4, 2);
    CheckpointMotionTrace trace;

    for (int gate = 0; gate < 4; ++gate)
    {
        const auto before = At(gate, -650.0);
        const auto after = At(gate, 500.0);
        Require(!Feed(route, trace, gate, before,
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "initial sample establishes baseline, never a free rival lap");
        Require(Distance(gate, after) > 440.0,
                "forward sweep test intentionally ends outside old sphere");
        const auto observation = Feed(route, trace, gate, after,
                                      ForwardX(gate), ForwardY(gate));
        Require(observation.Advanced && !observation.NeedsRecovery &&
                !observation.Completed &&
                route.GetNextWaypoint() == (gate + 1) % 4,
                "real forward 3D gate sweep advances correct next waypoint");
        trace.Reset(); // real rival resets on every accepted gate
    }
    Require(route.GetCompletedLaps() == 1,
            "four correct directional plane crossings produce one approximate lap");
    for (int gate = 0; gate < 4; ++gate)
    {
        Feed(route, trace, gate, At(gate, -650), ForwardX(gate), ForwardY(gate));
        const auto passed = Feed(route, trace, gate, At(gate, 500),
                                 ForwardX(gate), ForwardY(gate));
        Require(passed.Advanced && passed.Completed == (gate == 3),
                "finished route requires exact sequential crossing on final lap");
        trace.Reset();
    }
    Require(route.IsFinished() && route.GetCompletedLaps() == 2,
            "no more rival progression after selected lap goal");
    Require(!Feed(route, trace, 0, At(0, 0), 1600, 0).Advanced,
            "finished race cannot score phantom extra lap");

    for (int gate = 0; gate < 4; ++gate)
    {
        RivalRouteProgress guarded(4, 3);
        CheckpointMotionTrace motion;
        // Bring current next gate to this gate without unverified skips.
        for (int previous = 0; previous < gate; ++previous)
        {
            motion.Reset();
            Feed(guarded, motion, previous, At(previous, -650),
                 ForwardX(previous), ForwardY(previous));
            Require(Feed(guarded, motion, previous, At(previous, 500),
                         ForwardX(previous), ForwardY(previous)).Advanced,
                    "test setup only advances via verified prior gates");
        }
        motion.Reset();
        Require(!Feed(guarded, motion, gate, At(gate, 0),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "mere proximity cannot credit a gate");
        Require(!Feed(guarded, motion, gate, At(gate, 0),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "dwelling inside gate sphere cannot repeatedly credit laps");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, -200),
             -ForwardX(gate), -ForwardY(gate));
        Require(!Feed(guarded, motion, gate, At(gate, 200),
                      -ForwardX(gate), -ForwardY(gate)).Advanced,
                "wrong-way velocity cannot claim forward positional crossing");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, 200),
             -ForwardX(gate), -ForwardY(gate));
        Require(!Feed(guarded, motion, gate, At(gate, -200),
                      -ForwardX(gate), -ForwardY(gate)).Advanced,
                "actual reverse plane traversal cannot count");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, -200, 600),
             ForwardX(gate), ForwardY(gate));
        Require(!Feed(guarded, motion, gate, At(gate, 200, 600),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "outside the real checkpoint lateral corridor cannot count");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, -200, 0, 750),
             ForwardX(gate), ForwardY(gate));
        Require(!Feed(guarded, motion, gate, At(gate, 200, 0, 750),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "high airborne shortcut cannot claim a rival gate");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, -1500),
             ForwardX(gate), ForwardY(gate));
        Require(!Feed(guarded, motion, gate, At(gate, 500),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "teleport-sized displacement cannot claim a checkpoint");
        motion.Reset();
        Feed(guarded, motion, gate, At(gate, -200),
             ForwardX(gate), ForwardY(gate));
        motion.Reset(); // race interruption/recovery just occurred
        Require(!Feed(guarded, motion, gate, At(gate, 200),
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "reset trace cannot bridge the stopped/teleported interval");
        Require(guarded.GetNextWaypoint() == gate &&
                guarded.GetCompletedLaps() == 0,
                "failed or spoofed crossings leave current rival gate unchanged");
    }

    // A verified crossing cannot fabricate progression while the recovery
    // request is awaiting the actual engine move result.
    RivalRouteProgress pending(4, 3);
    CheckpointMotionTrace pendingTrace;
    Require(pending.ObserveVerifiedGate(3000.0, 0.0, 8.0, false).NeedsRecovery,
            "existing physical stall requests bounded recovery");
    Require(!pending.ObserveVerifiedGate(0.0, 600.0, 0.05, true).Advanced,
            "pending teleport blocks even a purported valid directional gate");
    Require(pending.GetNextWaypoint() == 0 &&
            pending.ReportRecoveryResult(false) &&
            pending.GetFailedRecoveryCount() == 1,
            "failed physical placement is still audited, never credited");
    pendingTrace.Reset();
    Feed(pending, pendingTrace, 0, At(0, -250), 900, 0);
    Require(Feed(pending, pendingTrace, 0, At(0, 250), 900, 0).Advanced,
            "new real gate crossing can advance after resolved recovery");

    // Invalid position/velocity may not preserve an old trace baseline.
    CheckpointMotionTrace damaged;
    damaged.Sample(0, 0, At(0, -200), 1800, 0);
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    Require(!damaged.Sample(0, 0, At(0, -100), nan, 0),
            "bad velocity clears prior gate trace");
    Require(!damaged.Sample(0, 0, At(0, 200), 1800, 0),
            "recovering from bad physics requires fresh baseline");
    Require(!damaged.Sample(0, 0, At(0, 300), 1800, 0),
            "post-rebaseline same-side position cannot grant fake crossing");

    // Exercise the exact actual gate-trace/route composition for 200000
    // deterministic actor samples. Every valid frame pair is a real
    // short forward sweep; never touch the next gate out of order.
    RivalRouteProgress stress(4, 3);
    CheckpointMotionTrace sensor;
    for (int i = 0; i < 200000; ++i)
    {
        const int gate = i % 4;
        if (gate == 0)
        {
            stress.Reset();
        }
        sensor.Reset();
        const double lateral = static_cast<double>((i * 37) % 801) - 400.0;
        const double height = 80.0 + static_cast<double>(i % 230);
        const auto before = At(gate, -640.0, lateral, height);
        const auto after = At(gate, 520.0, lateral, height);
        Require(!Feed(stress, sensor, gate, before,
                      ForwardX(gate), ForwardY(gate)).Advanced,
                "no first-frame baseline can grant an AI gate");
        const auto accepted = Feed(stress, sensor, gate, after,
                                   ForwardX(gate), ForwardY(gate));
        Require(accepted.Advanced && !accepted.Completed &&
                stress.GetNextWaypoint() == (gate + 1) % 4,
                "200000 real trace samples preserve directional ordered progress");
    }

    std::cout << "RivalDirectionalGates: 200000 genuine plane samples passed (no UE)\n";
    return EXIT_SUCCESS;
}
