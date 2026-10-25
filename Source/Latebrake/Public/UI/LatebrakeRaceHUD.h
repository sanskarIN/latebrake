#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Racing/WrongWayIndicator.h"
#include "Track/OffTrackAdvisor.h"
#include "Track/RivalProximityIndicator.h"
#include "Track/RivalRelativeMotion.h"
#include "UI/MenuDetailPager.h"
#include "LatebrakeRaceHUD.generated.h"

class APawn;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    void ScrollMenuDetails(int direction);

private:
    // Advisory only: no race awarding, control input or network telemetry.
    latebrake::WrongWayIndicator WrongWayHint;
    latebrake::OffTrackIndicator OffTrackHint;
    latebrake::RivalProximityIndicator RivalProximityHint;
    latebrake::RivalRelativeMotionTracker RivalRelativeMotionHint;
    latebrake::MenuDetailPager MenuPager;
    TWeakObjectPtr<APawn> LastWayfindingPawn;
    uint64 LastWayfindingSession = 0;
    uint64 LastWayfindingTeleport = 0;
    int32 LastRivalVerifiedRecoveries = 0;
};
