#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/CircuitGateCrossing.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

// Entirely opt-in and only available for development/editor builds.
// No network telemetry; no per-frame output when disabled (default).
namespace
{
TAutoConsoleVariable<int32> CVarLatebrakeCheckpointDiagnostics(
    TEXT("latebrake.CheckpointDiagnostics"), 0,
    TEXT("0 (default): no diagnostic log. 1: local gate event/rejection logs. "
         "Requires an authorized, non-shipping Unreal build."),
    ECVF_Default);
} // namespace
#endif

ALatebrakeCheckpoint::ALatebrakeCheckpoint()
{
    // True plane crossing is decided from successive player positions,
    // not the BEGIN OVERLAP event at the leading rim of a 600cm sphere.
    // Four lightweight actor ticks (no allocations or traces) evaluate
    // the shared engine-neutral circuit gate model.
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("CheckpointTrigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(600.f);
    // Retain the legacy root component for actor transform/visual marker;
    // a geometric crossing no longer needs an overlap collision event.
    Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Trigger->SetGenerateOverlapEvents(false);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMarker"));
    Marker->SetupAttachment(Trigger);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetRelativeScale3D(FVector(11.f, 11.f, 0.08f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        Marker->SetStaticMesh(Cylinder.Object);
    }
}

void ALatebrakeCheckpoint::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    ALatebrakeCarPawn* Car =
        Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!IsValid(Car))
    {
        LastObservedPlayer.Reset();
        MotionTrace.Reset();
#if !UE_BUILD_SHIPPING
        bHadLoggedDecision = false;
#endif
        return;
    }

    if (LastObservedPlayer.Get() != Car)
    {
        // A new pawn is not a continuation of the previous car's trajectory.
        LastObservedPlayer = Car;
        MotionTrace.Reset();
#if !UE_BUILD_SHIPPING
        bHadLoggedDecision = false;
#endif
    }

    const FVector Current = Car->GetActorLocation();
    const FVector Velocity = Car->GetVelocity();
    const latebrake::CheckpointTraceResult Result = MotionTrace.SampleDetailed(
        CheckpointIndex, Car->GetTeleportRevision(),
        {{Current.X, Current.Y}, Current.Z}, Velocity.X, Velocity.Y);

#if !UE_BUILD_SHIPPING
    if (CVarLatebrakeCheckpointDiagnostics.GetValueOnGameThread() != 0)
    {
        const bool bEvent = Result.Crossed() ||
            Result.Trace == latebrake::CheckpointTraceDecision::TeleportRebase ||
            Result.Trace == latebrake::CheckpointTraceDecision::InvalidPosition;
        const bool bNewUsefulRejection =
            Result.Trace == latebrake::CheckpointTraceDecision::GateRejected &&
            Result.Gate != latebrake::CircuitGateDecision::NoForwardPlaneCrossing &&
            (!bHadLoggedDecision || Result.Gate != LastLoggedDecision);
        if (bEvent || bNewUsefulRejection)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Latebrake checkpoint=%d trace=%s reason=%s dt=%.4f "
                     "vx=%.1f vy=%.1f generation=%llu"),
                CheckpointIndex,
                UTF8_TO_TCHAR(latebrake::CheckpointTraceDecisionName(Result.Trace)),
                UTF8_TO_TCHAR(latebrake::CircuitGateDecisionName(Result.Gate)),
                DeltaSeconds, Velocity.X, Velocity.Y,
                static_cast<unsigned long long>(Car->GetTeleportRevision()));
        }
        bHadLoggedDecision = true;
        LastLoggedDecision = Result.Gate;
    }
#endif

    // The shared 3D plane geometry rejects wrong-way, airborne and large
    // jumps. A changed teleport generation always rebases the trace first,
    // so even a short, paid RECOVER or RESTART cannot credit a checkpoint.
    // RaceProgress still requires the correct ORDERED gate and lap count.
    if (Result.Crossed())
    {
        if (ALatebrakeRaceGameMode* Mode =
                GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
        {
            Mode->RegisterCheckpoint(CheckpointIndex);
        }
    }
}
