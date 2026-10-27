#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Racing/RaceProgress.h"
#include "Racing/RaceSessionEpoch.h"
#include "Racing/RaceStartLights.h"
#include "Racing/LaunchSpeedMeter.h"
#include "Racing/BestLapGhost.h"
#include "Racing/RestartPlacementGuard.h"
#include "Racing/AppFocusRaceGuard.h"
#include "Racing/ManualRacePauseGuard.h"
#include "Racing/RaceEvents.h"
#include "Garage/GarageState.h"
#include "Garage/GarageTuningKey.h"
#include "Career/CareerProgress.h"
#include "UI/MenuNavigation.h"
#include "LatebrakeRaceGameMode.generated.h"

class AController;
class APlayerStart;
class AStaticMeshActor;
class ALatebrakeCarPawn;
class ALatebrakeRivalCar;

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
    bool HasRival() const;
    bool TryGetRivalRadarPosition(FVector& OutPosition) const;
    int32 GetRivalApproximateLaps() const;
    int32 GetRivalPlannedLaps() const;
    int32 GetRivalRecoveryCount() const;
    int32 GetRivalSuccessfulRecoveryCount() const;
    int32 GetRivalFailedRecoveryCount() const;
    bool HasRivalFinishedRoute() const;

    void RecoverPlayer(ALatebrakeCarPawn* Car);

    const latebrake::RaceProgress& GetRace() const { return Race; }
    bool IsRaceStartGreen() const { return StartLights.IsGreen(); }
    const latebrake::LaunchSpeedMeter& GetLaunchSpeedMeter() const { return LaunchMeter; }
    // A run-local cosmetic marker only. Not a saved or official record.
    bool HasBestLapGhost() const { return GhostRecorder.HasBest(); }
    double GetBestLapGhostSeconds() const { return GhostRecorder.GetBestSeconds(); }
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
    AStaticMeshActor* SpawnBlock(const TCHAR* Label, const FVector& Position,
        const FVector& Scale, const FLinearColor& Color);
    void LoadRecords();
    void SaveRecords();
    void RefreshRival();
    void ApplyGarageToPlayer();
    void UpdateBestLapGhostMarker(bool bReplayAuthorized);
    void SuspendBestLapGhost();
    void CloseMenuForRace();

    latebrake::EventKind SelectedEvent = latebrake::EventKind::Circuit;
    latebrake::CareerProgress Career;
    latebrake::MenuNavigation Menu;
    bool bMilesPerHour = false;
    latebrake::CareerAward LastCareerAward;
    bool bCareerRace = false;
    bool bFutureSaveSchema = false;
    int32 CareerMenuSelection = 0;
    int32 CareerStartedVehicleId = 0;
    FString CareerFeedback = TEXT("N select career event | M start career | 3/4 quick race");
    latebrake::RaceProgress Race{4, 3};
    // Explicit per-attempt start gate: no paid race clock or chassis force
    // until a genuine local player, map and safe grid are ready.
    latebrake::RaceStartLights StartLights;
    // Session-only advisory forward-speed estimate; never alters RaceProgress.
    latebrake::LaunchSpeedMeter LaunchMeter;
    // Fixed-size in-memory candidate/best traces never become SaveGame data.
    latebrake::BestLapGhost GhostRecorder;
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
    int32 CompletedRaceCount = 0;
    latebrake::GarageState Garage;
    latebrake::GarageTuningKey LastAppliedTuning;
    TWeakObjectPtr<ALatebrakeCarPawn> LastTunedPawn;
    bool bProgressSaveFailed = false;
    FString GarageFeedback = TEXT("Q unlock | E select | U engine | I grip");
    int32 LastRaceCredits = 0;
    int32 RecoveriesThisRace = 0;

    UPROPERTY(Transient)
    TObjectPtr<APlayerStart> RuntimePlayerStart;

    UPROPERTY(Transient)
    TObjectPtr<ALatebrakeRivalCar> RivalCar;

    // One non-colliding prototype beacon follows a valid best-lap trace.
    UPROPERTY(Transient)
    TObjectPtr<AStaticMeshActor> BestLapGhostMarker;
};
