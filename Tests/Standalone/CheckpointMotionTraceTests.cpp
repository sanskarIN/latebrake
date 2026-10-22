#include "Racing/CheckpointMotionTrace.h"
#include "Racing/RaceProgress.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* explanation)
{
    if (!condition)
    {
        std::cerr << "Checkpoint motion trace FAIL: " << explanation << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    constexpr double height = 150.0;
    RaceProgress race(4, 1);
    std::array<CheckpointMotionTrace, CircuitGateCount> traces{};

    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint center = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample before{
            {center.X - 110.0 * dx, center.Y - 110.0 * dy}, height};
        const CircuitGateSample after{
            {center.X + 110.0 * dx, center.Y + 110.0 * dy}, height};

        // This short reposition is INSIDE the old <1200cm motion limit.
        // With forward-looking velocity, the old geometric-only check
        // would award the gate even though the player teleported.
        Require(CrossesCircuitGatePlane(gate, before, after,
                                       dx * 300.0, dy * 300.0),
                "short forward teleport is intentionally geometry-indistinguishable");
        CheckpointMotionTrace& trace = traces[static_cast<std::size_t>(gate)];
        Require(!trace.HasBaseline(), "new gate has no previous sample");
        Require(!trace.Sample(gate, 0, before, dx * 300.0, dy * 300.0) &&
                trace.HasBaseline(), "first position is only baseline");
        Require(!trace.Sample(gate, 1, after, dx * 300.0, dy * 300.0),
                "recovery changes generation and cannot win a short gate");
        Require(race.GetNextCheckpoint() == gate,
                "short teleport does not advance authoritative race");
        Require(!trace.Sample(gate, 1, after, dx * 300.0, dy * 300.0),
                "stationary first tick after teleport is not a crossing");
        Require(!trace.Sample(gate, 1, before, dx * 300.0, dy * 300.0),
                "repositioning to approach side does not claim reverse pass");
        Require(trace.Sample(gate, 1, after, dx * 300.0, dy * 300.0),
                "subsequent genuine forward crossing counts normally");
        Require(race.PassCheckpoint(gate),
                "ordered race progression accepts one real crossing");
        Require(!trace.Sample(gate, 1, after, dx * 300.0, dy * 300.0),
                "remaining beyond gate does not count again");
    }
    Require(race.IsFinished() && race.GetCompletedLaps() == 1,
            "all four true crossings finish precisely one lap");

    CheckpointMotionTrace trace;
    constexpr CircuitGateSample approach{{EastX - 100.0, SouthY}, height};
    constexpr CircuitGateSample depart{{EastX + 100.0, SouthY}, height};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(!trace.Sample(0, 6, approach, 350.0, 0.0),
            "fresh sample never awards a gate");
    Require(!trace.Sample(0, 7, depart, 350.0, 0.0),
            "restart epoch changes even if the same pawn instance is reused");
    Require(!trace.Sample(0, 7, approach, -350.0, 0.0),
            "backward movement never claims gate");
    Require(trace.Sample(0, 7, depart, 350.0, 0.0),
            "subsequent physical forward crossing accepted after restart");

    trace.Reset(); // Equivalent to observing another local player pawn.
    Require(!trace.HasBaseline(), "unpossession/pawn swap erases prior location");
    Require(!trace.Sample(0, 7, depart, 350.0, 0.0),
            "newly possessed player cannot inherit old motion");
    Require(!trace.Sample(0, 7, {{nan, SouthY}, height}, 350.0, 0.0) &&
            !trace.HasBaseline(), "corrupt current physics position clears trace");
    Require(!trace.Sample(0, 7, approach, 350.0, 0.0) &&
            trace.HasBaseline(), "valid position after NaN only reestablishes trace");
    Require(trace.Sample(0, 7, depart, 350.0, 0.0),
            "valid crossing works again after corrupt-frame reset");
    Require(!trace.Sample(0, std::numeric_limits<std::uint64_t>::max(),
                          approach, 350.0, 0.0),
            "large generation switch cannot reuse a previous trajectory");
    Require(!trace.Sample(0, 0, depart, 350.0, 0.0),
            "generation wrap also rebases rather than authorizing a jump");
    trace.Reset();
    Require(!trace.Sample(0, 0, approach, 350.0, 0.0),
            "explicit reset starts a new baseline");
    Require(trace.Sample(0, 0, depart, 350.0, 0.0),
            "legitimate unchanged-generation driving still counts");

    // Frame hitches may carry a legitimately driven car entirely across
    // the old 600cm trigger footprint. The new swept gate rule must work
    // through CheckpointMotionTrace and ordered RaceProgress, not only
    // through its low-level geometry helper.
    RaceProgress lowFpsRace(4, 1);
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        CheckpointMotionTrace lowFps;
        const CircuitPoint center = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample sweptStart{
            {center.X - dx * 950.0, center.Y - dy * 950.0}, height};
        const CircuitGateSample sweptEnd{
            {center.X + dx * 200.0, center.Y + dy * 200.0}, height};
        Require(!lowFps.Sample(gate, 15, sweptStart,
                               dx * 1000.0, dy * 1000.0),
                "swept gate first frame establishes movement baseline");
        Require(lowFps.Sample(gate, 15, sweptEnd,
                              dx * 1000.0, dy * 1000.0) &&
                lowFpsRace.PassCheckpoint(gate),
                "high-speed bounded sweep counts with same teleport generation");

        // The EXACT same two positions are not a real crossing when a
        // recover/restart operation advances its discontinuity epoch.
        CheckpointMotionTrace recovery;
        Require(!recovery.Sample(gate, 15, sweptStart,
                                 dx * 1000.0, dy * 1000.0),
                "recovery trace establishes old baseline");
        Require(!recovery.Sample(gate, 16, sweptEnd,
                                 dx * 1000.0, dy * 1000.0),
                "bounded high-speed-looking teleport is never awarded");
        Require(!recovery.Sample(gate, 16, sweptStart,
                                 -dx * 1000.0, -dy * 1000.0),
                "returning after teleport does not claim wrong-way crossing");
        Require(recovery.Sample(gate, 16, sweptEnd,
                                dx * 1000.0, dy * 1000.0),
                "normal driving can earn a gate after teleport");
    }
    Require(lowFpsRace.IsFinished() && lowFpsRace.GetCompletedLaps() == 1,
            "swept low-FPS laps complete all four ordered sectors");

    std::cout << "Four-gate teleport-proof motion traces passed.\n";
    return EXIT_SUCCESS;
}
