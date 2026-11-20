#include "Vehicles/LatebrakeSkidTrailActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

ALatebrakeSkidTrailActor::ALatebrakeSkidTrailActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadSkidInstances"));
    RootComponent = Instances;
    Instances->SetMobility(EComponentMobility::Movable);
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Instances->SetCollisionObjectType(ECC_WorldDynamic);
    Instances->SetGenerateOverlapEvents(false);
    Instances->SetCanEverAffectNavigation(false);
    Instances->SetCastShadow(false);
    Instances->SetSimulatePhysics(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Instances->SetStaticMesh(Cube.Object);
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Material(
        TEXT("/Game/Materials/M_LatebrakeTireRubber.M_LatebrakeTireRubber"));
    RubberMaterial = Material.Get();
}

void ALatebrakeSkidTrailActor::BeginPlay()
{
    Super::BeginPlay();
    if (!RubberMaterial)
        RubberMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materials/M_LatebrakeTireRubber.M_LatebrakeTireRubber"));
    bRenderingReady = IsValid(Instances) && Instances->GetStaticMesh() && RubberMaterial;
    if (bRenderingReady)
    {
        Instances->SetMaterial(0, RubberMaterial);
        Instances->PreAllocateInstancesMemory(static_cast<int32>(Pool.MaximumCount()));
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LATEBRAKE_SKID_TRAILS_UNAVAILABLE reason=mesh-or-authored-rubber-material source_only_qa=required"));
    }
    if (Instances) Instances->SetVisibility(bRenderingReady);
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
        UE_LOG(LogTemp, Warning, TEXT("LATEBRAKE_SKID_TRAILS_UNAVAILABLE reason=instance-update-failed"));
    bRenderingReady = false;
    ResetTrails();
    if (IsValid(Instances)) Instances->SetVisibility(false);
}

bool ALatebrakeSkidTrailActor::WriteStrip(
    latebrake::SkidSlotIdentity Slot, const latebrake::SkidStrip& Strip)
{
    if (!Strip.Valid || !Pool.IsActive(Slot) || !IsValid(Instances)) return false;
    const FVector Forward(Strip.Forward.X, Strip.Forward.Y, Strip.Forward.Z);
    const FVector Up(Strip.Up.X, Strip.Up.Y, Strip.Up.Z);
    const FQuat Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
    const FTransform Pose(Rotation,
        FVector(Strip.Center.X, Strip.Center.Y, Strip.Center.Z),
        FVector(Strip.LengthCm/100.0, Strip.WidthCm/100.0, Strip.ThicknessCm/100.0));
    const int32 Index = static_cast<int32>(Slot.Slot);
    const int32 Count = Instances->GetInstanceCount();
    if (Index < Count)
        return Instances->UpdateInstanceTransform(Index, Pose, true, false, true);
    if (Index != Count || Count >= static_cast<int32>(Pool.MaximumCount())) return false;
    return Instances->AddInstance(Pose, true) == Index;
}

bool ALatebrakeSkidTrailActor::Submit(const latebrake::SurfaceSkidFrame& Frame)
{
    if (!bRenderingReady || !IsValid(Instances))
    {
        Trail.BreakContinuity();
        return false;
    }
    if (!Frame.DrivingAllowed)
    {
        Trail.BreakContinuity();
        return false;
    }
    latebrake::SkidExpiryBatch Expired;
    if (!Pool.Advance(Frame.DeltaSeconds, Expired))
    {
        Trail.BreakContinuity();
        return false;
    }
    // Retire old indices without RemoveInstance's index compaction.
    // Generation checks prevent a delayed expiry from hiding a reused slot.
    const FTransform Hidden(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
    for (unsigned i = 0; i < Expired.Count; ++i)
    {
        const auto Id = Expired.Retired[i];
        if (Pool.IsCurrent(Id) && !Pool.IsActive(Id) &&
            static_cast<int32>(Id.Slot) < Instances->GetInstanceCount() &&
            !Instances->UpdateInstanceTransform(static_cast<int32>(Id.Slot), Hidden, true, false, true))
        {
            DisableRendering();
            return false;
        }
    }
    const auto Batch = Trail.Advance(Frame);
    if (!Batch.Valid) return false;
    for (unsigned i = 0; i < Batch.Count; ++i)
    {
        const auto Id = Pool.Insert();
        if (!WriteStrip(Id, Batch.Strips[i]))
        {
            DisableRendering();
            return false;
        }
    }
    if (Expired.Count != 0 || Batch.Count != 0) Instances->MarkRenderStateDirty();
    return true;
}
