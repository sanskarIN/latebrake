#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Racing/WrongWayIndicator.h"
#include "LatebrakeRaceHUD.generated.h"

class APawn;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    // Advisory only: no race awarding, control input or network telemetry.
    latebrake::WrongWayIndicator WrongWayHint;
    TWeakObjectPtr<APawn> LastWayfindingPawn;
    uint64 LastWayfindingSession = 0;
    uint64 LastWayfindingTeleport = 0;
};
