#include "Track/OriginalAsphaltUnrealPreviewBridge.h"

#include "Track/OriginalPhysicalAsphaltSkin.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

#include <array>
#include <cstddef>

namespace latebrake::trackskin
{
namespace
{
struct UnrealAsphaltSection final
{
    TArray<FVector> Positions{};
    TArray<int32> Indices{};
    TArray<FVector> Normals{};
    TArray<FVector2D> Uvs{};
    TArray<FLinearColor> Colors{};
    TArray<FProcMeshTangent> Tangents{};
};
FLinearColor OriginalAsphaltColor(AsphaltSkinLayer layer) noexcept
{
    switch(layer)
    {
    case AsphaltSkinLayer::FreshAsphalt:
        return FLinearColor(0.10f,0.115f,0.125f,1.0f);
    case AsphaltSkinLayer::MidtoneAsphalt:
        return FLinearColor(0.125f,0.138f,0.145f,1.0f);
    case AsphaltSkinLayer::WeatheredAsphalt:
        return FLinearColor(0.16f,0.17f,0.175f,1.0f);
    case AsphaltSkinLayer::RubberGrain:
        return FLinearColor(0.025f,0.027f,0.030f,1.0f);
    case AsphaltSkinLayer::JointSeam:
        return FLinearColor(0.034f,0.034f,0.035f,1.0f);
    case AsphaltSkinLayer::RepairedAsphalt:
        return FLinearColor(0.08f,0.085f,0.09f,1.0f);
    case AsphaltSkinLayer::Count:
        break;
    }
    return FLinearColor::Black;
}
bool BuildUnrealSections(
    const AsphaltSkinMesh& mesh,
    std::array<UnrealAsphaltSection,
        static_cast<std::size_t>(AsphaltSkinLayer::Count)>& sections)
{
    if(!mesh.Ready || !mesh.VisualOnly ||
       mesh.HasIndependentCollision ||
       mesh.ChangesRaceTiming)
        return false;
    for(const auto& tri:mesh.Triangles)
    {
        const auto material=static_cast<std::size_t>(tri.Layer);
        if(material>=sections.size())
            return false;
        auto& section=sections[material];
        const int32 first=section.Positions.Num();
        if(first<0 || first+3>static_cast<int32>(MaximumSkinVertices))
            return false;
        for(std::uint32_t vertexId:tri.Indices)
        {
            if(vertexId>=mesh.Vertices.size())
                return false;
            const auto& vertex=mesh.Vertices[vertexId];
            section.Positions.Add(FVector(
                vertex.Position.X,vertex.Position.Y,vertex.Position.Z));
            section.Normals.Add(FVector(
                vertex.Normal.X,vertex.Normal.Y,vertex.Normal.Z));
            section.Uvs.Add(FVector2D(
                vertex.Uv.U,vertex.Uv.V));
            section.Colors.Add(OriginalAsphaltColor(tri.Layer));
        }
        section.Indices.Add(first);
        section.Indices.Add(first+1);
        section.Indices.Add(first+2);
    }
    int32 sumTriangles=0;
    for(const auto& section:sections)
    {
        const int32 vertexCount=section.Positions.Num();
        if(vertexCount!=section.Indices.Num() ||
           vertexCount!=section.Normals.Num() ||
           vertexCount!=section.Uvs.Num() ||
           vertexCount!=section.Colors.Num() ||
           vertexCount%3!=0)
            return false;
        sumTriangles+=section.Indices.Num()/3;
    }
    return sumTriangles==static_cast<int32>(mesh.Triangles.size());
}
}

// Use only inside an intentionally enabled, non-shipping, real Unreal world.
// This source supplies visual detail to existing physical asphalt, never
// changes that asphalt's BodySetup or creates drivable collision.
bool SpawnOriginalAsphaltSkinPreview(
    UWorld* world,UStaticMesh* sourceMesh)
{
    if(!IsValid(world) || !IsValid(sourceMesh))
        return false;
    UMaterialInterface* templateMaterial=sourceMesh->GetMaterial(0);
    if(!IsValid(templateMaterial))
        return false;
    const OriginalPhysicalAsphaltSkin author{};
    const auto result=author.Build();
    if(!result.Valid || !author.ValidateMesh(result.Mesh))
        return false;

    std::array<UnrealAsphaltSection,
        static_cast<std::size_t>(AsphaltSkinLayer::Count)> sections{};
    if(!BuildUnrealSections(result.Mesh,sections))
        return false;
    FActorSpawnParameters parameters;
    parameters.SpawnCollisionHandlingOverride=
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* owner=world->SpawnActor<AActor>(
        AActor::StaticClass(),FVector::ZeroVector,
        FRotator::ZeroRotator,parameters);
    if(!IsValid(owner))
        return false;
#if WITH_EDITOR
    owner->SetActorLabel(TEXT("LatebrakeOriginalAsphaltSkinPreview"));
#endif
    USceneComponent* root=NewObject<USceneComponent>(
        owner,TEXT("OriginalAsphaltRoot"));
    if(!root)
    {
        owner->Destroy();
        return false;
    }
    root->SetMobility(EComponentMobility::Static);
    owner->SetRootComponent(root);
    owner->AddInstanceComponent(root);
    root->RegisterComponent();

    UProceduralMeshComponent* paint=NewObject<UProceduralMeshComponent>(
        owner,TEXT("OriginalAsphaltSkin"));
    if(!paint)
    {
        owner->Destroy();
        return false;
    }
    paint->SetupAttachment(root);
    paint->SetMobility(EComponentMobility::Static);
    paint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    paint->SetGenerateOverlapEvents(false);
    paint->SetCanEverAffectNavigation(false);
    paint->SetCastShadow(false);
    paint->bUseAsyncCooking=false;
    owner->AddInstanceComponent(paint);
    paint->RegisterComponent();

    for(std::size_t i=0;i<sections.size();++i)
    {
        const auto& section=sections[i];
        if(section.Positions.IsEmpty())
            continue;
        const int32 sectionIndex=static_cast<int32>(i);
        paint->CreateMeshSection_LinearColor(
            sectionIndex,
            section.Positions,
            section.Indices,
            section.Normals,
            section.Uvs,
            section.Colors,
            section.Tangents,
            false, // Authoritative asphalt collision remains unchanged.
            false);
        paint->SetMaterial(sectionIndex,templateMaterial);
        if(UMaterialInstanceDynamic* material=
               paint->CreateDynamicMaterialInstance(sectionIndex))
        {
            material->SetVectorParameterValue(
                TEXT("Color"),
                OriginalAsphaltColor(
                    static_cast<AsphaltSkinLayer>(i)));
        }
    }
    // Cosmetic shape registration is not engine installed-game QA.
    // The caller may log the preview but never use it for start readiness,
    // checkpoint validity, tire friction or lap credit.
    return true;
}
} // namespace latebrake::trackskin
