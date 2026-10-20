#include "Game/LatebrakePlayerController.h"

ALatebrakePlayerController::ALatebrakePlayerController()
{
    // The car pawn implements the full-screen, multi-finger input controls;
    // these events are supported by APlayerController in Unreal Engine 5.8.
#if PLATFORM_ANDROID
    bEnableTouchEvents = true;
    bEnableTouchOverEvents = false;
    bShowMouseCursor = false;
#endif
}

void ALatebrakePlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

#if PLATFORM_ANDROID
    // Unreal's stock virtual thumbsticks would overlap the game-drawn
    // touch regions. All buttons are rendered and hit-tested by Latebrake.
    ActivateTouchInterface(nullptr);
    SetInputMode(FInputModeGameOnly());
    bEnableTouchEvents = true;
#endif
}
