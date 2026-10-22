#include "UI/LatebrakeRaceHUD.h"

#include "UI/HudLayout.h"
#include "Racing/FinishedRaceInsights.h"
#include "UI/TouchControlLayout.h"
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

    if (Mode->IsMenuOpen())
    {
        // This is a functional asset-free development UI, NOT a polished
        // production UMG menu. Navigation stays paused and cannot steer.
        const float UIScale = static_cast<float>(Layout.Scale);
        const float X = static_cast<float>(Layout.Left);
        float Y = static_cast<float>(Layout.Top);
        const FLinearColor White(0.96f, 0.97f, 1.f);
        const FLinearColor Gold(1.f, 0.73f, 0.17f);
        const auto MenuLine = [&](const FString& Value, bool bAccent = false)
        {
            DrawText(Value, bAccent ? Gold : White, X, Y,
                GEngine->GetSmallFont(), UIScale);
            Y += 28.f * UIScale;
        };
        MenuLine(FString::Printf(TEXT("LATEBRAKE / %s"), *Mode->GetMenuPageTitle()), true);
        MenuLine(TEXT("TAB CLOSE | PAGE DOWN/UP NAVIGATE | ENTER SELECT"));
        switch (Mode->GetMenuPage())
        {
        case latebrake::MenuPage::Main:
            MenuLine(TEXT("Offline racing. Select to open Quick Race."));
            break;
        case latebrake::MenuPage::QuickRace:
            MenuLine(TEXT("Select to start the original 3-lap circuit."));
            break;
        case latebrake::MenuPage::Career:
            MenuLine(FString::Printf(TEXT("Event: %s"), *Mode->GetCareerEventName()));
            MenuLine(FString::Printf(TEXT("Completed: %d / %d | N browse events"),
                Mode->GetCareer().CompletedCount(), latebrake::CareerEventCount));
            MenuLine(TEXT("Select to start; medals unlock following rounds."));
            break;
        case latebrake::MenuPage::Championships:
            MenuLine(FString::Printf(TEXT("Rookie Cup: %d/30 pts | Prologue Cup: %d/30 pts"),
                Mode->GetCareer().ChampionshipPoints(0),
                Mode->GetCareer().ChampionshipPoints(1)));
            MenuLine(FString::Printf(TEXT("Selected: %s"), *Mode->GetCareerEventName()));
            MenuLine(TEXT("BROWSE/N selects an eligible round; select starts."));
            break;
        case latebrake::MenuPage::Challenges:
            MenuLine(TEXT("Clock, clean, precision, consistency, Kestrel, endurance."));
            MenuLine(FString::Printf(TEXT("Selected: %s"), *Mode->GetCareerEventName()));
            MenuLine(TEXT("BROWSE/N selects an eligible challenge; select starts."));
            break;
        case latebrake::MenuPage::Garage:
            MenuLine(FString::Printf(TEXT("%s | Credits: %d"),
                *Mode->GetSelectedVehicleName(), Mode->GetTotalCredits()));
            MenuLine(TEXT("Select next owned vehicle; Q/U/I unlock and tune."));
            MenuLine(Mode->GetGarageFeedback());
            break;
        case latebrake::MenuPage::Results:
        {
            MenuLine(FString::Printf(TEXT("Last event reward: +%d credits"),
                Mode->GetLastRaceCredits()));
            const latebrake::FinishedRaceInsights Insights =
                latebrake::BuildFinishedRaceInsights(Mode->GetRace());
            if (Insights.Valid)
            {
                MenuLine(FString::Printf(TEXT("Best timed lap: %.2fs"),
                    Insights.BestLapSeconds));
                if (Insights.HasTheoreticalGap)
                {
                    MenuLine(FString::Printf(
                        TEXT("Gap to best-sectors lap (this run): %.2fs"),
                        Insights.BestToTheoreticalGapSeconds));
                }
                if (Insights.HasLapSpread && !bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Timed lap spread: %.2fs (%d laps)"),
                        Insights.LapSpreadSeconds, Insights.TimedLaps));
                }
                if (Insights.HasLatestVsPrevious && !bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Lap %d vs lap %d: %+.2fs (positive = slower)"),
                        Insights.LatestTimedLapNumber,
                        Insights.PreviousTimedLapNumber,
                        Insights.LatestVsPreviousDeltaSeconds));
                }
            }
            MenuLine(TEXT("Select to retry the current event."));
            break;
        }
        case latebrake::MenuPage::Pause:
            MenuLine(TEXT("Select to resume the paused race."));
            break;
        case latebrake::MenuPage::Settings:
            MenuLine(Mode->UsesMilesPerHour()
                ? TEXT("Speed display: MPH") : TEXT("Speed display: KM/H"));
            MenuLine(TEXT("Select to switch speed units and save."));
            break;
        case latebrake::MenuPage::Profile:
            MenuLine(FString::Printf(TEXT("Completed races: %d | Career medals: %d"),
                Mode->GetCompletedRaceCount(), Mode->GetCareer().CompletedCount()));
            MenuLine(FString::Printf(TEXT("Banked credits: %d"),
                Mode->GetTotalCredits()));
            break;
        default: break;
        }
        if (Mode->HasProgressSaveError())
        {
            MenuLine(TEXT("WARNING: PROGRESS NOT SAVED"), true);
        }
