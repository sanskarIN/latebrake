#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LatebrakeCheckpoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class LATEBRAKE_API ALatebrakeCheckpoint : public AActor
{
    GENERATED_BODY()

public:
    ALatebrakeCheckpoint();

    void SetCheckpointIndex(int32 InIndex) { CheckpointIndex = InIndex; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Marker;

    int32 CheckpointIndex = INDEX_NONE;

    UFUNCTION()
    void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};
