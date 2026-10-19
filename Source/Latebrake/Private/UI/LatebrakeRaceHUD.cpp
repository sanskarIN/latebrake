#include "UI/LatebrakeRaceHUD.h"

#include "UI/HudLayout.h"
#include "Track/CircuitMap.h"
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
    if (!GEngine || !PlayerOwner || !Canvas)
    {
        return;
    }

    const ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>();
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(PlayerOwner->GetPawn());
    if (!Mode || !Car)
    {
        return;
    }

    const latebrake::HudLayout Layout = latebrake::CalculateHudLayout(
        static_cast<double>(Canvas->SizeX), static_cast<double>(Canvas->SizeY));
    bool bCompact = Layout.Compact;
#if PLATFORM_ANDROID
    bCompact = true;
#endif

    const FLinearColor Primary(0.95f, 0.96f, 1.f, 1.f);
    const FLinearColor Accent(1.f, 0.73f, 0.17f, 1.f);
    const float Scale = static_cast<float>(Layout.Scale);
    const float Left = static_cast<float>(Layout.Left);
    const float Step = static_cast<float>(Layout.LineHeight);
    float Y = static_cast<float>(Layout.Top);

    const auto Line = [&](const FString& Content, const FLinearColor& Color, bool bHeading = false)
    {
        DrawText(Content, Color, Left, Y,
            bHeading ? GEngine->GetMediumFont() : GEngine->GetSmallFont(),
            Scale * (bHeading ? 1.08f : 1.0f));
        Y += Step;
    };

    const latebrake::RaceProgress& Race = Mode->GetRace();
    const bool bTimeAttack = Mode->GetSelectedEvent() == latebrake::EventKind::TimeAttack;
    const int32 DisplayLap = FMath::Min(Race.GetCompletedLaps() + 1, Race.GetRequiredLaps());
    const double Elapsed = FMath::Clamp(Race.GetElapsedSeconds(), 0.0, 3599999.0);
    const int32 Minutes = FMath::FloorToInt(Elapsed / 60.0);
    const double Seconds = Elapsed - static_cast<double>(Minutes) * 60.0;

    Line(TEXT("LATEBRAKE  /  RACE BEYOND LIMITS"), Accent, true);
    Line(bTimeAttack ? TEXT("TIME ATTACK") : TEXT("CIRCUIT RACE"), Accent);
    Line(FString::Printf(TEXT("SPEED  %.0f km/h"), Car->GetSpeedKmH()), Primary);
    Line(FString::Printf(TEXT("LAP  %d / %d  |  GATE  %d / %d"),
        DisplayLap, Race.GetRequiredLaps(), Race.GetNextCheckpoint() + 1, Race.GetCheckpointCount()), Primary);
    Line(FString::Printf(TEXT("TIME  %02d:%05.2f"), Minutes, Seconds), Accent, true);

    if (!bCompact)
    {
        Line(FString::Printf(TEXT("CURRENT LAP  %.2fs  |  LAST  %.2fs"),
            Race.GetCurrentLapSeconds(), Race.GetLastLapSeconds()), Primary);
        Line(FString::Printf(TEXT("FASTEST LAP  %.2fs  |  LAST SPLIT  %.2fs"),
            Race.GetBestLapSeconds(), Race.GetLastSectorSeconds()), Primary);
        if (Mode->GetPersonalBestLapSeconds() > 0.f)
        {
            Line(FString::Printf(TEXT("ALL-TIME FASTEST LAP  %.2fs"),
                Mode->GetPersonalBestLapSeconds()), Accent);
        }
        if (!bTimeAttack)
        {
            Line(Mode->HasRival()
                ? FString::Printf(TEXT("RIVAL ROUTE  %d LAPS | RECOVERIES  %d%s"),
                    Mode->GetRivalApproximateLaps(), Mode->GetRivalRecoveryCount(),
                    Mode->HasRivalFinishedRoute() ? TEXT(" | DONE") : TEXT(""))
                : FString(TEXT("RIVAL UNAVAILABLE")), Primary);
        }
    }

    // Prototype north-up circuit overview: the next checkpoint and live car
    // position share the same track geometry as the race and AI systems.
    // Hidden on compact viewports until a production adaptive UI is authored.
    if (!bCompact && Canvas->SizeX >= 1120)
    {
        const float MapSize = 150.f * Scale;
        const float MapLeft = Canvas->SizeX - MapSize - 22.f * Scale;
        const float MapTop = 72.f * Scale;
        const auto ToScreen = [&](latebrake::CircuitPoint Position)
        {
            const latebrake::CircuitMapPoint M = latebrake::NormalizeCircuitMap(Position);
            return FVector2D(MapLeft + static_cast<float>(M.U) * MapSize,
                             MapTop + static_cast<float>(1.0 - M.V) * MapSize);
        };

        DrawText(TEXT("CIRCUIT / NORTH UP"), Accent,
            MapLeft, MapTop - 30.f * Scale, GEngine->GetSmallFont(), 0.8f * Scale);
        for (int32 Index = 0; Index < latebrake::CircuitGateCount; ++Index)
        {
            const FVector2D From = ToScreen(latebrake::CircuitGate(Index));
            const FVector2D To = ToScreen(latebrake::CircuitGate(
                (Index + 1) % latebrake::CircuitGateCount));
            DrawLine(From.X, From.Y, To.X, To.Y,
                FLinearColor(0.75f, 0.78f, 0.85f), 2.f * Scale);
        }
        const FVector2D Next = ToScreen(latebrake::CircuitGate(Race.GetNextCheckpoint()));
        DrawRect(Accent, Next.X - 5.f * Scale, Next.Y - 5.f * Scale,
                 10.f * Scale, 10.f * Scale);
        const FVector CarPosition = Car->GetActorLocation();
        const FVector2D Player = ToScreen({CarPosition.X, CarPosition.Y});
        DrawRect(FLinearColor(0.08f, 0.80f, 0.95f),
                 Player.X - 4.f * Scale, Player.Y - 4.f * Scale,
                 8.f * Scale, 8.f * Scale);
    }

    if (Mode->GetEventBestSeconds() > 0.f)
    {
        Line(FString::Printf(TEXT("BEST EVENT  %.2fs"), Mode->GetEventBestSeconds()), Primary);
    }
    Line(Car->GetHandlingMode() == ELatebrakeHandlingMode::Arcade
        ? TEXT("HANDLING  ARCADE") : TEXT("HANDLING  SIM-INSPIRED"), Primary);
    Line(FString::Printf(TEXT("GARAGE  %s"), *Mode->GetSelectedVehicleName()), Accent);
    Line(FString::Printf(TEXT("ENGINE %d/3 (NEXT %d) | GRIP %d/3 (NEXT %d)"),
        Mode->GetEngineUpgradeLevel(), Mode->GetNextEngineUpgradeCost(),
        Mode->GetGripUpgradeLevel(), Mode->GetNextGripUpgradeCost()), Primary);
    if (!bCompact)
    {
        const int32 Cost = Mode->GetNextLockedVehicleCost();
        Line(Cost > 0
            ? FString::Printf(TEXT("NEXT CAR UNLOCK  %d CREDITS"), Cost)
            : FString(TEXT("ALL PROTOTYPE PROFILES UNLOCKED")), Primary);
    }
    Line(FString::Printf(TEXT("CREDITS  %d"), Mode->GetTotalCredits()), Accent);
    Line(Mode->GetGarageFeedback(), Primary);
    if (Mode->HasProgressSaveError())
    {
        Line(TEXT("WARNING: LOCAL PROGRESS COULD NOT BE SAVED"), Accent);
    }

    if (Race.IsFinished())
    {
        Line(FString::Printf(TEXT("EVENT REWARD  +%d CREDITS"),
            Mode->GetLastRaceCredits()), Accent);
    }
    if (UGameplayStatics::IsGamePaused(this))
    {
        Line(TEXT("PAUSED  -  ESC TO RESUME"), Accent, true);
    }
    else if (Race.IsFinished())
    {
        Line(TEXT("FINISHED! PRESS T TO RACE AGAIN"), Accent, true);
    }

    // Keep controls visible without overlapping the main telemetry panel.
    const float FooterY = FMath::Max(Y + 3.f, Canvas->SizeY - 28.f * Scale);
#if PLATFORM_ANDROID
    DrawText(TEXT("TOUCH: LEFT STEER  |  RIGHT TOP GAS  |  RIGHT BOTTOM BRAKE"),
        Accent, Left, FooterY, GEngine->GetSmallFont(), Scale);
#else
    DrawText(bCompact
        ? TEXT("W/S GAS  A/D STEER  SPACE BRAKE  T RESTART")
        : TEXT("W/S DRIVE  A/D STEER  SPACE BRAKE  R RECOVER  T RESTART  C CAMERA  1/2 HANDLING  3/4 EVENT  Q/E GARAGE  U/I TUNING"),
        Primary, Left, FooterY, GEngine->GetSmallFont(), Scale * (bCompact ? 0.90f : 0.78f));
#endif
}
