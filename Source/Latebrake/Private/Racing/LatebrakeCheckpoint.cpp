#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/GateValidation.h"

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
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("CheckpointTrigger"));
    RootComponent = Trigger;
    Trigger->InitSphereRadius(600.f);
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetGenerateOverlapEvents(true);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMarker"));
    Marker->SetupAttachment(Trigger);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetRelativeScale3D(FVector(11.f, 11.f, 0.08f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        Marker->SetStaticMesh(Cylinder.Object);
    }

    Trigger->OnComponentBeginOverlap.AddDynamic(this, &ALatebrakeCheckpoint::OnTriggerOverlap);
}

void ALatebrakeCheckpoint::OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(OtherActor);
    // Rivals and traffic may overlap the same gates, but only the local
    // player's pawn can advance their own RaceProgress.
    if (!Car || OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
    {
        return;
    }

    const FVector Velocity = Car->GetVelocity();
    if (!latebrake::CanPassCircuitGate(CheckpointIndex, Velocity.X, Velocity.Y))
    {
        return;
    }

    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RegisterCheckpoint(CheckpointIndex);
    }
}
