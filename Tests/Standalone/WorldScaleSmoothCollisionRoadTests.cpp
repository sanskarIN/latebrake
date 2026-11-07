#include "Track/WorldScaleSmoothCollisionRoad.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>

namespace
{
void Require(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "WorldScaleSmoothCollisionRoad FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double a, double b, double tolerance = 1e-7)
{
    return std::isfinite(a) && std::isfinite(b) &&
           std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    const auto physical = BuildWorldScaleCollisionCandidateMesh();
    const auto visual = BuildWorldScaleVisualRoadMesh();
    Require(physical.Vertices.size() == 4096 &&
            physical.Triangles.size() == 8192 &&
            visual.Vertices.size() == physical.Vertices.size(),
            "closed physical road must have 4096 vertices and 8192 faces");
    Require(WorldScaleCollisionHalfWidthCm == 450.0 &&
            WorldScaleCollisionTopCm == -1.0 &&
            WorldScaleCollisionBottomCm == -43.0,
            "real 9-m road thickness and top must not scale with lap");

    // Every physical station has two precisely 9m-apart top vertices and
    // a 42-cm downward vertical wall. Cosmetic ribbon is only 8.5m wide.
    for (int station = 0; station < WorldScaleRoadMeshSectionCount; ++station)
    {
        const std::size_t i = static_cast<std::size_t>(4 * station);
        const auto lt = physical.Vertices[i];
        const auto rt = physical.Vertices[i + 1];
        const auto lb = physical.Vertices[i + 2];
        const auto rb = physical.Vertices[i + 3];
        const auto leftPaint = visual.Vertices[i];
        const auto rightPaint = visual.Vertices[i + 1];
        Require(IsFiniteRoundedSmoothVertex(lt) &&
                IsFiniteRoundedSmoothVertex(rt) &&
                IsFiniteRoundedSmoothVertex(lb) &&
                IsFiniteRoundedSmoothVertex(rb),
                "all physical vertices finite");
        Require(Close(lt.Z, -1.0) && Close(rt.Z, -1.0) &&
                Close(lb.Z, -43.0) && Close(rb.Z, -43.0) &&
                Close(lt.X, lb.X) && Close(lt.Y, lb.Y) &&
                Close(rt.X, rb.X) && Close(rt.Y, rb.Y),
                "top must be flat and bottom must be directly below");
        Require(Close(std::hypot(lt.X - rt.X, lt.Y - rt.Y), 900.0, 1e-5),
                "physical width is 9m everywhere, never 90m");
        Require(Close(std::hypot(leftPaint.X - rightPaint.X,
                                 leftPaint.Y - rightPaint.Y), 850.0, 1e-5),
                "cosmetic top stays at 8.5m width");
        const double centerX = (lt.X + rt.X) / 2.0;
        const double centerY = (lt.Y + rt.Y) / 2.0;
        const double distance = WorldScaleCircuitLapCm * station /
                                WorldScaleRoadMeshSectionCount;
        const auto pose = WorldScaleCenterlineAt(distance);
        Require(pose.Valid && Close(centerX, pose.Pose.Position.X, 1e-5) &&
                Close(centerY, pose.Pose.Position.Y, 1e-5) &&
                IsOnWorldScaleRoad({centerX, centerY}),
                "physical mesh follows full-size collision-backed route");
        // The physical edge is 25 cm outboard of its corresponding paint
        // edge, including all four curved corners and the loop seam.
        Require(Close(std::hypot(lt.X - leftPaint.X,
                                 lt.Y - leftPaint.Y), 25.0, 1e-5) &&
                Close(std::hypot(rt.X - rightPaint.X,
                                 rt.Y - rightPaint.Y), 25.0, 1e-5),
                "paint is always inset 25cm inside future physical asphalt");
    }

    std::map<std::pair<std::uint32_t,std::uint32_t>,std::array<int,2>> edges;
    for (std::size_t t = 0; t < physical.Triangles.size(); ++t)
    {
        const auto ids = physical.Triangles[t].Indices;
        for (const auto index : ids)
            Require(index < physical.Vertices.size(),
                    "no invalid triangle reference");
        const auto a = physical.Vertices[ids[0]];
        const auto b = physical.Vertices[ids[1]];
        const auto c = physical.Vertices[ids[2]];
        const double ux=b.X-a.X, uy=b.Y-a.Y, uz=b.Z-a.Z;
        const double vx=c.X-a.X, vy=c.Y-a.Y, vz=c.Z-a.Z;
        const double nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
        Require(std::isfinite(nx) && std::isfinite(ny) && std::isfinite(nz) &&
                nx*nx+ny*ny+nz*nz > 1e-4,
                "all original physical road triangles must have real area");
        if (t % 8 < 2) Require(nz > 0.001, "road top faces upward");
        if (t % 8 >= 2 && t % 8 < 4)
            Require(nz < -0.001, "underside faces downward");
        for (int e = 0; e < 3; ++e)
        {
            const auto x=ids[static_cast<std::size_t>(e)];
            const auto y=ids[static_cast<std::size_t>((e+1)%3)];
            const auto key=std::minmax(x,y);
            auto& counts=edges[{key.first,key.second}];
            if (x < y) ++counts[0]; else ++counts[1];
        }
    }
    for (const auto& pair : edges)
        Require(pair.second == std::array<int,2>{1,1},
                "every seam and sidewall edge has exactly two opposite faces");
    std::cout << "Proposed 1.70-km fully closed 9-m physical road manifold: "
              << "1024 sections / 4096 vertices / 8192 triangles, "
              << "42-cm depth, continuous 25-cm visual inset PASS. "
              << "UNCOOKED, NOT CHAOS-CONTACT VERIFIED.\n";
    return EXIT_SUCCESS;
}
