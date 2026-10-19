#pragma once

#include "CoreMinimal.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "LatebrakeRivalCar.generated.h"

/**
 * Experimental autonomous competitor, driving the same starter rigid-body car
 * as the player via the shared portable waypoint pilot.
 * A future release will replace this with a race-aware Chaos Vehicles AI.
 */
UCLASS()
class LATEBRAKE_API ALatebrakeRivalCar : public ALatebrakeCarPawn
{
    GENERATED_BODY()

public:
    ALatebrakeRivalCar();
    virtual void Tick(float DeltaSeconds) override;
    void ResetPilot() { NextWaypoint = 0; }

private:
    int32 NextWaypoint = 0;
};
