#include "Vehicles/OriginalVehicleUnrealPreviewBridge.h"

#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include "Vehicles/VehiclePaintRecipe.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

#include <array>
#include <cstddef>

namespace latebrake::vehiclemesh
{
namespace
{
struct UnrealOriginalSection final
{
    TArray<FVector> Positions;
    TArray<int32> Indices;
    TArray<FVector> Normals;
    TArray<FVector2D> Uvs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
};
FLinearColor SectionColor(
    OriginalVehicleMaterial material,
    const latebrake::VehiclePaintRecipe& paint) noexcept
{
    const auto fromRgb=[](latebrake::VehiclePaintRgb p)
    {
        return FLinearColor(
            static_cast<float>(p.R),
            static_cast<float>(p.G),
            static_cast<float>(p.B),1.0f);
    };
    switch(material)
    {
    case OriginalVehicleMaterial::BodyPaint:
        return fromRgb(paint.Body);
    case OriginalVehicleMaterial::Accent:
        return fromRgb(paint.Accent);
    case OriginalVehicleMaterial::ClearGlass:
        return fromRgb(paint.Canopy);
    case OriginalVehicleMaterial::Rubber:
        return FLinearColor(0.035f,0.038f,0.044f,1.0f);
    case OriginalVehicleMaterial::Alloy:
        return FLinearColor(0.48f,0.52f,0.58f,1.0f);
    case OriginalVehicleMaterial::BrakeRotor:
        return FLinearColor(0.31f,0.34f,0.37f,1.0f);
    case OriginalVehicleMaterial::Headlamp:
        return FLinearColor(0.92f,0.96f,1.00f,1.0f);
    case OriginalVehicleMaterial::Taillamp:
        return FLinearColor(0.90f,0.032f,0.018f,1.0f);
    case OriginalVehicleMaterial::Intake:
        return FLinearColor(0.055f,0.07f,0.08f,1.0f);
    case OriginalVehicleMaterial::CarbonTrim:
        return FLinearColor(0.07f,0.08f,0.09f,1.0f);
    case OriginalVehicleMaterial::Count:
        break;
    }
    return FLinearColor::Black;
}
bool AssignMeshTriangles(
    const OriginalVehicleMesh& mesh,
    const latebrake::VehiclePaintRecipe& paint,
    std::array<UnrealOriginalSection,
        static_cast<std::size_t>(OriginalVehicleMaterial::Count)>& sections)
{
    if(!mesh.Ready || !mesh.VisualOnly ||
       mesh.ChangesChaosContact)
        return false;
    for(const auto& triangle:mesh.Triangles)
    {
        const std::size_t group=
            static_cast<std::size_t>(triangle.Material);
        if(group>=sections.size())
            return false;
        auto& section=sections[group];
        if(section.Indices.Num()>
            static_cast<int32>(OriginalVehicleMaxVertices-3))
            return false;
        const int32 first=section.Positions.Num();
        if(first>static_cast<int32>(OriginalVehicleMaxVertices-3))
            return false;
        for(std::uint32_t index:triangle.Index)
        {
            if(index>=mesh.Vertices.size())
                return false;
            const auto& v=mesh.Vertices[index];
            section.Positions.Add(FVector(v.Position.X,
                                          v.Position.Y,
                                          v.Position.Z));
            section.Normals.Add(FVector(v.Normal.X,
                                        v.Normal.Y,
                                        v.Normal.Z));
            section.Uvs.Add(FVector2D(v.TexCoord.U,v.TexCoord.V));
            section.Colors.Add(SectionColor(triangle.Material,paint));
        }
        section.Indices.Add(first);
        section.Indices.Add(first+1);
        section.Indices.Add(first+2);
    }
    return true;
}
}
bool CreateOriginalVehicleVisualSections(
    UProceduralMeshComponent* preview,
    UStaticMeshComponent* originalChassis,
    int vehicleId)
{
    if(!IsValid(preview) || !IsValid(originalChassis) ||
       vehicleId<0 || vehicleId>=6 ||
       !originalChassis->GetStaticMesh())
        return false;
    // No asset-loading, physics cooking or geometry regeneration happens
    // in Tick. This is intended for BeginPlay or an actual garage change.
    const OriginalVehicleSurfaceComposer builder{};
    const OriginalVehicleMeshResult result=builder.Build(vehicleId);
    if(!result.Valid || !builder.ValidateMesh(result.Mesh))
        return false;
    UMaterialInterface* base=originalChassis->GetMaterial(0);
    if(!IsValid(base))
        return false;
    std::array<UnrealOriginalSection,
        static_cast<std::size_t>(OriginalVehicleMaterial::Count)> sections{};
    const auto paint=latebrake::PaintForVehicle(vehicleId);
    if(!AssignMeshTriangles(result.Mesh,paint,sections))
        return false;
    for(const auto& section:sections)
    {
        const int32 vertices=section.Positions.Num();
        if(vertices!=section.Indices.Num() ||
           section.Normals.Num()!=vertices ||
           section.Uvs.Num()!=vertices ||
           section.Colors.Num()!=vertices)
            return false;
    }

    // The existing licensed Chaos rigid body remains the only physical
    // contact authority. Visual sections NEVER generate collision.
    preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    preview->SetGenerateOverlapEvents(false);
    preview->bUseAsyncCooking=false;
    preview->ClearAllMeshSections();
    for(std::size_t i=0;i<sections.size();++i)
    {
        const auto& section=sections[i];
        if(section.Indices.IsEmpty())
            continue;
        const int32 sectionId=static_cast<int32>(i);
        preview->CreateMeshSection_LinearColor(
            sectionId,
            section.Positions,
            section.Indices,
            section.Normals,
            section.Uvs,
            section.Colors,
            section.Tangents,
            false, // No runtime collision mesh, ever.
            false);
        preview->SetMaterial(sectionId,base);
        if(UMaterialInstanceDynamic* material=
               preview->CreateDynamicMaterialInstance(sectionId))
        {
            material->SetVectorParameterValue(
                TEXT("Color"),
                SectionColor(static_cast<OriginalVehicleMaterial>(i),paint));
        }
    }
    return true;
}
} // namespace latebrake::vehiclemesh
