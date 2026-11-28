#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Racing/RaceProgress.h"
#include "Racing/PlayerSectorMotionReview.h"
#include "Racing/PlayerLapMotionJournal.h"
#include "UI/HudTelemetryPriority.h"
#include "Racing/PlayerRecoveryClearance.h"
#include "Track/TrackLimitsSession.h"
#include "Racing/RivalGateGap.h"
#include "Racing/RivalMatchedGateSplits.h"
#include "Racing/RivalFinishObservation.h"
#include "Racing/RivalFinishClockReview.h"
#include "AI/RivalDifficultyPreset.h"
#include "AI/ExperimentalFleetAttempt.h"
#include "AI/ExperimentalFleetLiveRecovery.h"
#include "AI/FleetRaceReview.h"
#include "Racing/FleetSlipstreamDraft.h"
#include "Racing/FleetReplayCapture.h"
#include "AI/FleetGridPresentation.h"
#include "AI/FleetStartupWatchdog.h"
#include "AI/FleetFrameAuthority.h"
#include "Racing/QuickRaceModePicker.h"
#include "Racing/PracticeSessionHistory.h"
#include "Racing/PracticeTrialSetup.h"
#include "Racing/PracticeSetupCatalogue.h"
#include "Racing/PracticeExperimentPlan.h"
#include "Racing/PracticeSetupAnalysisCache.h"
#include "Racing/PracticeLaunchPreview.h"
#include "Racing/PracticeGateReference.h"
#include "Career/CareerMenuSelection.h"
#include "Racing/RaceSessionEpoch.h"
#include "Racing/RaceStartLights.h"
#include "Racing/LaunchSpeedMeter.h"
#include "Racing/BestLapGhost.h"
#include "Racing/GhostSetupGuard.h"
#include "Racing/GhostDisplayPreference.h"
#include "Racing/RestartPlacementGuard.h"
#include "Racing/AppFocusRaceGuard.h"
#include "Racing/ManualRacePauseGuard.h"
#include "Racing/RaceEvents.h"
#include "Garage/GarageState.h"
#include "Garage/GarageAppearance.h"
#include "Garage/GarageDirectAppearance.h"
#include "Garage/GarageGalleryCursor.h"
#include "Garage/GarageEquipUiPolicy.h"
#include "Garage/GaragePurchaseConfirmation.h"
#include "Garage/GaragePurchasePanel.h"
#include "Garage/GarageTuningKey.h"
#include "Career/CareerProgress.h"
#include "UI/MenuNavigation.h"
#include "UI/MainMenuDashboard.h"
#include "UI/SettingsMenuSelection.h"
#include "UI/QuickRaceChoicePanel.h"
#include "LatebrakeRaceGameMode.generated.h"

