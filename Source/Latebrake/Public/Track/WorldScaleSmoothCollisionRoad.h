#pragma once

#include "Track/WorldScaleRoundedCircuit.h"

#include <cmath>
#include <cstddef>

namespace latebrake
{
// Experimental GEOMETRY CONTRACT ONLY for a future continuous collision
// surface, not an actual Chaos BodySetup, cooked UE5.8 StaticMesh, drivable
// game track, or replacement for existing 68 independent BlockAll slabs.
// Unlike the 8.5-m NON-COLLIDING display ribbon, the planned continuous
// physical surface spans the full 9-m asphalt corridor at z=-1 cm.
// It retains real road thickness rather than cooking a zero-volume plane.
inline constexpr double WorldScaleCollisionHalfWidthCm =
    WorldScaleRoadHalfWidthCm;
inline constexpr double WorldScaleCollisionTopCm = RoundedRoadTopCm;
inline constexpr double WorldScaleCollisionBottomCm =
    2.0 * RoundedRoadCenterHeightCm - RoundedRoadTopCm;
static_assert(WorldScaleCollisionHalfWidthCm == 450.0);
static_assert(WorldScaleCollisionTopCm == -1.0);
static_assert(WorldScaleCollisionBottomCm == -43.0);
static_assert(WorldScaleCollisionHalfWidthCm >
              WorldScaleVisibleHalfWidthCm);

// Use the same closed, correctly wound manifold as the 1.7-km visual
// road, but RECONSTRUCT every collision vertex from the centreline. This
// prevents cosmetic UV/height/offset choices being treated as physics.
// LT,RT,LB,RB at each station; two top + two bottom + four wall faces.
inline RoundedSmoothRoadMesh BuildWorldScaleCollisionCandidateMesh()
{
    RoundedSmoothRoadMesh mesh = BuildWorldScaleVisualRoadMesh();
    if (mesh.Vertices.size() != RoundedSmoothVertices ||
        mesh.Triangles.size() != RoundedSmoothTriangles) return {};

    for (int section = 0; section < WorldScaleRoadMeshSectionCount; ++section)
    {
        const double distance = WorldScaleCircuitLapCm *
            static_cast<double>(section) /
            static_cast<double>(WorldScaleRoadMeshSectionCount);
        const auto center = WorldScaleCenterlineAt(distance);
        if (!center.Valid) return {};
        const double heading = center.Pose.HeadingDegrees * RoundedPi / 180.0;
        const double nx = -std::sin(heading);
        const double ny = std::cos(heading);
        for (int corner = 0; corner < 4; ++corner)
        {
            const double side = corner % 2 == 0 ? 1.0 : -1.0;
            auto& vertex = mesh.Vertices[
                static_cast<std::size_t>(section * 4 + corner)];
            vertex.X = center.Pose.Position.X +
                side * WorldScaleCollisionHalfWidthCm * nx;
            vertex.Y = center.Pose.Position.Y +
                side * WorldScaleCollisionHalfWidthCm * ny;
            vertex.Z = corner < 2 ?
                WorldScaleCollisionTopCm : WorldScaleCollisionBottomCm;
            if (!IsFiniteRoundedSmoothVertex(vertex)) return {};
        }
    }
    return mesh;
}
} // namespace latebrake
