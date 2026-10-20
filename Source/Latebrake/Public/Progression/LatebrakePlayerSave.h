#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LatebrakePlayerSave.generated.h"

/** Minimal, offline-only persisted career statistics for the starter circuit. */
UCLASS()
class LATEBRAKE_API ULatebrakePlayerSave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    float BestRaceSeconds = 0.f;

    // Added in v0.1.1: older SaveGame slots default to zero.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    float BestLapSeconds = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    float BestTimeAttackSeconds = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    int32 CompletedRaces = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    int32 TotalCredits = 0;

    // Added in v0.3.0. Legacy SaveGame objects deserialize as zero and
    // GarageState::Load repairs the missing starter-vehicle ownership bit.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Garage")
    int32 OwnedVehicleMask = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Garage")
    int32 SelectedVehicleId = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Garage")
    int32 EngineUpgradeLevel = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Garage")
    int32 GripUpgradeLevel = 0;

    // Version 0/absent = legacy v0.4.x. Schema 2 stores fixed event-ID medals.
    // Unknown newer schema values are read-only until supported by migration.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Career")
    int32 SaveSchemaVersion = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Career")
    int32 ActiveCareerEventId = -1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Career")
    TArray<uint8> CareerMedals;

};