class AController;
class APlayerStart;
class ADirectionalLight;
class ASkyLight;
class AStaticMeshActor;
class ALatebrakeCarPawn;
class ALatebrakeRivalCar;
class ALatebrakeCheckpoint;
class UPointLightComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UStaticMesh;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALatebrakeRaceGameMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    void RegisterCheckpoint(int32 CheckpointIndex);
    void RestartRace();
    void SelectRaceEvent(latebrake::EventKind Kind);
    bool StartQuickPractice(latebrake::QuickPracticePreset Preset);
    latebrake::QuickRaceChoiceSnapshot GetQuickRaceChoiceSnapshot() const;
    bool SelectQuickRaceChoice(int32 Index);
    bool HandlePracticeTrialInput(latebrake::PracticeTrialAction Action,int32 Vehicle=-1);
    latebrake::PracticeExperimentPlan GetPendingPracticeExperimentPlan() const;
    latebrake::PracticeLaunchPreview GetPendingPracticeLaunchPreview() const;
    latebrake::PracticeExperimentChoices GetPendingPracticeExperimentChoices() const
    {return latebrake::GetPracticeExperimentChoices(GetPendingPracticeExperimentPlan());}
    latebrake::PracticeTrialReadout GetPendingPracticeTrial() const noexcept
    {return PracticeTrial.Pending(Garage.Snapshot());}
    latebrake::PracticeTrialReadout GetActivePracticeTrial() const noexcept
    {return PracticeTrial.Active(ActivePractice,Garage.Snapshot());}
    bool IsPracticeRace() const { return latebrake::IsQuickPractice(ActivePractice); }
    latebrake::QuickPracticePreset GetActivePractice() const { return ActivePractice; }
    latebrake::QuickPracticePreset GetQuickPracticeSelection() const { return QuickPracticeSelection; }
    latebrake::PracticeComparison GetPracticeComparison() const noexcept
    {return PracticeHistory.LastComparison();}
    latebrake::PracticeSetupCatalogue GetPracticeSetupCatalogue(
        latebrake::PracticeConfiguration Key) const noexcept
    {return PracticeAnalysis.ReadCatalogue(PracticeHistory,Key);}
    latebrake::PracticeAttemptSeriesReview GetPracticeAttemptSeries(
        latebrake::PracticeConfiguration Key) const noexcept
    {return PracticeHistory.ReadSeries(Key);}
    latebrake::PracticeGateReview GetPracticeGateReview() const noexcept
    {return PracticeGateHistory.Read();}
    void NotifyPracticeHandlingChange(const ALatebrakeCarPawn* Car);
    bool StartCareerEvent(int32 Id);
    void StartNextAvailableCareerEvent();
    void CycleCareerEvent();
    // Quick Race pending tier only. Career keeps the original Standard AI.
    latebrake::RivalDifficultyPreset GetQuickRaceDifficultySelection() const
    {
        return QuickRaceDifficultySelection;
    }
    bool GetQuickRaceSoloTimeAttackSelection() const
    {
        return bQuickRaceSoloTimeAttackSelection;
    }
    latebrake::RivalDifficultyPreset GetActiveRivalDifficulty() const
    {
        return ActiveRivalDifficulty;
    }
    void ToggleMenu();
    void ToggleRacePause();
    void PauseForAppInterruption();
    bool IsFocusLossGuardActive() const { return AppFocusGuard.IsSuspended(); }
    bool IsManualRacePauseActive() const { return ManualPause.IsSuspended(); }
    latebrake::MainMenuDashboardSnapshot GetMainMenuDashboardSnapshot() const;
    bool NavigateMainMenuDashboard(latebrake::MenuPage Destination,
        const latebrake::MainMenuDashboardSnapshot& Expected);
    void MenuNext();
    void MenuPrevious();
    void MenuActivate();
    void ToggleCircuitLightingPreview();
    bool IsDuskLightingPreview() const noexcept { return bDuskLightingPreview; }
    bool IsMenuOpen() const { return Menu.IsOpen(); }
    latebrake::MenuPage GetMenuPage() const { return Menu.Current(); }
    FString GetMenuPageTitle() const { return FString(UTF8_TO_TCHAR(Menu.Title())); }
    bool UsesMilesPerHour() const { return bMilesPerHour; }
    latebrake::HudTelemetryMode GetHudTelemetryMode() const noexcept { return HudTelemetryMode; }
    void CycleHudTelemetryMode();
    latebrake::CameraMotionMode GetCameraMotionMode() const noexcept {return CameraMotion;}
    latebrake::SettingsMenuSnapshot GetSettingsMenuSnapshot() const;
    void ActivateSettingsControl(latebrake::SettingsControl Control);
    bool IsCareerRace() const { return bCareerRace; }
    int32 GetActiveCareerEventId() const { return bCareerRace ? Career.SelectedEvent() : -1; }
    int32 GetMenuCareerEventId() const
    {
        if (!latebrake::IsCareerMenuPage(Menu.Current())) return CareerMenuSelection;
        return latebrake::ResolveCareerMenuTarget(Career, Menu.Current(), CareerMenuSelection).EventId;
    }
    const latebrake::CareerProgress& GetCareer() const { return Career; }
    const latebrake::CareerAward& GetLastCareerAward() const { return LastCareerAward; }
    FString GetCareerEventName() const;
    FString GetCareerSelectionFeedback() const { return CareerFeedback; }
    latebrake::EventKind GetSelectedEvent() const { return SelectedEvent; }
    float GetEventBestSeconds() const;
    float GetAttemptPreviousBestSeconds() const { return AttemptPreviousBestSeconds; }
    bool HasRival() const;
    bool IsExperimentalFleetRequested() const noexcept;
    bool IsExperimentalFleetDriveReady() const;
    bool IsFleetStartPresentationReady() const;
    latebrake::FleetStartupReadout GetFleetStartupReadout() const noexcept
    {return FleetStartup.Readout();}
    bool TryGetExperimentalGridSettlingSlot(const ALatebrakeCarPawn* Actor,int32& OutSlot) const;
    bool IsRegisteredRaceRival(const ALatebrakeRivalCar* Actor) const;
    latebrake::FleetAttemptReadout GetFleetAttemptReadout() const noexcept
    { return FleetAttempt.Readout(); }
    int32 GetPhysicalFleetCount() const;
    latebrake::FleetRaceReview GetFleetRaceReview() const;
    void ObserveFleetReplayCar(const ALatebrakeCarPawn* Car,latebrake::PhysicalReplayCarSample Sample);
    latebrake::FleetReplayReview GetFleetReplayReview() const;
    latebrake::PhysicalReplayPlaybackFrame EvaluateFleetReplay(double Seconds) const;
    void ObserveFleetRivalGate(const ALatebrakeRivalCar* Actor);
    void ObserveFleetRivalRecovery(const ALatebrakeRivalCar* Actor,bool Verified);
    void ObserveFleetRivalDecision(const ALatebrakeRivalCar* Actor,latebrake::FleetDecisionSample Sample);
    bool TryGetFleetRivalProgress(int32 Slot,int32& Laps,int32& NextGate,bool& Finished) const;
    bool TryGetFreshFleetTraffic(const ALatebrakeRivalCar* Actor,
        bool BrakeQualified,double BrakeScale,latebrake::RivalFleetTrafficSample& Out) const;
    latebrake::FleetRivalRecovery GetFleetRivalRecovery(const ALatebrakeRivalCar* Actor,int32 Gate) const;
    bool TryGetSkidRoadSurface(const UPrimitiveComponent* HitComponent,
        int32& OutBlockIndex) const;
    bool TryGetRivalRadarPosition(FVector& OutPosition) const;
    // Reject interposed track collision in the real world before handing
    // a rival state to the optional player-only drafting policy. Observer
    // is the actual following pawn, not an inferred camera or HUD actor.
    bool TryGetRivalDraftingState(const ALatebrakeCarPawn* Observer,
        FVector& OutPosition, FVector& OutForward, FVector& OutVelocity) const;
    bool TryGetFleetDraftFrame(const ALatebrakeCarPawn* Observer,
        const latebrake::FleetDraftFollower& Follower,latebrake::FleetDraftFrame& Out) const;
    int32 GetRivalApproximateLaps() const;
    int32 GetRivalPlannedLaps() const;
    int32 GetRivalRecoveryCount() const;
    int32 GetRivalSuccessfulRecoveryCount() const;
    int32 GetRivalFailedRecoveryCount() const;
    bool HasRivalFinishedRoute() const;
    latebrake::RivalFinishObservation GetLastRivalFinishObservation() const
    {
        return LastRivalFinishObservation;
    }
    // Qualified Quick Race timing observation only. Not official placement.
    latebrake::RivalFinishClockComparison GetLastRivalFinishClockComparison() const
    {
        return LastRivalFinishClockComparison;
    }
    // Exact difference in *credited forward gates*, not race position,
    // finish ranking, physical gap, collision risk or saved career state.
    bool TryGetRivalCreditedGateGap(latebrake::RivalGateGap& OutGap) const;
    // Called only after the physical rival's existing directional gate pass.
    void ObserveRivalVerifiedGate();
    bool TryGetLatestRivalMatchedGateSplit(
        latebrake::RivalMatchedGateSplit& OutSplit) const;

    void RecoverPlayer(ALatebrakeCarPawn* Car);
    latebrake::PlayerRecoveryNotice GetRecoveryNotice() const noexcept
    {
        return RecoveryNotice.At(Race.GetElapsedSeconds());
    }

    const latebrake::RaceProgress& GetRace() const { return Race; }
    latebrake::SectorMotionReview GetPlayerSectorMotionReview() const noexcept;
    latebrake::SectorMotionHistory GetPlayerSectorMotionHistory() const noexcept;
    latebrake::PlayerLapMotionReview GetPlayerLapMotionReview() const noexcept;
    const latebrake::TrackLimitsSession& GetTrackLimits() const { return TrackLimits; }
    latebrake::TrackLimitsReview GetTrackLimitsReview() const noexcept
    {
        return TrackLimits.Review(Race,RecoveriesThisRace);
    }
    bool IsRaceStartGreen() const { return StartLights.IsGreen() && IsExperimentalFleetDriveReady(); }
    const latebrake::LaunchSpeedMeter& GetLaunchSpeedMeter() const { return LaunchMeter; }
    // A run-local cosmetic marker only. Not a saved or official record.
    bool HasBestLapGhost() const { return GhostSetup.IsBound() && GhostRecorder.HasBest(); }
    bool IsBestLapGhostDisplayEnabled() const { return GhostDisplay.IsEnabled(); }
    void ToggleBestLapGhostView();
    double GetBestLapGhostSeconds() const { return GhostRecorder.GetBestSeconds(); }
    // Optional world-space distance to the visible sampled best-lap beacon.
    // Never determines lead/lag, official timing, classification or rewards.
    bool TryGetBestLapGhostSeparationMeters(double& OutMeters) const;
    latebrake::RaceStartPhase GetRaceStartPhase() const
    {
        return StartLights.Phase();
    }
    bool IsCircuitReadyForGameplay() const { return bCircuitReady; }
    bool HasLocalPlayerDriver() const;
    bool IsRestartPlacementReady() const;
    uint64 GetCheckpointSessionRevision() const { return CheckpointSession.Current(); }
    float GetPersonalBestSeconds() const { return PersonalBestSeconds; }
    float GetPersonalBestLapSeconds() const { return PersonalBestLapSeconds; }
    int32 GetCompletedRaceCount() const { return CompletedRaceCount; }
    int32 GetTotalCredits() const { return Garage.Credits(); }
    // Read-only copy for the offline catalogue. No ownership or save mutation.
    latebrake::GarageSnapshot GetGarageSnapshot() const { return Garage.Snapshot(); }
    int32 GetSelectedVehicleId() const { return Garage.SelectedVehicle(); }
    int32 GetGaragePreviewVehicleId() const { return GarageGallery.Viewed(); }
    // Read-only style for the SHOWROOM car (may be locked), never the
    // authoritative equipped-car physics or ownership selection.
    latebrake::GarageLook GetGaragePreviewLook() const noexcept
    {
        return GarageLooks.Get(GarageGallery.Viewed());
    }
    bool CanBrowseGarage() const;
    latebrake::GarageEquipUiState GetGarageEquipUiState() const;
    bool CanEquipGarageViewed() const;
    void GarageBrowseNext();
    void GarageBrowsePrevious();
    void GarageBrowseVehicle(int32 VehicleId);
    void GarageSelectViewedVehicle();
    latebrake::GarageLook GetSelectedGarageLook() const noexcept
    {
        return GarageLooks.Get(Garage.SelectedVehicle());
    }
    bool CanCustomizeGarage() const;
    bool BeginGarageAppearancePreview();
    FString GetSelectedVehicleName() const;
    int32 GetEngineUpgradeLevel() const { return Garage.EngineLevel(); }
    int32 GetGripUpgradeLevel() const { return Garage.GripLevel(); }
    int32 GetNextLockedVehicleCost() const;
    FString GetGarageFeedback() const { return GarageFeedback; }
    // Read-only live countdown or expiration feedback for the paused Garage UI.
    FString GetGaragePurchaseDisplayFeedback() const;
    latebrake::GaragePurchasePanelView GetGaragePurchasePanel() const;
    bool HasProgressSaveError() const { return bProgressSaveFailed; }
    bool IsProfileReadOnly() const { return bFutureSaveSchema; }
    int32 GetNextEngineUpgradeCost() const { return Garage.NextUpgradeCost(latebrake::UpgradeKind::Engine); }
    int32 GetNextGripUpgradeCost() const { return Garage.NextUpgradeCost(latebrake::UpgradeKind::Grip); }
    void GarageNextVehicle();
    // Explicit six-slot equip is menu-only and never buys a car.
    void GarageSelectVehicle(int32 VehicleId);
    void GarageUnlockNext();
    void GarageUpgradeEngine();
    void GarageUpgradeGrip();
    void GarageCyclePaint();
    void GarageCycleStripe();
    void GarageCycleWheels();
    void GarageResetAppearance();
    void GarageChooseAppearance(latebrake::GarageDirectAppearanceRequest Request);
    int32 GetLastRaceCredits() const { return LastRaceCredits; }

    static FVector GetStartPosition();
    static FRotator GetStartRotation();

