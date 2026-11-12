#include "Track/OriginalArmcoUnrealPreviewBridge.h"

#include "Track/OriginalArmcoBarrierMesh.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

#include <array>
#include <cstddef>

namespace latebrake::armco
{
namespace
{
struct UnrealArmcoSection final
{
    TArray<FVector> Positions{};
    TArray<int32> Indices{};
    TArray<FVector> Normals{};
    TArray<FVector2D> Uvs{};
    TArray<FLinearColor> Colors{};
    TArray<FProcMeshTangent> Tangents{};
};
FLinearColor OriginalBarrierSteelColor(ArmcoLayer layer) noexcept
{
    switch(layer)
    {
    case ArmcoLayer::CorrugatedSteel:
        return FLinearColor(0.42f,0.46f,0.49f,1.0f);
    case ArmcoLayer::PostSteel:
        return FLinearColor(0.26f,0.30f,0.34f,1.0f);
    case ArmcoLayer::BoltFace:
        return FLinearColor(0.68f,0.71f,0.72f,1.0f);
    case ArmcoLayer::RedSafetyMarker:
        return FLinearColor(0.82f,0.13f,0.085f,1.0f);
    case ArmcoLayer::Count:
        break;
    }
    return FLinearColor::Black;
}
bool PrepareOriginalBarrierSections(
    const ArmcoMesh& mesh,
    std::array<UnrealArmcoSection,
        static_cast<std::size_t>(ArmcoLayer::Count)>& sections)
{
    if(!mesh.Ready || !mesh.VisualOnly ||
       !mesh.NoSeparateCollision ||
       !mesh.OriginalRoadCollisionUntouched)
        return false;
    for(const auto& triangle:mesh.Triangles)
    {
        const auto group=static_cast<std::size_t>(triangle.Layer);
        if(group>=sections.size() ||
           triangle.PhysicalBarrier<OriginalBarrierFirstBlock ||
           triangle.PhysicalBarrier>OriginalBarrierLastBlock)
            return false;
        auto& section=sections[group];
        const int32 first=section.Positions.Num();
        if(first<0 ||
           first+3>static_cast<int32>(OriginalArmcoMaxVertices))
            return false;
        for(const std::uint32_t index:triangle.Indices)
        {
            if(index>=mesh.Vertices.size())
                return false;
            const auto& v=mesh.Vertices[index];
            section.Positions.Add(FVector(
                v.Position.X,v.Position.Y,v.Position.Z));
            section.Normals.Add(FVector(
                v.Normal.X,v.Normal.Y,v.Normal.Z));
            section.Uvs.Add(FVector2D(v.Uv.U,v.Uv.V));
            section.Colors.Add(
                OriginalBarrierSteelColor(triangle.Layer));
        }
        section.Indices.Add(first);
        section.Indices.Add(first+1);
        section.Indices.Add(first+2);
    }
    std::size_t receivedTriangles=0;
    for(const auto& section:sections)
    {
        const auto count=section.Positions.Num();
        if(count!=section.Indices.Num() ||
           count!=section.Normals.Num() ||
           count!=section.Uvs.Num() ||
           count!=section.Colors.Num() ||
           count%3!=0)
            return false;
        receivedTriangles+=static_cast<std::size_t>(count/3);
    }
    return receivedTriangles==mesh.Triangles.size();
}
}
// Source is an existing engine cube and original 4 collision-bearing walls.
// The preview never changes those physical actors or makes drivable geometry.
bool SpawnOriginalArmcoPreview(
    UWorld* world,UStaticMesh* sourceMesh)
{
    if(!IsValid(world) || !IsValid(sourceMesh))
        return false;
    UMaterialInterface* materialTemplate=
        sourceMesh->GetMaterial(0);
    if(!IsValid(materialTemplate))
        return false;
    OriginalArmcoBarrierMeshBuilder builder;
    const auto built=builder.Build();
    if(!built.Valid || !builder.ValidateMesh(built.Mesh))
        return false;
    std::array<UnrealArmcoSection,
        static_cast<std::size_t>(ArmcoLayer::Count)> sections{};
    if(!PrepareOriginalBarrierSections(built.Mesh,sections))
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
    owner->SetActorLabel(
        TEXT("LatebrakeOriginalFourWallArmcoPreview_NoCollision"));
#endif
    owner->SetActorEnableCollision(false);
    USceneComponent* root=NewObject<USceneComponent>(
        owner,TEXT("OriginalArmcoRoot"));
    if(!root)
    {
        owner->Destroy();
        return false;
    }
    root->SetMobility(EComponentMobility::Static);
    owner->SetRootComponent(root);
    owner->AddInstanceComponent(root);
    root->RegisterComponent();

    UProceduralMeshComponent* mesh=NewObject<UProceduralMeshComponent>(
        owner,TEXT("OriginalBarrierRails"));
    if(!mesh)
    {
        owner->Destroy();
        return false;
    }
    mesh->SetupAttachment(root);
    mesh->SetMobility(EComponentMobility::Static);
    mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    mesh->SetGenerateOverlapEvents(false);
    mesh->SetCanEverAffectNavigation(false);
    mesh->SetCastShadow(false);
    mesh->bUseAsyncCooking=false;
    owner->AddInstanceComponent(mesh);
    mesh->RegisterComponent();

    int emitted=0;
    for(std::size_t i=0;i<sections.size();++i)
    {
        const auto& section=sections[i];
        if(section.Positions.IsEmpty())
            continue;
        const auto index=static_cast<int32>(i);
        mesh->CreateMeshSection_LinearColor(
            index,
            section.Positions,
            section.Indices,
            section.Normals,
            section.Uvs,
            section.Colors,
            section.Tangents,
            false, // Never create alternate collision on a barrier.
            false);
        mesh->SetMaterial(index,materialTemplate);
        if(UMaterialInstanceDynamic* material=
               mesh->CreateDynamicMaterialInstance(index))
        {
            material->SetVectorParameterValue(
                TEXT("Color"),
                OriginalBarrierSteelColor(
                    static_cast<ArmcoLayer>(i)));
        }
        ++emitted;
    }
    if(emitted!=static_cast<int>(
           ArmcoLayer::Count))
    {
        owner->Destroy();
        return false;
    }
    // Four original BlockAll world walls still own their physics.
    // No timing, vehicle force, lap, score or safety flag is changed.
    return true;
}
} // namespace latebrake::armco
