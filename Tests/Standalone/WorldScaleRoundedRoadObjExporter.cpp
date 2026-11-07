#include "Track/WorldScaleRoundedCircuit.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string>

namespace
{
bool WriteObj(const std::filesystem::path& file)
{
    using namespace latebrake;
    const RoundedSmoothRoadMesh mesh = BuildWorldScaleVisualRoadMesh();
    if (mesh.Vertices.size() != RoundedSmoothVertices ||
        mesh.Triangles.size() != RoundedSmoothTriangles) return false;
    const auto mtl = file.parent_path() /
                     (file.stem().string() + ".mtl");
    std::ofstream material(mtl, std::ios::binary | std::ios::trunc);
    if (!material) return false;
    material << "# Original LateBrake non-colliding smooth road materials.\n"
                "newmtl LatebrakeAsphalt\n"
                "Ka 0.05 0.05 0.055\nKd 0.13 0.14 0.16\n"
                "Ks 0.03 0.03 0.03\nNs 8.0\nillum 2\n"
                "newmtl LatebrakeRoadSides\n"
                "Ka 0.025 0.025 0.028\nKd 0.07 0.075 0.08\n"
                "Ks 0.01 0.01 0.01\nNs 3.0\nillum 2\n";
    material.close();
    if (!material.good()) return false;

    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(9);
    stream << "# ORIGINAL LateBrake WORLD-SCALE 1.7-KM rounded circuit: visual mesh ONLY.\n"
              "# collision disabled; original 68 physical road boxes remain authoritative.\n"
           << "mtllib " << mtl.filename().string() << "\n"
           << "o LatebrakeRoundedSmoothRoadVisual\n";
    for (const auto& vertex : mesh.Vertices)
        stream << "v " << vertex.X << ' ' << vertex.Y << ' '
               << vertex.Z << '\n';

    // One extra UV section closes the lap smoothly without stretching a
    // texture from U=lapLength back to U=0 across the last two faces.
    for (int i = 0; i <= RoundedSmoothRoadSections; ++i)
    {
        const double u = (static_cast<double>(i) * WorldScaleCircuitLapCm) /
                         (RoundedSmoothRoadSections * RoundedSmoothUvRepeatCm);
        for (int edge = 0; edge < 4; ++edge)
        {
            const double v = edge % 2 == 0 ? 1.0 : 0.0;
            stream << "vt " << u << ' ' << v << '\n';
        }
    }

    const auto uvIndex = [](int section, std::uint32_t vertex) -> std::uint32_t
    {
        // Section N uses vertex positions from 0 but its own U-lap UVs.
        if (section == RoundedSmoothRoadSections - 1 &&
            vertex / 4U == 0U)
        {
            return static_cast<std::uint32_t>(4 * RoundedSmoothRoadSections) +
                   vertex % 4U + 1U;
        }
        return vertex + 1U;
    };
    for (int group = 0; group < 4; ++group)
    {
        stream << (group == 0 ? "g RoadTop\nusemtl LatebrakeAsphalt\n" :
                   group == 1 ? "g RoadUnderside\nusemtl LatebrakeRoadSides\n" :
                   group == 2 ? "g RoadLeftShoulder\nusemtl LatebrakeRoadSides\n" :
                                "g RoadRightShoulder\nusemtl LatebrakeRoadSides\n");
        for (int i = 0; i < RoundedSmoothRoadSections; ++i)
        {
            for (int half = 0; half < 2; ++half)
            {
                const auto& triangle = mesh.Triangles[
                    static_cast<std::size_t>(8 * i + 2 * group + half)];
                stream << "f";
                for (const auto vertex : triangle.Indices)
                    stream << ' ' << vertex + 1U << '/' << uvIndex(i, vertex);
                stream << '\n';
            }
        }
    }
    stream.close();
    return stream.good();
}
}

int main(int argc, char** argv)
{
    if (argc != 2 || !argv[1] || !argv[1][0])
    {
        std::cerr << "Usage: RoundedSmoothRoadObjExporter <output.obj>\n";
        return 2;
    }
    const std::filesystem::path dest = argv[1];
    if (dest.extension() != ".obj")
    {
        std::cerr << "Output must be a .obj mesh path\n";
        return 3;
    }
    if (!WriteObj(dest))
    {
        std::cerr << "Original world-scale 1.7-km visual road mesh export FAILED\n";
        return 4;
    }
    std::cout << "Created original WORLD-SCALE 1.7-km visual-only 3D ribbon: "
              << latebrake::RoundedSmoothVertices << " vertices, "
              << latebrake::RoundedSmoothTriangles << " triangles, "
              << "unbroken closed-loop top/bottom/sides, collision=off.\n";
    return 0;
}
