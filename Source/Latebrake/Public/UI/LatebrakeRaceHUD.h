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
#include "Garage/GarageBudgetAdvisor.h"
#include "UI/GaragePaletteNavigation.h"
#include "UI/FleetReplayPanel.h"
#include "UI/SectorMotionResultsPanel.h"
#include "UI/SettingsMenuPanel.h"
#include "UI/HudDriveCommandPanel.h"
#include "LatebrakeRaceHUD.generated.h"

class APawn;
class ALatebrakeRaceGameMode;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    void ScrollMenuDetails(int direction);
    bool HandleFleetReplayAction(latebrake::FleetReplayAction Action);
    bool TryHandleFleetReplayPointer(double X,double Y);
    bool TryHandleSectorMotionPointer(double X,double Y);
    bool TryHandleSettingsPointer(double X,double Y);
    bool ToggleGaragePalette(latebrake::GarageLookLayer Layer);
    bool AdvanceGaragePaletteLayer();
    bool HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput Input);
    void CloseGaragePalette();
    void ApplyGaragePaletteChoice(latebrake::DesktopMenuMouseAction Action);
    bool TryScrollMenuMouse(double x, double y, int direction);
    latebrake::DesktopMenuMouseAction HitTestMenuMouse(double x, double y) const;

private:
    void DrawDriveCommandPanel(const latebrake::HudDriveCommandPanel& Panel,
        const latebrake::DriveCommandReview& Review);
    void DrawSettingsPanel(const ALatebrakeRaceGameMode* Mode,
        latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
        const latebrake::HudMenuReadablePanel& Text,bool bTouch);
    latebrake::SettingsPanelPresentation PresentedSettings{};
    void DrawFleetReplayPanel(const ALatebrakeRaceGameMode* Mode,
        latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
        const latebrake::HudMenuReadablePanel& Text,bool bTouch);
    void DrawSectorMotionPanel(const ALatebrakeRaceGameMode* Mode,
        latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
        const latebrake::HudMenuReadablePanel& Text,bool bTouch);
    latebrake::SectorPanelViewer SectorViewer;
    latebrake::SectorPanelLayout PresentedSector{};
    std::uint64_t PresentedSectorRevision=0;
    latebrake::FleetReplayViewer ReplayViewer;
    latebrake::FleetReplayPanelLayout PresentedReplay{};
    std::uint64_t PresentedReplayAttempt=0;
    latebrake::FleetReplayMapReadout PresentedReplayMap{};
    double PresentedReplaySeconds=0.0;
    // Advisory only: no race awarding, control input or network telemetry.
    latebrake::WrongWayIndicator WrongWayHint;
    latebrake::StuckRecoveryHint StuckHint;
    latebrake::OffTrackIndicator OffTrackHint;
    latebrake::RivalProximityIndicator RivalProximityHint;
    latebrake::RivalRelativeMotionTracker RivalRelativeMotionHint;
    latebrake::MenuDetailPager MenuPager;
    latebrake::GarageBudgetAdvisor BudgetAdvisor;
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
