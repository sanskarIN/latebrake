// Portable ORIGINAL LateBrake physical-road geometry ray-test contract.
//
// This casts real mathematical vertical rays against the actual C++ 8192
// candidate triangles. It proves SOURCE mesh coverage; NOT licensed Unreal
// Editor, Chaos cooking, wheel/suspension contact, gameplay or Win64 build.

#include "Track/WorldScaleSmoothCollisionRoad.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace
{
using latebrake::RoundedSmoothRoadMesh;
using latebrake::RoundedSmoothTriangle;
using latebrake::RoundedSmoothVertex;

constexpr int kStations = 256;
constexpr int kExtraSeamStations = 2;
constexpr int kExpectedSamples = (kStations + kExtraSeamStations) * 9;
constexpr int kMidpointStations = latebrake::WorldScaleRoadMeshSectionCount;
constexpr double kRoadHalfWidthCm = 450.0;
constexpr double kInsideCm = 400.0;
constexpr double kNearInsideCm = 449.0;
constexpr double kNearOutsideCm = 451.0;
constexpr double kOutsideCm = 550.0;
constexpr double kEpsilon = 1e-9;

// Intersect a downward (-Z) vertical ray with a known flat-z triangle.
// A 2-D barycentric test is sufficient on these strictly horizontal
// top/bottom faces and intentionally excludes zero-projection side walls.
// This computes PHYSICAL cover independently of the mesh winding check.
bool VerticalRayMeetsFace(const RoundedSmoothRoadMesh& mesh,
                          const RoundedSmoothTriangle& triangle,
                          double x, double y, double& outZ)
{
    const auto& a = mesh.Vertices[triangle.Indices[0]];
    const auto& b = mesh.Vertices[triangle.Indices[1]];
    const auto& c = mesh.Vertices[triangle.Indices[2]];
    if (a.Z != b.Z || a.Z != c.Z) return false;

    const double abx = b.X - a.X;
    const double aby = b.Y - a.Y;
    const double acx = c.X - a.X;
    const double acy = c.Y - a.Y;
    const double determinant = abx * acy - aby * acx;
    if (!std::isfinite(determinant) || std::abs(determinant) < 1e-7)
        throw std::runtime_error("Degenerate top/bottom physics triangle");
    const double dx = x - a.X;
    const double dy = y - a.Y;
    const double u = (dx * acy - dy * acx) / determinant;
    const double v = (abx * dy - aby * dx) / determinant;
    if (!std::isfinite(u) || !std::isfinite(v)) return false;
    if (u < -kEpsilon || v < -kEpsilon || u + v > 1.0 + kEpsilon)
        return false;
    outZ = a.Z;
    return true;
}

struct RoadHitSummary final
{
    bool Top = false;
    bool Bottom = false;
    std::size_t HitTriangles = 0;
};

RoadHitSummary TraceOriginalCollisionMesh(const RoundedSmoothRoadMesh& mesh,
                                          double x, double y)
{
    RoadHitSummary found{};
    for (std::size_t index = 0; index < mesh.Triangles.size(); ++index)
    {
        // The SOURCE builder adds top, bottom, left and right in groups
        // of two per station. The vertical ray ignores parallel side walls.
        const std::size_t local = index % 8;
        if (local >= 4) continue;
        double z = 0.0;
        if (!VerticalRayMeetsFace(mesh, mesh.Triangles[index], x, y, z))
            continue;
        ++found.HitTriangles;
        if (local < 2)
        {
            if (z != latebrake::WorldScaleCollisionTopCm)
                throw std::runtime_error("Road top contact at wrong elevation");
            found.Top = true;
        }
        else
        {
            if (z != latebrake::WorldScaleCollisionBottomCm)
                throw std::runtime_error("Road bottom contact at wrong elevation");
            found.Bottom = true;
        }
    }
    return found;
}

void RequireCoverage(const RoundedSmoothRoadMesh& mesh,
                     double travelledCm, double lateralCm,
                     bool expectRoad)
{
    const auto center = latebrake::WorldScaleCenterlineAt(travelledCm);
    if (!center.Valid)
        throw std::runtime_error("Invalid source centreline query station");
    const double heading = center.Pose.HeadingDegrees *
                           latebrake::RoundedPi / 180.0;
    const double x = center.Pose.Position.X - std::sin(heading) * lateralCm;
    const double y = center.Pose.Position.Y + std::cos(heading) * lateralCm;
    if (!std::isfinite(x) || !std::isfinite(y))
        throw std::runtime_error("Invalid finite physical contact point");

    const auto hit = TraceOriginalCollisionMesh(mesh, x, y);
    if (hit.Top != expectRoad || hit.Bottom != expectRoad)
    {
        std::cerr << "Unexpected original collision mesh ray: s=" << travelledCm
                  << " lateral=" << lateralCm << " cm, top=" << hit.Top
                  << " bottom=" << hit.Bottom
                  << " triangle_hits=" << hit.HitTriangles << '\n';
        throw std::runtime_error("Continuous 9-m road coverage gap or off-road overlap");
    }
    if (expectRoad && hit.HitTriangles < 2)
        throw std::runtime_error("Inside road has no top + bottom volume");
    if (!expectRoad && hit.HitTriangles != 0)
        throw std::runtime_error("Outside physical road has extra geometry");
}
}

