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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Progression")
    int32 CompletedRaces = 0;
};
