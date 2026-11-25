#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Racing/WrongWayIndicator.h"
#include "Racing/StuckRecoveryHint.h"
#include "Track/OffTrackAdvisor.h"
#include "Track/RivalProximityIndicator.h"
#include "Track/RivalRelativeMotion.h"
#include "UI/MenuDetailPager.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/GaragePurchaseMouseLayout.h"
#include "UI/GarageAppearancePalette.h"
#include "UI/GaragePaletteNavigation.h"
#include "LatebrakeRaceHUD.generated.h"

class APawn;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    void ScrollMenuDetails(int direction);
    bool ToggleGaragePalette(latebrake::GarageLookLayer Layer);
    bool AdvanceGaragePaletteLayer();
    bool HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput Input);
    void CloseGaragePalette();
    void ApplyGaragePaletteChoice(latebrake::DesktopMenuMouseAction Action);
    bool TryScrollMenuMouse(double x, double y, int direction);
    latebrake::DesktopMenuMouseAction HitTestMenuMouse(double x, double y) const;

private:
    // Advisory only: no race awarding, control input or network telemetry.
    latebrake::WrongWayIndicator WrongWayHint;
    latebrake::StuckRecoveryHint StuckHint;
    latebrake::OffTrackIndicator OffTrackHint;
    latebrake::RivalProximityIndicator RivalProximityHint;
    latebrake::RivalRelativeMotionTracker RivalRelativeMotionHint;
    latebrake::MenuDetailPager MenuPager;
    latebrake::GaragePresentedOffers PresentedGarageOffers{};
    latebrake::GaragePaletteSession GaragePalette{};
    latebrake::GaragePresentedPalette PresentedGaragePalette{};
    int32 GaragePaletteFocus = -1;
    int32 DrawnGaragePaletteFocus = -1;
    TWeakObjectPtr<APawn> LastWayfindingPawn;
    uint64 LastWayfindingSession = 0;
    uint64 LastWayfindingTeleport = 0;
    int32 LastRivalVerifiedRecoveries = 0;
};
