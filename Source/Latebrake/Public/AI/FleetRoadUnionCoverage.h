#pragma once

#include "Track/CircuitStartGrid.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Strictly bounded geometric fallback for an original racing chassis
// crossing the seam between ORIGINAL overlapping asphalt slabs.
// Fast single-slab containment remains the common driving path.
//
// Portable C++20. No heap, no simulation of Unreal/Chaos contact.
inline constexpr int FleetRoadMaximumAsphaltSlabs = 4;
inline constexpr int FleetRoadMaximumClippedVertices = 12;
inline constexpr double FleetRoadAreaToleranceCm2 = 0.000001;

struct FleetRoadRectangle final
{
    double MinX = 0.0;
    double MaxX = 0.0;
    double MinY = 0.0;
    double MaxY = 0.0;
};

struct FleetRoadPolygon final
{
    std::array<CircuitPoint, FleetRoadMaximumClippedVertices> Vertices{};
    int Count = 0;
    bool Valid = true;
};

inline bool AppendFleetRoadVertex(
    FleetRoadPolygon& polygon, CircuitPoint point) noexcept
{
    if (polygon.Count >= FleetRoadMaximumClippedVertices ||
        !std::isfinite(point.X) || !std::isfinite(point.Y))
    {
        polygon.Valid = false;
        return false;
    }
    polygon.Vertices[static_cast<std::size_t>(polygon.Count++)] = point;
    return true;
}

inline FleetRoadPolygon FleetBodyPolygon(
    const StartGridRect& body) noexcept
{
    FleetRoadPolygon polygon;
    for (int i = 0; i < 4; ++i)
    {
        const double forward = (i == 0 || i == 3) ? -1.0 : 1.0;
        const double right = (i < 2) ? -1.0 : 1.0;
        const CircuitPoint corner{
            body.X + forward * body.HalfLength * body.ForwardX +
                right * body.HalfWidth * body.RightX,
            body.Y + forward * body.HalfLength * body.ForwardY +
                right * body.HalfWidth * body.RightY
        };
        if (!AppendFleetRoadVertex(polygon, corner))
        {
            return polygon;
        }
    }
    return polygon;
}

// Sutherland-Hodgman convex clipping. A padded OBB starts with four
// corners, and four axis-aligned clipping planes require <=8 vertices.
inline FleetRoadPolygon ClipFleetRoadHalfplane(
    const FleetRoadPolygon& original,
    bool useX, double limit, bool keepGreater) noexcept
{
    if (!original.Valid || original.Count > FleetRoadMaximumClippedVertices)
    {
        return {{}, 0, false};
    }
    if (original.Count < 3)
    {
        return {};
    }
    FleetRoadPolygon clipped;
    CircuitPoint previous =
        original.Vertices[static_cast<std::size_t>(original.Count - 1)];
    const auto coordinate = [useX](CircuitPoint p) noexcept
    {
        return useX ? p.X : p.Y;
    };
    const auto inside = [limit, keepGreater](double component) noexcept
    {
        return keepGreater ? component >= limit : component <= limit;
    };
    bool previousInside = inside(coordinate(previous));
    for (int i = 0; i < original.Count; ++i)
    {
        const CircuitPoint current =
            original.Vertices[static_cast<std::size_t>(i)];
        const double start = coordinate(previous);
        const double finish = coordinate(current);
        const bool currentInside = inside(finish);
        if (previousInside != currentInside)
        {
            const double denominator = finish - start;
            if (!std::isfinite(denominator) || denominator == 0.0)
            {
                return {{}, 0, false};
            }
            const double fraction =
                std::clamp((limit - start) / denominator, 0.0, 1.0);
            const CircuitPoint intersection{
                previous.X + fraction * (current.X - previous.X),
                previous.Y + fraction * (current.Y - previous.Y)
            };
            if (!AppendFleetRoadVertex(clipped, intersection))
            {
                return clipped;
            }
        }
        if (currentInside && !AppendFleetRoadVertex(clipped, current))
        {
            return clipped;
        }
        previous = current;
        previousInside = currentInside;
    }
    return clipped;
}

