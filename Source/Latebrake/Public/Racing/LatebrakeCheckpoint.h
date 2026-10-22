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

    virtual void Tick(float DeltaSeconds) override;

    void SetCheckpointIndex(int32 InIndex) { CheckpointIndex = InIndex; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Marker;

    int32 CheckpointIndex = INDEX_NONE;

    // Sample only the local authoritative player's position, never rivals.
    // Changing/losing pawn identity discards the previous trajectory rather
    // than accidentally crediting a teleport/restart/possession transition.
    TWeakObjectPtr<AActor> LastObservedPlayer;
    FVector PreviousPlayerLocation = FVector::ZeroVector;
    bool bHasPreviousSample = false;
};
