#pragma once

#include "CoreMinimal.h"
#include "AI/RivalRouteProgress.h"
#include "AI/RivalFollowingBrakeHold.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "LatebrakeRivalCar.generated.h"

/**
 * Experimental circuit rival, not yet qualified as competitive race AI.
 * Portable route tracking counts approximate waypoint laps and recovers when
 * physically stuck; authoritative player rewards remain separate.
 */
UCLASS()
class LATEBRAKE_API ALatebrakeRivalCar : public ALatebrakeCarPawn
{
    GENERATED_BODY()

public:
    ALatebrakeRivalCar();
    virtual void Tick(float DeltaSeconds) override;
    void ResetPilot();
    // Event configuration controls route-length telemetry; it never grants
    // authoritative race placement or player checkpoint progression.
    void ConfigurePilot(int32 CheckpointCount, int32 RequiredLaps);
    int32 GetExpectedLaps() const { return Route.GetRequiredLaps(); }
    int32 GetApproximateLaps() const { return Route.GetCompletedLaps(); }
    int32 GetPilotRecoveries() const { return Route.GetRecoveryCount(); }
    int32 GetPilotSuccessfulRecoveries() const { return Route.GetSuccessfulRecoveryCount(); }
    int32 GetPilotFailedRecoveries() const { return Route.GetFailedRecoveryCount(); }
    bool HasCompletedRoute() const { return Route.IsFinished(); }

private:
    latebrake::RivalRouteProgress Route{4, 3};
    latebrake::RivalFollowingBrakeHold FollowingBrakeHold;
};
