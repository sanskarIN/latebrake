#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Racing/CheckpointMotionTrace.h"
#include "Racing/GateDiagnosticCounters.h"
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

    // Player identity change and per-pawn RecoverTo generations invalidate
    // an old sample before attempting to credit a physical plane crossing.
    TWeakObjectPtr<AActor> LastObservedPlayer;
    latebrake::CheckpointMotionTrace MotionTrace;

    // Plain POD fields (no UHT preprocessor branches inside UCLASS).
    // Only accessed by opt-in diagnostics code in non-shipping builds.
    latebrake::CircuitGateDecision LastLoggedDecision =
        latebrake::CircuitGateDecision::NoForwardPlaneCrossing;
    bool bHadLoggedDecision = false;
    latebrake::GateDiagnosticCounters DiagnosticWindow;
    double DiagnosticWindowSeconds = 0.0;
};