#if PLATFORM_ANDROID
        const auto Pads = latebrake::CalculateTouchControlLayout(
            Canvas->SizeX, Canvas->SizeY);
        if (Pads.Valid)
        {
            const auto DrawMenuPad = [&](const latebrake::TouchRect& Rect,
                                         const TCHAR* Label)
            {
                DrawRect(FLinearColor(0.15f, 0.23f, 0.32f, 0.65f),
                    static_cast<float>(Rect.X), static_cast<float>(Rect.Y),
                    static_cast<float>(Rect.Width), static_cast<float>(Rect.Height));
                DrawText(FString(Label), Gold, static_cast<float>(Rect.X + 5.0),
                    static_cast<float>(Rect.Y + 4.0),
                    GEngine->GetSmallFont(), UIScale);
            };
            DrawMenuPad(Pads.CareerNext, TEXT("NEXT"));
            DrawMenuPad(Pads.CareerStart, TEXT("SELECT"));
            DrawMenuPad(Pads.MenuBrowse, TEXT("BROWSE"));
            DrawMenuPad(Pads.Pause, TEXT("CLOSE"));
        }
#endif
        return;
    }

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

    Line(bCompact ? TEXT("LATEBRAKE") : TEXT("LATEBRAKE  /  RACE BEYOND LIMITS"),
        Accent, true);
    Line(Mode->IsCareerRace()
        ? (bCompact ? FString(TEXT("CAREER RACE"))
                    : FString::Printf(TEXT("CAREER  %s"), *Mode->GetCareerEventName()))
        : (bTimeAttack ? FString(TEXT("TIME ATTACK"))
                       : FString(TEXT("CIRCUIT RACE"))), Accent);
    if (Mode->IsCareerRace() && !bCompact)
    {
        const int32 Id = Mode->GetActiveCareerEventId();
        const latebrake::CareerEvent* Event = latebrake::GetCareerEvent(Id);
        if (Event && Event->Series >= 0)
        {
            Line(FString::Printf(TEXT("SERIES %d  |  POINTS %d  |  ROUND %d/3"),
                Event->Series + 1, Mode->GetCareer().ChampionshipPoints(Event->Series),
                Event->Round + 1), Primary);
        }
    }
    Line(Mode->UsesMilesPerHour()
        ? FString::Printf(TEXT("SPEED  %.0f mph"), Car->GetSpeedKmH() * 0.621371f)
        : FString::Printf(TEXT("SPEED  %.0f km/h"), Car->GetSpeedKmH()), Primary);
    Line(FString::Printf(TEXT("LAP  %d / %d  |  GATE  %d / %d"),
        DisplayLap, Race.GetRequiredLaps(), Race.GetNextCheckpoint() + 1, Race.GetCheckpointCount()), Primary);
    Line(FString::Printf(TEXT("TIME  %02d:%05.2f"), Minutes, Seconds), Accent, true);

    if (!bCompact)
    {
        Line(FString::Printf(TEXT("CURRENT LAP  %.2fs  |  LAST  %.2fs"),
            Race.GetCurrentLapSeconds(), Race.GetLastLapSeconds()), Primary);
        Line(FString::Printf(TEXT("FASTEST LAP  %.2fs  |  LAST SPLIT  %.2fs"),
            Race.GetBestLapSeconds(), Race.GetLastSectorSeconds()), Primary);
        if (Race.GetLastSectorIndex() >= 0)
        {
            // Compare a split only with the previous best of its own sector.
            // Negative values mean faster. No prior record = no delta claim.
            if (Race.HasLastSectorComparison())
            {
                Line(FString::Printf(TEXT("SECTOR %d  %.2fs  |  DELTA %+.2fs"),
                    Race.GetLastSectorIndex() + 1,
                    Race.GetLastSectorSeconds(),
                    Race.GetLastSectorDeltaSeconds()), Primary);
            }
            else
            {
                Line(FString::Printf(TEXT("SECTOR %d  %.2fs  |  FIRST SPLIT"),
                    Race.GetLastSectorIndex() + 1,
                    Race.GetLastSectorSeconds()), Primary);
            }
        }
        const double IdealLap = Race.GetTheoreticalBestLapSeconds();
        if (IdealLap > 0.0)
        {
            // This is a mathematical sum from the CURRENT run, not an
            // achieved personal best or a persisted global leaderboard time.
            Line(FString::Printf(TEXT("THEORETICAL LAP (THIS RUN)  %.2fs"),
                IdealLap), Accent);
        }
        if (Mode->GetPersonalBestLapSeconds() > 0.f)
        {
            Line(FString::Printf(TEXT("ALL-TIME FASTEST LAP  %.2fs"),
                Mode->GetPersonalBestLapSeconds()), Accent);
        }
        if (!bTimeAttack)
        {
            Line(Mode->HasRival()
                ? FString::Printf(TEXT("RIVAL ROUTE  %d/%d LAPS | RECOVERIES  %d%s"),
                    Mode->GetRivalApproximateLaps(), Mode->GetRivalPlannedLaps(),
                    Mode->GetRivalRecoveryCount(),
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

    if (bCompact)
    {
        // Reserve the lower third for large phone touch targets instead of
        // letting garage details crowd into the steering and pedal pads.
        Line(FString::Printf(TEXT("%s  |  %s"),
            Car->GetHandlingMode() == ELatebrakeHandlingMode::Arcade
                ? TEXT("ARCADE") : TEXT("SIM-INSPIRED"),
            *Mode->GetSelectedVehicleName()), Primary);
        Line(FString::Printf(TEXT("CREDITS  %d"), Mode->GetTotalCredits()), Accent);
    }
    else
    {
        if (Mode->GetEventBestSeconds() > 0.f)
        {
            Line(FString::Printf(TEXT("BEST EVENT  %.2fs"),
                Mode->GetEventBestSeconds()), Primary);
        }
        Line(Car->GetHandlingMode() == ELatebrakeHandlingMode::Arcade
            ? TEXT("HANDLING  ARCADE") : TEXT("HANDLING  SIM-INSPIRED"), Primary);
        Line(FString::Printf(TEXT("GARAGE  %s"), *Mode->GetSelectedVehicleName()), Accent);
        Line(FString::Printf(TEXT("ENGINE %d/3 (NEXT %d) | GRIP %d/3 (NEXT %d)"),
            Mode->GetEngineUpgradeLevel(), Mode->GetNextEngineUpgradeCost(),
            Mode->GetGripUpgradeLevel(), Mode->GetNextGripUpgradeCost()), Primary);
        const int32 Cost = Mode->GetNextLockedVehicleCost();
        Line(Cost > 0
            ? FString::Printf(TEXT("NEXT CAR UNLOCK  %d CREDITS"), Cost)
            : FString(TEXT("ALL PROTOTYPE PROFILES UNLOCKED")), Primary);
        Line(FString::Printf(TEXT("CREDITS  %d"), Mode->GetTotalCredits()), Accent);
        Line(Mode->GetGarageFeedback(), Primary);
    }
    if (Mode->HasProgressSaveError())
    {
        Line(TEXT("WARNING: LOCAL PROGRESS COULD NOT BE SAVED"), Accent);
    }

    if (Race.IsFinished())
    {
        // A finished result is only displayed when its clock was valid and
        // recordable. "Gap to ideal" is within-run sector potential, not
        // an achieved all-time lap or a fabricated best.
        const latebrake::FinishedRaceInsights Insights =
            latebrake::BuildFinishedRaceInsights(Race);
        if (Insights.Valid && !bCompact)
        {
            if (Insights.HasTheoreticalGap)
            {
                Line(FString::Printf(
                    TEXT("BEST LAP %.2fs  |  GAP TO IDEAL %.2fs"),
                    Insights.BestLapSeconds,
                    Insights.BestToTheoreticalGapSeconds), Accent);
            }
            if (Insights.HasLapSpread)
            {
                Line(FString::Printf(
                    TEXT("TIMED LAP SPREAD  %.2fs  (%d LAPS)"),
                    Insights.LapSpreadSeconds, Insights.TimedLaps), Primary);
            }
            if (Insights.HasLatestVsPrevious)
            {
                Line(FString::Printf(
                    TEXT("FINAL LAP vs PREVIOUS  %+.2fs (+ SLOWER)"),
                    Insights.LatestVsPreviousDeltaSeconds), Primary);
            }
        }
        Line(FString::Printf(TEXT("EVENT REWARD  +%d CREDITS"),
            Mode->GetLastRaceCredits()), Accent);
        if (Mode->IsCareerRace())
        {
            const auto Award = Mode->GetLastCareerAward();
            Line(FString::Printf(TEXT("CAREER MEDAL %d/3  |  SERIES BONUS +%d"),
                static_cast<int32>(Award.Medal), Award.ChampionshipBonus), Accent);
        }
    }
    else if (!bCompact)
    {
        Line(Mode->GetCareerSelectionFeedback(), Primary);
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
    (void)FooterY;
    // Draw exactly the same pixel rectangles that the touch dispatcher
    // hit-tests. Highlighted pads acknowledge actual held touch state.
    const latebrake::TouchControlLayout Pads =
        latebrake::CalculateTouchControlLayout(Canvas->SizeX, Canvas->SizeY);
    if (Pads.Valid)
    {
        const float LabelScale = FMath::Max(0.9f, Scale);
        const auto DrawPad = [&](const latebrake::TouchRect& Rect,
                                 const TCHAR* Label, bool bPressed)
        {
            DrawRect(bPressed
                ? FLinearColor(0.97f, 0.68f, 0.16f, 0.65f)
                : FLinearColor(0.11f, 0.18f, 0.25f, 0.52f),
                static_cast<float>(Rect.X), static_cast<float>(Rect.Y),
                static_cast<float>(Rect.Width), static_cast<float>(Rect.Height));
            DrawText(FString(Label), Primary,
                static_cast<float>(Rect.X + Rect.Width * 0.18),
                static_cast<float>(Rect.Y + Rect.Height * 0.35),
                GEngine->GetSmallFont(), LabelScale);
        };
        DrawPad(Pads.Steering, TEXT("STEER"), Car->IsTouchSteeringActive());
        DrawPad(Pads.Throttle, TEXT("GAS"), Car->IsTouchThrottleActive());
        DrawPad(Pads.Brake, TEXT("BRAKE"), Car->IsTouchBrakeActive());
        DrawPad(Pads.Pause, TEXT("PAUSE"), UGameplayStatics::IsGamePaused(this));
        DrawPad(Pads.Restart, TEXT("RESTART"), false);
        DrawPad(Pads.Recover, TEXT("RECOVER"), false);
        DrawPad(Pads.CareerNext, TEXT("NEXT"), false);
        DrawPad(Pads.CareerStart, TEXT("CAREER"), false);
    }
#else
    DrawText(bCompact
        ? TEXT("W/S GAS  A/D STEER  SPACE BRAKE  T RESTART")
        : TEXT("W/S DRIVE  A/D STEER  SPACE BRAKE  R RECOVER  T RESTART  C CAMERA  1/2 HANDLING  3/4 QUICK  N/M CAREER  Q/E GARAGE  U/I TUNING"),
        Primary, Left, FooterY, GEngine->GetSmallFont(), Scale * (bCompact ? 0.90f : 0.78f));
#endif
}
