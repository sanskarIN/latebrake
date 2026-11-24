#pragma once
#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace latebrake::vehiclemesh
{
// The engine adapter consumes *one* genuine original mesh and maps its
// authored material tags to ten independent render-only material sections.
// All vertex indices are validated before any Unreal rendering APIs run.
// This owns triangle IDs, not a second copy of car geometry.
// Root collision cube is located at local Z=0 with legacy decorative
// wheel centres at Z=-22. The source mesh wheel centres are at +40cm.
inline constexpr double OriginalVehicleVisualOffsetZCm = -62.0;
static_assert(OriginalVehicleVisualOffsetZCm+40.0 == -22.0,
    "Preserve visible wheel and existing physics-root alignment.");
inline constexpr std::size_t OriginalVehicleSectionCount =
    static_cast<std::size_t>(OriginalVehicleMaterial::Count);

struct OriginalVehicleRenderSections final
{
    std::array<std::vector<std::uint32_t>,OriginalVehicleSectionCount>
        TriangleIds{};
    std::size_t TotalTriangles=0;
    bool Valid=false;
};

inline OriginalVehicleRenderSections MakeOriginalVehicleRenderSections(
    const OriginalVehicleMesh& mesh)
{
    OriginalVehicleRenderSections out{};
    if (!mesh.Ready || !mesh.VisualOnly || mesh.ChangesChaosContact ||
        !mesh.OriginalGeometry || mesh.Triangles.empty() ||
        mesh.Triangles.size()>OriginalVehicleMaxTriangles ||
        !OriginalVehicleSurfaceComposer{}.ValidateMesh(mesh))
        return out;

    for (std::size_t i=0;i<mesh.Triangles.size();++i)
    {
        const std::size_t material =
            static_cast<std::size_t>(mesh.Triangles[i].Material);
        if (material>=out.TriangleIds.size())
            return {};
        out.TriangleIds[material].push_back(
            static_cast<std::uint32_t>(i));
        ++out.TotalTriangles;
    }
    out.Valid=out.TotalTriangles==mesh.Triangles.size();
    return out;
}
static_assert(OriginalVehicleSectionCount==10u,
    "Update Unreal material palette when surface tags change.");
} // namespace latebrake::vehiclemesh
