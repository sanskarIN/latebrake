#include "UI/LatebrakeRaceHUD.h"

#include "UI/HudLayout.h"
#include "UI/HudVerticalPlan.h"
#include "UI/HudTextBounds.h"
#include "UI/HudMenuVerticalPlan.h"
#include "Racing/FinishedRaceInsights.h"
#include "Racing/RecentLapReview.h"
#include "Racing/RecentLapPaceStatistics.h"
#include "Racing/BestSectorReview.h"
#include "Racing/SampledDrivingReview.h"
#include "Racing/LiveRacePaceProjection.h"
#include "Racing/CurrentLapPotential.h"
#include "Racing/LiveSectorTarget.h"
#include "UI/TouchControlLayout.h"
#include "Track/CircuitMap.h"
#include "Track/RaceMiniMap.h"
#include "Track/CircuitRouteAdvisor.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/PlayerController.h"
#include "Vehicles/LatebrakeCarPawn.h"

void ALatebrakeRaceHUD::ScrollMenuDetails(int direction)
{
    // Called exclusively from a newly admitted keyboard/touch action while
    // the paused menu owns focus. No game-state authority is mutated here.
    if (direction > 0)
    {
        MenuPager.Next();
    }
    else if (direction < 0)
    {
        MenuPager.Previous();
    }
}

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
        WrongWayHint.Reset();
        OffTrackHint.Reset();
        RivalProximityHint.Reset();
        RivalRelativeMotionHint.Reset();
        LastWayfindingPawn.Reset();
        MenuPager.Reset();
        return;
    }

    const latebrake::HudLayout Layout = latebrake::CalculateHudLayout(
        static_cast<double>(Canvas->SizeX), static_cast<double>(Canvas->SizeY));
    bool bCompact = Layout.Compact;
    bool bTouchHUD = false;
#if PLATFORM_ANDROID
    bCompact = true;
    bTouchHUD = true;
