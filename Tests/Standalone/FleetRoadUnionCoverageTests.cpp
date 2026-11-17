#include "AI/RivalFleetCollisionForecast.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "FleetRoadUnionCoverage FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool WithinSingleAsphaltSlab(latebrake::CircuitPose pose)
{
    using namespace latebrake;
    const StartGridRect body = MakeStartGridRect(pose);
    const double px = StartGridProjectionRadius(body, 1.0, 0.0);
    const double py = StartGridProjectionRadius(body, 0.0, 1.0);
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const CircuitCollisionBlock slab = PlannedCollisionBlock(i);
        if (slab.Style == CollisionBlockStyle::Asphalt &&
            std::abs(body.X - slab.Center.X) + px <= 50.0 * slab.ScaleX &&
            std::abs(body.Y - slab.Center.Y) + py <= 50.0 * slab.ScaleY)
        {
            return true;
        }
    }
    return false;
}

void TestIntersectionAreaAndClipping()
{
    using namespace latebrake;
    const CircuitPose original{{2050.0, -1150.0}, 45.0};
    const StartGridRect bounds = MakeStartGridRect(original);
    const FleetRoadPolygon polygon = FleetBodyPolygon(bounds);
    const double expected = 4.0 * GridPaddedHalfLengthCm *
        GridPaddedHalfWidthCm;
    Require(polygon.Valid && polygon.Count == 4 &&
            std::abs(FleetClippedPolygonArea(polygon) - expected) < 1.0e-7,
            "full rigid chassis area comes from real padded body dimensions");

    const FleetRoadRectangle entirelyInside{
        1000.0, 2900.0, -2000.0, -900.0
    };
    const double cut = FleetClippedBodyArea(polygon, entirelyInside);
    Require(cut > 0.0 && cut < expected,
            "single-clipped real slab cannot cover two arms of a corner");

    const FleetRoadRectangle enclosing{
        -6000.0, 6000.0, -6000.0, 6000.0
    };
    Require(std::abs(FleetClippedBodyArea(polygon, enclosing) -
                     expected) < 1.0e-7,
            "full bounding box retains exact oriented chassis area");
    const FleetRoadRectangle disjoint{5000.0, 6000.0, 5000.0, 6000.0};
    Require(FleetClippedBodyArea(polygon, disjoint) == 0.0,
            "nonintersecting slab has exactly zero body coverage");

    FleetRoadPolygon invalid;
    invalid.Valid = false;
    Require(std::isnan(FleetClippedPolygonArea(invalid)),
            "invalid area propagation always fails closed");
}

void TestSafeCornerUnions()
{
    using namespace latebrake;
    const std::array<CircuitPose, 4> seams{{
        {{2050.0, -1150.0}, 45.0},
        {{2050.0, 1150.0}, -45.0},
        {{-2050.0, -1150.0}, -45.0},
        {{-2050.0, 1150.0}, 45.0}
    }};
    for (const CircuitPose seam : seams)
    {
        Require(IsOnPavedSurface(seam.Position) &&
                !WithinSingleAsphaltSlab(seam) &&
                !StartGridFootprintTouchesBarrier(seam),
                "fixture is a true road-union seam, not a single slab");
        Require(FleetBodyCoveredByAsphaltUnion(MakeStartGridRect(seam)) &&
                FleetBodyInsideAuthoredAsphalt(seam),
                "continuous pavement over two slabs permits cornering");
    }

    for (const CircuitPose p : {
        CircuitPose{{0.0, SouthY}, 0.0},
        CircuitPose{{0.0, NorthY}, 180.0},
        CircuitPose{{EastX, 0.0}, 90.0},
        CircuitPose{{WestX, 0.0}, -90.0}})
    {
        Require(WithinSingleAsphaltSlab(p) &&
                FleetBodyInsideAuthoredAsphalt(p),
                "ordinary straights retain the constant-time fast path");
    }
}

