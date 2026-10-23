#include "Racing/CheckpointMotionTrace.h"
#include "Racing/CheckpointSessionTrace.h"
#include "Racing/GateDiagnosticCounters.h"
#include "Racing/RaceProgress.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* description)
{
    if (!ok)
    {
        std::cerr << "Checkpoint velocity continuity FAIL: "
                  << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    constexpr double height = 150.0;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double posInf = std::numeric_limits<double>::infinity();
    const double negInf = -std::numeric_limits<double>::infinity();

    // Use ORIGINAL RaceProgress: a diagnostic rejection alone is not
    // enough. No invalid physics sample may contribute to an ordered lap.
    RaceProgress race(CircuitGateCount, 1);
    GateDiagnosticCounters diagnostics;
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint center = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample before{
            {center.X - dx * 100.0, center.Y - dy * 100.0}, height};
        const CircuitGateSample approach{
            {center.X - dx * 10.0, center.Y - dy * 10.0}, height};
        const CircuitGateSample after{
            {center.X + dx * 100.0, center.Y + dy * 100.0}, height};
        CheckpointMotionTrace trace;
        const std::uint64_t generation = static_cast<std::uint64_t>(gate + 10);

        const auto invalidFirst = trace.SampleDetailed(
            gate, generation, before, nan, dy * 250.0);
        Require(invalidFirst.Trace == CheckpointTraceDecision::GateRejected &&
                invalidFirst.Gate == CircuitGateDecision::InvalidVelocity &&
                !trace.HasBaseline(),
                "invalid first velocity never establishes a baseline");
        diagnostics.Observe(invalidFirst);

        const auto initial = trace.SampleDetailed(
            gate, generation, before, dx * 250.0, dy * 250.0);
        Require(initial.Trace == CheckpointTraceDecision::InitialBaseline &&
                trace.HasBaseline(),
                "first fully valid sample can establish a baseline");

        // The invalid frame is still BEFORE the gate, so reusing this
        // untrusted position would let the subsequent valid AFTER sample
        // score a false crossing. Reset must discard that old trajectory.
        for (double invalid : {nan, posInf, negInf})
        {
            const auto bad = trace.SampleDetailed(
                gate, generation, approach,
                dx == 0.0 ? 0.0 : invalid,
                dy == 0.0 ? 0.0 : invalid);
            Require(bad.Trace == CheckpointTraceDecision::GateRejected &&
                    bad.Gate == CircuitGateDecision::InvalidVelocity &&
                    !trace.HasBaseline(),
                    "each nonfinite forward velocity clears the trace");
            diagnostics.Observe(bad);
            Require(trace.SampleDetailed(
                        gate, generation, before, dx * 250.0, dy * 250.0
                    ).Trace == CheckpointTraceDecision::InitialBaseline,
                    "valid sample after corrupt physics cannot inherit old point");
        }

        const auto rejected = trace.SampleDetailed(
            gate, generation, approach, nan, nan);
        Require(rejected.Trace == CheckpointTraceDecision::GateRejected &&
                rejected.Gate == CircuitGateDecision::InvalidVelocity &&
                !trace.HasBaseline(),
                "both velocity axes corrupt is a full trace discontinuity");
        diagnostics.Observe(rejected);

        // A single valid position AFTER the previous corrupt frame can
        // only rebase. It MUST NOT complete the real ordered checkpoint.
        const auto behindAfterCorruption = trace.SampleDetailed(
            gate, generation, after, dx * 250.0, dy * 250.0);
        Require(behindAfterCorruption.Trace ==
                    CheckpointTraceDecision::InitialBaseline &&
                !behindAfterCorruption.Crossed() &&
                race.GetNextCheckpoint() == gate,
                "crossing immediately after invalid velocity is not scored");

        Require(!trace.Sample(
            gate, generation, before, -dx * 250.0, -dy * 250.0),
            "return to approach side is not a forward crossing");
        Require(trace.Sample(
            gate, generation, after, dx * 250.0, dy * 250.0) &&
            race.PassCheckpoint(gate),
            "actual valid approach-then-forward sweep can still score");
    }

    Require(race.IsFinished() && race.GetCompletedLaps() == 1 &&
            race.GetNextCheckpoint() == 0,
            "four real gates, no phantom credits, complete exactly one lap");
    Require(diagnostics.TotalSamples() == 4U * CircuitGateCount &&
            diagnostics.CountGateRejection(
                CircuitGateDecision::InvalidVelocity) ==
                4U * CircuitGateCount,
            "diagnostics count every invalid-velocity discontinuity exactly once");

    // A bad frame also invalidates the cached baseline within a session.
    // The next good sample cannot reuse the pre-corruption position even
    // when BOTH the pause/session and teleport epochs stay unchanged.
    CheckpointSessionTrace session;
    constexpr CircuitGateSample before0{{EastX - 80.0, SouthY}, height};
    constexpr CircuitGateSample after0{{EastX + 80.0, SouthY}, height};
    Require(session.SampleDetailed(0, 18, 1234, before0, 270.0, 0.0)
                .Trace == CheckpointTraceDecision::InitialBaseline,
            "valid session starts with a baseline");
    const auto badSession = session.SampleDetailed(
        0, 18, 1234, before0, nan, 0.0);
    Require(badSession.Gate == CircuitGateDecision::InvalidVelocity &&
            !session.HasBaseline(),
            "nonfinite velocity invalidates even an unchanged session");
    Require(session.SampleDetailed(0, 18, 1234, after0, 270.0, 0.0)
                .Trace == CheckpointTraceDecision::InitialBaseline,
            "first valid sample after bad session velocity only rebases");
    Require(!session.SampleDetailed(0, 18, 1234, after0, 270.0, 0.0)
                .Crossed(),
            "repeated stationary after sample never wins a phantom gate");

    std::cout << "Checkpoint invalid-velocity continuity tests passed.\n";
    return EXIT_SUCCESS;
}
