#include "Racing/CheckpointSessionTrace.h"
#include "Racing/RaceProgress.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Checkpoint session trace FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    constexpr double height = 150.0;
    RaceProgress race(CircuitGateCount, 1);
    std::array<CheckpointSessionTrace, CircuitGateCount> traces{};

    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint point = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample before{
            {point.X - dx * 100.0, point.Y - dy * 100.0}, height};
        const CircuitGateSample after{
            {point.X + dx * 100.0, point.Y + dy * 100.0}, height};

        Require(CrossesCircuitGatePlane(gate, before, after,
                                        dx * 300.0, dy * 300.0),
                "same positions would otherwise make a plausible crossing");

        CheckpointSessionTrace& trace = traces[static_cast<std::size_t>(gate)];
        Require(trace.IsNewSession(0), "first session has no baseline");
        Require(trace.SampleDetailed(gate, 4, 0, before,
                                    dx * 300.0, dy * 300.0).Trace ==
                    CheckpointTraceDecision::InitialBaseline,
                "first sample only establishes baseline");
        Require(!trace.IsNewSession(0) && trace.HasBaseline(),
                "same session keeps a valid sample");
        Require(trace.IsNewSession(1), "opening pause marks a discontinuity");
        Require(trace.SampleDetailed(gate, 4, 1, before,
                                    dx * 300.0, dy * 300.0).Trace ==
                    CheckpointTraceDecision::InitialBaseline,
                "pause boundary must reset the previous sample");
        Require(trace.IsNewSession(2), "resuming must invalidate paused motion");
        Require(!trace.SampleDetailed(gate, 4, 2, after,
                                     dx * 300.0, dy * 300.0).Crossed(),
                "resumed car cannot use pre-pause position for a reward");
        Require(race.GetNextCheckpoint() == gate,
                "resuming across gate cannot progress authoritative race");

        Require(!trace.SampleDetailed(gate, 4, 2, before,
                                     -dx * 300.0, -dy * 300.0).Crossed(),
                "moving back to approach side is not a scored crossing");
        Require(trace.SampleDetailed(gate, 4, 2, after,
                                    dx * 300.0, dy * 300.0).Crossed(),
                "genuine forward motion in resumed session scores");
        Require(race.PassCheckpoint(gate),
                "the real ordered race accepts an eligible gate");
    }
    Require(race.IsFinished() && race.GetCompletedLaps() == 1,
            "four real post-resume gates finish one ordered lap");

    CheckpointSessionTrace trace;
    const CircuitGateSample approach{{EastX - 100.0, SouthY}, height};
    const CircuitGateSample depart{{EastX + 100.0, SouthY}, height};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(!trace.SampleDetailed(0, 0, 9, approach, 300.0, 0.0).Crossed(),
            "fresh session is baseline");
    Require(!trace.SampleDetailed(0, 1, 9, depart, 300.0, 0.0).Crossed(),
            "teleport generation remains independent of menu session");
    trace.Reset();
    Require(trace.IsNewSession(9) && !trace.HasBaseline(),
            "unpossession reset removes session and previous position");
    Require(!trace.SampleDetailed(0, 1, 9, depart, 300.0, 0.0).Crossed(),
            "same revision after pawn change still starts new baseline");
    Require(!trace.SampleDetailed(0, 1, 9, {{nan, SouthY}, height},
                                  300.0, 0.0).Crossed() &&
            !trace.HasBaseline(), "nonfinite position cannot score");
    Require(!trace.SampleDetailed(0, 1, 9, approach, 300.0, 0.0).Crossed() &&
            trace.HasBaseline(), "valid motion reestablishes baseline");
    Require(trace.SampleDetailed(0, 1, 9, depart, 300.0, 0.0).Crossed(),
            "valid crossing works after corrupted position");

    Require(!trace.SampleDetailed(0, 1,
                    std::numeric_limits<std::uint64_t>::max(),
                    approach, 300.0, 0.0).Crossed(),
            "large revision cannot inherit old motion");
    Require(!trace.SampleDetailed(0, 1, 0, depart, 300.0, 0.0).Crossed(),
            "session revision wrap cannot earn a gate");
    Require(!trace.SampleDetailed(0, 1, 0, approach, -300.0, 0.0).Crossed(),
            "reverse return is not a crossing after wrap");
    Require(trace.SampleDetailed(0, 1, 0, depart, 300.0, 0.0).Crossed(),
            "real driving resumes after session wrap");

    std::cout << "Checkpoint pause session isolation passed.\n";
    return EXIT_SUCCESS;
}
