#pragma once

#include "Track/RoundedCircuitSmoothMesh.h"

#include <cmath>

namespace latebrake
{
// FUTURE CIRCUIT ONLY: upscale the X/Y centreline by 10x to make a
// 1.70265-km road, but keep the 900-cm actual physical road width and
// 425-cm visual half-width unchanged. Never scale the whole mesh / 3D
// car; that would create a 90-m wide road and implausible vehicles.
// This does NOT change the live GameMode, original 170-m geometry lab,
// vehicle physics, AI, checkpoint logic or release-build defaults.
inline constexpr double WorldScaleCircuitFactor = 10.0;
inline constexpr double WorldScaleCircuitLapCm =
    RoundedLapLengthCm * WorldScaleCircuitFactor;
inline constexpr double WorldScaleRoadHalfWidthCm = RoundedHalfRoadWidthCm;
inline constexpr double WorldScaleVisibleHalfWidthCm = RoundedSmoothHalfWidthCm;
inline constexpr int WorldScaleRoadPieceCount = RoundedPhysicalRoadCount;
inline constexpr int WorldScaleRoadMeshSectionCount = RoundedSmoothRoadSections;
static_assert(WorldScaleRoadPieceCount == 68);
static_assert(WorldScaleRoadHalfWidthCm == 450.0);
static_assert(WorldScaleVisibleHalfWidthCm == 425.0);
static_assert(WorldScaleCircuitLapCm > 160000.0);
static_assert(WorldScaleCircuitLapCm < 180000.0);

// Physical road consists of the same 68 independently testable rotated
// collision boxes, with *only* world-space X/Y and LOCAL X (tangential
// length) scaled. LOCAL Y is kept at the original 900-cm paved width.
// The curve remains physically collision-backed; this visual overlay is not.
inline RoundedPhysicalRoadPiece PlannedWorldScaleRoadPiece(int index) noexcept
{
    auto piece = PlannedRoundedPhysicalRoadPiece(index);
    if (!piece.Valid) return piece;
    piece.Center.X *= WorldScaleCircuitFactor;
    piece.Center.Y *= WorldScaleCircuitFactor;
    piece.ScaleX *= WorldScaleCircuitFactor;
    return piece;
}

inline bool IsOnWorldScaleRoad(CircuitPoint point) noexcept
{
    if (!std::isfinite(point.X) || !std::isfinite(point.Y)) return false;
    for (int index = 0; index < WorldScaleRoadPieceCount; ++index)
    {
        if (ContainsRoundedPhysicalRoadPoint(
                PlannedWorldScaleRoadPiece(index), point)) return true;
    }
    return false;
}

inline RoundedCenterlinePose WorldScaleCenterlineAt(
    double worldDistanceCm) noexcept
{
    if (!std::isfinite(worldDistanceCm)) return {};
    auto pose = RoundedRoadCenterlineAt(
        worldDistanceCm / WorldScaleCircuitFactor);
    if (!pose.Valid) return {};
    pose.Pose.Position.X *= WorldScaleCircuitFactor;
    pose.Pose.Position.Y *= WorldScaleCircuitFactor;
    if (!std::isfinite(pose.Pose.Position.X) ||
        !std::isfinite(pose.Pose.Position.Y)) return {};
    return pose;
}

// Reuse the verified manifold/winding/triangulation from the original
// rounded ribbon, transforming each vertex around its centreline station.
// This holds the visible road at 8.5 m while scaling the lap to 1.7 km;
// Z, road height, vertex count and normals stay identical. NON-COLLIDING.
inline RoundedSmoothRoadMesh BuildWorldScaleVisualRoadMesh()
{
    auto mesh = BuildRoundedSmoothRoadMesh();
    if (mesh.Vertices.size() != RoundedSmoothVertices ||
        mesh.Triangles.size() != RoundedSmoothTriangles) return {};
    for (int section = 0; section < WorldScaleRoadMeshSectionCount; ++section)
    {
        const double baselineDistance =
            RoundedLapLengthCm * static_cast<double>(section) /
            static_cast<double>(WorldScaleRoadMeshSectionCount);
        const auto baseline = RoundedRoadCenterlineAt(baselineDistance);
        const auto target = WorldScaleCenterlineAt(
            baselineDistance * WorldScaleCircuitFactor);
        if (!baseline.Valid || !target.Valid) return {};
        for (int point = 0; point < 4; ++point)
        {
            auto& vertex = mesh.Vertices[static_cast<std::size_t>(
                4 * section + point)];
            vertex.X += target.Pose.Position.X - baseline.Pose.Position.X;
            vertex.Y += target.Pose.Position.Y - baseline.Pose.Position.Y;
            if (!IsFiniteRoundedSmoothVertex(vertex)) return {};
        }
    }
    return mesh;
}
} // namespace latebrake
