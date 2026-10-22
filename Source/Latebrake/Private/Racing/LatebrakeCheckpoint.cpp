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
        bHasPreviousSample = false;
        return;
    }

    const FVector Current = Car->GetActorLocation();
    if (LastObservedPlayer.Get() != Car || !bHasPreviousSample)
    {
        LastObservedPlayer = Car;
        PreviousPlayerLocation = Current;
        bHasPreviousSample = true;
        return;
    }

    const FVector Velocity = Car->GetVelocity();
    const bool bCrossed = latebrake::CrossesCircuitGatePlane(
        CheckpointIndex,
        {{PreviousPlayerLocation.X, PreviousPlayerLocation.Y},
         PreviousPlayerLocation.Z},
        {{Current.X, Current.Y}, Current.Z},
        Velocity.X, Velocity.Y);
    PreviousPlayerLocation = Current;

    // The shared 3D plane geometry rejects premature trigger entries,
    // wrong-way passes, out-of-lane/airborne shortcuts and large jumps.
    // RaceProgress still requires the correct ORDERED gate and lap count.
    if (bCrossed)
    {
        if (ALatebrakeRaceGameMode* Mode =
                GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
        {
            Mode->RegisterCheckpoint(CheckpointIndex);
        }
    }
}
