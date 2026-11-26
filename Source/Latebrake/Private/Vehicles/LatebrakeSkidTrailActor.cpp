#include "Vehicles/LatebrakeSkidTrailActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Vehicles/ArcadeSkidRoadGeometry.h"

ALatebrakeSkidTrailActor::ALatebrakeSkidTrailActor()
{
    PrimaryActorTick.bCanEverTick=false;
    SetReplicates(false);
    Instances=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadSkidInstances"));
    RootComponent=Instances;
    Instances->SetMobility(EComponentMobility::Movable);
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Instances->SetCollisionObjectType(ECC_WorldDynamic);
    Instances->SetGenerateOverlapEvents(false);
    Instances->SetCanEverAffectNavigation(false);
    Instances->SetCastShadow(false);
    Instances->SetSimulatePhysics(false);
    Instances->SetVisibility(false);
    SetActorEnableCollision(false);
}

bool ALatebrakeSkidTrailActor::PrepareMaterial()
{
    if (!IsValid(Instances)) return false;
    // Prefer the project-owned material if actually authored in Unreal.
    // Otherwise reuse a real stock material only if it exposes an
    // actual global vector tint, never an assumed shader parameter.
    auto* Authored=LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materials/M_LatebrakeTireRubber.M_LatebrakeTireRubber"));
    if (IsValid(Authored))
    {
        SelectedMaterial=Authored;
        bAuthoredMaterial=true;
    }
    else
    {
        auto* Base=LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (!IsValid(Base)) return false;
        TArray<FMaterialParameterInfo> Parameters;
        TArray<FGuid> Guids;
        Base->GetAllVectorParameterInfo(Parameters,Guids);
        const FName Candidates[]={TEXT("Color"),TEXT("Colour"),TEXT("BaseColor")};
        FName TintName=NAME_None;
        for(const FName& Candidate:Candidates)
        {
            if (Parameters.ContainsByPredicate([&Candidate](const FMaterialParameterInfo& Info)
                {
                    return Info.Name==Candidate &&
                        Info.Association==EMaterialParameterAssociation::GlobalParameter;
                }))
            {
                TintName=Candidate;
                break;
            }
        }
        if (TintName.IsNone()) return false;
        StockTint=UMaterialInstanceDynamic::Create(Base,this);
        if (!IsValid(StockTint)) return false;
        StockTint->SetVectorParameterValue(TintName,FLinearColor(0.012f,0.011f,0.010f,1.0f));
        SelectedMaterial=StockTint;
        bAuthoredMaterial=false;
    }
    Instances->SetMaterial(0,SelectedMaterial);
    return IsValid(SelectedMaterial) && Instances->GetMaterial(0)==SelectedMaterial;
}

bool ALatebrakeSkidTrailActor::IsComponentSafe() const
{
    return IsValid(Instances) && Instances->IsRegistered() &&
        Instances->GetCollisionEnabled()==ECollisionEnabled::NoCollision &&
        !Instances->GetGenerateOverlapEvents() && !Instances->IsSimulatingPhysics() &&
        !GetActorEnableCollision() && !GetIsReplicated() &&
        Instances->GetInstanceCount()>=0 &&
        Instances->GetInstanceCount()<=static_cast<int32>(Pool.MaximumCount()) &&
        GetActorTransform().Equals(FTransform::Identity,0.001) &&
        IsValid(Instances->GetStaticMesh());
}

void ALatebrakeSkidTrailActor::BeginPlay()
{
    Super::BeginPlay();
    if (!IsValid(Instances)) return;
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (IsValid(Cube)) Instances->SetStaticMesh(Cube);
    if (!IsComponentSafe() || !PrepareMaterial())
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_SKID_TRAILS_UNAVAILABLE reason=mesh-material-or-safety"));
        DisableRendering();
        return;
    }
    Instances->PreAllocateInstancesMemory(static_cast<int32>(Pool.MaximumCount()));
    bRenderingReady=true;
    Instances->SetVisibility(true);
    UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_SKID_TRAILS_READY source=arcade-probe material=%s capacity=%d"),
        bAuthoredMaterial?TEXT("authored"):TEXT("stock-tint"),
        static_cast<int32>(Pool.MaximumCount()));
}

