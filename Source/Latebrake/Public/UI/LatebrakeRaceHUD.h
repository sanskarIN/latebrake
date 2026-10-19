#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LatebrakeRaceHUD.generated.h"

UCLASS()
class LATEBRAKE_API ALatebrakeRaceHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