private:
    void BuildCircuit();
    void BuildBrakingReferenceBoards(UStaticMesh* Mesh);
    void BuildCornerApproachChevrons(UStaticMesh* Mesh);
    void BuildStartGantry();
    void UpdateStartGantryVisual();
    AStaticMeshActor* SpawnBlock(const TCHAR* Label, const FVector& Position,
        const FVector& Scale, const FLinearColor& Color);
    void LoadRecords();
    void SaveRecords();
    void RefreshRival();
    void BeginExperimentalFleetAttempt();
    void DestroyExtraFleet();
    void RegisterFleetTickDependent(AActor* Actor);
    void ClearFleetTickPrerequisites();
    bool HasCurrentFleetFrameAuthority() const noexcept;
    void SpawnExtraFleet();
    bool ObserveExperimentalFleet(float DeltaSeconds,bool Inactive);
    void ObserveFleetPlayerTraffic(double LiveDelta,bool bClockAdvanced);
    void BeginFleetReplayFrame(double LiveDelta,bool bClockAdvanced);
    void BeginFleetDecisionFrame(double LiveDelta,bool bClockAdvanced);
    bool CollectFleetFrame(latebrake::FleetPhysicalFrame& Out,
        float DeltaSeconds,bool IncludeGridProbes) const;
    ALatebrakeRivalCar* FleetRivalAt(int32 Slot) const;
    latebrake::FleetRaceReview CollectFleetRaceReview() const;
    void ApplyGarageToPlayer();
    void CommitGarageVehicleSelection(int32 VehicleId);
    bool RejectGarageMutationIfBlocked();
    latebrake::GarageEquipUiState GetGarageMutationUiState() const;
    void SyncGarageShowroomPreview();
    void DismissGaragePalette();
    void GarageCycleAppearance(latebrake::GarageLookLayer layer,
                              bool bReset = false,
                              const latebrake::GarageDirectAppearanceRequest* Direct = nullptr);
    void UpdateBestLapGhostMarker(bool bReplayAuthorized);
    void SuspendBestLapGhost();
    void RefreshGhostConfiguration(const ALatebrakeCarPawn* Car);
    void CloseMenuForRace();
    void SyncDesktopMenuPointerMode();
    void ApplyCircuitLightingPreview();