void ALatebrakeSkidTrailActor::BreakContinuity()
{
    Trail.BreakContinuity();
}

void ALatebrakeSkidTrailActor::ResetTrails()
{
    Trail.BreakContinuity();
    Pool.Reset();
    if (IsValid(Instances)) Instances->ClearInstances();
}

void ALatebrakeSkidTrailActor::DisableRendering()
{
    if (bRenderingReady)
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_SKID_TRAILS_UNAVAILABLE reason=runtime-instance-or-safety"));
    bRenderingReady=false;
    ResetTrails();
    if (IsValid(Instances)) Instances->SetVisibility(false);
}

bool ALatebrakeSkidTrailActor::RetireStrip(latebrake::SkidSlotIdentity Slot)
{
    if (!Pool.IsCurrent(Slot)) return true; // stale identity cannot hide a reused index
    if (Pool.IsActive(Slot)) return false;
    const int32 Index=static_cast<int32>(Slot.Slot);
    if (!IsValid(Instances) || Index>=Instances->GetInstanceCount()) return false;
    // No RemoveInstance or index compaction; keep pool/renderer identity.
    const FTransform Hidden(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector);
    return Instances->UpdateInstanceTransform(Index,Hidden,true,false,true);
}

bool ALatebrakeSkidTrailActor::WriteStrip(
    latebrake::SkidSlotIdentity Slot,const latebrake::SkidStrip& Strip)
{
    if (!latebrake::IsRenderableSkidStrip(Strip) || !Pool.IsActive(Slot) ||
        !IsValid(Instances)) return false;
    const FVector Forward(Strip.Forward.X,Strip.Forward.Y,Strip.Forward.Z);
    const FVector Up(Strip.Up.X,Strip.Up.Y,Strip.Up.Z);
    const FQuat Rotation=FRotationMatrix::MakeFromXZ(Forward,Up).ToQuat();
    const FTransform Pose(Rotation,FVector(Strip.Center.X,Strip.Center.Y,Strip.Center.Z),
        FVector(Strip.LengthCm/100.0,Strip.WidthCm/100.0,Strip.ThicknessCm/100.0));
    if (Pose.ContainsNaN()) return false;
    const int32 Index=static_cast<int32>(Slot.Slot);
    const int32 Count=Instances->GetInstanceCount();
    if (Index<Count) return Instances->UpdateInstanceTransform(Index,Pose,true,false,true);
    if (Index!=Count || Count>=static_cast<int32>(Pool.MaximumCount())) return false;
    return Instances->AddInstance(Pose,true)==Index;
}

bool ALatebrakeSkidTrailActor::Submit(const latebrake::SurfaceSkidFrame& Frame)
{
    if (!bRenderingReady)
    {
        Trail.BreakContinuity();
        return false;
    }
    if (!IsComponentSafe() || Instances->GetMaterial(0)!=SelectedMaterial)
    {
        DisableRendering();
        return false;
    }
    if (!Frame.DrivingAllowed)
    {
        Trail.BreakContinuity();
        return false;
    }
    latebrake::SkidExpiryBatch Expired;
    if (!Pool.Advance(Frame.DeltaSeconds,Expired))
    {
        Trail.BreakContinuity();
        return false;
    }
    for (unsigned Index=0;Index<Expired.Count;++Index)
    {
        if (!RetireStrip(Expired.Retired[Index]))
        {
            DisableRendering();
            return false;
        }
    }
    const auto Batch=Trail.Advance(Frame);
    if (!Batch.Valid || Batch.Count>Batch.Strips.size())
    {
        DisableRendering();
        return false;
    }
    // Validate the whole batch before mutating stable renderer indices.
    for (unsigned Index=0;Index<Batch.Count;++Index)
    {
        if (!latebrake::IsRenderableSkidStrip(Batch.Strips[Index]))
        {
            DisableRendering();
            return false;
        }
    }
    for (unsigned Index=0;Index<Batch.Count;++Index)
    {
        if (!WriteStrip(Pool.Insert(),Batch.Strips[Index]))
        {
            DisableRendering();
            return false;
        }
    }
    if (Expired.Count!=0 || Batch.Count!=0) Instances->MarkRenderStateDirty();
    return true;
}
