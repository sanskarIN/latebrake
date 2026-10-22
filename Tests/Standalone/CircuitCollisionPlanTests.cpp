#include "Track/CircuitCollisionPlan.h"
#include "Track/CircuitStartupAudit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Circuit collision plan FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double x, double y) { return std::abs(x - y) < 1e-9; }
}

int main()
{
    using namespace latebrake;
    static_assert(CircuitCollisionBlockCount == 9);
    static_assert(CircuitStartupAudit::RequiredCollisionBlocks ==
                  CircuitCollisionBlockCount);
    static_assert(CircuitCollisionBlocks.size() == 9U);
    const auto ground = PlannedCollisionBlock(0);
    Require(ground.Style == CollisionBlockStyle::Ground &&
            Near(ground.Height, -100.0) &&
            Near(ground.ScaleX, 110.0) &&
            Near(ground.ScaleY, 85.0),
            "ground retains original centimeter transform");
    Require(PlannedCollisionBlock(1).Style == CollisionBlockStyle::Asphalt &&
            Near(PlannedCollisionBlock(1).Center.Y, SouthY) &&
            Near(PlannedCollisionBlock(2).Center.Y, NorthY),
            "north/south roads follow shared course layout");
    Require(Near(PlannedCollisionBlock(3).Center.X, EastX) &&
            Near(PlannedCollisionBlock(4).Center.X, WestX),
            "east/west roads follow checkpoint and AI geometry");

    int groundCount = 0;
    int roadCount = 0;
    int barrierCount = 0;
    CircuitStartupAudit audit;
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const auto block = PlannedCollisionBlock(i);
        Require(IsValidCollisionBlock(block), "positive finite geometry dimensions");
        switch (block.Style)
        {
        case CollisionBlockStyle::Ground: ++groundCount; break;
        case CollisionBlockStyle::Asphalt: ++roadCount; break;
        case CollisionBlockStyle::Barrier: ++barrierCount; break;
        }
        audit.RecordCollisionBlock(true);
    }
    Require(groundCount == 1 && roadCount == 4 && barrierCount == 4,
            "exact expected ground, road and barrier style grouping");
    Require(audit.CollisionReady(),
            "portable geometry plan feeds strict startup acceptance");

    for (int i = 5; i < CircuitCollisionBlockCount; ++i)
    {
        const auto block = PlannedCollisionBlock(i);
        Require(block.Style == CollisionBlockStyle::Barrier &&
                (std::abs(block.Center.X) > EastX ||
                 std::abs(block.Center.Y) > NorthY),
                "barriers remain outside all four waypoint centers");
    }
    Require(!IsValidCollisionBlock(PlannedCollisionBlock(-1)) &&
            !IsValidCollisionBlock(PlannedCollisionBlock(9)),
            "invalid plan indices cannot create valid collision geometry");

    std::cout << "Collision plan remains aligned with core circuit and startup audit.\n";
    return EXIT_SUCCESS;
}
