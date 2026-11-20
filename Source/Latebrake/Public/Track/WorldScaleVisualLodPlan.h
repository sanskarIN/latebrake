#pragma once

#include "Track/WorldScaleRoundedCircuit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace latebrake
{
// Exact source-only vertex layouts for eventual render-only UE mesh LODs.
// No mesh here may be used as a separate Chaos collision road, a track
// extension, an actor, a camera, or official lap/sector authority.
enum class WorldScaleVisualLod : std::uint8_t
{
    High = 0, Medium = 1, Far = 2
};
inline constexpr unsigned WorldScaleVisualLodCount = 3u;
inline constexpr std::array<unsigned,WorldScaleVisualLodCount>
    WorldScaleVisualLodSections{{1024u,512u,256u}};
inline constexpr std::array<double,WorldScaleVisualLodCount>
    WorldScaleVisualLodMaxMidpointErrorCm{{2.0,5.0,25.0}};
static_assert(WorldScaleVisualLodSections[0] ==
    WorldScaleRoadMeshSectionCount);

struct WorldScaleVisualLodSettings final
{
    double VisibleHalfWidthCm = WorldScaleVisibleHalfWidthCm;
    double VisibleTopHeightCm = RoundedSmoothTopCm;
    double VisibleBottomHeightCm = RoundedSmoothBottomCm;
    double MediumSwitchDistanceCm = 5000.0;
    double FarSwitchDistanceCm = 15000.0;
    double HysteresisDistanceCm = 1000.0;
    bool KeepOriginalPhysicalCollision = true;
    bool ForceRenderOnly = true;
    bool NeverChangeRaceClassification = true;
};

struct WorldScaleVisualLodMesh final
{
    WorldScaleVisualLod Level = WorldScaleVisualLod::High;
    RoundedSmoothRoadMesh Mesh{};
    unsigned Sections = 0;
    unsigned ValidatedOriginalRoadEndpoints = 0;
    double MaximumMidpointChordErrorCm = 0.0;
    double MaximumVisualWidthCm = 0.0;
    bool HasSeparateCollision = false;
    bool OriginalWorldStaticRoadUnmodified = true;
    bool Valid = false;
};

struct WorldScaleVisualLodPackage final
{
    std::array<WorldScaleVisualLodMesh,
               WorldScaleVisualLodCount> Levels{};
    std::size_t TotalVertices = 0;
    std::size_t TotalTriangles = 0;
    bool NoIndependentRoadCollision = true;
    bool Valid = false;
};

inline bool ValidWorldScaleVisualLodSettings(
    const WorldScaleVisualLodSettings& p) noexcept
{
    return std::isfinite(p.VisibleHalfWidthCm) &&
        p.VisibleHalfWidthCm >= 400.0 &&
        p.VisibleHalfWidthCm <= WorldScaleVisibleHalfWidthCm &&
        std::isfinite(p.VisibleTopHeightCm) &&
        p.VisibleTopHeightCm > RoundedRoadTopCm &&
        p.VisibleTopHeightCm <= RoundedRoadTopCm + 0.75 &&
        std::isfinite(p.VisibleBottomHeightCm) &&
        p.VisibleBottomHeightCm >= RoundedRoadTopCm &&
        p.VisibleBottomHeightCm < p.VisibleTopHeightCm &&
        std::isfinite(p.MediumSwitchDistanceCm) &&
        p.MediumSwitchDistanceCm >= 2500.0 &&
        p.MediumSwitchDistanceCm <= 10000.0 &&
        std::isfinite(p.FarSwitchDistanceCm) &&
        p.FarSwitchDistanceCm >= p.MediumSwitchDistanceCm + 2000.0 &&
        p.FarSwitchDistanceCm <= 30000.0 &&
        std::isfinite(p.HysteresisDistanceCm) &&
        p.HysteresisDistanceCm >= 0.0 &&
        p.HysteresisDistanceCm <= 1500.0 &&
        p.KeepOriginalPhysicalCollision &&
        p.ForceRenderOnly &&
        p.NeverChangeRaceClassification;
}

inline unsigned WorldScaleVisualLodSectionCount(
    WorldScaleVisualLod level) noexcept
{
    const auto index = static_cast<unsigned>(level);
    return index < WorldScaleVisualLodCount ?
        WorldScaleVisualLodSections[index] : 0u;
}

inline double WorldScaleVisualLodSectionMidpointError(
    unsigned section,
    unsigned sectionCount) noexcept
{
    if (sectionCount == 0u || section >= sectionCount)
        return std::numeric_limits<double>::quiet_NaN();
    const double s = WorldScaleCircuitLapCm *
        static_cast<double>(section) / sectionCount;
    const double e = WorldScaleCircuitLapCm *
        static_cast<double>(section + 1u) / sectionCount;
    const auto a = WorldScaleCenterlineAt(s);
    const auto b = WorldScaleCenterlineAt(e);
    const auto mid = WorldScaleCenterlineAt((s + e) * 0.5);
    if (!a.Valid || !b.Valid || !mid.Valid)
        return std::numeric_limits<double>::quiet_NaN();
    return std::hypot(
        mid.Pose.Position.X -
            (a.Pose.Position.X + b.Pose.Position.X) * 0.5,
        mid.Pose.Position.Y -
            (a.Pose.Position.Y + b.Pose.Position.Y) * 0.5);
}

inline WorldScaleVisualLodMesh BuildWorldScaleVisualLod(
    WorldScaleVisualLod level,
    WorldScaleVisualLodSettings settings = {})
{
    WorldScaleVisualLodMesh out{};
    out.Level = level;
    if (!ValidWorldScaleVisualLodSettings(settings))
        return out;
    const auto index = static_cast<unsigned>(level);
    if (index >= WorldScaleVisualLodCount)
        return out;
    const unsigned sections = WorldScaleVisualLodSections[index];
    auto& mesh = out.Mesh;
    mesh.Vertices.reserve(4u * sections);
    mesh.Triangles.reserve(8u * sections);
    for (unsigned section = 0; section < sections; ++section)
    {
        const double s = WorldScaleCircuitLapCm *
            static_cast<double>(section) / sections;
        const auto center = WorldScaleCenterlineAt(s);
        if (!center.Valid ||
            !IsOnWorldScaleRoad(center.Pose.Position))
            return {};
        const double heading =
            center.Pose.HeadingDegrees * RoundedPi / 180.0;
        const double nx = -std::sin(heading);
        const double ny = std::cos(heading);
        for (double height : {
            settings.VisibleTopHeightCm, settings.VisibleBottomHeightCm})
        {
            for (double side : {1.0,-1.0})
            {
                const RoundedSmoothVertex vertex{
                    center.Pose.Position.X +
                        nx * side * settings.VisibleHalfWidthCm,
                    center.Pose.Position.Y +
                        ny * side * settings.VisibleHalfWidthCm,
                    height
                };
                if (!IsFiniteRoundedSmoothVertex(vertex) ||
                    !IsOnWorldScaleRoad({vertex.X,vertex.Y}))
                    return {};
                mesh.Vertices.push_back(vertex);
            }
        }
        out.ValidatedOriginalRoadEndpoints += 4u;
        const double error = WorldScaleVisualLodSectionMidpointError(
            section,sections);
        if (!std::isfinite(error) ||
            error > WorldScaleVisualLodMaxMidpointErrorCm[index])
            return {};
        out.MaximumMidpointChordErrorCm = std::max(
            out.MaximumMidpointChordErrorCm, error);
    }
    // Winding matches the original closed high-resolution ribbon.
    // Indices are LT/RT/LB/RB at every source section. No caps.
    for (std::uint32_t i = 0; i < sections; ++i)
    {
        const std::uint32_t j = (i + 1u) % sections;
        const std::uint32_t a = 4u * i;
        const std::uint32_t b = 4u * j;
        for (const std::array<std::uint32_t, 3>& triangle : {
            std::array<std::uint32_t,3>{a,a+1u,b+1u},
            std::array<std::uint32_t,3>{a,b+1u,b},
            std::array<std::uint32_t,3>{a+2u,b+3u,a+3u},
            std::array<std::uint32_t,3>{a+2u,b+2u,b+3u},
            std::array<std::uint32_t,3>{a,b,b+2u},
            std::array<std::uint32_t,3>{a,b+2u,a+2u},
            std::array<std::uint32_t,3>{a+1u,a+3u,b+3u},
            std::array<std::uint32_t,3>{a+1u,b+3u,b+1u}
        })
        {
            mesh.Triangles.push_back({triangle});
        }
    }
    out.Sections = sections;
    out.MaximumVisualWidthCm =
        settings.VisibleHalfWidthCm * 2.0;
    out.Valid = mesh.Vertices.size() == 4u * sections &&
        mesh.Triangles.size() == 8u * sections &&
        out.ValidatedOriginalRoadEndpoints == 4u * sections;
    if (!out.Valid) return {};
    return out;
}

inline WorldScaleVisualLodPackage BuildWorldScaleVisualLods(
    WorldScaleVisualLodSettings settings = {})
{
    WorldScaleVisualLodPackage package{};
    if (!ValidWorldScaleVisualLodSettings(settings))
        return package;
    for (unsigned i = 0; i < WorldScaleVisualLodCount; ++i)
    {
        auto& level = package.Levels[i];
        level = BuildWorldScaleVisualLod(
            static_cast<WorldScaleVisualLod>(i), settings);
        if (!level.Valid) return {};
        package.TotalVertices += level.Mesh.Vertices.size();
        package.TotalTriangles += level.Mesh.Triangles.size();
    }
    package.Valid = package.TotalVertices ==
        4u * (1024u+512u+256u) &&
        package.TotalTriangles == 8u * (1024u+512u+256u);
    return package;
}

// Client-side render selection is advisory only. Hysteresis keeps the
// LOD from oscillating at precisely the camera switch distance.
// This is a pure decision; no camera state, tick, mesh or GPU is accessed.
inline WorldScaleVisualLod ChooseWorldScaleVisualLod(
    double measuredCameraDistanceCm,
    WorldScaleVisualLod previous,
    WorldScaleVisualLodSettings settings = {}) noexcept
{
    if (!ValidWorldScaleVisualLodSettings(settings) ||
        !std::isfinite(measuredCameraDistanceCm) ||
        measuredCameraDistanceCm < 0.0 ||
        measuredCameraDistanceCm > 1000000.0 ||
        static_cast<unsigned>(previous) >= WorldScaleVisualLodCount)
        return previous;

    const double middle = settings.MediumSwitchDistanceCm;
    const double distant = settings.FarSwitchDistanceCm;
    const double hysteresis = settings.HysteresisDistanceCm;
    if (previous == WorldScaleVisualLod::High)
    {
        return measuredCameraDistanceCm > middle + hysteresis ?
            WorldScaleVisualLod::Medium : previous;
    }
    if (previous == WorldScaleVisualLod::Medium)
    {
        if (measuredCameraDistanceCm < middle - hysteresis)
            return WorldScaleVisualLod::High;
        if (measuredCameraDistanceCm > distant + hysteresis)
            return WorldScaleVisualLod::Far;
        return previous;
    }
    return measuredCameraDistanceCm < distant - hysteresis ?
        WorldScaleVisualLod::Medium : previous;
}

inline bool AuditWorldScaleVisualLod(
    const WorldScaleVisualLodMesh& lod) noexcept
{
    if (!lod.Valid || lod.HasSeparateCollision ||
        !lod.OriginalWorldStaticRoadUnmodified ||
        lod.Sections == 0u ||
        lod.Sections != WorldScaleVisualLodSectionCount(lod.Level) ||
        lod.Mesh.Vertices.size() != 4u * lod.Sections ||
        lod.Mesh.Triangles.size() != 8u * lod.Sections)
    {
        return false;
    }
    for (const auto& vertex : lod.Mesh.Vertices)
    {
        if (!IsFiniteRoundedSmoothVertex(vertex) ||
            !IsOnWorldScaleRoad({vertex.X,vertex.Y}))
            return false;
    }
    for (const auto& triangle : lod.Mesh.Triangles)
    {
        const auto& vertices = triangle.Indices;
        for (const auto index : vertices)
        {
            if (index >= lod.Mesh.Vertices.size()) return false;
        }
        if (vertices[0] == vertices[1] ||
            vertices[1] == vertices[2] ||
            vertices[2] == vertices[0])
            return false;
    }
    return std::isfinite(lod.MaximumMidpointChordErrorCm) &&
        lod.MaximumMidpointChordErrorCm <=
            WorldScaleVisualLodMaxMidpointErrorCm[
                static_cast<unsigned>(lod.Level)];
}
} // namespace latebrake
