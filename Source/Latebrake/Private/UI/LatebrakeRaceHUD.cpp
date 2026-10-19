#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Vehicles/LatebrakeCarPawn.h"

void ALatebrakeRaceHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!GEngine || !PlayerOwner)
    {
        return;
    }

    const ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>();
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(PlayerOwner->GetPawn());
    if (!Mode || !Car)
    {
        return;
    }

    const latebrake::RaceProgress& Race = Mode->GetRace();
    const bool bTimeAttack = Mode->GetSelectedEvent() == latebrake::EventKind::TimeAttack;
    const FLinearColor Primary(0.95f, 0.96f, 1.f, 1.f);
    const FLinearColor Accent(1.f, 0.73f, 0.17f, 1.f);

    const int32 DisplayLap = FMath::Min(Race.GetCompletedLaps() + 1, Race.GetRequiredLaps());
    const double Elapsed = Race.GetElapsedSeconds();
    const int32 Minutes = FMath::FloorToInt(Elapsed / 60.0);
    const double Seconds = Elapsed - static_cast<double>(Minutes) * 60.0;

    DrawText(TEXT("LATEBRAKE  /  RACE BEYOND LIMITS"), Accent, 35.f, 30.f, GEngine->GetMediumFont(), 1.10f);
    DrawText(bTimeAttack ? TEXT("EVENT  TIME ATTACK") : TEXT("EVENT  CIRCUIT RACE"),
        Accent, 35.f, 56.f, GEngine->GetSmallFont());
    DrawText(FString::Printf(TEXT("SPEED  %03.0f km/h"), Car->GetSpeedKmH()), Primary, 35.f, 72.f, GEngine->GetMediumFont());
    DrawText(FString::Printf(TEXT("LAP  %d / %d"), DisplayLap, Race.GetRequiredLaps()),
        Primary, 35.f, 108.f, GEngine->GetMediumFont());
    DrawText(FString::Printf(TEXT("NEXT GATE  %d / %d"), Race.GetNextCheckpoint() + 1, Race.GetCheckpointCount()),
        Primary, 35.f, 142.f, GEngine->GetMediumFont());
    DrawText(FString::Printf(TEXT("TIME  %02d:%05.2f"), Minutes, Seconds),
        Accent, 35.f, 176.f, GEngine->GetMediumFont());
    DrawText(FString::Printf(TEXT("LAP TIME  %.2fs"), Race.GetCurrentLapSeconds()),
        Primary, 35.f, 210.f, GEngine->GetSmallFont());
    DrawText(FString::Printf(TEXT("LAST LAP  %.2fs    FASTEST THIS RACE  %.2fs"),
        Race.GetLastLapSeconds(), Race.GetBestLapSeconds()),
        Primary, 35.f, 232.f, GEngine->GetSmallFont());
    DrawText(FString::Printf(TEXT("LAST SPLIT  %.2fs"), Race.GetLastSectorSeconds()),
        Primary, 35.f, 254.f, GEngine->GetSmallFont());
    if (Mode->GetEventBestSeconds() > 0.f)
    {
        DrawText(FString::Printf(TEXT("BEST RACE  %.2fs"), Mode->GetEventBestSeconds()),
            Primary, 35.f, 275.f, GEngine->GetSmallFont());
    }
    if (Mode->GetPersonalBestLapSeconds() > 0.f)
    {
        DrawText(FString::Printf(TEXT("ALL-TIME BEST LAP  %.2fs"), Mode->GetPersonalBestLapSeconds()),
            Accent, 35.f, 292.f, GEngine->GetSmallFont());
    }

    DrawText(FString::Printf(TEXT("EARNED CREDITS  %d"), Mode->GetTotalCredits()),
        Accent, 35.f, 343.f, GEngine->GetSmallFont());
    if (Race.IsFinished())
    {
        DrawText(FString::Printf(TEXT("EVENT REWARD  +%d CREDITS"), Mode->GetLastRaceCredits()),
            Accent, 35.f, 388.f, GEngine->GetSmallFont());
    }

    DrawText(Car->GetHandlingMode() == ELatebrakeHandlingMode::Arcade ? TEXT("HANDLING  ARCADE") : TEXT("HANDLING  SIM-INSPIRED"),
        Primary, 35.f, 317.f, GEngine->GetSmallFont());

    if (UGameplayStatics::IsGamePaused(this))
    {
        DrawText(TEXT("PAUSED  -  ESC TO RESUME"), Accent, 35.f, 415.f, GEngine->GetLargeFont());
    }
    else if (Race.IsFinished())
    {
        DrawText(TEXT("FINISH!  PRESS T TO RACE AGAIN"), Accent, 35.f, 415.f, GEngine->GetLargeFont(), 1.15f);
    }

#if PLATFORM_ANDROID
    DrawText(TEXT("LEFT HALF: STEER   RIGHT UPPER: GAS   RIGHT LOWER: BRAKE"),
        Accent, 35.f, Canvas->SizeY - 80.f, GEngine->GetSmallFont(), 0.82f);
#else
    DrawText(TEXT("W/S drive  A/D steer  SPACE brake  R recover  T restart  C camera  1/2 handling  ESC pause  3/4 event"),
        Primary, 35.f, Canvas->SizeY - 45.f, GEngine->GetSmallFont(), 0.82f);
#endif
}