inline double FleetClippedPolygonArea(
    const FleetRoadPolygon& polygon) noexcept
{
    if (!polygon.Valid)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (polygon.Count < 3)
    {
        return 0.0;
    }
    // Relative coordinates improve precision away from world origin.
    const CircuitPoint origin = polygon.Vertices[0];
    double twiceArea = 0.0;
    for (int i = 1; i + 1 < polygon.Count; ++i)
    {
        const CircuitPoint a =
            polygon.Vertices[static_cast<std::size_t>(i)];
        const CircuitPoint b =
            polygon.Vertices[static_cast<std::size_t>(i + 1)];
        twiceArea += (a.X - origin.X) * (b.Y - origin.Y) -
                     (a.Y - origin.Y) * (b.X - origin.X);
    }
    return std::abs(twiceArea) * 0.5;
}

inline double FleetClippedBodyArea(
    const FleetRoadPolygon& body,
    FleetRoadRectangle region) noexcept
{
    if (!(region.MinX < region.MaxX) ||
        !(region.MinY < region.MaxY))
    {
        return 0.0;
    }
    FleetRoadPolygon clipped =
        ClipFleetRoadHalfplane(body, true, region.MinX, true);
    clipped = ClipFleetRoadHalfplane(clipped, true, region.MaxX, false);
    clipped = ClipFleetRoadHalfplane(clipped, false, region.MinY, true);
    clipped = ClipFleetRoadHalfplane(clipped, false, region.MaxY, false);
    return FleetClippedPolygonArea(clipped);
}

// Four asphalt slabs overlap at the corners. Inclusion/exclusion over
// 15 nonempty rectangle subsets calculates true OBB-covered union area,
// not corner-only samples, which can miss an interior unpaved gap.
//
// An accepted body is contained to within 1e-6 square centimetres,
// a floating-point tolerance, not a physical tire/safety margin.
// The calling policy separately excludes original physical barriers.
inline bool FleetBodyCoveredByAsphaltUnion(
    const StartGridRect& body) noexcept
{
    const FleetRoadPolygon footprint = FleetBodyPolygon(body);
    if (!footprint.Valid)
    {
        return false;
    }
    std::array<FleetRoadRectangle, FleetRoadMaximumAsphaltSlabs> slabs{};
    int count = 0;
    for (int i = 0; i < CircuitCollisionBlockCount; ++i)
    {
        const CircuitCollisionBlock slab = PlannedCollisionBlock(i);
        if (slab.Style != CollisionBlockStyle::Asphalt)
        {
            continue;
        }
        if (!IsValidCollisionBlock(slab) ||
            count >= FleetRoadMaximumAsphaltSlabs)
        {
            return false;
        }
        const double halfX = 50.0 * slab.ScaleX;
        const double halfY = 50.0 * slab.ScaleY;
        slabs[static_cast<std::size_t>(count++)] = {
            slab.Center.X - halfX, slab.Center.X + halfX,
            slab.Center.Y - halfY, slab.Center.Y + halfY
        };
    }
    // Changed or missing source road plan requires a new review.
    if (count != FleetRoadMaximumAsphaltSlabs)
    {
        return false;
    }
    double coveredArea = 0.0;
    for (unsigned mask = 1; mask < (1U << count); ++mask)
    {
        FleetRoadRectangle common{
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::infinity()
        };
        for (int i = 0; i < count; ++i)
        {
            if ((mask & (1U << i)) == 0)
            {
                continue;
            }
            const FleetRoadRectangle& slab =
                slabs[static_cast<std::size_t>(i)];
            common.MinX = std::max(common.MinX, slab.MinX);
            common.MaxX = std::min(common.MaxX, slab.MaxX);
            common.MinY = std::max(common.MinY, slab.MinY);
            common.MaxY = std::min(common.MaxY, slab.MaxY);
        }
        const double area = FleetClippedBodyArea(footprint, common);
        if (!std::isfinite(area))
        {
            return false;
        }
        coveredArea += (std::popcount(mask) % 2U == 1U) ? area : -area;
    }
    const double fullArea =
        4.0 * body.HalfLength * body.HalfWidth;
    return std::isfinite(coveredArea) && std::isfinite(fullArea) &&
        fullArea > 0.0 &&
        std::abs(coveredArea - fullArea) <= FleetRoadAreaToleranceCm2;
}
} // namespace latebrake
