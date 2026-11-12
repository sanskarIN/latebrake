#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Vehicles/SurfaceSkidTrail.h"
#include "Vehicles/SkidSegmentPool.h"
#include "LatebrakeSkidTrailActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

// Run-local cosmetic holder. It is deliberately not attached to the
// moving vehicle: every accepted strip has an immutable world transform.
UCLASS(NotBlueprintable, Transient)
class LATEBRAKE_API ALatebrakeSkidTrailActor : public AActor
{
    GENERATED_BODY()

public:
    ALatebrakeSkidTrailActor();
    virtual void BeginPlay() override;
    bool Submit(const latebrake::SurfaceSkidFrame& Frame);
    void BreakContinuity();
    void ResetTrails();

private:
    bool WriteStrip(latebrake::SkidSlotIdentity Slot, const latebrake::SkidStrip& Strip);
    void DisableRendering();

    UPROPERTY(VisibleAnywhere, Category = "Vehicle|Effects")
    TObjectPtr<UInstancedStaticMeshComponent> Instances;

    UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Effects")
    TObjectPtr<UMaterialInterface> RubberMaterial;

    latebrake::SurfaceSkidTrail Trail;
    latebrake::SkidSegmentPool<> Pool;
    bool bRenderingReady = false;
};
