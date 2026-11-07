#include "Track/WorldScaleRoundedCircuit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool good, const char* reason)
{
    if (!good)
    {
        std::cerr << "World-scale circuit FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double a, double b, double tolerance = 1e-8)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    static_assert(WorldScaleCircuitFactor == 10.0);
    static_assert(WorldScaleRoadPieceCount == 68);
    static_assert(RoundedSmoothVertices == 4096);
    static_assert(RoundedSmoothTriangles == 8192);
    Require(WorldScaleCircuitLapCm > 160000.0 &&
            WorldScaleCircuitLapCm < 180000.0,
            "full-scale lap must be ~1.7km, not 170m");
    Require(!PlannedWorldScaleRoadPiece(-1).Valid &&
            !PlannedWorldScaleRoadPiece(68).Valid,
            "invalid road indices fail closed");
    for (int i = 0; i < WorldScaleRoadPieceCount; ++i)
    {
        const auto original = PlannedRoundedPhysicalRoadPiece(i);
        const auto scaled = PlannedWorldScaleRoadPiece(i);
        Require(IsValidRoundedPhysicalRoadPiece(scaled),
                "all 68 proposed full-scale road pieces have real geometry");
        Require(Close(scaled.Center.X, original.Center.X * 10.0) &&
                Close(scaled.Center.Y, original.Center.Y * 10.0),
                "only XY route positions scale by factor 10");
        Require(Close(scaled.ScaleX, original.ScaleX * 10.0) &&
                Close(scaled.ScaleY, original.ScaleY) &&
                Close(scaled.ScaleY, 9.0),
                "scale along-road length without inflating 9-m road width");
        Require(Close(scaled.ScaleZ, original.ScaleZ) &&
                Close(scaled.HeightCm, original.HeightCm) &&
                Close(scaled.HeightCm + 50.0 * scaled.ScaleZ, -1.0),
                "physical road elevation and thickness stay stable");
        Require(Close(scaled.YawDegrees, original.YawDegrees) &&
                ContainsRoundedPhysicalRoadPoint(scaled, scaled.Center),
                "corner heading remains correct, centre collision-backed");
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Require(!IsOnWorldScaleRoad({nan,0}) &&
            !IsOnWorldScaleRoad({0,inf}) &&
            !WorldScaleCenterlineAt(nan).Valid &&
            !WorldScaleCenterlineAt(inf).Valid,
            "nonfinite world-scale input rejected");
    Require(!IsOnWorldScaleRoad({0,0}) &&
            !IsOnWorldScaleRoad({28000+3500,0}) &&
            !IsOnWorldScaleRoad({0,18000+3500}),
            "infield and far-outside terrain are NOT paved");

    const auto mesh = BuildWorldScaleVisualRoadMesh();
    Require(mesh.Vertices.size() == RoundedSmoothVertices &&
            mesh.Triangles.size() == RoundedSmoothTriangles,
            "full-scale original continuous mesh retains known bounded topology");
    for (std::size_t i = 0; i < mesh.Vertices.size(); ++i)
    {
        const auto v = mesh.Vertices[i];
        Require(IsFiniteRoundedSmoothVertex(v) &&
                Close(v.Z, i % 4 < 2 ? RoundedSmoothTopCm :
                                        RoundedSmoothBottomCm),
                "finite coordinates preserve exact physical-facing height");
        Require(IsOnWorldScaleRoad({v.X, v.Y}),
                "EVERY visible full-scale mesh vertex has physical asphalt");
    }
    for (std::size_t t = 0; t < mesh.Triangles.size(); ++t)
    {
        const auto tri = mesh.Triangles[t].Indices;
        Require(tri[0] < mesh.Vertices.size() &&
                tri[1] < mesh.Vertices.size() &&
                tri[2] < mesh.Vertices.size(),
                "all full-scale face indices are bounded");
        if (t % 8 >= 2) continue;
        const auto a = mesh.Vertices[tri[0]];
        const auto b = mesh.Vertices[tri[1]];
        const auto c = mesh.Vertices[tri[2]];
        const double nz = (b.X-a.X)*(c.Y-a.Y) -
                          (b.Y-a.Y)*(c.X-a.X);
        Require(std::isfinite(nz) && nz > 0.001,
                "asphalt top winding and area remain positive");
        for (const CircuitPoint point : {
            CircuitPoint{(a.X+b.X+c.X)/3,(a.Y+b.Y+c.Y)/3},
            CircuitPoint{(a.X+b.X)/2,(a.Y+b.Y)/2},
            CircuitPoint{(a.X+c.X)/2,(a.Y+c.Y)/2},
            CircuitPoint{(b.X+c.X)/2,(b.Y+c.Y)/2}
        })
            Require(IsOnWorldScaleRoad(point),
                    "ALL surface face interiors backed by real physical road");
    }

    const auto start = WorldScaleCenterlineAt(0.0);
    const auto end = WorldScaleCenterlineAt(WorldScaleCircuitLapCm);
    const auto back = WorldScaleCenterlineAt(-WorldScaleCircuitLapCm);
    Require(start.Valid && end.Valid && back.Valid &&
            Close(start.Pose.Position.X,end.Pose.Position.X) &&
            Close(start.Pose.Position.Y,back.Pose.Position.Y),
            "1.7km lap and negative wrap are continuous");
    for (int s = 0; s < 8192; ++s)
    {
        const double distance = WorldScaleCircuitLapCm * s / 8192;
        const auto pose = WorldScaleCenterlineAt(distance);
        const auto next = WorldScaleCenterlineAt(distance + 1.0);
        Require(pose.Valid && next.Valid &&
                IsOnWorldScaleRoad(pose.Pose.Position),
                "all 8192 full-scale lap stations remain physically on road");
        const double theta = pose.Pose.HeadingDegrees * RoundedPi / 180.0;
        const double dx = next.Pose.Position.X - pose.Pose.Position.X;
        const double dy = next.Pose.Position.Y - pose.Pose.Position.Y;
        Require(dx*std::cos(theta)+dy*std::sin(theta) > 0.98,
                "continuous full-scale heading and direction around all turns");
    }
    std::cout << "World-scale 1.70-km lap, 9m physical road width, "
              << "68 collision-backed boxes, 4096 mesh vertices, "
              << "8192 triangles and 8192 forward-centerline checks PASSED.\n";
    return EXIT_SUCCESS;
}
