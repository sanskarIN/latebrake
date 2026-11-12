#include "Track/RoundedCircuitSmoothMesh.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace
{
void Require(bool okay, const char* message)
{
    if (!okay)
    {
        std::cerr << "RoundedSmoothRoadMesh FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint64_t EdgeKey(std::uint32_t a, std::uint32_t b)
{
    const auto smaller = a < b ? a : b;
    const auto larger = a < b ? b : a;
    return (static_cast<std::uint64_t>(smaller) << 32U) | larger;
}
struct EdgeCount final {int Uses = 0; int Balance = 0;};
}

int main()
{
    using namespace latebrake;
    static_assert(RoundedSmoothRoadSections == 1024);
    static_assert(RoundedSmoothVertices == 4096);
    static_assert(RoundedSmoothTriangles == 8192);
    static_assert(RoundedSmoothHalfWidthCm < RoundedHalfRoadWidthCm);
    static_assert(RoundedSmoothTopCm > RoundedRoadTopCm);
    static_assert(RoundedSmoothBottomCm == RoundedRoadTopCm);
    const auto mesh = BuildRoundedSmoothRoadMesh();
    Require(mesh.Vertices.size() == RoundedSmoothVertices &&
            mesh.Triangles.size() == RoundedSmoothTriangles,
            "fixed bounded visible mesh topology");

    std::unordered_map<std::uint64_t, EdgeCount> edges;
    edges.reserve(mesh.Triangles.size() * 3);
    double minArea = 1e99;
    for (std::size_t i = 0; i < mesh.Vertices.size(); ++i)
    {
        const auto vertex = mesh.Vertices[i];
        Require(IsFiniteRoundedSmoothVertex(vertex),
                "no NaN/Inf in authored mesh");
        Require(std::abs(vertex.Z -
            (i % 4 < 2 ? RoundedSmoothTopCm : RoundedSmoothBottomCm)) < 1e-10,
            "unambiguous, flat thin-shell top/bottom");
        Require(IsOnRoundedPhysicalRoad({vertex.X, vertex.Y}),
                "ALL 4096 visible vertices are backed by real collision asphalt");
    }
    for (std::size_t triIndex = 0; triIndex < mesh.Triangles.size(); ++triIndex)
    {
        const auto& tri = mesh.Triangles[triIndex].Indices;
        for (int e = 0; e < 3; ++e)
        {
            const auto a = tri[e];
            const auto b = tri[(e + 1) % 3];
            Require(a < mesh.Vertices.size() && b < mesh.Vertices.size() &&
                    a != b, "triangle indices valid, nondegenerate");
            auto& info = edges[EdgeKey(a,b)];
            ++info.Uses;
            info.Balance += a < b ? 1 : -1;
        }
        const auto a = mesh.Vertices[tri[0]];
        const auto b = mesh.Vertices[tri[1]];
        const auto c = mesh.Vertices[tri[2]];
        const double x = b.X - a.X, y = b.Y - a.Y, z = b.Z - a.Z;
        const double u = c.X - a.X, v = c.Y - a.Y, w = c.Z - a.Z;
        const double nx = y*w-z*v, ny = z*u-x*w, nz = x*v-y*u;
        const double area2 = std::sqrt(nx*nx+ny*ny+nz*nz);
        Require(std::isfinite(area2) && area2 > 0.001,
                "every face has finite nonzero area");
        if (area2 < minArea) minArea = area2;
        const int group = static_cast<int>(triIndex % 8) / 2;
        if (group == 0) Require(nz > 0.0, "all asphalt top normals face up");
        if (group == 1) Require(nz < 0.0, "all underside normals face down");
        if (group >= 2) Require(std::abs(nz) < 1e-6, "side walls are vertical");
        if (group == 0)
        {
            // Centroid and half-edge samples must also be collision-backed:
            // vertices alone cannot prove the rendered triangles stay on road.
            const CircuitPoint centroid{
                (a.X + b.X + c.X) / 3.0,
                (a.Y + b.Y + c.Y) / 3.0
            };
            const CircuitPoint ab{(a.X+b.X)/2.0,(a.Y+b.Y)/2.0};
            const CircuitPoint bc{(b.X+c.X)/2.0,(b.Y+c.Y)/2.0};
            const CircuitPoint ca{(c.X+a.X)/2.0,(c.Y+a.Y)/2.0};
            Require(IsOnRoundedPhysicalRoad(centroid) &&
                    IsOnRoundedPhysicalRoad(ab) &&
                    IsOnRoundedPhysicalRoad(bc) &&
                    IsOnRoundedPhysicalRoad(ca),
                    "surface interior cannot paint over unsupported grass");
        }
    }
    for (const auto& edge : edges)
        Require(edge.second.Uses == 2 && edge.second.Balance == 0,
                "the road mesh is a closed, consistently oriented 2-manifold");
    Require(edges.size() == static_cast<std::size_t>(12 * RoundedSmoothRoadSections),
            "expected 12288 two-sided mesh edges including seam");

    const auto a = mesh.Vertices[0];
    const auto b = mesh.Vertices[4 * (RoundedSmoothRoadSections-1)];
    Require(std::hypot(a.X-b.X,a.Y-b.Y) < 40.0,
            "the final-to-first lap seam is short and continuous");
    std::cout << "4096 real-road-backed visual vertices; 8192 triangles; "
              << edges.size() << " closed manifold edges; min doubled face area="
              << minArea << "; no unbacked grass or top-face inversion.\n";
    return EXIT_SUCCESS;
}
