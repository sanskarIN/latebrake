#include "Racing/LatebrakeCheckpoint.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "UObject/ConstructorHelpers.h"

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
    if (!Cast<ALatebrakeCarPawn>(OtherActor))
    {
        return;
    }

    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RegisterCheckpoint(CheckpointIndex);
    }
}
