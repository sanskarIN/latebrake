#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Racing/RaceProgress.h"
#include "Racing/RaceEvents.h"
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
    latebrake::EventKind GetSelectedEvent() const { return SelectedEvent; }
    float GetEventBestSeconds() const;

    void RecoverPlayer(ALatebrakeCarPawn* Car);

    const latebrake::RaceProgress& GetRace() const { return Race; }
    float GetPersonalBestSeconds() const { return PersonalBestSeconds; }
    float GetPersonalBestLapSeconds() const { return PersonalBestLapSeconds; }
    int32 GetCompletedRaceCount() const { return CompletedRaceCount; }

    static FVector GetStartPosition();
    static FRotator GetStartRotation();

private:
    void BuildCircuit();
    AStaticMeshActor* SpawnBlock(const TCHAR* Label, const FVector& Position,
        const FVector& Scale, const FLinearColor& Color);
    void LoadRecords();
    void SaveRecords();
    void RefreshRival();

    latebrake::EventKind SelectedEvent = latebrake::EventKind::Circuit;
    latebrake::RaceProgress Race{4, 3};
    float PersonalBestSeconds = 0.f;
    float BestTimeAttackSeconds = 0.f;
    float PersonalBestLapSeconds = 0.f;
    int32 CompletedRaceCount = 0;

    UPROPERTY(Transient)
    TObjectPtr<APlayerStart> RuntimePlayerStart;

    UPROPERTY(Transient)
    TObjectPtr<ALatebrakeRivalCar> RivalCar;
};
