#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LatebrakePlayerController.generated.h"

/**
 * Local player control for prototype racing. The car pawn owns the actual
 * driving bindings; the controller only configures viewport input routing.
 * Not a networked multiplayer controller.
 */
UCLASS()
class LATEBRAKE_API ALatebrakePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ALatebrakePlayerController();

protected:
    virtual void BeginPlay() override;
};
