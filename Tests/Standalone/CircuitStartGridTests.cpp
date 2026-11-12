#include "Track/CircuitStartGrid.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* what)
{
    if (!pass)
    {
        std::cerr << "CircuitStartGrid FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Same(latebrake::CircuitPose a, latebrake::CircuitPose b)
{
    return std::abs(a.Position.X - b.Position.X) < 1.e-9 &&
        std::abs(a.Position.Y - b.Position.Y) < 1.e-9 &&
        std::abs(a.HeadingDegrees - b.HeadingDegrees) < 1.e-9;
}
}

int main()
{
    using namespace latebrake;
    static_assert(StartGridCapacity == 6);
    static_assert(HumanGridSlot == 0);
    static_assert(PrimaryRivalGridSlot == 1);
    Require(IsPlannedStartGridSafe(), "original six candidate positions all safe");
    Require(Same(StartGridPoses[0], PlayerStart) &&
            Same(StartGridPoses[1], RivalStart),
            "the existing physical player and only rival do not move");
    CircuitPose out{};
    for (int i = 0; i < StartGridCapacity; ++i)
    {
        Require(TryGetStartGridPose(i, out) &&
                Same(out, StartGridPoses[static_cast<std::size_t>(i)]),
                "every candidate slot resolves to its authored position");
        Require(StartGridFootprintOnPavement(out),
                "all four padded corners of each slot are paved");
        Require(!StartGridFootprintTouchesBarrier(out),
                "padded chassis does not intersect planned physical barriers");
        for (int j = 0; j < i; ++j)
        {
            Require(!StartGridFootprintsOverlap(
                out, StartGridPoses[static_cast<std::size_t>(j)]),
                "candidate cars have disjoint padded collision rectangles");
        }
    }
    for (int bad : {-1, StartGridCapacity, 100000})
    {
        out = PlayerStart;
        Require(!TryGetStartGridPose(bad, out) &&
                Same(out, CircuitPose{}),
                "invalid slot never falls back to real occupied grid");
    }
    auto changed = StartGridPoses;
    changed[4] = changed[3];
    const StartGridValidation overlapping = ValidateStartGrid(changed);
    Require(overlapping.Failure == StartGridFailure::CarOverlap &&
            overlapping.Slot == 4 && overlapping.OtherSlot == 3,
            "duplicate physical grid position is rejected with slot identities");

    changed = StartGridPoses;
    changed[4].Position = {0.0, 0.0};
    Require(ValidateStartGrid(changed).Failure == StartGridFailure::OffPavement,
            "center dirt cannot become a legal grid");
    changed = StartGridPoses;
    changed[4].Position = {WestX, 0.0};
    Require(IsOnPavedSurface(changed[4].Position) &&
            ValidateStartGrid(changed).Failure == StartGridFailure::OffPavement,
            "grid cannot migrate to a different asphalt slab");
    changed = StartGridPoses;
    changed[4].Position.Y = SouthY - 730.0;
    Require(IsOnPavedSurface(changed[4].Position) &&
            ValidateStartGrid(changed).Failure == StartGridFailure::OffPavement,
            "paved centre cannot hide a body extending beyond south asphalt");
    changed = StartGridPoses;
    changed[4].Position.X = -3700.0;
    Require(ValidateStartGrid(changed).Failure == StartGridFailure::BarrierOverlap,
            "car body footprint blocks a grid too close to west barrier");

    changed = StartGridPoses;
    changed[4].Position.X = std::numeric_limits<double>::quiet_NaN();
    Require(ValidateStartGrid(changed).Failure == StartGridFailure::InvalidPose,
            "nonfinite position rejects full grid");
    changed = StartGridPoses;
    changed[4].HeadingDegrees = std::numeric_limits<double>::infinity();
    Require(ValidateStartGrid(changed).Failure == StartGridFailure::InvalidPose,
            "nonfinite heading rejects full grid");
    changed = StartGridPoses;
    changed[4].HeadingDegrees = 36001.0;
    Require(ValidateStartGrid(changed).Failure == StartGridFailure::InvalidPose,
            "unbounded heading rejects full grid");

    // Diagonal cars are checked as oriented rectangles, not just distance
    // between centres. Rotate a car in place near another lane boundary.
    changed = StartGridPoses;
    changed[2].HeadingDegrees = 45.0;
    Require(StartGridFootprintOnPavement(changed[2]),
            "legal angled candidate remains inside actual asphalt");
    Require(!StartGridFootprintTouchesBarrier(changed[2]),
            "oriented body has barrier clearance");
    changed[2].Position = {changed[3].Position.X - 200.0,
                           changed[3].Position.Y - 200.0};
    Require(StartGridFootprintsOverlap(changed[2], changed[3]),
            "diagonal padded chassis overlap is caught");

    // Compare separation under rigid rotation of two cars in open space:
    // symmetric result, including touching padded margins.
    for (int i = 0; i < 3600; ++i)
    {
        const double heading = static_cast<double>(i) / 10.0;
        const CircuitPose a{{0.0, 0.0}, heading};
        const CircuitPose b{{0.0, 0.0}, heading + 180.0};
        Require(StartGridFootprintsOverlap(a, b) &&
                StartGridFootprintsOverlap(b, a),
                "all orientation samples reject coincident cars");
        const CircuitPose far{{3000.0, 3000.0}, heading};
        Require(!StartGridFootprintsOverlap(a, far) &&
                !StartGridFootprintsOverlap(far, a),
                "all orientation samples separate far-away cars");
    }

    // No stored navigation/AI/record state. The current runtime still has
    // one player and ONE qualified rival, not five spawned extra opponents.
    std::cout << "Six-slot original circuit grid geometry preflight passed.\n";
    return EXIT_SUCCESS;
}