void TestRealOffRoadAndWallsRemainRejected()
{
    using namespace latebrake;
    for (const CircuitPose offroad : {
        CircuitPose{{2050.0, -900.0}, 45.0},
        CircuitPose{{2050.0, 900.0}, -45.0},
        CircuitPose{{-2050.0, -900.0}, -45.0},
        CircuitPose{{-2050.0, 900.0}, 45.0},
        CircuitPose{{0.0, SouthY + RoadHalfWidth - 60.0}, 0.0},
        CircuitPose{{0.0, SouthY - RoadHalfWidth + 60.0}, 0.0}})
    {
        Require(IsOnPavedSurface(offroad.Position),
                "unsafe fixture has genuinely paved centre");
        Require(!FleetBodyCoveredByAsphaltUnion(MakeStartGridRect(offroad)) &&
                !FleetBodyInsideAuthoredAsphalt(offroad),
                "a full-body overlap with central hole/edge must fail");
    }
    const CircuitPose wall{{3680.0, SouthY}, 0.0};
    Require(IsOnPavedSurface(wall.Position) &&
            StartGridFootprintTouchesBarrier(wall) &&
            !FleetBodyInsideAuthoredAsphalt(wall),
            "the real original physical wall still blocks propulsion");
    const CircuitPose nowhere{{0.0, 0.0}, 45.0};
    Require(!FleetBodyInsideAuthoredAsphalt(nowhere),
            "completely unpaved circuit interior never qualifies");
    const CircuitPose invalid{{
        std::numeric_limits<double>::quiet_NaN(), SouthY}, 0.0};
    Require(!FleetBodyInsideAuthoredAsphalt(invalid),
            "invalid world transform never looks like safe asphalt");
}

// Bounded deterministic changes in real chassis location/yaw. Any
// accepted body must have all boundary samples on original authored
// pavement, even when both asphalt arms are needed at a junction.
void TestCornerBoundaryPerturbations()
{
    using namespace latebrake;
    std::uint32_t seed = 0xA57A17U;
    const auto random32 = [&seed]() noexcept -> std::uint32_t
    {
        seed = 1664525U * seed + 1013904223U;
        return seed;
    };
    int accepted = 0;
    int rejected = 0;
    for (int trial = 0; trial < 16000; ++trial)
    {
        const double x = 1600.0 + (random32() % 850U);
        const double y = -1600.0 + (random32() % 880U);
        const double heading =
            static_cast<double>(random32() % 181U) - 90.0;
        const CircuitPose pose{{x, y}, heading};
        const bool allowed = FleetBodyInsideAuthoredAsphalt(pose);
        if (!allowed)
        {
            ++rejected;
            continue;
        }
        ++accepted;
        const FleetRoadPolygon polygon =
            FleetBodyPolygon(MakeStartGridRect(pose));
        Require(polygon.Valid && polygon.Count == 4 &&
                !StartGridFootprintTouchesBarrier(pose),
                "accepted road polygon is finite and excludes real walls");
        for (int edge = 0; edge < 4; ++edge)
        {
            const CircuitPoint a =
                polygon.Vertices[static_cast<std::size_t>(edge)];
            const CircuitPoint b =
                polygon.Vertices[static_cast<std::size_t>((edge + 1) % 4)];
            for (int step = 0; step <= 12; ++step)
            {
                const double t = static_cast<double>(step) / 12.0;
                Require(IsOnPavedSurface({
                    a.X + t * (b.X - a.X),
                    a.Y + t * (b.Y - a.Y)}),
                    "no accepted padded edge can leave authored road");
            }
        }
    }
    Require(accepted > 100 && rejected > 100,
            "deterministic sweep exercises both safe and unsafe corners");
}
} // namespace

int main()
{
    TestIntersectionAreaAndClipping();
    TestSafeCornerUnions();
    TestRealOffRoadAndWallsRemainRejected();
    TestCornerBoundaryPerturbations();
    std::cout << "Four-slab asphalt union + 16,000 corner sweeps passed.\n";
    return EXIT_SUCCESS;
}
