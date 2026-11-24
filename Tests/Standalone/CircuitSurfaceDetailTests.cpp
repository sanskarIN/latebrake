#include "Track/CircuitSurfaceDetail.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "CircuitSurfaceDetail FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(CircuitSurfaceDetailCount == 56);
    static_assert(CircuitSurfaceEdgeCount == 8);
    static_assert(CircuitRubberCount == 48);
    Require(PlannedSurfaceStyleCount(CircuitSurfaceStyle::EdgePaint) == 8 &&
            PlannedSurfaceStyleCount(CircuitSurfaceStyle::Rubber) == 48 &&
            PlannedSurfaceStyleCount(CircuitSurfaceStyle::Count) == 0,
            "exact two independent decoration batch budgets");

    int edges = 0;
    int rubber = 0;
    for (int index = 0; index < CircuitSurfaceDetailCount; ++index)
    {
        const auto mark = PlannedCircuitSurfaceDetail(index);
        Require(mark.Valid && IsSurfaceDetailOnPhysicalAsphalt(mark),
                "EVERY full marker footprint inside a real asphalt slab");
        Require(std::abs(mark.Height - 0.15) < 1e-9 &&
                std::abs(mark.ScaleZ - 0.022) < 1e-9,
                "visible marking bottom flush with physical asphalt top");
        if (mark.Style == CircuitSurfaceStyle::EdgePaint)
        {
            ++edges;
            Require(index < CircuitSurfaceEdgeCount,
                    "only eight real straight-edge lines");
            const auto mate = PlannedCircuitSurfaceDetail(index ^ 1);
            Require(mate.Valid && mate.Style == CircuitSurfaceStyle::EdgePaint,
                    "two opposite roadside stripes per straight");
        }
        else if (mark.Style == CircuitSurfaceStyle::Rubber)
        {
            ++rubber;
            Require(index >= CircuitSurfaceEdgeCount,
                    "skid rubber is painted only in four braking approaches");
        }
        else
        {
            Require(false, "no phantom decoration group");
        }
    }
    Require(edges == 8 && rubber == 48,
            "complete painted-edge/rubber fixture count");
    Require(!PlannedCircuitSurfaceDetail(-1).Valid &&
            !PlannedCircuitSurfaceDetail(CircuitSurfaceDetailCount).Valid &&
            !PlannedCircuitSurfaceDetail(500).Valid,
            "invalid indices fail closed");

    auto invalid = PlannedCircuitSurfaceDetail(0);
    invalid.Center.X = std::numeric_limits<double>::quiet_NaN();
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "NaN transform refused");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.ScaleY = -0.5;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "negative footprint refused");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.Height = 10.0;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "floating line refused");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.Height = -5.0;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "buried line refused");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.ScaleX = 900.0;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "line covering grass rejected even if center on asphalt");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.Center.Y = -100.0;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "grass-center paint refused");
    invalid = PlannedCircuitSurfaceDetail(0);
    invalid.Style = CircuitSurfaceStyle::Count;
    Require(!IsSurfaceDetailOnPhysicalAsphalt(invalid),
            "unknown color group refused");

    std::cout << "56 physical-footprint-safe surface detail placements passed\n";
    return EXIT_SUCCESS;
}
