#include "Track/WorldScaleSmoothCollisionRoad.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string>

// Original LateBrake 1.7-km physical collision-candidate OBJ.
// This is NOT a Chaos-cooked StaticMesh or evidence of vehicle contact.
// The full 9-m road is built independently of the 8.5-m cosmetic ribbon.
namespace
{
bool WriteObj(const std::filesystem::path& destination)
{
    using namespace latebrake;
    const RoundedSmoothRoadMesh mesh = BuildWorldScaleCollisionCandidateMesh();
    if (mesh.Vertices.size() != RoundedSmoothVertices ||
        mesh.Triangles.size() != RoundedSmoothTriangles) return false;
    const auto materialPath = destination.parent_path() /
                              (destination.stem().string() + ".mtl");
    std::ofstream material(materialPath, std::ios::binary | std::ios::trunc);
    if (!material) return false;
    material << "# ORIGINAL LATEBRAKE procedural candidate, NOT an Unreal physical material.\n"
                "newmtl LatebrakePhysicalRoadTop\n"
                "Ka 0.04 0.04 0.04\nKd 0.11 0.115 0.12\n"
                "Ks 0.025 0.025 0.025\nNs 6.0\nillum 2\n"
                "newmtl LatebrakePhysicalRoadSides\n"
                "Ka 0.015 0.015 0.015\nKd 0.04 0.045 0.05\n"
                "Ks 0.0 0.0 0.0\nNs 1.0\nillum 2\n";
    material.close();
    if (!material.good()) return false;

    std::ofstream out(destination, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(9);
    out << "# ORIGINAL 1.70-km, 9-m-wide full physical road candidate.\n"
           "# UNCOOKED per-poly geometry; no validated Chaos tyre contacts.\n"
        << "mtllib " << materialPath.filename().string() << "\n"
        << "o LatebrakeWorldScaleContinuousPhysicalRoad\n";
    for (const auto& vertex : mesh.Vertices)
        out << "v " << vertex.X << ' ' << vertex.Y << ' '
            << vertex.Z << '\n';

    // Separate terminal UV station makes lap closure continuous in U,
    // although geometry is welded and its final station is section zero.
    for (int i = 0; i <= RoundedSmoothRoadSections; ++i)
    {
        const double u = WorldScaleCircuitLapCm * static_cast<double>(i) /
                         (RoundedSmoothRoadSections * RoundedSmoothUvRepeatCm);
        for (int corner = 0; corner < 4; ++corner)
            out << "vt " << u << ' '
                << (corner % 2 == 0 ? 1.0 : 0.0) << '\n';
    }
    for (int surface = 0; surface < 4; ++surface)
    {
        out << (surface == 0 ?
                "g PhysicalRoadTop\nusemtl LatebrakePhysicalRoadTop\n" :
                surface == 1 ?
                "g PhysicalRoadBottom\nusemtl LatebrakePhysicalRoadSides\n" :
                surface == 2 ?
                "g PhysicalRoadLeftWall\nusemtl LatebrakePhysicalRoadSides\n" :
                "g PhysicalRoadRightWall\nusemtl LatebrakePhysicalRoadSides\n");
        for (int section = 0; section < RoundedSmoothRoadSections; ++section)
        {
            for (int half = 0; half < 2; ++half)
            {
                const auto& triangle = mesh.Triangles[
                    static_cast<std::size_t>(8 * section + 2 * surface + half)];
                out << "f";
                for (const std::uint32_t vertex : triangle.Indices)
                {
                    const std::uint32_t uv =
                        section == RoundedSmoothRoadSections - 1 &&
                        vertex / 4U == 0U ?
                        static_cast<std::uint32_t>(4 * RoundedSmoothRoadSections) +
                        vertex % 4U + 1U : vertex + 1U;
                    out << ' ' << vertex + 1U << '/' << uv;
                }
                out << '\n';
            }
        }
    }
    out.close();
    return out.good();
}
}

int main(int argc, char** argv)
{
    if (argc != 2 || !argv[1] || !argv[1][0])
    {
        std::cerr << "Usage: WorldScaleCollisionRoadObjExporter <destination.obj>\n";
        return 2;
    }
    const std::filesystem::path destination(argv[1]);
    if (destination.extension() != ".obj")
    {
        std::cerr << "Collision candidate destination must have .obj extension\n";
        return 3;
    }
    if (!WriteObj(destination))
    {
        std::cerr << "Full-scale continuous collision road source export failed\n";
        return 4;
    }
    std::cout << "Original 1.70-km full-width physical road candidate: "
              << latebrake::RoundedSmoothVertices << " vertices, "
              << latebrake::RoundedSmoothTriangles
              << " triangles, 9.0m wide, 42cm depth; "
                 "collision=MODEL_NOT_COOKED, NOT_DRIVEN.\n";
    return 0;
}
