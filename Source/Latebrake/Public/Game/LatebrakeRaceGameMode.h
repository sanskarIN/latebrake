#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Racing/RaceProgress.h"
#include "Racing/RivalGateGap.h"
#include "Racing/RivalMatchedGateSplits.h"
#include "Racing/RivalFinishObservation.h"
#include "Racing/RivalFinishClockReview.h"
#include "AI/RivalDifficultyPreset.h"
#include "AI/GridRivalLedger.h"
#include "AI/RivalRecoveryOccupancy.h"
#include "Racing/QuickRaceModePicker.h"
#include "Racing/RaceSessionEpoch.h"
#include "Racing/RaceStartLights.h"
#include "Racing/LaunchSpeedMeter.h"
#include "Racing/BestLapGhost.h"
#include "Racing/GhostDisplayPreference.h"
#include "Racing/RestartPlacementGuard.h"
#include "Racing/AppFocusRaceGuard.h"
#include "Racing/ManualRacePauseGuard.h"
#include "Racing/RaceEvents.h"
#include "Garage/GarageState.h"
#include "Garage/GaragePurchaseConfirmation.h"
#include "Garage/GarageTuningKey.h"
#include "Career/CareerProgress.h"
#include "UI/MenuNavigation.h"
#include "LatebrakeRaceGameMode.generated.h"

class AController;
class APlayerStart;
class AStaticMeshActor;
class ALatebrakeCarPawn;
class ALatebrakeRivalCar;
class UPointLightComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class LATEBRAKE_API ALatebrakeRaceGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALatebrakeRaceGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    void RegisterCheckpoint(int32 CheckpointIndex);
    void RestartRace();
    void SelectRaceEvent(latebrake::EventKind Kind);
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
    void MenuNext();
    void MenuPrevious();
    void MenuActivate();
    bool IsMenuOpen() const { return Menu.IsOpen(); }
    latebrake::MenuPage GetMenuPage() const { return Menu.Current(); }
    FString GetMenuPageTitle() const { return FString(UTF8_TO_TCHAR(Menu.Title())); }
    bool UsesMilesPerHour() const { return bMilesPerHour; }
    bool IsCareerRace() const { return bCareerRace; }
    int32 GetActiveCareerEventId() const { return bCareerRace ? Career.SelectedEvent() : -1; }
    int32 GetMenuCareerEventId() const { return CareerMenuSelection; }
    const latebrake::CareerProgress& GetCareer() const { return Career; }
    const latebrake::CareerAward& GetLastCareerAward() const { return LastCareerAward; }
    FString GetCareerEventName() const;
    FString GetCareerSelectionFeedback() const { return CareerFeedback; }
    latebrake::EventKind GetSelectedEvent() const { return SelectedEvent; }
    float GetEventBestSeconds() const;
    float GetAttemptPreviousBestSeconds() const { return AttemptPreviousBestSeconds; }
    bool HasRival() const;
    bool TryGetRivalRadarPosition(FVector& OutPosition) const;
    bool TryGetRivalDraftingState(FVector& OutPosition,
        FVector& OutForward, FVector& OutVelocity) const;
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
    // Independent read-only ledger receives only verified physical rival
    // recovery outcomes. No clock, scoring or movement authority.
    void ObserveRivalRecoveryResult(const ALatebrakeRivalCar* Actor,
                                   bool bEnginePlacementVerified);
    bool IsPrimaryRivalActor(const ALatebrakeRivalCar* Actor) const
    {
        return IsValid(RivalCar) && RivalCar.Get() == Actor;
    }
    // The extra four experimental rivals NEVER replace the primary
    // opponent's official HUD, finish comparison or scoring.
    bool IsRegisteredRivalActor(const ALatebrakeRivalCar* Actor) const;
    void ObserveAdditionalRivalVerifiedGate(const ALatebrakeRivalCar* Actor);
    void ObserveAdditionalRivalRecoveryResult(
        const ALatebrakeRivalCar* Actor, bool bEnginePlacementVerified);
    bool TryCollectPhysicalTraffic(
        const ALatebrakeRivalCar* Subject,
        std::array<latebrake::RecoveryObstacle,
                   latebrake::MaxRecoveryObstacles>& Out,
        int32& OutCount) const;
    // Player recovery uses five distinct physical rival blockers rather
    // than the AI collector, which requires a subject AI actor.
    // Always initializes outputs and refuses incomplete/anonymous grids.
    bool TryCollectPlayerRecoveryTraffic(
        const ALatebrakeCarPawn* Player,
        std::array<latebrake::RecoveryObstacle,
                   latebrake::MaxRecoveryObstacles>& Out,
        int32& OutCount) const;
    bool IsExperimentalSixCarGridEnabled() const
    {
        // A command-line flag alone cannot modify solo, career or
        // partially-spawned opponent sessions.
        return bExperimentalSixCarGrid && !bCareerRace &&
            SelectedEvent == latebrake::EventKind::Circuit &&
            ExperimentalRivals.Num() == latebrake::StartGridCapacity - 2;
    }
    // GO is denied until ALL real extra actors are bound and their Chaos
    // chassis are ready; source-only geometry never qualifies a launch.
    bool IsExperimentalFleetReadyForStart() const;
    latebrake::GridRivalStatus GetGridRivalStatus(int32 Slot) const noexcept
    {
        return GridRivals.Snapshot(Slot);
    }
    bool TryGetLatestRivalMatchedGateSplit(
        latebrake::RivalMatchedGateSplit& OutSplit) const;

    void RecoverPlayer(ALatebrakeCarPawn* Car);

    const latebrake::RaceProgress& GetRace() const { return Race; }
    bool IsRaceStartGreen() const { return StartLights.IsGreen(); }
    const latebrake::LaunchSpeedMeter& GetLaunchSpeedMeter() const { return LaunchMeter; }
    // A run-local cosmetic marker only. Not a saved or official record.
    bool HasBestLapGhost() const { return GhostRecorder.HasBest(); }
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
    FString GetSelectedVehicleName() const;
    int32 GetEngineUpgradeLevel() const { return Garage.EngineLevel(); }
    int32 GetGripUpgradeLevel() const { return Garage.GripLevel(); }
    int32 GetNextLockedVehicleCost() const;
    FString GetGarageFeedback() const { return GarageFeedback; }
    bool HasProgressSaveError() const { return bProgressSaveFailed; }
    int32 GetNextEngineUpgradeCost() const { return Garage.NextUpgradeCost(latebrake::UpgradeKind::Engine); }
    int32 GetNextGripUpgradeCost() const { return Garage.NextUpgradeCost(latebrake::UpgradeKind::Grip); }
    void GarageNextVehicle();
    void GarageUnlockNext();
    void GarageUpgradeEngine();
    void GarageUpgradeGrip();
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
    void RefreshExperimentalRivals();
    void DestroyExperimentalRivals();
    void ApplyGarageToPlayer();
    void UpdateBestLapGhostMarker(bool bReplayAuthorized);
    void SuspendBestLapGhost();
    void CloseMenuForRace();
    void SyncDesktopMenuPointerMode();