#if !UE_BUILD_SHIPPING
    // Event-only, no per-frame telemetry, no effect on scoring or saves.
    void StartRaceAuditAttempt(int32 GridAtBegin);
    void LogPlayerSectorMotionAudit(int32 Gate,int32 AcceptedOrdinal) const;
    int32 RaceAuditAttempt = 0;
#endif

    latebrake::EventKind SelectedEvent = latebrake::EventKind::Circuit;
    latebrake::CareerProgress Career;
    latebrake::MenuNavigation Menu;
    latebrake::SettingsMenuSelection SettingsSelection;
    std::uint64_t QuickChoiceRevision=1;
    void BumpQuickChoiceRevision() noexcept {if(++QuickChoiceRevision==0)++QuickChoiceRevision;}
    bool bMilesPerHour = false;
    // Session-only display preference, never saved with progression.
    latebrake::HudTelemetryMode HudTelemetryMode=latebrake::HudTelemetryMode::Balanced;
    latebrake::CameraMotionMode CameraMotion=latebrake::CameraMotionMode::Dynamic;
    // Rendering-only session toggle. Never written to ULatebrakePlayerSave.
    bool bDuskLightingPreview = false;
    latebrake::CareerAward LastCareerAward;
    bool bCareerRace = false;
    bool bFutureSaveSchema = false;
    int32 CareerMenuSelection = 0;
    // Not persisted. Changes in Quick Race menu affect NEXT race only.
    latebrake::RivalDifficultyPreset QuickRaceDifficultySelection =
        latebrake::RivalDifficultyPreset::Standard;
    // Pending option only. Real event selection occurs on MenuActivate.
    bool bQuickRaceSoloTimeAttackSelection = false;
    latebrake::QuickPracticePreset QuickPracticeSelection = latebrake::QuickPracticePreset::None;
    latebrake::QuickPracticePreset ActivePractice = latebrake::QuickPracticePreset::None;
    latebrake::PracticeTrialSetup PracticeTrial;
    latebrake::PracticeTrialAccess GetPracticeTrialAccess() const;
    latebrake::PracticeSessionHistory PracticeHistory;
    mutable latebrake::PracticeSetupAnalysisCache PracticeAnalysis;
    latebrake::PracticeGateReferenceHistory PracticeGateHistory;
    latebrake::PracticeConfiguration ReadPracticeConfiguration(const ALatebrakeCarPawn* Car) const;
    latebrake::RivalDifficultyPreset ActiveRivalDifficulty =
        latebrake::RivalDifficultyPreset::Standard;
    int32 CareerStartedVehicleId = 0;
    FString CareerFeedback = TEXT("N select career event | M start career | 3/4 quick race");
    void ResetPlayerSectorMotion() noexcept;
    void ObservePlayerSectorMotion(double LiveDelta,bool bClockAdvanced);
    void ObservePlayerSectorGate(int32 Gate,int32 AcceptedOrdinal) noexcept;
    latebrake::RaceProgress Race{4, 3};
    latebrake::PlayerSectorMotionJournal PlayerSectorMotion;
    latebrake::PlayerLapMotionJournal PlayerLapMotion;
    TWeakObjectPtr<ALatebrakeCarPawn> SectorMotionPawn;
    bool bSectorMotionPawnBound=false;
    latebrake::TrackLimitsSession TrackLimits;
    TWeakObjectPtr<ALatebrakeCarPawn> StewardTrackedPawn;
    uint64 StewardTeleportRevision=0;
    uint64 StewardCheckpointSessionRevision=0;
    // Explicit per-attempt start gate: no paid race clock or chassis force
    // until a genuine local player, map and safe grid are ready.
    latebrake::RaceStartLights StartLights;
    latebrake::RivalMatchedGateSplits RivalSplits;
    latebrake::RivalFinishClockReview RivalFinishClocks;
    latebrake::RivalFinishClockComparison LastRivalFinishClockComparison;
    // Only set at a player's timing-eligible final gate. Not a race rank,
    // reward, finish time or saved career score.
    latebrake::RivalFinishObservation LastRivalFinishObservation;
    // Session-only advisory forward-speed estimate; never alters RaceProgress.
    latebrake::LaunchSpeedMeter LaunchMeter;
    // Fixed-size in-memory candidate/best traces never become SaveGame data.
    latebrake::BestLapGhost GhostRecorder;
    latebrake::GhostSetupGuard GhostSetup;
    latebrake::GhostDisplayPreference GhostDisplay;
    TWeakObjectPtr<ALatebrakeCarPawn> GhostTrackedPawn;
    uint64 GhostTeleportRevision = 0;
    uint64 GhostSessionRevision = 0;
    bool bCircuitReady = false;
    // Every menu pause/resume invalidates pre-transition gate positions.
    latebrake::RaceSessionEpoch CheckpointSession;
    latebrake::RestartPlacementGuard RestartPlacement;
    // UE weak pointer verifies the actual actor instance and object generation.
    // Explicit restart authorization must not transfer to a new possessed car.
    TWeakObjectPtr<ALatebrakeCarPawn> VerifiedRestartPawn;
    latebrake::AppFocusRaceGuard AppFocusGuard;
    latebrake::ManualRacePauseGuard ManualPause;
    float PersonalBestSeconds = 0.f;
    float BestTimeAttackSeconds = 0.f;
    float PersonalBestLapSeconds = 0.f;
    // Snapshot of the stored quick-race record before this attempt.
    float AttemptPreviousBestSeconds = 0.f;
    int32 CompletedRaceCount = 0;
    latebrake::GarageState Garage;
    latebrake::GarageAppearanceState GarageLooks;
    latebrake::GarageGalleryCursor GarageGallery;
    latebrake::GaragePurchaseConfirmation GaragePurchaseConfirm;
    latebrake::GarageTuningKey LastAppliedTuning;
    TWeakObjectPtr<ALatebrakeCarPawn> LastTunedPawn;
    bool bProgressSaveFailed = false;
    FString GarageFeedback = TEXT("E select | Q/U/I twice to confirm spending");
    int32 LastRaceCredits = 0;
    int32 RecoveriesThisRace = 0;
    latebrake::PlayerRecoveryNoticeState RecoveryNotice;

    // Exact live original-asphalt components, indexed by collision plan.
    TArray<TWeakObjectPtr<UStaticMeshComponent>> SkidRoadComponents;

    // The two existing source-spawned track lights are retained for
    // Settings preview; no additional light actors on each toggle.
    UPROPERTY(Transient)
    TObjectPtr<ADirectionalLight> RuntimeSun;
    UPROPERTY(Transient)
    TObjectPtr<ASkyLight> RuntimeSky;

    UPROPERTY(Transient)
    TObjectPtr<APlayerStart> RuntimePlayerStart;

    UPROPERTY(Transient)
    TObjectPtr<ALatebrakeRivalCar> RivalCar;
    UPROPERTY(Transient)
    TArray<TObjectPtr<ALatebrakeRivalCar>> ExtraFleet;
    latebrake::ExperimentalFleetAttempt FleetAttempt;
    latebrake::FleetStartupWatchdog FleetStartup;
    latebrake::FleetFrameAuthority FleetFrameAuthority;
    TArray<TWeakObjectPtr<ALatebrakeCheckpoint>> FleetCheckpointActors;
    TArray<TWeakObjectPtr<AActor>> FleetTickDependents;
    latebrake::GridRivalLedger FleetReviewLedger;
    latebrake::FleetGateTimingJournal FleetGateTiming;
    latebrake::FleetPlayerTrafficJournal FleetPlayerTraffic;
    latebrake::FleetReplayCapture FleetReplay;
    latebrake::FleetDecisionJournal FleetDecisions;
    latebrake::FleetRaceReview FleetFinishReview;
    std::array<TWeakObjectPtr<ALatebrakeCarPawn>,latebrake::StartGridCapacity> FleetBoundActors{};
    bool bFleetActorsBound=false;
    uint64 FleetAttemptToken=0;
    uint64 FleetFrameToken=0;
    uint64 FleetObservedEpoch=0;
    bool bExperimentalFleetFlag=false;

    // Decorative asset-free fixture. Components belong to its Actor and
    // never participate in gate overlap or physics. No replicated state.
    UPROPERTY(Transient)
    TObjectPtr<AActor> StartGantryActor;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPointLightComponent>> StartGantryLamps;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> StartGantryBulbs;

    // Avoid eight redundant render-component writes per regular game tick.
    int32 LastStartGantryLampMask = -1;

    // One non-colliding prototype beacon follows a valid best-lap trace.
    UPROPERTY(Transient)
    TObjectPtr<AStaticMeshActor> BestLapGhostMarker;
};
