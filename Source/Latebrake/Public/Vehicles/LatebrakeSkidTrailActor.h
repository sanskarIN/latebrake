#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Vehicles/SurfaceSkidTrail.h"
#include "Vehicles/SkidSegmentPool.h"
#include "LatebrakeSkidTrailActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// One optional world-space renderer for each actual live arcade car.
// The holder is never attached to the moving body and cannot collide,
// generate overlaps, affect navigation, award scores or simulate physics.
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
    bool IsRenderingReady() const noexcept { return bRenderingReady; }
    int32 ActiveStrips() const noexcept
    {
        return bRenderingReady ? static_cast<int32>(Pool.ActiveCount()) : 0;
    }

private:
    bool PrepareMaterial();
    bool WriteStrip(latebrake::SkidSlotIdentity Slot,const latebrake::SkidStrip& Strip);
    bool RetireStrip(latebrake::SkidSlotIdentity Slot);
    bool IsComponentSafe() const;
    void DisableRendering();

    UPROPERTY(VisibleAnywhere,Category="Vehicle|Effects")
    TObjectPtr<UInstancedStaticMeshComponent> Instances;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> SelectedMaterial;
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StockTint;

    latebrake::SurfaceSkidTrail Trail;
    latebrake::SkidSegmentPool<> Pool;
    bool bRenderingReady=false;
    bool bAuthoredMaterial=false;
};
