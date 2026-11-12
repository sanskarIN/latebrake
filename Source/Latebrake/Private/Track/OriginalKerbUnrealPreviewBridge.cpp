#include "Track/OriginalKerbUnrealPreviewBridge.h"
#include "Track/OriginalCircuitKerbMesh.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include <array>

namespace latebrake::kerbing
{
namespace
{
struct NativeKerbSection final
{
    TArray<FVector> Positions;
    TArray<int32> Indices;
    TArray<FVector> Normals;
    TArray<FVector2D> Uvs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
};
FLinearColor KerbTint(KerbSurface layer) noexcept
{
    switch(layer)
    {
    case KerbSurface::RedPaint:return {0.94f,0.08f,0.07f};
    case KerbSurface::WhitePaint:return {0.91f,0.93f,0.88f};
    case KerbSurface::BlackAsphaltBlend:return {0.08f,0.09f,0.10f};
    case KerbSurface::YellowDrainage:return {0.72f,0.60f,0.09f};
    case KerbSurface::EdgeShadow:return {0.02f,0.025f,0.03f};
    case KerbSurface::Count:break;
    }
    return FLinearColor::Black;
}
bool PackKerbSections(
    const KerbMesh& input,
    std::array<NativeKerbSection,
        static_cast<std::size_t>(KerbSurface::Count)>& output)
{
    if(!input.Ready || !input.PurelyVisual ||
       !input.OriginalCollisionUnmodified)
        return false;
    for(const auto& triangle:input.Triangles)
    {
        const auto material=static_cast<std::size_t>(triangle.Surface);
        if(material>=output.size())
            return false;
        auto& section=output[material];
        const int32 base=section.Positions.Num();
        if(base+3>static_cast<int32>(OriginalKerbMaxVertices))
            return false;
        for(const auto id:triangle.Indices)
        {
            if(id>=input.Vertices.size())
                return false;
            const auto& vertex=input.Vertices[id];
            section.Positions.Add(FVector(
                vertex.Position.X,vertex.Position.Y,vertex.Position.Z));
            section.Normals.Add(FVector(
                vertex.Normal.X,vertex.Normal.Y,vertex.Normal.Z));
            section.Uvs.Add(FVector2D(vertex.Uv.U,vertex.Uv.V));
            section.Colors.Add(KerbTint(triangle.Surface));
        }
        section.Indices.Add(base);
        section.Indices.Add(base+1);
        section.Indices.Add(base+2);
    }
    return true;
}
}
bool SpawnOriginalKerbPreview(UWorld* world,UStaticMesh* engineCube)
{
    if(!IsValid(world) || !IsValid(engineCube))
        return false;
    UMaterialInterface* material=engineCube->GetMaterial(0);
    if(!IsValid(material))
        return false;
    OriginalCircuitKerbComposer builder;
    const auto authored=builder.Build();
    if(!authored.Valid || !builder.Validate(authored.Mesh))
        return false;
    std::array<NativeKerbSection,
        static_cast<std::size_t>(KerbSurface::Count)> sections{};
    if(!PackKerbSections(authored.Mesh,sections))
        return false;
    FActorSpawnParameters args;
    args.SpawnCollisionHandlingOverride=
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* owner=world->SpawnActor<AActor>(
        AActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,args);
    if(!IsValid(owner))
        return false;
    owner->SetActorEnableCollision(false);
#if WITH_EDITOR
    owner->SetActorLabel(TEXT("LatebrakeOriginalKerbPaint_NoCollision"));
#endif
    USceneComponent* root=NewObject<USceneComponent>(
        owner,TEXT("KerbRoot"));
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
        owner,TEXT("KerbVisualSections"));
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
    int created=0;
    for(std::size_t i=0;i<sections.size();++i)
    {
        const auto& section=sections[i];
        if(section.Positions.IsEmpty())
            continue;
        mesh->CreateMeshSection_LinearColor(
            static_cast<int32>(i),section.Positions,section.Indices,
            section.Normals,section.Uvs,section.Colors,section.Tangents,
            false,false); // No new physics collision.
        mesh->SetMaterial(static_cast<int32>(i),material);
        if(auto* dynamic=mesh->CreateDynamicMaterialInstance(
               static_cast<int32>(i)))
            dynamic->SetVectorParameterValue(
                TEXT("Color"),KerbTint(static_cast<KerbSurface>(i)));
        ++created;
    }
    if(created!=static_cast<int>(KerbSurface::Count))
    {
        owner->Destroy();
        return false;
    }
    return true;
}
} // namespace latebrake::kerbing
