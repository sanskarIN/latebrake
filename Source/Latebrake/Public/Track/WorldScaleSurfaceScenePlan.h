#pragma once

#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake
{
// A source-authored mesh-placement plan for the FUTURE world-scale route.
// None of these quads carries collision, friction, lap/sector timing,
// checkpoint credit or road authority. Future Unreal adapters must assign
// non-colliding render sections and NEVER create physical road from them.
inline constexpr unsigned WorldScaleSceneSections =
    WorldScaleRoadMeshSectionCount;
inline constexpr std::size_t WorldScaleSceneLayerCount = 5u;
inline constexpr std::size_t WorldScaleSceneMaximumQuads =
    WorldScaleSceneSections * 5u;
static_assert(WorldScaleSceneSections == 1024);
static_assert(WorldScaleVisibleHalfWidthCm <
              WorldScaleRoadHalfWidthCm);

enum class WorldScaleSceneLayer : std::uint8_t
{
    LeftEdgePaint = 0,
    RightEdgePaint = 1,
    CenterDashPaint = 2,
    LeftKerbPaint = 3,
    RightKerbPaint = 4
};

struct WorldScaleSceneVertex final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

struct WorldScaleSceneQuad final
{
    std::array<WorldScaleSceneVertex, 4> Corners{};
    double StartDistanceCm = 0.0;
    double EndDistanceCm = 0.0;
    unsigned SourceSection = 0;
    WorldScaleSceneLayer Layer = WorldScaleSceneLayer::CenterDashPaint;
    bool RedKerb = false;
    bool VisualOnly = true;
};

struct WorldScaleSceneSettings final
{
    double EdgePaintDistanceCm = 382.0;
    double EdgePaintWidthCm = 11.0;
    double CenterDashWidthCm = 9.0;
    double KerbDistanceCm = 406.0;
    double KerbWidthCm = 25.0;
    double KerbCurvatureThresholdPerCm = 0.00006;
    double MarkingLiftAboveRoadCm = 0.30;
    unsigned DashedLengthSections = 4;
    unsigned DashedPeriodSections = 8;
    bool RequireOriginalPhysicalRoadSupport = true;
    bool DisableIndependentCollision = true;
    bool LeaveCheckpointAuthorityUnchanged = true;
};

struct WorldScaleSceneGuidePlan final
{
    std::vector<WorldScaleSceneQuad> Quads{};
    std::array<unsigned, WorldScaleSceneLayerCount> LayerQuads{};
    double TotalVisibleAreaSquareCm = 0.0;
    double MaximumQuadLengthCm = 0.0;
    unsigned PhysicalSourceSectionsVerified = 0;
    unsigned BendSections = 0;
    bool VisualOnly = true;
    bool CollisionRoadUnchanged = true;
    bool OfficialProgressUnchanged = true;
    bool Valid = false;
};

inline bool FiniteWorldScaleSceneVertex(
    WorldScaleSceneVertex point) noexcept
{
    return std::isfinite(point.X) && std::isfinite(point.Y) &&
        std::isfinite(point.Z) && std::abs(point.X) < 1000000.0 &&
        std::abs(point.Y) < 1000000.0;
}

inline double WorldScaleSceneSignedDoubleArea(
    const WorldScaleSceneQuad& q) noexcept
{
    const auto& a = q.Corners[0];
    const auto& b = q.Corners[1];
    const auto& c = q.Corners[2];
    return (b.X - a.X) * (c.Y - a.Y) -
           (b.Y - a.Y) * (c.X - a.X);
}

inline bool ValidWorldScaleSceneSettings(
    const WorldScaleSceneSettings& p) noexcept
{
    return std::isfinite(p.EdgePaintDistanceCm) &&
        p.EdgePaintDistanceCm >= 100.0 &&
        p.EdgePaintDistanceCm <= 400.0 &&
        std::isfinite(p.EdgePaintWidthCm) &&
        p.EdgePaintWidthCm >= 2.0 && p.EdgePaintWidthCm <= 24.0 &&
        std::isfinite(p.CenterDashWidthCm) &&
        p.CenterDashWidthCm >= 2.0 &&
        p.CenterDashWidthCm <= 20.0 &&
        std::isfinite(p.KerbDistanceCm) &&
        p.KerbDistanceCm >= p.EdgePaintDistanceCm + 10.0 &&
        p.KerbDistanceCm <= 410.0 &&
        std::isfinite(p.KerbWidthCm) &&
        p.KerbWidthCm >= 10.0 && p.KerbWidthCm <= 30.0 &&
        p.KerbDistanceCm + p.KerbWidthCm * 0.5 <
            WorldScaleVisibleHalfWidthCm &&
        p.EdgePaintDistanceCm + p.EdgePaintWidthCm * 0.5 <
            p.KerbDistanceCm - p.KerbWidthCm * 0.5 &&
        std::isfinite(p.KerbCurvatureThresholdPerCm) &&
        p.KerbCurvatureThresholdPerCm >= 0.00002 &&
        p.KerbCurvatureThresholdPerCm <= 0.00015 &&
        std::isfinite(p.MarkingLiftAboveRoadCm) &&
        p.MarkingLiftAboveRoadCm >= 0.10 &&
        p.MarkingLiftAboveRoadCm <= 0.50 &&
        p.DashedLengthSections >= 1u &&
        p.DashedLengthSections < p.DashedPeriodSections &&
        p.DashedPeriodSections <= 32u &&
        p.RequireOriginalPhysicalRoadSupport &&
        p.DisableIndependentCollision &&
        p.LeaveCheckpointAuthorityUnchanged;
}

inline WorldScaleSceneVertex WorldScaleScenePoint(
    const RoundedCenterlinePose& center,
    double signedLateralCm,
    double z) noexcept
{
    const double angle =
        center.Pose.HeadingDegrees * WorldScalePaceRadPerDegree;
    const double nx = -std::sin(angle);
    const double ny = std::cos(angle);
    return {
        center.Pose.Position.X + signedLateralCm * nx,
        center.Pose.Position.Y + signedLateralCm * ny, z
    };
}

inline bool WorldScaleSceneQuadSupported(
    const WorldScaleSceneQuad& quad) noexcept
{
    if (quad.SourceSection >= WorldScaleSceneSections ||
        !quad.VisualOnly || !std::isfinite(quad.StartDistanceCm) ||
        !std::isfinite(quad.EndDistanceCm) ||
        quad.StartDistanceCm < 0.0 ||
        quad.EndDistanceCm > WorldScaleCircuitLapCm ||
        quad.EndDistanceCm <= quad.StartDistanceCm ||
        static_cast<std::size_t>(quad.Layer) >=
            WorldScaleSceneLayerCount ||
        !std::isfinite(WorldScaleSceneSignedDoubleArea(quad)) ||
        WorldScaleSceneSignedDoubleArea(quad) <= 1.0e-5)
    {
        return false;
    }
    for (const auto& p : quad.Corners)
    {
        if (!FiniteWorldScaleSceneVertex(p) ||
            p.Z <= RoundedRoadTopCm ||
            p.Z >= RoundedRoadTopCm + 1.0 ||
            !IsOnWorldScaleRoad({p.X, p.Y}))
        {
            return false;
        }
    }
    return true;
}

inline bool AppendWorldScaleSceneQuad(
    WorldScaleSceneGuidePlan& plan,
    const RoundedCenterlinePose& first,
    const RoundedCenterlinePose& second,
    unsigned section,
    double startCm,
    double endCm,
    double signedOffsetCm,
    double stripeWidthCm,
    double z,
    WorldScaleSceneLayer layer,
    bool redKerb = false)
{
    if (plan.Quads.size() >= WorldScaleSceneMaximumQuads ||
        !first.Valid || !second.Valid)
    {
        return false;
    }
    const double lo = signedOffsetCm - stripeWidthCm * 0.5;
    const double hi = signedOffsetCm + stripeWidthCm * 0.5;
    WorldScaleSceneQuad quad{};
    quad.Corners = {
        WorldScaleScenePoint(first, lo, z),
        WorldScaleScenePoint(second, lo, z),
        WorldScaleScenePoint(second, hi, z),
        WorldScaleScenePoint(first, hi, z)
    };
    quad.StartDistanceCm = startCm;
    quad.EndDistanceCm = endCm;
    quad.SourceSection = section;
    quad.Layer = layer;
    quad.RedKerb = redKerb;
    if (!WorldScaleSceneQuadSupported(quad))
    {
        return false;
    }

    const auto& a = quad.Corners[0];
    const auto& b = quad.Corners[1];
    const double run = std::hypot(
        b.X - a.X, b.Y - a.Y);
    const double quadArea =
        0.5 * WorldScaleSceneSignedDoubleArea(quad) +
        0.5 * ((quad.Corners[2].X - a.X) *
                   (quad.Corners[3].Y - a.Y) -
               (quad.Corners[2].Y - a.Y) *
                   (quad.Corners[3].X - a.X));
    if (!std::isfinite(run) || !std::isfinite(quadArea) ||
        run <= 0.0 || quadArea <= 1.0e-5)
    {
        return false;
    }
    plan.MaximumQuadLengthCm = std::max(
        plan.MaximumQuadLengthCm, run);
    plan.TotalVisibleAreaSquareCm += quadArea;
    ++plan.LayerQuads[static_cast<std::size_t>(layer)];
    plan.Quads.push_back(quad);
    return true;
}

// Every marking vertex is individually checked against the exact
// 68-piece authored physical-road UNION, not merely the centreline
// distance. A visual strip may not become a hidden drivable shoulder.
inline WorldScaleSceneGuidePlan BuildWorldScaleSceneGuidePlan(
    WorldScaleSceneSettings p = {})
{
    if (!ValidWorldScaleSceneSettings(p))
    {
        return {};
    }
    WorldScaleSceneGuidePlan plan{};
    plan.Quads.reserve(WorldScaleSceneMaximumQuads);
    const double length = WorldScaleCircuitLapCm /
        static_cast<double>(WorldScaleSceneSections);
    const double z = RoundedRoadTopCm +
        p.MarkingLiftAboveRoadCm;
    for (unsigned i = 0; i < WorldScaleSceneSections; ++i)
    {
        const double start = i * length;
        const double end = (i + 1u) * length;
        const auto first = WorldScaleCenterlineAt(start);
        const auto second = WorldScaleCenterlineAt(end);
        if (!first.Valid || !second.Valid ||
            !IsOnWorldScaleRoad(first.Pose.Position) ||
            !IsOnWorldScaleRoad(second.Pose.Position))
        {
            return {};
        }
        ++plan.PhysicalSourceSectionsVerified;
        if (!AppendWorldScaleSceneQuad(
                plan, first, second, i, start, end,
                p.EdgePaintDistanceCm, p.EdgePaintWidthCm,
                z, WorldScaleSceneLayer::LeftEdgePaint) ||
            !AppendWorldScaleSceneQuad(
                plan, first, second, i, start, end,
                -p.EdgePaintDistanceCm, p.EdgePaintWidthCm,
                z, WorldScaleSceneLayer::RightEdgePaint))
        {
            return {};
        }

        if ((i % p.DashedPeriodSections) <
             p.DashedLengthSections)
        {
            if (!AppendWorldScaleSceneQuad(
                    plan, first, second, i, start, end,
                    0.0, p.CenterDashWidthCm,
                    z, WorldScaleSceneLayer::CenterDashPaint))
            {
                return {};
            }
        }

        const double curvature = std::abs(WorldScaleSampleCurvature(
            start + 0.5 * length, std::min(150.0, length * 0.45)));
        if (!std::isfinite(curvature)) return {};
        if (curvature > p.KerbCurvatureThresholdPerCm)
        {
            ++plan.BendSections;
            // Striping is symmetric so drivers can identify either
            // outer edge of the same original world-scale road.
            const bool red = (i / 3u) % 2u == 0u;
            if (!AppendWorldScaleSceneQuad(
                    plan, first, second, i, start, end,
                    p.KerbDistanceCm, p.KerbWidthCm, z,
                    WorldScaleSceneLayer::LeftKerbPaint, red) ||
                !AppendWorldScaleSceneQuad(
                    plan, first, second, i, start, end,
                    -p.KerbDistanceCm, p.KerbWidthCm, z,
                    WorldScaleSceneLayer::RightKerbPaint, red))
            {
                return {};
            }
        }
    }
    if (plan.PhysicalSourceSectionsVerified !=
            WorldScaleSceneSections ||
        plan.LayerQuads[0] != WorldScaleSceneSections ||
        plan.LayerQuads[1] != WorldScaleSceneSections ||
        plan.LayerQuads[2] == 0u ||
        plan.LayerQuads[3] == 0u ||
        plan.LayerQuads[3] != plan.LayerQuads[4] ||
        plan.BendSections < 20u ||
        plan.Quads.size() > WorldScaleSceneMaximumQuads ||
        !std::isfinite(plan.TotalVisibleAreaSquareCm) ||
        !std::isfinite(plan.MaximumQuadLengthCm) ||
        plan.TotalVisibleAreaSquareCm <= 100000.0)
    {
        return {};
    }
    plan.Valid = true;
    return plan;
}

inline bool AuditWorldScaleSceneGuidePlan(
    const WorldScaleSceneGuidePlan& plan) noexcept
{
    if (!plan.Valid || !plan.VisualOnly ||
        !plan.CollisionRoadUnchanged ||
        !plan.OfficialProgressUnchanged ||
        plan.Quads.size() > WorldScaleSceneMaximumQuads)
    {
        return false;
    }
    std::array<unsigned, WorldScaleSceneLayerCount> counted{};
    double totalArea = 0.0;
    for (const auto& quad : plan.Quads)
    {
        if (!WorldScaleSceneQuadSupported(quad)) return false;
        ++counted[static_cast<std::size_t>(quad.Layer)];
        const auto& a = quad.Corners[0];
        const auto& c = quad.Corners[2];
        const auto& d = quad.Corners[3];
        totalArea += 0.5 * WorldScaleSceneSignedDoubleArea(quad) +
            0.5 * ((c.X - a.X) * (d.Y - a.Y) -
                   (c.Y - a.Y) * (d.X - a.X));
    }
    return counted == plan.LayerQuads &&
        std::isfinite(totalArea) &&
        std::abs(totalArea - plan.TotalVisibleAreaSquareCm) <=
            std::max(0.01, totalArea * 1.0e-10);
}
} // namespace latebrake