#if !UE_BUILD_SHIPPING
    // Event-only, no per-frame telemetry, no effect on scoring or saves.
    void StartRaceAuditAttempt(int32 GridAtBegin);
    int32 RaceAuditAttempt = 0;
#endif

    latebrake::EventKind SelectedEvent = latebrake::EventKind::Circuit;
    latebrake::CareerProgress Career;
    latebrake::MenuNavigation Menu;
    bool bMilesPerHour = false;
    latebrake::CareerAward LastCareerAward;
    bool bCareerRace = false;
    bool bFutureSaveSchema = false;
    int32 CareerMenuSelection = 0;
    // Not persisted. Changes in Quick Race menu affect NEXT race only.
    latebrake::RivalDifficultyPreset QuickRaceDifficultySelection =
        latebrake::RivalDifficultyPreset::Standard;
    // Pending option only. Real event selection occurs on MenuActivate.
    bool bQuickRaceSoloTimeAttackSelection = false;
    latebrake::RivalDifficultyPreset ActiveRivalDifficulty =
        latebrake::RivalDifficultyPreset::Standard;
    int32 CareerStartedVehicleId = 0;
    FString CareerFeedback = TEXT("N select career event | M start career | 3/4 quick race");
    latebrake::RaceProgress Race{4, 3};
    // Explicit per-attempt start gate: no paid race clock or chassis force
    // until a genuine local player, map and safe grid are ready.
    latebrake::RaceStartLights StartLights;
    latebrake::RivalMatchedGateSplits RivalSplits;
    latebrake::GridRivalLedger GridRivals;
    // Opt-in only via explicit non-shipping -LatebrakeExperimentalSixCarGrid.
    // The default retail game intentionally keeps its one actual AI rival.
    bool bExperimentalSixCarGrid = false;
    // A failed collision-aware actor spawn, post-teleport verification or
    // invalid grid geometry is terminal for THIS accepted race attempt.
    // Never silently turn the requested six-car run into a two-car GO.
    bool bExperimentalFleetFaulted = false;
    latebrake::RivalFinishClockReview RivalFinishClocks;
    latebrake::RivalFinishClockComparison LastRivalFinishClockComparison;
    // Only set at a player's timing-eligible final gate. Not a race rank,
    // reward, finish time or saved career score.
    latebrake::RivalFinishObservation LastRivalFinishObservation;
    // Session-only advisory forward-speed estimate; never alters RaceProgress.
    latebrake::LaunchSpeedMeter LaunchMeter;
    // Fixed-size in-memory candidate/best traces never become SaveGame data.
    latebrake::BestLapGhost GhostRecorder;
    latebrake::GhostDisplayPreference GhostDisplay;
    TWeakObjectPtr<ALatebrakeCarPawn> GhostTrackedPawn;
    uint64 GhostTeleportRevision = 0;
    uint64 GhostSessionRevision = 0;
    bool bCircuitReady = false;
    // The original south-straight collision actor, not arbitrary
    // WorldStatic ground or another mesh, is the only permitted pre-GO
    // six-car support. Weak identity expires if the level actor disappears.
    TWeakObjectPtr<AStaticMeshActor> SouthStraightCollisionActor;
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
    latebrake::GaragePurchaseConfirmation GaragePurchaseConfirm;
    latebrake::GarageTuningKey LastAppliedTuning;
    TWeakObjectPtr<ALatebrakeCarPawn> LastTunedPawn;
    bool bProgressSaveFailed = false;
    FString GarageFeedback = TEXT("E select | Q/U/I twice to confirm spending");
    int32 LastRaceCredits = 0;
    int32 RecoveriesThisRace = 0;

    UPROPERTY(Transient)
    TObjectPtr<APlayerStart> RuntimePlayerStart;

    UPROPERTY(Transient)
    TObjectPtr<ALatebrakeRivalCar> RivalCar;

    // Up to four non-shipping additional physical AI actors, each assigned
    // a unique real grid slot. They can never own primary rival rewards.
    UPROPERTY(Transient)
    TArray<TObjectPtr<ALatebrakeRivalCar>> ExperimentalRivals;

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