int main()
{
    using namespace latebrake;
    const auto mesh = BuildWorldScaleCollisionCandidateMesh();
    if (mesh.Vertices.size() != RoundedSmoothVertices ||
        mesh.Triangles.size() != RoundedSmoothTriangles ||
        WorldScaleCollisionHalfWidthCm != kRoadHalfWidthCm)
    {
        std::cerr << "Missing ORIGINAL 1.70-km physical source geometry\n";
        return 1;
    }

    try
    {
        std::size_t inside = 0;
        std::size_t outside = 0;
        // The inner/outer physical-rim pairs have only a 2cm gap.
        // This catches support lost or false coverage at the actual edge.
        for (int station = 0; station < kStations + kExtraSeamStations; ++station)
        {
            const double distance =
                station < kStations ?
                WorldScaleCircuitLapCm * static_cast<double>(station) /
                    static_cast<double>(kStations) :
                station == kStations ? 0.25 : WorldScaleCircuitLapCm - 0.25;
            for (double offset : {0.0, -kInsideCm, kInsideCm,
                                  -kNearInsideCm, kNearInsideCm})
            {
                RequireCoverage(mesh, distance, offset, true);
                ++inside;
            }
            for (double offset : {-kNearOutsideCm, kNearOutsideCm,
                                  -kOutsideCm, kOutsideCm})
            {
                RequireCoverage(mesh, distance, offset, false);
                ++outside;
            }
        }

        // Sample INSIDE every actual triangulation interval (not only
        // section vertices). Reject stitching gaps or missing top panels.
        std::size_t midpoints = 0;
        for (int section = 0; section < kMidpointStations; ++section)
        {
            const double distance =
                WorldScaleCircuitLapCm *
                (static_cast<double>(section) + 0.5) /
                static_cast<double>(kMidpointStations);
            for (double offset : {0.0, -kInsideCm, kInsideCm})
            {
                RequireCoverage(mesh, distance, offset, true);
                ++midpoints;
            }
        }

        if (inside != 1290 || outside != 1032 ||
            static_cast<int>(inside + outside) != kExpectedSamples ||
            midpoints != 3072)
        {
            throw std::runtime_error("Unexpected coverage sample count");
        }
        std::cout << "Original FULL-WIDTH continuous collision mesh: "
                  << "1290 in-road top/bottom rays, 1032 off-road no-hit rays, "
                  << midpoints
                  << " independent polygon-midpoint rays PASS. "
                     "C++ triangle geometry ONLY, not Unreal/Chaos.\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Continuous collision source ray proof FAILED: "
                  << ex.what() << '\n';
        return 2;
    }
}