#endif
    const latebrake::HudVerticalPlan VerticalPlan =
        latebrake::CalculateHudVerticalPlan(
            static_cast<double>(Canvas->SizeX),
            static_cast<double>(Canvas->SizeY), Layout, bTouchHUD);

    // On Android, reserve the entire right-hand touch-action column.
    // Width is measured with the *actual* Unreal font and DrawText scale,
    // not a guessed fixed character count. Binary-search only the first
    // visible glyph units when a message exceeds the protected lane.
    const latebrake::HudTextBounds HorizontalBounds =
        latebrake::CalculateHudTextBounds(
            static_cast<double>(Canvas->SizeX),
            static_cast<double>(Canvas->SizeY), Layout, bTouchHUD);
    const auto FitHudText = [&](const FString& Value, UFont* Font,
                                float TextScale) -> FString
    {
        if (!HorizontalBounds.Valid || !Font || Value.IsEmpty() ||
            !FMath::IsFinite(TextScale) || TextScale <= 0.f)
        {
            return {};
        }
        const auto Measure = [&](const FString& Text) -> double
        {
            float MeasuredWidth = 0.f;
            float MeasuredHeight = 0.f;
            GetTextSize(Text, MeasuredWidth, MeasuredHeight, Font, TextScale);
            return static_cast<double>(MeasuredWidth);
        };
        const FString Suffix(TEXT("..."));
        const int32 Prefix = latebrake::ChooseHudTextPrefix(
            Value.Len(), HorizontalBounds.MaxWidthPixels,
            Measure(Value), Measure(Suffix),
            [&](int32 Units) -> double
            {
                return Measure(Value.Left(Units) + Suffix);
            });
        if (Prefix < 0)
        {
            return {};
        }
        if (Prefix == Value.Len())
        {
            return Value;
        }
        int32 SafePrefix = Prefix;
        // Avoid cutting UTF-16 surrogate pairs on mobile builds. The
        // existing HUD language is ASCII, but user-facing car names may
        // eventually contain supplementary Unicode characters.
        if (SafePrefix > 0 && SafePrefix < Value.Len())
        {
            const uint32 Previous = static_cast<uint32>(Value[SafePrefix - 1]);
            const uint32 Following = static_cast<uint32>(Value[SafePrefix]);
            if (Previous >= 0xD800u && Previous <= 0xDBFFu &&
                Following >= 0xDC00u && Following <= 0xDFFFu)
            {
                --SafePrefix;
            }
        }
        return Value.Left(SafePrefix) + Suffix;
    };

    const latebrake::RaceProgress& Race = Mode->GetRace();

    if (Mode->IsMenuOpen())
    {
        // This is a functional asset-free development UI, NOT a polished
        // production UMG menu. Navigation stays paused and cannot steer.
        const float UIScale = static_cast<float>(Layout.Scale);
        const float X = static_cast<float>(Layout.Left);
        float Y = static_cast<float>(Layout.Top);
        const latebrake::HudMenuVerticalPlan MenuPlan =
            latebrake::CalculateHudMenuVerticalPlan(
                static_cast<double>(Canvas->SizeX),
                static_cast<double>(Canvas->SizeY), Layout, bTouchHUD);
        int32 DrawnMenuRows = 0;
        bool bMenuOverflow = false;
        const FLinearColor White(0.96f, 0.97f, 1.f);
        const FLinearColor Gold(1.f, 0.73f, 0.17f);

        // Build the logical page first. The former unbounded MenuLine
        // simply discarded overflow, including essential Results details.
        // Keep the title/nav pinned and paginate ALL generated details.
        TArray<TPair<FString, bool>> MenuDetails;
        const auto MenuLine = [&](const FString& Value, bool bAccent = false)
        {
            const FString Fitted = FitHudText(
                Value, GEngine->GetSmallFont(), UIScale);
            if (!Fitted.IsEmpty())
            {
                MenuDetails.Emplace(Fitted, bAccent);
            }
        };
        if (MenuPlan.Valid)
        {
            const auto DrawPinned = [&](const FString& Text, bool bAccent)
            {
                const FString Fitted = FitHudText(
                    Text, GEngine->GetSmallFont(), UIScale);
                if (!Fitted.IsEmpty() && DrawnMenuRows < MenuPlan.MaxDetailsRows)
                {
                    DrawText(Fitted, bAccent ? Gold : White, X, Y,
                        GEngine->GetSmallFont(), UIScale);
                    ++DrawnMenuRows;
                    Y += 28.f * UIScale;
                }
            };
            DrawPinned(FString::Printf(TEXT("LATEBRAKE / %s"),
                *Mode->GetMenuPageTitle()), true);
            DrawPinned(bTouchHUD
                ? TEXT("UP/DOWN DETAILS | NEXT MENU | SELECT")
                : TEXT("TAB CLOSE | PAGE UP/DOWN MENU | UP/DOWN DETAILS | ENTER"),
                false);
        }
        if (!Mode->IsCircuitReadyForGameplay())
        {
            MenuLine(TEXT("TRACK INCOMPLETE - RACE CLOCK AND SCORING DISABLED"), true);
        }
        else if (!Mode->IsRestartPlacementReady())
        {
            MenuLine(TEXT("RESTART GRID NOT VERIFIED - RACE DISABLED"), true);
        }
        else if (!Mode->HasLocalPlayerDriver())
        {
            MenuLine(TEXT("PLAYER CAR NOT READY - RACE CLOCK FROZEN"), true);
        }
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
            if (!latebrake::CanAwardFinishedRace(Mode->GetRace()))
            {
                MenuLine(TEXT("FINISH NOT RECORDABLE - missing active timing or invalid clock"), true);
            }
            else
            {
                MenuLine(FString::Printf(TEXT("Last event reward: +%d credits"),
                    Mode->GetLastRaceCredits()));
            }
            if (Mode->GetLaunchSpeedMeter().HasSample())
            {
                MenuLine(FString::Printf(
                    TEXT("0-5 km/h launch estimate (session): %.2fs"),
                    Mode->GetLaunchSpeedMeter().SecondsToFiveKmh()));
            }
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
            if (Race.GetTimePenaltyCount() > 0)
            {
                MenuLine(FString::Printf(TEXT("Time penalties included: %d (+%.2fs)"),
                    Race.GetTimePenaltyCount(), Race.GetTotalPenaltySeconds()));
                if (Race.GetLastLapPenaltySeconds() > 0.0)
                {
                    MenuLine(FString::Printf(TEXT("Final lap penalties included: +%.2fs"),
                        Race.GetLastLapPenaltySeconds()));
                }
            }
            // Review the actual run's latest eight timed laps. Results
            // pagination keeps each row reachable even on small Android
            // viewports, without changing records or reward authority.
            const latebrake::RecentLapReview LapReview =
                latebrake::BuildRecentLapReview(Race);
            if (LapReview.Valid)
            {
                MenuLine(TEXT("RECENT TIMED LAPS (NEWEST FIRST)"), true);
                for (int32 Offset = 0; Offset < LapReview.Count; ++Offset)
                {
                    const auto& Row = LapReview.Rows[Offset];
                    const FString BestLabel = Row.EqualsRunBest
                        ? FString(TEXT("RUN BEST"))
                        : FString::Printf(TEXT("+%.2fs TO BEST"),
                            Row.SecondsBehindRunBest);
                    MenuLine(FString::Printf(
                        TEXT("LAP %d: %.2fs | %s"),
                        Row.CompletedLap, Row.LapSeconds, *BestLabel));
                    if (!bCompact && Row.HasPrevious)
                    {
                        MenuLine(FString::Printf(
                            TEXT("  LAP %d vs prior timed: %+.2fs"),
                            Row.CompletedLap, Row.DeltaToPreviousSeconds));
                    }
                }
            }
            // Run-local pace distribution. The most recent at most eight
            // genuine timed laps are analyzed; earlier laps may have been
            // evicted and can never be reconstructed from this buffer.
            const latebrake::RecentLapPaceStatistics PaceStatistics =
                latebrake::CalculateRecentLapPaceStatistics(Race);
            if (PaceStatistics.Valid)
            {
                MenuLine(FString::Printf(
                    TEXT("LAST %d TIMED LAPS: %d-%d"),
                    PaceStatistics.SampleCount,
                    PaceStatistics.OldestLap,
                    PaceStatistics.NewestLap), true);
                MenuLine(FString::Printf(
                    TEXT("Recent avg %.2fs | median %.2fs"),
                    PaceStatistics.AverageSeconds,
                    PaceStatistics.MedianSeconds));
                MenuLine(FString::Printf(
                    TEXT("Latest vs recent avg: %+.2fs"),
                    PaceStatistics.NewestVsAverageSeconds));
                if (!bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Recent range %.2f-%.2fs | std dev %.2fs"),
                        PaceStatistics.FastestSampleSeconds,
                        PaceStatistics.SlowestSampleSeconds,
                        PaceStatistics.PopulationDeviationSeconds));
                }
                if (PaceStatistics.ConsecutiveImprovements > 0)
                {
                    MenuLine(FString::Printf(
                        TEXT("Faster in last %d consecutive lap comparisons"),
                        PaceStatistics.ConsecutiveImprovements));
                }
            }

            // Best splits come from measured ordered gates and may belong to
            // DIFFERENT laps. Theoretical sum is not an achieved record.
            const latebrake::BestSectorReview SectorReview =
                latebrake::BuildBestSectorReview(Race);
            if (SectorReview.Valid)
            {
                MenuLine(FString::Printf(
                    TEXT("MIXED BEST SECTORS: %.2fs (NOT A TIMED LAP)"),
                    SectorReview.MixedBestSectorsSeconds), true);
                for (int32 Index = 0; Index < SectorReview.Count; ++Index)
                {
                    MenuLine(FString::Printf(
                        TEXT("GATE %d best split %.2fs"),
                        Index + 1, SectorReview.SectorSeconds[Index]));
                }
            }

            // Chassis telemetry is sampled independently from the race
            // clock. Never label speed-integrated distance official.
            if (latebrake::CanAwardFinishedRace(Race))
            {
                const latebrake::SampledDrivingReview DriveReview =
                    latebrake::BuildSampledDrivingReview(
                        Car->GetDrivingStats());
                if (DriveReview.Valid)
                {
                    MenuLine(TEXT("SAMPLED CAR TELEMETRY (ESTIMATED)"), true);
                    MenuLine(FString::Printf(
                        TEXT("%.2fkm speed-est | peak %.0f km/h"),
                        DriveReview.EstimatedDistanceKm,
                        DriveReview.PeakSpeedKmH));
                    MenuLine(FString::Printf(
                        TEXT("Brake %.0f%% sampled | %d applications"),
                        DriveReview.BrakingPercent,
                        DriveReview.BrakeApplications));
                    if (!bCompact)
                    {
                        MenuLine(FString::Printf(
                            TEXT("High lateral slip %.0f%% of sampled time"),
                            DriveReview.HighLateralSlipPercent));
                    }
                }
            }
            // Observational chassis direction mismatch, not tire slip
            // physics, race awards or a saved drifting leaderboard.
            if (latebrake::CanAwardFinishedRace(Race) &&
                Car->GetChassisSlipAngleMeter().GetAcceptedSamples() > 0)
            {
                const latebrake::ChassisSlipAngleMeter& Slip =
                    Car->GetChassisSlipAngleMeter();
                MenuLine(TEXT("CHASSIS SLIP ANGLE (EST, SESSION)"), true);
                MenuLine(FString::Printf(
                    TEXT("PEAK CHASSIS SLIP (EST) %.1f deg | HIGH-ANGLE %.1fs"),
                    Slip.GetPeakAbsoluteDegrees(),
                    Slip.GetHighAngleSeconds()));
            }
            // G peaks are optional per-run world-velocity estimates,
            // never official lap, car performance, award or safety ratings.
            if (latebrake::CanAwardFinishedRace(Race) &&
                Car->GetVehicleGForceMeter().GetAcceptedPairs() > 0)
            {
                const latebrake::VehicleGForceMeter& G =
                    Car->GetVehicleGForceMeter();
                MenuLine(TEXT("CHASSIS G-FORCE ESTIMATES (SESSION)"), true);
                MenuLine(FString::Printf(
                    TEXT("Peak braking %.2fg | peak cornering %.2fg"),
                    G.GetPeakBrakingG(), G.GetPeakCorneringG()));
            }
            // A run-local visual reference, neither a saved all-time
            // leaderboard lap nor a qualifying race classification.
            if (Mode->HasBestLapGhost())
            {
                MenuLine(FString::Printf(
                    TEXT("SESSION GHOST BEACON: %.2fs BEST RECORDED LAP (UNOFFICIAL)"),
                    Mode->GetBestLapGhostSeconds()), true);
            }
            // Unofficial style score from real chassis slip samples;
            // finished-race rewards never consume this value.
            if (latebrake::CanAwardFinishedRace(Race))
            {
                const latebrake::DriftComboMeter& Drift =
                    Car->GetDriftMeter();
                if (Drift.GetComboCount() > 0)
                {
                    MenuLine(TEXT("ARCADE DRIFT SCORE (NO CREDITS)"), true);
                    MenuLine(FString::Printf(
                        TEXT("Drift %.0f pts | combos %d | best %.0f"),
                        Drift.GetTotalPoints(),
                        Drift.GetComboCount(), Drift.GetBestComboPoints()));
                    if (!bCompact)
                    {
                        MenuLine(FString::Printf(
                            TEXT("Qualified slip time %.1fs this run"),
                            Drift.GetTotalDriftSeconds()));
                    }
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
        // Page all generated detail rows in a bounded window instead of
        // silently losing late Results/Career lines. A page transition,
        // resize or changed detail count clamps stale offsets to a valid
        // range. PageUp/PageDown still navigate *menu categories*.
        MenuPager.Synchronize(Mode->GetMenuPage(), MenuDetails.Num(),
            MenuPlan.Valid ? FMath::Max(0, MenuPlan.MaxDetailsRows -
                DrawnMenuRows) : 0);
        const latebrake::MenuDetailWindow DetailWindow = MenuPager.Window();
        if (MenuPlan.Valid)
        {
            for (int32 Index = DetailWindow.Begin;
                 Index < DetailWindow.End; ++Index)
            {
                if (DrawnMenuRows >= MenuPlan.MaxDetailsRows)
                {
                    bMenuOverflow = true;
                    break;
                }
                const auto& Detail = MenuDetails[Index];
                DrawText(Detail.Key, Detail.Value ? Gold : White,
                    X, Y, GEngine->GetSmallFont(), UIScale);
                ++DrawnMenuRows;
                Y += 28.f * UIScale;
            }
            bMenuOverflow = bMenuOverflow ||
                DetailWindow.HasPrevious || DetailWindow.HasNext;
        }
        if (MenuPlan.Valid && bMenuOverflow)
        {
            const FString ClippedNotice = FitHudText(
                DetailWindow.PageCount > 1
                    ? FString::Printf(TEXT("DETAIL PAGE %d/%d | UP/DOWN"),
                        DetailWindow.PageNumber, DetailWindow.PageCount)
                    : FString(TEXT("MORE MENU DETAILS HIDDEN")),
                GEngine->GetSmallFont(), UIScale);
            if (!ClippedNotice.IsEmpty())
            {
                DrawText(ClippedNotice, Gold, X,
                    static_cast<float>(MenuPlan.OverflowNoticeY),
                    GEngine->GetSmallFont(), UIScale);
            }
        }
        // SAVE failures must stay visible even if every optional menu
        // detail row has already been consumed.
        if (MenuPlan.Valid && Mode->HasProgressSaveError())
        {
            const FString SaveNotice = FitHudText(
                bTouchHUD ? TEXT("SAVE FAILED")
                          : TEXT("WARNING: PROGRESS NOT SAVED"),
                GEngine->GetSmallFont(), UIScale);
            if (!SaveNotice.IsEmpty())
            {
                DrawText(SaveNotice, Gold, X,
                    static_cast<float>(MenuPlan.SaveWarningY),
                    GEngine->GetSmallFont(), UIScale);
            }
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
            DrawMenuPad(Pads.Restart, TEXT("UP"));
            DrawMenuPad(Pads.Recover, TEXT("DOWN"));
            DrawMenuPad(Pads.CareerNext, TEXT("NEXT"));
            DrawMenuPad(Pads.CareerStart, TEXT("SELECT"));
            DrawMenuPad(Pads.MenuBrowse, TEXT("BROWSE"));
            DrawMenuPad(Pads.Pause, TEXT("CLOSE"));
        }
#endif
        // Menu navigation must not retain an old driving-direction alert.
        WrongWayHint.Reset();
        OffTrackHint.Reset();
        RivalProximityHint.Reset();
        RivalRelativeMotionHint.Reset();
        return;
    }

    // Closing the menu must not retain a previous page's view offset.
    MenuPager.Reset();

    const FLinearColor Primary(0.95f, 0.96f, 1.f, 1.f);
    const FLinearColor Accent(1.f, 0.73f, 0.17f, 1.f);
    const float Scale = static_cast<float>(Layout.Scale);
    const float Left = static_cast<float>(Layout.Left);
    const float Step = static_cast<float>(Layout.LineHeight);
    float Y = static_cast<float>(Layout.Top);
    int32 DrawnTelemetryLines = 0;

    // Detailed source-only statistics can exceed a real screen height.
    // Clip optional rows before the reserved SAVE and FINISH/PAUSE region,
    // never below the fixed controls or into Android GAS/BRAKE targets.
    const auto Line = [&](const FString& Content, const FLinearColor& Color, bool bHeading = false)
    {
        if (!VerticalPlan.Valid ||
            DrawnTelemetryLines >= VerticalPlan.MaxTelemetryLines ||
            Content.IsEmpty())
        {
            return;
        }
        UFont* Font = bHeading
            ? GEngine->GetMediumFont() : GEngine->GetSmallFont();
        const float TextScale = Scale * (bHeading ? 1.08f : 1.0f);
        const FString Fitted = FitHudText(Content, Font, TextScale);
        if (Fitted.IsEmpty())
        {
            return;
        }
        DrawText(Fitted, Color, Left, Y, Font, TextScale);
        ++DrawnTelemetryLines;
        Y += Step;
    };

    const bool bTimeAttack = Mode->GetSelectedEvent() == latebrake::EventKind::TimeAttack;
    const int32 DisplayLap = FMath::Min(Race.GetCompletedLaps() + 1, Race.GetRequiredLaps());
    const double Elapsed = FMath::Clamp(Race.GetElapsedSeconds(), 0.0, 3599999.0);
    const int32 Minutes = FMath::FloorToInt(Elapsed / 60.0);
    const double Seconds = Elapsed - static_cast<double>(Minutes) * 60.0;

    Line(bCompact ? TEXT("LATEBRAKE") : TEXT("LATEBRAKE  /  RACE BEYOND LIMITS"),
        Accent, true);
    if (!Mode->IsCircuitReadyForGameplay())
    {
        Line(TEXT("TRACK NOT READY - RACE CLOCK AND GATES DISABLED"), Accent);
    }
    else if (!Mode->IsRestartPlacementReady())
    {
        Line(bCompact ? TEXT("RESTART FAILED - RACE DISABLED")
                      : TEXT("RESTART GRID NOT VERIFIED - PRESS RESTART AGAIN"),
             Accent);
    }
    else if (!Mode->HasLocalPlayerDriver())
    {
        Line(TEXT("PLAYER CAR NOT READY - RACE CLOCK FROZEN"), Accent);
    }
    // The focus-loss latch can outlive a failed/OS-controlled world pause.
    // Keep the player informed that racing time remains blocked until an
    // intentional PAUSE/resume, menu exit or restart.
    if (Mode->IsFocusLossGuardActive())
    {
        Line(bCompact ? TEXT("FOCUS LOST - RACE SUSPENDED")
                      : TEXT("APP FOCUS LOST - RACE SUSPENDED; RESUME EXPLICITLY"),
             Accent);
    }
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

    // Always prioritize authoritative lap/next gate and race clock above
    // optional proximity, wayfinding and driving telemetry on small phones.
    Line(bTouchHUD
        ? FString::Printf(TEXT("L%d/%d G%d/%d"),
            DisplayLap, Race.GetRequiredLaps(),
            Race.GetNextCheckpoint() + 1, Race.GetCheckpointCount())
        : FString::Printf(TEXT("LAP  %d / %d  |  GATE  %d / %d"),
            DisplayLap, Race.GetRequiredLaps(),
            Race.GetNextCheckpoint() + 1, Race.GetCheckpointCount()), Primary);
    Line(FString::Printf(TEXT("TIME  %02d:%05.2f"), Minutes, Seconds), Accent, true);
    // A genuinely guarded race-start sequence. Countdowns take a bounded
    // priority row only when track, physical player and grid are ready.
    // The authoritative clock / gate / physics / rival policies all use
    // this same GameMode start light (not an independent HUD timer).
    if (Mode->IsCircuitReadyForGameplay() &&
        Mode->HasLocalPlayerDriver() && Mode->IsRestartPlacementReady())
    {
        switch (Mode->GetRaceStartPhase())
        {
        case latebrake::RaceStartPhase::Three:
            Line(TEXT("START LIGHTS  3"), Accent, true);
            break;
        case latebrake::RaceStartPhase::Two:
            Line(TEXT("START LIGHTS  2"), Accent, true);
            break;
        case latebrake::RaceStartPhase::One:
            Line(TEXT("START LIGHTS  1"), Accent, true);
            break;
        case latebrake::RaceStartPhase::Go:
            Line(TEXT("GO!"), Accent, true);
            break;
        default:
            break;
        }
    }
    // Approximate physics sampling only. Never affects official race time,
    // awards or saved progression and never displaces compact phone rows.
    if (!bCompact && Mode->GetLaunchSpeedMeter().HasSample() &&
        Race.GetElapsedSeconds() <= 15.0)
    {
        Line(FString::Printf(TEXT("LAUNCH 0-5 km/h  %.2fs EST"),
            Mode->GetLaunchSpeedMeter().SecondsToFiveKmh()), Primary);
    }
    // Show only after a completed, valid sampled lap. A small non-colliding
    // 3D reference beacon follows the best *this attempt*, not profile data.
    if (!bCompact && Mode->HasBestLapGhost() &&
        Race.GetCompletedLaps() > 0 && !Race.IsFinished() &&
        Mode->IsRaceStartGreen() && Mode->HasLocalPlayerDriver())
    {
        if (Mode->IsBestLapGhostDisplayEnabled())
        {
            Line(FString::Printf(
                TEXT("BEST-LAP GHOST %.2fs | G HIDE (RUN ONLY)"),
                Mode->GetBestLapGhostSeconds()), Primary);
        }
        else
        {
            Line(TEXT("BEST-LAP GHOST HIDDEN | G SHOW (RUN ONLY)"), Primary);
        }
    }
    // Purely geometric 3D distance at the SAME lap clock, not a race
    // position/pace delta. Hidden/paused ghosts and compact mobile layouts
    // never add this optional telemetry row.
    if (!bCompact)
    {
        double GhostDistanceMeters = 0.0;
        if (Mode->TryGetBestLapGhostSeparationMeters(GhostDistanceMeters))
        {
            if (Mode->UsesMilesPerHour())
            {
                Line(FString::Printf(
                    TEXT("GHOST DIST  %.0f ft (SAME LAP TIME)"),
                    GhostDistanceMeters * 3.280839895), Primary);
            }
            else
            {
                Line(FString::Printf(
                    TEXT("GHOST DIST  %.0f m (SAME LAP TIME)"),
                    GhostDistanceMeters), Primary);
            }
        }
    }
    // Keyboard/gamepad camera choice is advisory. Place after official
    // lap/time readouts, and reserve scarce phone rows for driving.
    if (!bCompact)
    {
        Line(FString::Printf(TEXT("CAMERA %s | C TO CYCLE"),
            ANSI_TO_TCHAR(latebrake::CameraViewName(Car->GetCameraView()))),
            Primary);
    }

    // The warning is pure HUD feedback. It must reset on pawn replacement,
    // checkpoint-session boundary and teleport, and must never influence
    // the real ordered checkpoint or award system.
    const uint64 CurrentWayfindingSession = Mode->GetCheckpointSessionRevision();
    const uint64 CurrentWayfindingTeleport = Car->GetTeleportRevision();
    if (LastWayfindingPawn.Get() != Car ||
        LastWayfindingSession != CurrentWayfindingSession ||
        LastWayfindingTeleport != CurrentWayfindingTeleport)
    {
        WrongWayHint.Reset();
        OffTrackHint.Reset();
        RivalProximityHint.Reset();
        RivalRelativeMotionHint.Reset();
        LastWayfindingPawn = Car;
        LastWayfindingSession = CurrentWayfindingSession;
        LastWayfindingTeleport = CurrentWayfindingTeleport;
    }
    const bool bWayfindingEligible =
        Mode->IsCircuitReadyForGameplay() && Mode->HasLocalPlayerDriver() &&
        !UGameplayStatics::IsGamePaused(this) &&
        !Mode->IsFocusLossGuardActive() &&
        !Mode->IsManualRacePauseActive() && !Race.IsFinished();
    const FVector WayfindingPosition = Car->GetActorLocation();
    const FVector WayfindingVelocity = Car->GetVelocity();

    // Use the same real, validated radar sample for desktop map and the
    // all-viewport warning. Time Attack, invalid physics and blocked race
    // states must never manufacture an opponent for the UI.
    const int32 CurrentRivalVerifiedRecoveries =
        Mode->GetRivalSuccessfulRecoveryCount();
    if (LastRivalVerifiedRecoveries != CurrentRivalVerifiedRecoveries)
    {
        RivalProximityHint.Reset();
        RivalRelativeMotionHint.Reset();
        LastRivalVerifiedRecoveries = CurrentRivalVerifiedRecoveries;
    }
    const bool bRivalRace = !bTimeAttack && Mode->HasRival();
    FVector RivalWorldPosition = FVector::ZeroVector;
    const bool bRivalPositionVerified =
        bRivalRace && bWayfindingEligible &&
        Mode->TryGetRivalRadarPosition(RivalWorldPosition);
    const latebrake::RaceMiniMapSnapshot Radar =
        latebrake::BuildRaceMiniMapSnapshot(
            {WayfindingPosition.X, WayfindingPosition.Y},
            Car->GetActorRotation().Yaw,
            bWayfindingEligible, Race.GetNextCheckpoint(),
            bRivalRace,
            {RivalWorldPosition.X, RivalWorldPosition.Y},
            bRivalPositionVerified);
    if (RivalProximityHint.Update(
            Radar, GetWorld()->GetDeltaSeconds(),
            bWayfindingEligible && bRivalRace))
    {
        Line(FString::Printf(
            bCompact ? TEXT("RIVAL CLOSE | %.0fm DIRECT")
                     : TEXT("RIVAL CLOSE | %.0fm DIRECT (ADVISORY)"),
            Radar.DirectRivalSeparationMeters), Accent);
    }
    // This is a signed change in DIRECT radar separation, not placement,
    // impact probability or opponent speed. Short-lived/jittery observations
    // and teleport-sized discontinuities are deliberately suppressed.
    const latebrake::RivalRelativeMotionAdvice RivalTrend =
        RivalRelativeMotionHint.Update(
            Radar, GetWorld()->GetDeltaSeconds(),
            bWayfindingEligible && bRivalRace);
    if (RivalTrend.Trend == latebrake::RivalRelativeTrend::Closing)
    {
        Line(FString::Printf(
            bCompact ? TEXT("RIVAL CLOSING | %.0fm/s DIRECT")
                     : TEXT("RIVAL CLOSING | %.1fm/s DIRECT RATE"),
            RivalTrend.SeparationChangeMetersPerSecond), Accent);
    }
    else if (!bCompact &&
             RivalTrend.Trend == latebrake::RivalRelativeTrend::Opening)
    {
        Line(FString::Printf(
            TEXT("RIVAL SEPARATING | %.1fm/s DIRECT RATE"),
            RivalTrend.SeparationChangeMetersPerSecond), Primary);
    }
    if (WrongWayHint.Update(
            {WayfindingPosition.X, WayfindingPosition.Y},
            {WayfindingVelocity.X, WayfindingVelocity.Y},
            GetWorld()->GetDeltaSeconds(), bWayfindingEligible))
    {
        Line(bCompact ? TEXT("WRONG WAY - TURN AROUND")
                      : TEXT("WRONG WAY - RETURN TO CIRCUIT DIRECTION"), Accent);
    }
    // A second, independent advisory surfaces sustained off-pavement
    // driving and a bounded nearest-road direction. Its geometry uses
    // the same four strips as IsOnPavedSurface and never awards gates.
    const latebrake::CircuitPoint PlanarPosition{
        WayfindingPosition.X, WayfindingPosition.Y};
    if (OffTrackHint.Update(
            PlanarPosition, GetWorld()->GetDeltaSeconds(),
            bWayfindingEligible))
    {
        const latebrake::RoadReturnAdvice ToRoad =
            latebrake::AdviseReturnToCircuit(PlanarPosition);
        if (ToRoad.Valid && ToRoad.OffPavement)
        {
            const TCHAR* ReturnDirection =
                FMath::Abs(ToRoad.UnitDirection.X) >
                    FMath::Abs(ToRoad.UnitDirection.Y)
                    ? (ToRoad.UnitDirection.X >= 0.0 ? TEXT("EAST") : TEXT("WEST"))
                    : (ToRoad.UnitDirection.Y >= 0.0 ? TEXT("NORTH") : TEXT("SOUTH"));
            Line(FString::Printf(
                bCompact ? TEXT("OFF TRACK | ROAD %.0fm %s")
                         : TEXT("OFF TRACK | NEAREST PAVED ROAD %.0fm %s"),
                ToRoad.DistanceMeters, ReturnDirection), Accent);
        }
    }
    // Guidance is projected from the actual next authoritative gate;
    // the direct-line range is NOT a track distance or a scoring shortcut.
    const latebrake::CircuitRouteAdvice RouteAdvice =
        latebrake::AdviseCircuitRoute(
            Race.GetNextCheckpoint(),
            {WayfindingPosition.X, WayfindingPosition.Y},
            Car->GetActorRotation().Yaw, bWayfindingEligible);
    if (RouteAdvice.Valid && (!bCompact || RouteAdvice.Approaching))
    {
        const TCHAR* Hint = TEXT("STRAIGHT");
        switch (RouteAdvice.Hint)
        {
        case latebrake::RouteHint::Left: Hint = TEXT("LEFT"); break;
        case latebrake::RouteHint::Right: Hint = TEXT("RIGHT"); break;
        case latebrake::RouteHint::TurnAround: Hint = TEXT("TURN AROUND"); break;
        default: break;
        }
        Line(FString::Printf(
            bCompact ? TEXT("NEXT GATE %.0fm | %s")
                     : TEXT("DIRECT TO NEXT GATE %.0fm | BEARING %s"),
            RouteAdvice.DirectDistanceMeters, Hint), Primary);
    }

    // Optional compact drift feedback, AFTER authentic lap/clock and
    // wayfinding lines; bounded HUD budget may hide it on small phones.
    // Display only measured, qualified in-game drift, never credits.
    if (bCompact && Car->GetDriftMeter().HasActiveCombo())
    {
        Line(FString::Printf(TEXT("DRIFT %.0f | x%.2f"),
            Car->GetDriftMeter().GetCurrentComboPoints(),
            Car->GetDriftMeter().GetCurrentComboMultiplier()), Accent);
    }

    if (!bCompact)
    {
        const latebrake::DrivingSessionStats& DriveStats =
            Car->GetDrivingStats();
        if (DriveStats.GetAcceptedSamples() > 0)
        {
            Line(FString::Printf(
                TEXT("DRIVE DATA  %.2f km | PEAK %.0f km/h | BRAKES %d"),
                DriveStats.GetDistanceKm(),
                DriveStats.GetPeakSpeedKmH(),
                DriveStats.GetBrakeApplications()), Primary);
            Line(FString::Printf(
                TEXT("MOTION  %.1fs BRAKING | %.1fs HIGH SIDE SLIP"),
                DriveStats.GetBrakingSeconds(),
                DriveStats.GetHighSlipSeconds()), Primary);
            const latebrake::VehicleGForceMeter& G =
                Car->GetVehicleGForceMeter();
            if (G.GetAcceptedPairs() > 0)
            {
                Line(FString::Printf(
                    TEXT("CHASSIS G (EST)  LONG %+.2fg | LAT %+.2fg"),
                    G.GetLiveLongitudinalG(), G.GetLiveLateralG()), Primary);
            }
            // Optional signed direction-of-motion estimate from real
            // chassis motion, never the player's steering command.
            const latebrake::ChassisSlipAngleMeter& Slip =
                Car->GetChassisSlipAngleMeter();
            // Do not let optional extra telemetry consume the limited line
            // budget on a 720p window before the essential lap timings.
            if (Slip.HasLiveReading() && VerticalPlan.MaxTelemetryLines >= 28)
            {
                Line(FString::Printf(
                    TEXT("CHASSIS SLIP (EST)  %+.1f deg | HIGH-ANGLE %.1fs"),
                    Slip.GetLiveDegrees(), Slip.GetHighAngleSeconds()),
                    Primary);
            }
            // Visually styled combo, never official race classification.
            const latebrake::DriftComboMeter& Drift =
                Car->GetDriftMeter();
            if (Drift.HasActiveCombo())
            {
                Line(FString::Printf(
                    TEXT("DRIFT %.0f pts | x%.2f | %.1fs"),
                    Drift.GetCurrentComboPoints(),
                    Drift.GetCurrentComboMultiplier(),
                    Drift.GetCurrentComboSeconds()), Accent);
            }
        }
    }


    if (!bCompact)
    {
        Line(FString::Printf(TEXT("CURRENT LAP  %.2fs  |  LAST  %.2fs"),
            Race.GetCurrentLapSeconds(), Race.GetLastLapSeconds()), Primary);
        Line(FString::Printf(TEXT("FASTEST LAP  %.2fs  |  LAST SPLIT  %.2fs"),
            Race.GetBestLapSeconds(), Race.GetLastSectorSeconds()), Primary);
        // Source-only run-local accounting, not an extra charge or refund.
        // Charged time is already part of TIME, sector and final lap clocks.
        if (Race.GetTimePenaltyCount() > 0)
        {
            Line(FString::Printf(TEXT("TIME PENALTIES  %d  |  +%.2fs ALREADY IN TIME"),
                Race.GetTimePenaltyCount(), Race.GetTotalPenaltySeconds()), Primary);
            if (!Race.IsFinished() && Race.GetCurrentLapPenaltySeconds() > 0.0)
            {
                Line(FString::Printf(TEXT("THIS LAP PENALTIES  +%.2fs (IN LAP TIME)"),
                    Race.GetCurrentLapPenaltySeconds()), Primary);
            }
        }
        // Estimate based only on actual completed timed laps. Never
        // display a fabricated finish pace before the first valid lap.
        const latebrake::LiveRacePaceProjection Pace =
            latebrake::ProjectLiveRacePace(Race);
        if (Pace.Valid)
        {
            Line(FString::Printf(TEXT("PACE  %.1fs/lap (%d recent) | EST. FINISH  %.1fs"),
                Pace.ReferenceLapSeconds, Pace.SampledLaps,
                Pace.ProjectedFinishSeconds), Accent);
        }
        // A within-run best-sector potential, not a promised finish time.
        // Wait for an actual timed lap and complete positive split history.
        const latebrake::CurrentLapPotential Potential =
            latebrake::EstimateCurrentLapPotential(Race);
        if (Potential.Valid)
        {
            Line(FString::Printf(
                TEXT("LAP POTENTIAL (BEST SPLITS)  %.1fs  |  VS BEST  %+.1fs"),
                Potential.PotentialLapSeconds,
                Potential.DeltaToBestLapSeconds), Accent);
        }
        // Compare the unfinished split against an already measured best
        // of the SAME sector, never against another checkpoint's pace.
        const latebrake::LiveSectorTarget SectorTarget =
            latebrake::MeasureLiveSectorTarget(Race);
        if (SectorTarget.Valid)
        {
            if (SectorTarget.OverBest)
            {
                Line(FString::Printf(
                    TEXT("SECTOR %d  OVER PREVIOUS BEST  +%.1fs"),
                    SectorTarget.SectorIndex + 1,
                    SectorTarget.SecondsOverBest), Accent);
            }
            else
            {
                Line(FString::Printf(
                    TEXT("SECTOR %d  %.1fs UNTIL PREVIOUS BEST"),
                    SectorTarget.SectorIndex + 1,
                    SectorTarget.SecondsUntilBest), Primary);
            }
        }
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
                ? FString::Printf(TEXT("RIVAL ROUTE  %d/%d LAPS | RECOVERY ATTEMPTS  %d%s"),
                    Mode->GetRivalApproximateLaps(), Mode->GetRivalPlannedLaps(),
                    Mode->GetRivalRecoveryCount(),
                    Mode->HasRivalFinishedRoute() ? TEXT(" | DONE") : TEXT(""))
                : FString(TEXT("RIVAL UNAVAILABLE")), Primary);
            if (Mode->HasRival() && Mode->GetRivalRecoveryCount() > 0)
            {
                Line(FString::Printf(
                    TEXT("RIVAL TELEPORTS VERIFIED %d | REJECTED %d"),
                    Mode->GetRivalSuccessfulRecoveryCount(),
                    Mode->GetRivalFailedRecoveryCount()), Primary);
            }
        }
    }

    // Source-only prototype racing radar: project real sampled player and
    // rival actor positions; suppress invalid/unavailable vehicle markers.
    // It is deliberately NOT an order/placement or reward calculation.
    if (!bCompact && Canvas->SizeX >= 1120)
    {
        const float MapSize = 150.f * Scale;
        const float MapLeft = Canvas->SizeX - MapSize - 22.f * Scale;
        const float MapTop = 72.f * Scale;
        const auto ToScreen = [&](latebrake::CircuitMapPoint M)
        {
            return FVector2D(
                MapLeft + static_cast<float>(M.U) * MapSize,
                MapTop + static_cast<float>(1.0 - M.V) * MapSize);
        };
        const auto ScreenMarker = [&](latebrake::RadarMarker M)
        {
            return ToScreen({M.U, M.V});
        };
        DrawText(TEXT("RACE RADAR / NORTH UP"), Accent,
            MapLeft, MapTop - 30.f * Scale,
            GEngine->GetSmallFont(), 0.8f * Scale);
        for (int32 Index = 0; Index < latebrake::CircuitGateCount; ++Index)
        {
            const FVector2D From = ToScreen(
                latebrake::NormalizeCircuitMap(latebrake::CircuitGate(Index)));
            const FVector2D To = ToScreen(
                latebrake::NormalizeCircuitMap(latebrake::CircuitGate(
                    (Index + 1) % latebrake::CircuitGateCount)));
            DrawLine(From.X, From.Y, To.X, To.Y,
                FLinearColor(0.75f, 0.78f, 0.85f), 2.f * Scale);
        }
        if (Radar.NextGate.Valid)
        {
            const FVector2D Next = ScreenMarker(Radar.NextGate);
            DrawRect(Accent, Next.X - 5.f * Scale, Next.Y - 5.f * Scale,
                     10.f * Scale, 10.f * Scale);
        }
        if (Radar.Rival.Valid)
        {
            const FVector2D Rival = ScreenMarker(Radar.Rival);
            DrawRect(FLinearColor(1.f, 0.39f, 0.23f),
                Rival.X - 4.f * Scale, Rival.Y - 4.f * Scale,
                8.f * Scale, 8.f * Scale);
            if (Radar.DirectRivalSeparationValid)
            {
                DrawText(FString::Printf(
                    TEXT("RIVAL %.0fm DIRECT"), Radar.DirectRivalSeparationMeters),
                    Accent, MapLeft, MapTop + MapSize + 8.f * Scale,
                    GEngine->GetSmallFont(), 0.75f * Scale);
            }
        }
        if (Radar.Player.Position.Valid)
        {
            const FVector2D Player = ScreenMarker(Radar.Player.Position);
            const FLinearColor PlayerColor(0.08f, 0.80f, 0.95f);
            DrawRect(PlayerColor,
                Player.X - 4.f * Scale, Player.Y - 4.f * Scale,
                8.f * Scale, 8.f * Scale);
            // A heading tick makes a marker useful when player and rival
            // cross. Screen Y increases downward; radar already converts yaw.
            DrawLine(Player.X, Player.Y,
                Player.X + static_cast<float>(Radar.Player.DirectionX) * 12.f * Scale,
                Player.Y + static_cast<float>(Radar.Player.DirectionY) * 12.f * Scale,
                PlayerColor, 2.f * Scale);
        }
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
    // Dedicated warning baseline is independent of optional text overflow.
    // A save failure must remain visible after all racing stats are clipped.
    if (Mode->HasProgressSaveError() && VerticalPlan.Valid)
    {
        const FString SaveMessage = FitHudText(
            bTouchHUD ? TEXT("SAVE FAILED - PROGRESS NOT STORED")
                      : TEXT("WARNING: LOCAL PROGRESS COULD NOT BE SAVED"),
            GEngine->GetSmallFont(), Scale);
        if (!SaveMessage.IsEmpty())
        {
            DrawText(SaveMessage, Accent, Left,
                static_cast<float>(VerticalPlan.SaveWarningY),
                GEngine->GetSmallFont(), Scale);
        }
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
        if (!latebrake::CanAwardFinishedRace(Race))
        {
            Line(TEXT("FINISH NOT RECORDABLE - UNTIMED LAP OR INVALID CLOCK"), Accent);
        }
        else
        {
            Line(FString::Printf(TEXT("EVENT REWARD  +%d CREDITS"),
                Mode->GetLastRaceCredits()), Accent);
            if (Mode->IsCareerRace())
            {
                const auto Award = Mode->GetLastCareerAward();
                Line(FString::Printf(TEXT("CAREER MEDAL %d/3  |  SERIES BONUS +%d"),
                    static_cast<int32>(Award.Medal), Award.ChampionshipBonus), Accent);
            }
        }
    }
    else if (!bCompact)
    {
        Line(Mode->GetCareerSelectionFeedback(), Primary);
    }
    // A reserved lower status baseline stays visible even when the
    // bounded telemetry above exhausts every available row.
    if (VerticalPlan.Valid)
    {
        const float StatusY = static_cast<float>(VerticalPlan.RaceStatusY);
        if (UGameplayStatics::IsGamePaused(this) ||
            Mode->IsManualRacePauseActive())
        {
            const FString PauseMessage = FitHudText(
                bTouchHUD ? TEXT("PAUSED - TAP PAUSE")
                          : TEXT("PAUSED  -  ESC TO RESUME"),
                GEngine->GetSmallFont(), Scale);
            if (!PauseMessage.IsEmpty())
            {
                DrawText(PauseMessage, Accent, Left,
                    StatusY, GEngine->GetSmallFont(), Scale);
            }
        }
        else if (Race.IsFinished())
        {
            const FString FinishMessage = FitHudText(
                latebrake::CanAwardFinishedRace(Race)
                    ? (bTouchHUD ? TEXT("FINISHED - RESTART")
                                 : TEXT("FINISHED! PRESS T TO RACE AGAIN"))
                    : (bTouchHUD ? TEXT("INVALID FINISH")
                                 : TEXT("INVALID FINISH - PRESS T TO RETRY")),
                GEngine->GetSmallFont(), Scale);
            if (!FinishMessage.IsEmpty())
            {
                DrawText(FinishMessage, Accent, Left, StatusY,
                    GEngine->GetSmallFont(), Scale);
            }
        }
    }

    // A deterministic footer no longer follows an unbounded telemetry Y.
    const float FooterY = VerticalPlan.Valid
        ? static_cast<float>(VerticalPlan.FooterY)
        : FMath::Max(0.f, Canvas->SizeY - 28.f * Scale);
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
        DrawPad(Pads.Pause, TEXT("PAUSE"),
            UGameplayStatics::IsGamePaused(this) || Mode->IsManualRacePauseActive());
        DrawPad(Pads.Restart, TEXT("RESTART"), false);
        DrawPad(Pads.Recover, TEXT("RECOVER"), false);
        // The screen and touch dispatcher use the exact same geometry.
        // Visibility is run-local; no official records or timing change.
        DrawPad(Pads.GhostView,
            Mode->IsBestLapGhostDisplayEnabled()
                ? TEXT("GHOST ON") : TEXT("GHOST OFF"), false);
        DrawPad(Pads.CareerNext, TEXT("NEXT"), false);
        DrawPad(Pads.CareerStart, TEXT("CAREER"), false);
        // MenuBrowse has a disjoint reserved hit rectangle. Reuse it as
        // CAM in racing without moving the existing safety/control pads.
        // Visible numbered selection even when phone HUD is compact.
        const int32 ViewNumber =
            latebrake::CameraViewNumber(Car->GetCameraView());
        const TCHAR* CameraPadLabel =
            ViewNumber == 2 ? TEXT("CAM 2") :
            ViewNumber == 3 ? TEXT("CAM 3") :
            ViewNumber == 4 ? TEXT("CAM 4") : TEXT("CAM 1");
        DrawPad(Pads.MenuBrowse, CameraPadLabel, false);
    }
#else
    const float FooterScale = Scale * (bCompact ? 0.90f : 0.78f);
    const FString FooterMessage = FitHudText(
        bCompact
            ? TEXT("W/S GAS  A/D STEER  SPACE BRAKE  T RESTART")
            : TEXT("W/S DRIVE  A/D STEER  SPACE BRAKE  R RECOVER  T RESTART  C CAMERA  1/2 HANDLING  3/4 QUICK  N/M CAREER  Q/E GARAGE  U/I TUNING"),
        GEngine->GetSmallFont(), FooterScale);
    if (!FooterMessage.IsEmpty())
    {
        DrawText(FooterMessage, Primary, Left, FooterY,
            GEngine->GetSmallFont(), FooterScale);
    }
#endif
}
