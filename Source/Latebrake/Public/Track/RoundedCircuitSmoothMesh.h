#pragma once

#include "Track/RoundedCircuitRoadPlan.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace latebrake
{
// This is a *visual-only* continuous closed ribbon around the experimental
// centreline. It is NOT an independently authoritative collision surface,
// race track, driving upgrade or replacement for the 68 WorldStatic cubes.
// A conservative 25-cm inset from both physical edges avoids inventing
// asphalt on unbacked grass. Top is 0.45 cm above actual collision asphalt;
// this mesh MUST have collision disabled when placed in the lab.
inline constexpr int RoundedSmoothRoadSections = 1024;
inline constexpr double RoundedSmoothHalfWidthCm = 425.0;
inline constexpr double RoundedSmoothTopCm = -0.55;
inline constexpr double RoundedSmoothBottomCm = -1.0;
inline constexpr double RoundedSmoothUvRepeatCm = 400.0;
inline constexpr int RoundedSmoothVertices = 4 * RoundedSmoothRoadSections;
inline constexpr int RoundedSmoothTriangles = 8 * RoundedSmoothRoadSections;
static_assert(RoundedSmoothBottomCm == RoundedRoadTopCm);
static_assert(RoundedSmoothTopCm > RoundedRoadTopCm);
static_assert(RoundedSmoothHalfWidthCm < RoundedHalfRoadWidthCm);

struct RoundedSmoothVertex final
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

struct RoundedSmoothTriangle final
{
    std::array<std::uint32_t, 3> Indices{};
};

struct RoundedSmoothRoadMesh final
{
    std::vector<RoundedSmoothVertex> Vertices;
    std::vector<RoundedSmoothTriangle> Triangles;
};

inline bool IsFiniteRoundedSmoothVertex(RoundedSmoothVertex vertex) noexcept
{
    return std::isfinite(vertex.X) && std::isfinite(vertex.Y) &&
           std::isfinite(vertex.Z);
}

// Indices are LT, RT, LB, RB at each section in forward driving direction.
// On a forward-east straight the left edge is north (+Y).
inline RoundedSmoothRoadMesh BuildRoundedSmoothRoadMesh()
{
    RoundedSmoothRoadMesh mesh;
    mesh.Vertices.reserve(RoundedSmoothVertices);
    mesh.Triangles.reserve(RoundedSmoothTriangles);
    for (int section = 0; section < RoundedSmoothRoadSections; ++section)
    {
        const double s = RoundedLapLengthCm * static_cast<double>(section) /
                         static_cast<double>(RoundedSmoothRoadSections);
        const auto center = RoundedRoadCenterlineAt(s);
        if (!center.Valid) return {};
        const double heading = center.Pose.HeadingDegrees * RoundedPi / 180.0;
        const double nx = -std::sin(heading);
        const double ny = std::cos(heading);
        const double x = center.Pose.Position.X;
        const double y = center.Pose.Position.Y;
        for (const double height : {RoundedSmoothTopCm, RoundedSmoothBottomCm})
        {
            for (const double side : {1.0, -1.0})
            {
                const RoundedSmoothVertex vertex{
                    x + nx * side * RoundedSmoothHalfWidthCm,
                    y + ny * side * RoundedSmoothHalfWidthCm, height
                };
                if (!IsFiniteRoundedSmoothVertex(vertex)) return {};
                mesh.Vertices.push_back(vertex);
            }
        }
    }
    const auto add = [&mesh](std::uint32_t a, std::uint32_t b,
                              std::uint32_t c)
    {
        mesh.Triangles.push_back({{a, b, c}});
    };
    for (std::uint32_t i = 0; i < RoundedSmoothRoadSections; ++i)
    {
        const std::uint32_t j = (i + 1) % RoundedSmoothRoadSections;
        const std::uint32_t lt = 4 * i;
        const std::uint32_t rt = lt + 1;
        const std::uint32_t lb = lt + 2;
        const std::uint32_t rb = lt + 3;
        const std::uint32_t nlt = 4 * j;
        const std::uint32_t nrt = nlt + 1;
        const std::uint32_t nlb = nlt + 2;
        const std::uint32_t nrb = nlt + 3;
        // Triangulate top (+Z) and bottom (-Z) with opposite winding.
        add(lt, rt, nrt);
        add(lt, nrt, nlt);
        add(lb, nrb, rb);
        add(lb, nlb, nrb);
        // Smoothly turn the exterior side walls around the four bends.
        add(lt, nlt, nlb);
        add(lt, nlb, lb);
        add(rt, rb, nrb);
        add(rt, nrb, nrt);
    }
    return mesh;
}
} // namespace latebrake
