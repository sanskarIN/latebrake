#include "UI/LatebrakeRaceHUD.h"

#include "UI/HudLayout.h"
#include "UI/GarageShowroomSchematic.h"
#include "UI/GarageShowroomQuickView.h"
#include "UI/GaragePointerArbitration.h"
#include "UI/HudButtonCaptionFit.h"
#include "UI/GarageAppearanceGlyph.h"
#include "UI/GarageShowroomLane.h"
#include "UI/HudInstrumentCluster.h"
#include "UI/HudInstrumentDriveBadge.h"
#include "UI/NitroHudReadout.h"
#include "UI/RaceGateProgressGauge.h"
#include "Garage/GarageCatalogView.h"
#include "Garage/GaragePurchasePlanner.h"
#include "Garage/GarageComparisonView.h"
#include "Vehicles/VehicleBodyKitRecipe.h"
#include "Vehicles/VehiclePaintRecipe.h"
#include "UI/HudVerticalPlan.h"
#include "UI/HudTextBounds.h"
#include "UI/HudDetailTextFlow.h"
#include "UI/HudMenuVerticalPlan.h"
#include "UI/HudMenuReadablePanel.h"
#include "Racing/FinishedRaceInsights.h"
#include "Racing/RecentLapReview.h"
#include "Racing/RecentLapPaceStatistics.h"
#include "Racing/CleanLapPaceReview.h"
#include "Racing/BestSectorReview.h"
#include "Racing/SampledDrivingReview.h"
#include "Racing/LiveRacePaceProjection.h"
#include "Racing/GateAwareRacePace.h"
#include "Racing/TimeAttackPersonalBestTarget.h"
#include "Racing/QuickRaceRecordReview.h"
#include "Racing/RivalFinishObservation.h"
#include "Racing/RivalSameSegmentOrder.h"
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
#include "Vehicles/VehicleVisualPlaytestReadout.h"

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

bool ALatebrakeRaceHUD::TryScrollMenuMouse(
    double x, double y, int direction)
{
#if PLATFORM_ANDROID
    return false;
#else
    const ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Canvas || !Mode || !Mode->IsMenuOpen())
    {
        return false;
    }
    const double width = static_cast<double>(Canvas->SizeX);
    const double height = static_cast<double>(Canvas->SizeY);
    const auto hud = latebrake::CalculateHudLayout(width, height);
    auto zone = latebrake::CalculateDesktopMenuWheelZone(
        width, height, hud, false, true);
    const auto lane = latebrake::CalculateGarageShowroomLane(
        width, height, hud, false, true, Mode->GetMenuPage());
    if (lane.Valid)
        zone.Width = FMath::Min(zone.Width, lane.TextWidth);
    const auto available = latebrake::AvailableDesktopMenuDetails(
        Mode->GetMenuPage(), MenuPager);
    if (!latebrake::AdmitDesktopMenuWheel(
        zone, x, y, direction, available))
    {
        return false;
    }
    return direction == 1 ? MenuPager.Next() : MenuPager.Previous();
#endif
}

void ALatebrakeRaceHUD::CloseGaragePalette()
{
    GaragePalette = {};
    PresentedGaragePalette = {};
    GaragePaletteFocus = -1;
    DrawnGaragePaletteFocus = -1;
}

bool ALatebrakeRaceHUD::ToggleGaragePalette(latebrake::GarageLookLayer Layer)
{
    auto* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Canvas || !Mode || !Mode->CanCustomizeGarage() ||
        latebrake::GarageLookChoiceCount(Layer) <= 0)
    {
        CloseGaragePalette();
        return true; // blocked input is not a request to cycle
    }
    const double W = Canvas->SizeX, H = Canvas->SizeY;
    const auto Hud = latebrake::CalculateHudLayout(W, H);
    const auto Lane = latebrake::CalculateGarageShowroomLane(
        W,H,Hud,false,Mode->IsMenuOpen(),Mode->GetMenuPage());
    if (!latebrake::CalculateGaragePaletteLayout(Lane,W,H,Hud,Layer).Valid)
    {
        CloseGaragePalette();
        return false; // retain established cycle behavior on compact layouts
    }
    const int32 Viewed = Mode->GetGaragePreviewVehicleId();
    const auto Look = Mode->GetGaragePreviewLook();
    if (GaragePalette.Matches(Viewed, Look, true) && GaragePalette.Layer == Layer)
    {
        CloseGaragePalette();
        return true;
    }
    if (!Mode->BeginGarageAppearancePreview())
    {
        CloseGaragePalette();
        return true;
    }
    // Reject stale mouse confirmation cards before the next HUD frame.
    PresentedGarageOffers = {};
    GaragePalette = {true, Viewed, Look, Layer};
    GaragePaletteFocus = latebrake::InitialGaragePaletteFocus(GaragePalette);
    DrawnGaragePaletteFocus = -1;
    PresentedGaragePalette = {};
    return true;
}

bool ALatebrakeRaceHUD::AdvanceGaragePaletteLayer()
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !Mode->IsMenuOpen() ||
        Mode->GetMenuPage() != latebrake::MenuPage::Garage)
        return false;
    const auto Layer = GaragePalette.Matches(Mode->GetGaragePreviewVehicleId(),
        Mode->GetGaragePreviewLook(),Mode->CanCustomizeGarage())
        ? latebrake::NextGaragePaletteLayer(GaragePalette.Layer)
        : latebrake::GarageLookLayer::Paint;
    // Even a blocked or too-small Garage consumes this contextual command.
    // It must not fall through to an unrelated career action.
    ToggleGaragePalette(Layer);
    return true;
#endif
}

bool ALatebrakeRaceHUD::HandleGaragePaletteNavigation(
    latebrake::GaragePaletteNavigationInput Input)
{
    if (!GaragePalette.Open) return false;
    const auto* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !Canvas || !GaragePalette.Matches(
        Mode->GetGaragePreviewVehicleId(),Mode->GetGaragePreviewLook(),
        Mode->CanCustomizeGarage()))
    {
        CloseGaragePalette();
        return true;
    }
    const double W=Canvas->SizeX,H=Canvas->SizeY;
    const auto Hud=latebrake::CalculateHudLayout(W,H);
    const auto Lane=latebrake::CalculateGarageShowroomLane(
        W,H,Hud,false,Mode->IsMenuOpen(),Mode->GetMenuPage());
    const auto Layout=latebrake::CalculateGaragePaletteLayout(
        Lane,W,H,Hud,GaragePalette.Layer);
    const auto Decision=latebrake::NavigateGaragePalette(
        GaragePalette,Layout,PresentedGaragePalette,
        GaragePaletteFocus,DrawnGaragePaletteFocus,Input);
    switch (Decision.Effect)
    {
    case latebrake::GaragePaletteNavigationEffect::Move:
        GaragePaletteFocus=Decision.Choice;
        break;
    case latebrake::GaragePaletteNavigationEffect::Confirm:
        ApplyGaragePaletteChoice(latebrake::GaragePaletteChoiceAction(Decision.Choice));
        break;
    case latebrake::GaragePaletteNavigationEffect::Cancel:
        CloseGaragePalette();
        break;
    default: break;
    }
    // An open chooser owns these inputs even before its first draw.
    return true;
}

void ALatebrakeRaceHUD::ApplyGaragePaletteChoice(latebrake::DesktopMenuMouseAction Action)
{
    auto* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    const auto Session = GaragePalette;
    const int Choice = latebrake::GaragePaletteChoiceIndex(Action);
    // Recompute at the final mutation boundary, not just during hit test.
    // Resize, focus loss, ownership changes or stale draw receipts must
    // never select an invisible option even if an input handler is called.
    bool bPresented = false;
    if (Mode && Canvas && Choice >= 0 &&
        Session.Matches(Mode->GetGaragePreviewVehicleId(),
            Mode->GetGaragePreviewLook(),Mode->CanCustomizeGarage()))
    {
        const double W=Canvas->SizeX,H=Canvas->SizeY;
        const auto Hud=latebrake::CalculateHudLayout(W,H);
        const auto Lane=latebrake::CalculateGarageShowroomLane(
            W,H,Hud,false,Mode->IsMenuOpen(),Mode->GetMenuPage());
        const auto Layout=latebrake::CalculateGaragePaletteLayout(
            Lane,W,H,Hud,Session.Layer);
        bPresented=latebrake::CanClickPresentedGaragePalette(
            PresentedGaragePalette,Session,Layout,Action);
    }
    CloseGaragePalette();
    if (Mode && bPresented)
        Mode->GarageChooseAppearance({Session.VehicleId,Session.Look,Session.Layer,Choice});
}

latebrake::DesktopMenuMouseAction ALatebrakeRaceHUD::HitTestMenuMouse(
    double x, double y) const
{
#if PLATFORM_ANDROID
    return latebrake::DesktopMenuMouseAction::None;
#else
    const ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Canvas || !Mode || !Mode->IsMenuOpen())
    {
        return latebrake::DesktopMenuMouseAction::None;
    }
    const double width = static_cast<double>(Canvas->SizeX);
    const double height = static_cast<double>(Canvas->SizeY);
    const auto buttons = latebrake::CalculateDesktopMenuMouseLayout(
        width, height, latebrake::CalculateHudLayout(width, height),
        false, true);
    const auto availability = latebrake::AvailableDesktopMenuActions(
        Mode->GetMenuPage(), MenuPager, Mode->CanEquipGarageViewed());
    // Style buttons exist only on the actual visible Garage page.
    const auto style = latebrake::CalculateGarageStyleMouseLayout(
        width, height, latebrake::CalculateHudLayout(width, height),
        false, true, Mode->GetMenuPage());
    // Same six-chip geometry is used by DrawHUD and input hit tests.
    if (Mode->GetMenuPage() == latebrake::MenuPage::Garage)
    {
        const auto hud = latebrake::CalculateHudLayout(width, height);
        const auto lane = latebrake::CalculateGarageShowroomLane(
            width, height, hud, false, true, Mode->GetMenuPage());
        const auto orbit = latebrake::CalculateGarageOrbitButtons(lane, width, height, hud);
        const auto* car = PlayerOwner
            ? Cast<ALatebrakeCarPawn>(PlayerOwner->GetPawn()) : nullptr;
        const auto diagram = latebrake::CalculateGarageSchematic(
            width, height, hud, style, Mode->GetGaragePreviewVehicleId(),
            Mode->GetGaragePreviewLook());
        const auto quick = latebrake::CalculateGarageQuickViewLayout(diagram);
        const auto purchase = latebrake::CalculateGaragePurchaseMouseLayout(
            lane, width, height, hud);
        const auto offers = Mode->GetGaragePurchasePanel();
        const latebrake::GaragePointerPermissions permission{
            availability, Mode->CanBrowseGarage(), Mode->CanCustomizeGarage(),
            car && car->CanSelectGarageCameraOrbit(),
            {purchase.Valid && offers[0].Enabled() && latebrake::CanClickPresentedGarageOffer(
                PresentedGarageOffers[0], purchase.Buttons[0], offers[0]),
             purchase.Valid && offers[1].Enabled() && latebrake::CanClickPresentedGarageOffer(
                PresentedGarageOffers[1], purchase.Buttons[1], offers[1]),
             purchase.Valid && offers[2].Enabled() && latebrake::CanClickPresentedGarageOffer(
                PresentedGarageOffers[2], purchase.Buttons[2], offers[2])}};
        latebrake::GaragePointerClaim paletteClaim{};
        if (GaragePalette.Matches(Mode->GetGaragePreviewVehicleId(),
            Mode->GetGaragePreviewLook(),Mode->CanCustomizeGarage()))
        {
            const auto paletteLayout = latebrake::CalculateGaragePaletteLayout(
                lane,width,height,hud,GaragePalette.Layer);
            const auto paletteAction = latebrake::HitTestGaragePalette(paletteLayout,x,y);
            if (latebrake::GaragePaletteBlocksUnderlyingPointer(
                    true,paletteLayout,x,y,paletteAction))
                return latebrake::DesktopMenuMouseAction::None;
            paletteClaim = {paletteAction, paletteAction != latebrake::DesktopMenuMouseAction::None &&
                latebrake::CanClickPresentedGaragePalette(
                    PresentedGaragePalette,GaragePalette,paletteLayout,paletteAction)};
        }
        return latebrake::ResolveGaragePointerClaims(
            latebrake::CollectGaragePointerClaims(buttons, style, quick, orbit, purchase,
                x, y, permission, paletteClaim));
    }
    return latebrake::HitTestDesktopMenuMouse(buttons, x, y,
        availability);
#endif
}

void ALatebrakeRaceHUD::DrawHUD()
{
    Super::DrawHUD();
    PresentedGarageOffers = {};
    PresentedGaragePalette = {};
    DrawnGaragePaletteFocus = -1;
    if (!GEngine || !PlayerOwner || !Canvas)
    {
        CloseGaragePalette();
        return;
    }

    const ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>();
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(PlayerOwner->GetPawn());
    if (!Mode || !Car)
    {
        WrongWayHint.Reset();
        StuckHint.Reset();
        OffTrackHint.Reset();
        RivalProximityHint.Reset();
        RivalRelativeMotionHint.Reset();
        LastWayfindingPawn.Reset();
        MenuPager.Reset();
        CloseGaragePalette();
        return;
    }

    if (!GaragePalette.Matches(Mode->GetGaragePreviewVehicleId(),
        Mode->GetGaragePreviewLook(),Mode->CanCustomizeGarage()))
        CloseGaragePalette();

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
    // A distinct desktop-only dashboard reserves a right-side text lane.
    // Its values are observational: they never affect physics or awards.
    const latebrake::HudInstrumentCluster Instruments =
        latebrake::CalculateHudInstrumentCluster(
            static_cast<double>(Canvas->SizeX),
            static_cast<double>(Canvas->SizeY),
            Layout, bTouchHUD, Mode->IsMenuOpen(), Mode->GetRace().IsFinished(),
            static_cast<double>(Car->GetSpeedKmH()),
            Car->GetNitroBoost().ChargeFraction(),
            Car->GetEngineHeat().HeatFraction(),
            Car->GetCollisionDamage().DamageFraction());
    const latebrake::DesktopMenuMouseLayout MouseMenu =
        latebrake::CalculateDesktopMenuMouseLayout(
            static_cast<double>(Canvas->SizeX),
            static_cast<double>(Canvas->SizeY),
            Layout, bTouchHUD, Mode->IsMenuOpen());
    const latebrake::GarageStyleMouseLayout GarageStyleMenu =
        latebrake::CalculateGarageStyleMouseLayout(
            static_cast<double>(Canvas->SizeX),
            static_cast<double>(Canvas->SizeY),
            Layout, bTouchHUD, Mode->IsMenuOpen(), Mode->GetMenuPage());
    // Both the instruments and the interactive menu dock own distinct
    // right-hand lanes; truncate measured text before either rectangle.
    const double InstrumentTextWidth = Instruments.Visible
        ? FMath::Min(HorizontalBounds.MaxWidthPixels,
            Instruments.Panel.X - HorizontalBounds.Left -
                16.0 * Instruments.Scale)
        : HorizontalBounds.MaxWidthPixels;
    const auto ShowroomLane = latebrake::CalculateGarageShowroomLane(
        Canvas->SizeX, Canvas->SizeY, Layout, bTouchHUD,
        Mode->IsMenuOpen(), Mode->GetMenuPage());
    const double DockSafeTextWidth = MouseMenu.Valid
        ? FMath::Min(InstrumentTextWidth,
            MouseMenu.Left - HorizontalBounds.Left - 12.0 * Layout.Scale)
        : InstrumentTextWidth;
    const double SafeTextWidth = ShowroomLane.Valid
        ? FMath::Min(DockSafeTextWidth, ShowroomLane.TextWidth)
        : DockSafeTextWidth;
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
            Value.Len(), SafeTextWidth,
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

        // The live 3D scene may be visually busy behind fine menu type.
        // A responsive high-contrast backing uses the exact font-measured
        // left menu lane, not the desktop button or Android touch lanes.
        // Draw BEFORE the pinned heading and paginated details; no input
        // rectangles, saved state, game clock or credit transactions change.
        const latebrake::HudMenuReadablePanel ReadablePanel =
            latebrake::CalculateHudMenuReadablePanel(
                static_cast<double>(Canvas->SizeX),
                static_cast<double>(Canvas->SizeY),
                Layout, MenuPlan, SafeTextWidth, true);
        if (ReadablePanel.Valid)
        {
            const float BX = static_cast<float>(ReadablePanel.X);
            const float BY = static_cast<float>(ReadablePanel.Y);
            const float BW = static_cast<float>(ReadablePanel.Width);
            const float BH = static_cast<float>(ReadablePanel.Height);
            DrawRect(FLinearColor(0.025f, 0.045f, 0.075f, 0.94f),
                BX, BY, BW, BH);
            DrawRect(FLinearColor(0.075f, 0.12f, 0.19f, 0.98f),
                BX, BY, BW,
                static_cast<float>(ReadablePanel.HeaderHeight));
            DrawRect(Gold, BX, BY, 3.f * UIScale, BH);
            DrawRect(FLinearColor(0.18f, 0.29f, 0.39f, 0.95f),
                BX, BY + BH - 2.f * UIScale, BW, 2.f * UIScale);
        }

        // Keep the central scene clear; the two-dimensional schematic and
        // existing car selectors stay in the right dock, not over the mesh.
        const auto PurchaseButtons = latebrake::CalculateGaragePurchaseMouseLayout(
            ShowroomLane, Canvas->SizeX, Canvas->SizeY, Layout);
        if (ShowroomLane.Valid && Car->IsGarageCameraViewPresented())
        {
            const auto& A = ShowroomLane.Aperture;
            DrawText(TEXT("LIVE 3D | F4 ORBIT"), Gold,
                static_cast<float>(A.X),
                static_cast<float>(PurchaseButtons.Valid ?
                    PurchaseButtons.Buttons[0].Bounds.Y - 19.0 * Layout.Scale :
                    A.Y - 24.0 * Layout.Scale),
                GEngine->GetSmallFont(), 0.78f * UIScale);
        }

        // Use the same actual-font fit for every desktop control. The
        // optional right accessory is reserved before text is measured.
        const auto DrawButtonCaption = [&](const FString& Caption,
            const FString& ShortCaption, const latebrake::DesktopMenuMouseRect& Bounds,
            const FLinearColor& Ink, float PreferredScale, double AccessoryWidth) -> bool
        {
            UFont* Font = GEngine->GetSmallFont();
            if (!Font || Caption.IsEmpty()) return false;
            const auto Fit = latebrake::FitHudButtonCaption(Bounds, PreferredScale,
                6.0 * Layout.Scale, 3.0 * Layout.Scale, AccessoryWidth,
                !ShortCaption.IsEmpty(), [&](bool bShort, double Scale)
                {
                    float TW = 0.f, TH = 0.f;
                    GetTextSize(bShort ? ShortCaption : Caption, TW, TH,
                        Font, static_cast<float>(Scale));
                    return latebrake::HudCaptionExtent{TW, TH};
                });
            if (!Fit.Valid) return false;
            DrawText(Fit.ShortCaption ? ShortCaption : Caption, Ink,
                static_cast<float>(Fit.X), static_cast<float>(Fit.Y),
                Font, static_cast<float>(Fit.Scale));
            return true;
        };
        if (PurchaseButtons.Valid)
        {
            const auto Offers = Mode->GetGaragePurchasePanel();
            // Populate the presentation for synchronous hover evaluation.
            // A target is revoked before returning if either caption fails.
            for (std::size_t i=0; i<Offers.size(); ++i)
                PresentedGarageOffers[i] = {true, PurchaseButtons.Buttons[i].Bounds, Offers[i]};
            float OfferX = 0.f, OfferY = 0.f;
            const auto Hover = PlayerOwner->GetMousePosition(OfferX, OfferY)
                ? HitTestMenuMouse(OfferX, OfferY)
                : latebrake::DesktopMenuMouseAction::None;
            for (std::size_t i=0; i<Offers.size(); ++i)
            {
                const auto& Offer = Offers[i];
                const auto& Button = PurchaseButtons.Buttons[i];
                const auto& R = Button.Bounds;
                const bool bConfirm = Offer.Status == latebrake::GarageOfferStatus::Confirm;
                const bool bEnabled = Offer.Enabled();
                const FLinearColor Ink = !bEnabled
                    ? FLinearColor(0.62f,0.68f,0.75f) : bConfirm ? Gold : White;
                DrawRect(bConfirm ? FLinearColor(0.38f,0.25f,0.07f,0.98f)
                    : Hover==Button.Action && bEnabled
                        ? FLinearColor(0.19f,0.31f,0.43f,0.98f)
                        : FLinearColor(0.06f,0.10f,0.16f,0.96f),
                    static_cast<float>(R.X),static_cast<float>(R.Y),
                    static_cast<float>(R.Width),static_cast<float>(R.Height));
                FString Title = i==0 ? (Offer.Subject>=0
                    ? FString(UTF8_TO_TCHAR(latebrake::GarageState::Definition(Offer.Subject).Name))
                    : FString(TEXT("ALL CARS")))
                    : i==1 ? FString(TEXT("ENGINE")) : FString(TEXT("GRIP"));
                FString Status;
                switch (Offer.Status)
                {
                case latebrake::GarageOfferStatus::Preview:
                    Status=FString::Printf(TEXT("%d CR"),Offer.Price); break;
                case latebrake::GarageOfferStatus::Confirm:
                    Status=FString::Printf(TEXT("CONFIRM %.1fs"),Offer.SecondsRemaining); break;
                case latebrake::GarageOfferStatus::NeedCredits:
                    Status=FString::Printf(TEXT("NEED %d"),Offer.Shortfall); break;
                case latebrake::GarageOfferStatus::Complete:
                    Status=TEXT("MAX"); break;
                default: Status=TEXT("LOCKED"); break;
                }
                const latebrake::DesktopMenuMouseRect Upper{R.X,R.Y,R.Width,R.Height*0.5};
                const latebrake::DesktopMenuMouseRect Lower{R.X,R.Y+R.Height*0.5,R.Width,R.Height*0.5};
                const bool bTitleDrawn = DrawButtonCaption(Title,FString(),Upper,Ink,0.72f*UIScale,0.0);
                const bool bStatusDrawn = DrawButtonCaption(Status,bConfirm ? FString(TEXT("CONFIRM")) : FString(),
                    Lower,Ink,0.68f*UIScale,0.0);
                PresentedGarageOffers[i].Visible = bTitleDrawn && bStatusDrawn;
            }
        }
        // Direct choice overlay is a temporary showroom panel. The same
        // layout is used for rendering and hit admission; the car is
        // revealed again immediately after an explicit choice or close.
        if (GaragePalette.Open)
        {
            const auto Palette = latebrake::CalculateGaragePaletteLayout(
                ShowroomLane,Canvas->SizeX,Canvas->SizeY,Layout,GaragePalette.Layer);
            if (!Palette.Valid)
                CloseGaragePalette();
            else
            {
                PresentedGaragePalette.Session = GaragePalette;
                PresentedGaragePalette.Layout = Palette;
                const auto& P = Palette.Panel;
                DrawRect(FLinearColor(0.025f,0.045f,0.075f,0.98f),
                    P.X,P.Y,P.Width,P.Height);
                DrawRect(Gold,P.X,P.Y,P.Width,3.f*UIScale);
                const auto& Close = Palette.Close.Bounds;
                const latebrake::DesktopMenuMouseRect Header{
                    P.X,Close.Y,Close.X-P.X,Close.Height};
                const FString HeaderText = GaragePalette.Layer == latebrake::GarageLookLayer::Paint
                    ? TEXT("CHOOSE PAINT") : GaragePalette.Layer == latebrake::GarageLookLayer::Stripe
                    ? TEXT("CHOOSE STRIPES") : TEXT("CHOOSE RIMS");
                DrawButtonCaption(HeaderText,FString(TEXT("CHOOSE")),Header,Gold,0.78f*UIScale,0.0);
                DrawRect(FLinearColor(0.14f,0.20f,0.29f,0.98f),Close.X,Close.Y,Close.Width,Close.Height);
                PresentedGaragePalette.CloseVisible =
                    DrawButtonCaption(FString(TEXT("CLOSE")),FString(TEXT("X")),Close,White,0.76f*UIScale,0.0);
                float PX=0.f,PY=0.f;
                const auto Hover = PlayerOwner->GetMousePosition(PX,PY)
                    ? latebrake::HitTestGaragePalette(Palette,PX,PY)
                    : latebrake::DesktopMenuMouseAction::None;
                for (int32 i=0;i<Palette.Count;++i)
                {
                    const auto Choice = latebrake::DescribeGaragePaletteChoice(
                        GaragePalette.VehicleId,GaragePalette.Look,GaragePalette.Layer,i);
                    const auto& Button = Palette.Choices[static_cast<std::size_t>(i)];
                    const auto& R = Button.Bounds;
                    const bool bSelected = Choice.Selected;
                    const bool bFocused = GaragePaletteFocus == i;
                    const FLinearColor FocusInk(0.25f,0.84f,0.96f,1.f);
                    const FLinearColor Ink = bSelected ? Gold : White;
                    DrawRect(bSelected ? FLinearColor(0.27f,0.21f,0.09f,0.99f)
                        : Hover==Button.Action ? FLinearColor(0.19f,0.31f,0.43f,0.99f)
                        : FLinearColor(0.08f,0.13f,0.20f,0.99f),R.X,R.Y,R.Width,R.Height);
                    DrawRect(Ink,R.X,R.Y,3.f*UIScale,R.Height);
                    if (bFocused)
                    {
                        // Focus and active appearance are independent:
                        // the focused card gets a full high-contrast outline.
                        const float Edge=FMath::Max(2.f,2.f*UIScale);
                        DrawRect(FocusInk,R.X,R.Y,R.Width,Edge);
                        DrawRect(FocusInk,R.X,R.Y+R.Height-Edge,R.Width,Edge);
                        DrawRect(FocusInk,R.X+R.Width-Edge,R.Y,Edge,R.Height);
                    }
                    const float Chip = FMath::Clamp(15.f*UIScale,10.f,18.f);
                    const float CX = static_cast<float>(R.X+R.Width-Chip-6.f*UIScale);
                    const float CY = static_cast<float>(R.Y+0.5*(R.Height-Chip));
                    const bool bPaint = GaragePalette.Layer == latebrake::GarageLookLayer::Paint;
                    double Accessory = Chip+6.f*UIScale;
                    bool bPreviewDrawn = true;
                    if (bPaint)
                        DrawRect(FLinearColor(Choice.Swatch.R,Choice.Swatch.G,Choice.Swatch.B,1.f),
                            CX,CY,Chip,Chip);
                    else
                    {
                        // These are bounded schematics of the selected
                        // roof stripes or authored alloy faces, NOT GPU captures.
                        const auto Decision = latebrake::PreviewGarageDirectAppearance(
                            GaragePalette.VehicleId,GaragePalette.Look,
                            {GaragePalette.VehicleId,GaragePalette.Look,GaragePalette.Layer,i});
                        const double IconSize = FMath::Min(R.Height-10.0,48.0*Layout.Scale);
                        const latebrake::DesktopMenuMouseRect IconBounds{
                            R.X+R.Width-IconSize-6.0*Layout.Scale,
                            R.Y+(R.Height-IconSize)*0.5,IconSize,IconSize};
                        const auto Glyph = latebrake::CalculateGarageAppearanceGlyph(
                            IconBounds,GaragePalette.VehicleId,Decision.Result,GaragePalette.Layer);
                        bPreviewDrawn = Glyph.Valid;
                        if (Glyph.Valid)
                        {
                            Accessory = IconSize+6.0*Layout.Scale;
                            const auto Color = [&](latebrake::GarageGlyphInk Kind)
                            {
                                const auto RGB = latebrake::GarageGlyphColor(
                                    Kind,Glyph.VehicleId,Glyph.Look);
                                return FLinearColor(static_cast<float>(RGB.R),
                                    static_cast<float>(RGB.G),static_cast<float>(RGB.B),1.f);
                            };
                            DrawRect(FLinearColor(0.22f,0.27f,0.33f,1.f),
                                IconBounds.X,IconBounds.Y,IconBounds.Width,IconBounds.Height);
                            for (std::size_t j=0;j<Glyph.RectCount;++j)
                            {
                                const auto& Piece=Glyph.Rects[j];
                                const auto& B=Piece.Bounds;
                                DrawRect(Color(Piece.Ink),B.X,B.Y,B.Width,B.Height);
                            }
                            for (std::size_t j=0;j<Glyph.LineCount;++j)
                            {
                                const auto& L=Glyph.Lines[j];
                                DrawLine(L.A.X,L.A.Y,L.B.X,L.B.Y,Color(L.Ink),L.Thickness);
                            }
                        }
                    }
                    const latebrake::DesktopMenuMouseRect Title{
                        R.X,R.Y,R.Width,R.Height*0.60};
                    const latebrake::DesktopMenuMouseRect Footer{
                        R.X,R.Y+R.Height*0.60,R.Width,R.Height*0.40};
                    const bool bTitle = DrawButtonCaption(
                        FString(UTF8_TO_TCHAR(Choice.Label)),FString(UTF8_TO_TCHAR(Choice.ShortLabel)),
                        Title,Ink,0.76f*UIScale,Accessory);
                    const FString Tag = bSelected ? TEXT("ACTIVE") : Choice.Factory ? TEXT("FACTORY") : TEXT("SELECT");
                    const bool bTag = DrawButtonCaption(Tag,FString(),Footer,Ink,0.62f*UIScale,Accessory);
                    PresentedGaragePalette.ChoiceVisible[static_cast<std::size_t>(i)] = bTitle && bTag && bPreviewDrawn;
                    if (bFocused && bTitle && bTag && bPreviewDrawn)
                        DrawnGaragePaletteFocus = i;
                }
            }
        }
        const auto OrbitButtons = latebrake::CalculateGarageOrbitButtons(
            ShowroomLane, Canvas->SizeX, Canvas->SizeY, Layout);
        if (OrbitButtons.Valid)
        {
            const bool bCanOrbit = Car->CanSelectGarageCameraOrbit();
            float OrbitMouseX = 0.f, OrbitMouseY = 0.f;
            const auto OrbitHovered = PlayerOwner->GetMousePosition(OrbitMouseX, OrbitMouseY)
                ? HitTestMenuMouse(OrbitMouseX, OrbitMouseY)
                : latebrake::DesktopMenuMouseAction::None;
            for (const auto& Button : OrbitButtons.Buttons)
            {
                const int32 Index = latebrake::GarageOrbitIndex(Button.Action);
                const bool bSelected = Index == Car->GetGarageCameraOrbitIndex();
                const bool bHover = OrbitHovered == Button.Action;
                const auto& R = Button.Bounds;
                const float BX=static_cast<float>(R.X), BY=static_cast<float>(R.Y);
                const float BW=static_cast<float>(R.Width), BH=static_cast<float>(R.Height);
                const FLinearColor Ink = !bCanOrbit
                    ? FLinearColor(0.54f,0.59f,0.64f)
                    : bSelected ? Gold : White;
                DrawRect(bSelected
                    ? FLinearColor(0.27f,0.21f,0.09f,0.95f)
                    : bHover ? FLinearColor(0.19f,0.31f,0.43f,0.95f)
                             : FLinearColor(0.08f,0.13f,0.20f,0.95f),
                    BX,BY,BW,BH);
                DrawRect(Ink,BX,BY,BW,2.f);
                // Measure each caption and fall back to its ordinal on
                // narrow buttons. Never draw across another hit target.
                FString Caption(UTF8_TO_TCHAR(latebrake::GarageOrbitCaption(Index)));
                DrawButtonCaption(Caption, FString::FromInt(Index+1),
                    R, Ink, 0.70f * UIScale, 0.0);
            }
        }

        // Build the logical page first. The former unbounded MenuLine
        // simply discarded overflow, including essential Results details.
        // Keep the title/nav pinned and paginate ALL generated details.
        TArray<TPair<FString, bool>> MenuDetails;
        const auto MenuLine = [&](const FString& Value, bool bAccent = false)
        {
            UFont* Font = GEngine->GetSmallFont();
            if (!MenuPlan.Valid || !HorizontalBounds.Valid ||
                Value.IsEmpty() || !Font || SafeTextWidth <= 0.0)
            {
                return;
            }
            // Page the *complete* measured text. Previously every detail
            // was permanently ellipsized before MenuPager saw the content,
            // making long garage/results descriptions impossible to read.
            const int32 RemainingRows = FMath::Max(0, 1024 - MenuDetails.Num());
            if (RemainingRows <= 0)
            {
                return;
            }
            // Every menu row, including overflow diagnostics, must use
            // the same actual-font bounds as the normal detail rows.
            // A raw diagnostic can overflow a narrow screen even when all
            // wrapped content is correctly measured and paginated.
            const auto AddMeasuredMenuNotice = [&](const TCHAR* Message)
            {
                const FString Notice = FitHudText(Message, Font, UIScale);
                if (!Notice.IsEmpty() && MenuDetails.Num() < 1024)
                {
                    MenuDetails.Emplace(Notice, true);
                }
            };
            // Reserve one diagnostic row and enforce a per-entry cap.
            // A single maliciously long car name or result string must not
            // monopolize all 1024 pager rows and conceal later details.
            const int32 WrapBudget = FMath::Min(32, RemainingRows - 1);
            if (WrapBudget <= 0)
            {
                AddMeasuredMenuNotice(TEXT("DETAIL OMITTED - ROW LIMIT"));
                return;
            }
            const auto Wrapped = latebrake::WrapHudDetailText(
                Value.Len(), SafeTextWidth,
                [&](int Index) -> std::uint32_t
                {
                    return static_cast<std::uint32_t>(Value[Index]);
                },
                [&](int Begin, int Count) -> double
                {
                    float MeasuredWidth = 0.f;
                    float MeasuredHeight = 0.f;
                    GetTextSize(Value.Mid(Begin, Count), MeasuredWidth,
                        MeasuredHeight, Font, UIScale);
                    return static_cast<double>(MeasuredWidth);
                },
                [&](int Begin, int Count)
                {
                    MenuDetails.Emplace(Value.Mid(Begin, Count), bAccent);
                }, WrapBudget);
            if (!Wrapped.Complete)
            {
                // Fail visibly instead of silently discarding information
                // when a glyph cluster cannot fit or the row cap is hit.
                AddMeasuredMenuNotice(
                    TEXT("DETAIL TEXT COULD NOT BE FITTED"));
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
            if (Mode->GetMenuPage() == latebrake::MenuPage::Garage)
            {
                // The detail pager can be several screens deep. Purchase
                // confirmation, lock rejection and save rollback feedback
                // must remain visible on EVERY page, without adding a third
                // header row or consuming the small-screen detail budget.
                DrawPinned(Mode->GetGaragePurchaseDisplayFeedback(), true);
            }
            else
            {
                DrawPinned(bTouchHUD
                    ? TEXT("UP/DOWN DETAILS | NEXT MENU | SELECT")
                    : TEXT("TAB CLOSE | PAGE UP/DOWN MENU | UP/DOWN DETAILS | ENTER"),
                    false);
            }
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
            if (Mode->GetQuickRaceSoloTimeAttackSelection())
            {
                MenuLine(TEXT("MODE: SOLO TIME ATTACK | 1 LAP | NO RIVAL"), true);
                MenuLine(TEXT("The existing single-lap timed event and PB record."));
            }
            else
            {
                MenuLine(TEXT("MODE: CIRCUIT RACE | 3 LAPS | LOCAL RIVAL"), true);
                MenuLine(FString::Printf(TEXT("LOCAL RIVAL: %s (next quick race)"),
                    UTF8_TO_TCHAR(latebrake::RivalDifficultyName(
                        Mode->GetQuickRaceDifficultySelection()))));
            }
            MenuLine(TEXT("BROWSE/N or gamepad Right: ROOKIE / STANDARD / EXPERT / SOLO TIME ATTACK."));
            MenuLine(TEXT("Select starts choice. Career uses STANDARD; choices not saved."));
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
        {
            // A complete read-only catalogue of EVERY saved car ID, not
            // just the equipped one. The existing detail pager ensures
            // every row stays accessible on short desktop/Android screens.
            const latebrake::GarageCatalogView Catalog =
                latebrake::BuildGarageCatalogView(Mode->GetGarageSnapshot());
            MenuLine(FString::Printf(
                TEXT("GARAGE CAR %d/%d | OWNED %d/%d | %d CR"),
                Catalog.SelectedVehicle + 1, latebrake::VehicleCount,
                Catalog.OwnedVehicleCount, latebrake::VehicleCount,
                Catalog.Credits), true);
            // Keep transaction confirmation near the top, even when the
            // catalogue spans multiple paginated detail windows.
            MenuLine(Mode->GetGaragePurchaseDisplayFeedback(), true);
            // A normal Garage page pauses the world, but explicit OS
            // focus loss or standalone manual-pause is a separate gate.
            // Avoid displaying purchasable offers without the suspension
            // warning; this is a read-only view until explicit resume.
            if (Mode->IsFocusLossGuardActive() ||
                Mode->IsManualRacePauseActive())
                MenuLine(TEXT("GARAGE SUSPENDED: RESUME TO EQUIP, BUY OR STYLE"), true);
            if (Mode->IsProfileReadOnly())
                MenuLine(TEXT("NEWER SAVE PROFILE: READ-ONLY, NO PURCHASES"), true);
            // Read-only visual comparison may inspect any locked car. Only
            // the owned car in GarageState can ever be equipped/raced.
            const int32 PreviewId = Mode->GetGaragePreviewVehicleId();
            const auto& Preview = Catalog.Cars[
                static_cast<std::size_t>(PreviewId)];
            MenuLine(FString::Printf(
                TEXT("VIEWING %d/%d: %s | %s"),
                PreviewId + 1, latebrake::VehicleCount,
                UTF8_TO_TCHAR(Preview.Name),
                Preview.Ownership == latebrake::GarageCatalogOwnership::Equipped
                    ? TEXT("EQUIPPED")
                    : Preview.Ownership == latebrake::GarageCatalogOwnership::Owned
                        ? TEXT("OWNED / NOT EQUIPPED")
                        : TEXT("LOCKED / PREVIEW ONLY")), true);
            MenuLine(FString::Printf(
                TEXT("PREVIEW EST.: ACCEL %.2fx | GRIP %.2fx | SPEED %.2fx"),
                Preview.Effective.Acceleration, Preview.Effective.Grip,
                Preview.Effective.TopSpeed));
            // Sequential career unlocks intentionally do not follow the
            // preview cursor. Identify the actual paid target NEXT TO the
            // viewed car so browsing later locked IDs cannot mislead users
            // into thinking Q will buy the pictured prototype.
            if (latebrake::GarageState::IsVehicle(Catalog.NextUnlockId))
            {
                const auto& Next = Catalog.Cars[static_cast<std::size_t>(Catalog.NextUnlockId)];
                MenuLine(FString::Printf(TEXT("Q NEXT: %s | %d CR | NEED %d"),
                    UTF8_TO_TCHAR(Next.Name), Next.UnlockCost, Next.RouteCreditsShort), true);
                MenuLine(TEXT("LOCKED ROWS SHOW TOTAL SEQUENTIAL ROUTE COST."));
            }
            else
            {
                MenuLine(TEXT("ALL CARS OWNED: NO NEXT UNLOCK"), true);
            }
            if (latebrake::GarageState::IsVehicle(Catalog.NextUnlockId) &&
                PreviewId != Catalog.NextUnlockId)
                MenuLine(TEXT("Q UNLOCKS NEXT CAR, NOT THE VIEWED CAR."), true);
            // Q is sequential. Report the whole route to the viewed car,
            // not just its sticker price or today's first unlock.
            // This simulation owns a private snapshot; no real purchase.
            const auto ViewedPath = latebrake::PlanGaragePurchasePath(
                Mode->GetGarageSnapshot(),
                {PreviewId, Catalog.EngineLevel, Catalog.GripLevel, 0});
            if (ViewedPath.AdditionalCarsUnlocked > 0 &&
                ViewedPath.Status != latebrake::GaragePlanStatus::Invalid)
            {
                // Reserve is zero and route cost is the bounded sum of
                // six fixed car prices, so the shortfall fits int32.
                MenuLine(FString::Printf(
                    TEXT("VIEWED UNLOCK ROUTE: %d CAR(S) | %d CR TOTAL | NEED %d"),
                    ViewedPath.AdditionalCarsUnlocked, ViewedPath.UnlockCost,
                    static_cast<int32>(ViewedPath.CreditsShort)), true);
                if (ViewedPath.AdditionalCarsUnlocked > 1)
                    MenuLine(TEXT("Q BUYS ONE NEXT CAR AT A TIME; TOTAL IS A PREVIEW."));
            }
            MenuLine(TEXT("CLICK 1-6 ABOVE TOP VIEW: DIRECT CAR PREVIEW ONLY"));
            MenuLine(TEXT("F9/F10 or mouse: BROWSE ALL / ENTER: EQUIP IF OWNED"));
            MenuLine(TEXT("PAD: RB FORWARD / B BACK | E EQUIPS NEXT OWNED"));
            MenuLine(FString::Printf(TEXT("ENGINE %d/3 | GRIP %d/3"),
                Catalog.EngineLevel, Catalog.GripLevel));
            // Prices and relative tuning are read-only summaries of the
            // authoritative garage. These messages cannot authorize spending.
            if (!bCompact)
            {
                const auto EngineOffer = latebrake::PreviewGarageUpgrade(
                    Catalog.Credits, Catalog.EngineLevel, latebrake::UpgradeKind::Engine);
                const auto GripOffer = latebrake::PreviewGarageUpgrade(
                    Catalog.Credits, Catalog.GripLevel, latebrake::UpgradeKind::Grip);
                const auto OfferText = [](latebrake::GarageUpgradeOffer offer) -> FString
                {
                    switch (offer.Status)
                    {
                    case latebrake::GarageUpgradeOfferStatus::Affordable:
                        return FString::Printf(TEXT("%d CR READY"), offer.Cost);
                    case latebrake::GarageUpgradeOfferStatus::NeedCredits:
                        return FString::Printf(TEXT("%d CR | NEED %d"),
                            offer.Cost, offer.CreditsShort);
                    case latebrake::GarageUpgradeOfferStatus::Maxed:
                        return TEXT("MAX");
                    default:
                        return TEXT("UNAVAILABLE");
                    }
                };
                MenuLine(FString::Printf(TEXT("NEXT U ENGINE: %s | NEXT I GRIP: %s"),
                    *OfferText(EngineOffer), *OfferText(GripOffer)));
            }
            MenuLine(TEXT("ENTER/SELECT: EQUIP VIEWED OWNED CAR | E: NEXT OWNED"));
            MenuLine(TEXT("GARAGE ONLY: Q UNLOCKS | U/I UPGRADES"));
            if (!bTouchHUD)
                MenuLine(TEXT("CTRL+1..6: EQUIP OWNED CARS DIRECTLY (LOCKED CARS CANNOT EQUIP)"));
            MenuLine(TEXT("Q/U/I press twice within 8s to confirm."));
            MenuLine(TEXT("UP/DOWN: MORE GARAGE DETAILS"));
            MenuLine(TEXT("STYLE VIEWED OWNED CAR: F5/F6/F7 | F8 FACTORY (FREE)"), true);
            MenuLine(TEXT("F5 PAINT | F6 STRIPES | F7 RIMS | F8 FACTORY (FREE)"), true);
            if (ShowroomLane.Valid)
            {
                MenuLine(TEXT("MOUSE: PAINT / STRIPE / RIMS OPENS CHOOSER (FREE)."));
                MenuLine(TEXT("N / PAD RIGHT: OPEN OR SWITCH CHOOSER."));
                MenuLine(TEXT("UP/DOWN: FOCUS | ENTER/Y: CHOOSE | M/PAD LEFT: CLOSE."));
            }
            MenuLine(TEXT("PAD: R3 PAINT / L3 STRIPES / LB RIMS"), true);
            if (!Mode->CanCustomizeGarage() &&
                !Mode->IsFocusLossGuardActive() &&
                !Mode->IsManualRacePauseActive() &&
                !Mode->IsProfileReadOnly())
            {
                const bool bLockedPreview =
                    Preview.Ownership == latebrake::GarageCatalogOwnership::LockedAffordable ||
                    Preview.Ownership == latebrake::GarageCatalogOwnership::LockedInsufficientCredits;
                MenuLine(bLockedPreview
                    ? TEXT("CUSTOM LOOK LOCKED: OWN THE VIEWED CAR FIRST")
                    : TEXT("CUSTOM LOOK UNAVAILABLE DURING THIS CAREER RACE"), true);
            }
            // The real paused Pawn now shows the browsed car, not the
            // equipped car. Report source state honestly: this says which
            // engine components are requested, NOT that pixels were QA'd.
            const bool bLiveShowroom = Car->IsGarageShowroomPreviewActive() &&
                Car->GetPresentedGarageVehicleId() == PreviewId;
            MenuLine(FString::Printf(TEXT("LIVE SHOWROOM: %s | %s"),
                bLiveShowroom ? TEXT("VIEWED CAR (VISUAL ONLY)")
                    : TEXT("PREVIEW NOT ACTIVE"),
                Car->IsOriginalVehicleSurfaceCommitted()
                    ? TEXT("SOURCE 3D COMPONENTS")
                    : TEXT("LEGACY CUBE FALLBACK")), true);
            MenuLine(FString::Printf(TEXT("F4 GARAGE ANGLE %d/4 | %s"),
                Car->GetGarageCameraOrbitIndex() + 1,
                Car->IsGarageCameraViewPresented()
                    ? TEXT("SOURCE FRAMED VIEW")
                    : TEXT("NORMAL CAMERA / PREVIEW PENDING")), true);
            const latebrake::GarageLook ViewedLook = Mode->GetGaragePreviewLook();
            MenuLine(FString::Printf(
                TEXT("VIEWED LOOK: %s / %s / %s"),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Paint, ViewedLook.Paint)),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Stripe, ViewedLook.Stripe)),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Wheels, ViewedLook.Wheels))), true);
            const latebrake::GarageLook ActiveLook =
                Mode->GetSelectedGarageLook();
            MenuLine(FString::Printf(
                TEXT("EQUIPPED LOOK: %s / %s / %s"),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Paint, ActiveLook.Paint)),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Stripe, ActiveLook.Stripe)),
                UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                    latebrake::GarageLookLayer::Wheels, ActiveLook.Wheels))), true);
            MenuLine(TEXT("SIX UE PROTOTYPE CARS / 12-CAR 2D TRIAL IS SEPARATE"), true);
            for (const latebrake::GarageCatalogEntry& Entry : Catalog.Cars)
            {
                // A separate short identity line prevents very long route
                // budgets from pushing EQUIPPED/LOCKED or Q NEXT off-screen.
                FString State;
                const bool bLocked =
                    Entry.Ownership == latebrake::GarageCatalogOwnership::LockedAffordable ||
                    Entry.Ownership == latebrake::GarageCatalogOwnership::LockedInsufficientCredits;
                switch (Entry.Ownership)
                {
                case latebrake::GarageCatalogOwnership::Equipped:
                    State = TEXT("EQUIPPED");
                    break;
                case latebrake::GarageCatalogOwnership::Owned:
                    State = TEXT("OWNED");
                    break;
                case latebrake::GarageCatalogOwnership::LockedAffordable:
                case latebrake::GarageCatalogOwnership::LockedInsufficientCredits:
                    State = TEXT("LOCKED");
                    break;
                }
                // Only the first locked car is actionable with Q. Display
                // that control on the short identity line, not after a long
                // route estimate where the measured HUD clip could hide it.
                const TCHAR* NextFlag = Entry.VehicleId == Catalog.NextUnlockId
                    ? TEXT(" | Q NEXT") : TEXT("");
                const FString SlotLabel = bTouchHUD
                    ? FString::Printf(TEXT("%d."), Entry.VehicleId + 1)
                    : FString::Printf(TEXT("CTRL+%d"), Entry.VehicleId + 1);
                MenuLine(FString::Printf(TEXT("%s %s | %s%s%s"),
                    *SlotLabel, UTF8_TO_TCHAR(Entry.Name), *State, NextFlag,
                    Entry.VehicleId == PreviewId ? TEXT(" | VIEWING") : TEXT("")),
                    Entry.Ownership == latebrake::GarageCatalogOwnership::Equipped ||
                    Entry.VehicleId == PreviewId);
                if (bLocked)
                {
                    if (!Entry.RouteValid)
                    {
                        MenuLine(TEXT("   SEQUENTIAL UNLOCK ROUTE UNAVAILABLE"));
                    }
                    else
                    {
                        // A short companion line keeps the exact total and
                        // missing balance readable on compact touch screens.
                        MenuLine(Entry.RouteBudgetReady
                            ? FString::Printf(TEXT("   ROUTE %d CR | BUDGET READY"),
                                Entry.RouteCost)
                            : FString::Printf(TEXT("   ROUTE %d CR | NEED %d CR"),
                                Entry.RouteCost, Entry.RouteCreditsShort));
                        if (!bCompact)
                        {
                            MenuLine(FString::Printf(
                                TEXT("   VIA %d SEQUENTIAL Q UNLOCK(S)"),
                                Entry.RouteCars));
                        }
                    }
                }
                if (!bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("   KIT %s | FACTORY PAINT %s | ACCEL %.2fx | GRIP %.2fx | SPEED %.2fx"),
                        UTF8_TO_TCHAR(
                            latebrake::BodyKitForVehicle(Entry.VehicleId).Name),
                        UTF8_TO_TCHAR(
                            latebrake::PaintForVehicle(Entry.VehicleId).Name),
                        Entry.Effective.Acceleration, Entry.Effective.Grip,
                        Entry.Effective.TopSpeed));
                    const latebrake::GaragePerformanceComparison Comparison =
                        latebrake::CompareGaragePerformance(Catalog, Entry.VehicleId);
                    if (Comparison.Valid)
                    {
                        MenuLine(FString::Printf(
                            TEXT("   VS EQUIPPED: ACC %+.0f%% | GRIP %+.0f%% | SPEED %+.0f%% (EST)"),
                            Comparison.AccelerationPercent, Comparison.GripPercent,
                            Comparison.TopSpeedPercent));
                    }
                }
            }
            break;
        }
        case latebrake::MenuPage::Results:
        {
            const latebrake::QuickRaceRecordReview PB =
                latebrake::ReviewQuickRaceRecord(
                    Mode->GetSelectedEvent(), Mode->IsCareerRace(),
                    latebrake::CanAwardFinishedRace(Race),
                    Mode->HasProgressSaveError(), Race.GetElapsedSeconds(),
                    Mode->GetAttemptPreviousBestSeconds(),
                    Mode->GetEventBestSeconds());
            switch (PB.State)
            {
            case latebrake::QuickRaceRecordState::SaveFailed:
                MenuLine(TEXT("SAVE FAILED: best time and rewards NOT persisted"), true);
                break;
            case latebrake::QuickRaceRecordState::FirstRecord:
                MenuLine(FString::Printf(TEXT("FIRST SAVED PB: %.2fs"),
                    PB.SavedBestSeconds), true);
                break;
            case latebrake::QuickRaceRecordState::Improved:
                MenuLine(FString::Printf(TEXT("NEW SAVED PB: %.2fs | FASTER %.2fs"),
                    PB.SavedBestSeconds, PB.DeltaSeconds), true);
                break;
            case latebrake::QuickRaceRecordState::Matched:
                MenuLine(FString::Printf(TEXT("MATCHED SAVED PB: %.2fs"),
                    PB.SavedBestSeconds));
                break;
            case latebrake::QuickRaceRecordState::Slower:
                MenuLine(FString::Printf(TEXT("SAVED PB: %.2fs | BEHIND +%.2fs"),
                    PB.SavedBestSeconds, PB.DeltaSeconds));
                break;
            default: break; // No fake records for career or invalid results.
            }
            if (!latebrake::CanAwardFinishedRace(Mode->GetRace()))
            {
                MenuLine(TEXT("FINISH NOT RECORDABLE - missing active timing or invalid clock"), true);
            }
            else
            {
                MenuLine(FString::Printf(TEXT("Last event reward: +%d credits"),
                    Mode->GetLastRaceCredits()));
            }
            // Only the final, timing-eligible Quick Race Circuit checkpoint
            // may snapshot an independently accepted rival route. Never
            // present this as an official rank, winner or credit result.
            switch (Mode->GetLastRivalFinishObservation().State)
            {
            case latebrake::RivalFinishObservationState::RivalRouteAlreadyComplete:
                MenuLine(TEXT("UNOFFICIAL: RIVAL ALREADY CLEARED FINAL GATE"), true);
                break;
            case latebrake::RivalFinishObservationState::RivalStillOnRoute:
                MenuLine(TEXT("UNOFFICIAL: RIVAL ROUTE INCOMPLETE AT FINISH"), true);
                break;
            default: break;
            }
            // This additional result is a sampled post-GO clock margin,
            // not official race placement. Same-frame callbacks are uncertain.
            const latebrake::RivalFinishClockComparison RivalClock =
                Mode->GetLastRivalFinishClockComparison();
            switch (RivalClock.State)
            {
            case latebrake::RivalFinishClockState::RivalEarlierObserved:
                MenuLine(FString::Printf(
                    TEXT("UNOFFICIAL: RIVAL FINAL GATE OBSERVED %.2fs EARLIER"),
                    RivalClock.RivalEarlierSeconds), true);
                break;
            case latebrake::RivalFinishClockState::NearSimultaneousUncertain:
                MenuLine(TEXT("UNOFFICIAL: FINAL GATES TOO CLOSE TO TIME"), true);
                break;
            default: break;
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
            if (Mode->IsExperimentalFleetRequested())
            {
                MenuLine(TEXT("EXPERIMENTAL FLEET | NO SAVED REWARDS OR RANK"),true);
                const auto Review=Mode->GetFleetRaceReview();
                if (!Review.ValidRoster || !Review.PlayerRouteValid)
                    MenuLine(TEXT("Fleet finish observations unavailable"));
                else
                {
                    MenuLine(FString::Printf(TEXT("Verified routes completed: %d/5 at player finish"),
                        Review.ObservedCompletedRoutes));
                    const auto& Traffic=Review.TrafficHistory;
                    if (Traffic.HasSamples && Traffic.ObservedSeconds>0.0)
                    {
                        MenuLine(FString::Printf(TEXT("Traffic sampled %.1fs | alerts %.1fs | projected paths %.1fs"),
                            Traffic.ObservedSeconds,Traffic.AnyWarningSeconds,Traffic.AnyProjectedSeconds));
                        if (Traffic.HasDistance)
                            MenuLine(FString::Printf(TEXT("Closest sampled centres: AI %d | %.1fm | peak alerts %d"),
                                Traffic.NearestObservedSlot,Traffic.MinimumCentreDistanceCm/100.0,
                                Traffic.MaximumConcurrentWarnings));
                    }
                    for(const auto& Row:Review.Rivals)
                    {
                        if (!Row.RouteEvidenceValid)
                        {
                            MenuLine(FString::Printf(TEXT("AI %d | evidence unavailable"),Row.Slot));
                            continue;
                        }
                        FString Status;
                        switch(Row.FinishRelation)
                        {
                        case latebrake::FleetFinishRelation::EarlierObserved:
                            Status=FString::Printf(TEXT("final gate %.2fs earlier (active clock)"),Row.EarlierObservedSeconds);
                            break;
                        case latebrake::FleetFinishRelation::SameClockWindow:
                            Status=TEXT("final gate in uncertain clock window");break;
                        case latebrake::FleetFinishRelation::RouteIncompleteAtPlayerFinish:
                            Status=FString::Printf(TEXT("%d/%d verified gates"),Row.AcceptedGates,
                                Review.RequiredLaps*Review.GateCount);break;
                        default:Status=TEXT("finish comparison unavailable");break;
                        }
                        const auto& Timing=Review.Timing.Rivals[static_cast<std::size_t>(Row.Slot-1)];
                        if (Timing.Availability==latebrake::FleetTimingAvailability::Matched)
                        {
                            if (Timing.Relation==latebrake::FleetTimingRelation::SameClockWindow)
                                Status+=FString::Printf(TEXT(" | G%d close"),Timing.CommonOrdinal);
                            else
                                Status+=FString::Printf(TEXT(" | G%d %+.1fs"),Timing.CommonOrdinal,
                                    Timing.RivalMinusPlayerSeconds);
                        }
                        MenuLine(FString::Printf(TEXT("AI %d | %s | recovery %d/%d verified"),
                            Row.Slot,*Status,Row.SuccessfulRecoveries,Row.RecoveryAttempts));
                    }
                }
            }
            const auto TrackReview=Mode->GetTrackLimitsReview();
            if (TrackReview.Valid)
            {
                MenuLine(FString::Printf(TEXT("TRACK LIMITS: %d VIOLATIONS | %d CHARGES"),
                    TrackReview.Violations,TrackReview.Penalties),true);
                if (TrackReview.Penalties>0 || TrackReview.RecoveryCount>0)
                {
                    MenuLine(FString::Printf(
                        TEXT("Track fees +%.2fs | recovery fees +%.2fs (included)"),
                        TrackReview.TrackLimitsSeconds,TrackReview.RecoverySeconds));
                }
                if (!bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Warnings %d | rejoins %d | refused charges %d"),
                        TrackReview.Warnings,TrackReview.CleanRejoins,TrackReview.RefusedCharges));
                    for (int32 Index=0;Index<TrackReview.RetainedReceipts;++Index)
                    {
                        const auto Receipt=Mode->GetTrackLimits().Recent(Index);
                        if (Receipt.Event!=latebrake::TrackLimitsEvent::ViolationRecorded &&
                            Receipt.Event!=latebrake::TrackLimitsEvent::PenaltyApplied &&
                            Receipt.Event!=latebrake::TrackLimitsEvent::PenaltyRefused)
                            continue;
                        MenuLine(FString::Printf(
                            TEXT("Lap %d | next gate %d | clock %.2fs | fee +%.2fs"),
                            Receipt.Lap,Receipt.NextGate+1,Receipt.RaceSeconds,
                            Receipt.ActualChargedSeconds));
                    }
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
                    if (Row.PenaltyAttributionKnown &&
                        Row.IncludedPenaltySeconds > 0.0)
                    {
                        MenuLine(FString::Printf(
                            TEXT("  FEE +%.2fs INCLUDED | CLOCK EXCL. FEE %.2fs"),
                            Row.IncludedPenaltySeconds,
                            Row.ClockExcludingPenaltySeconds));
                    }
                    if (!bCompact && Row.HasPrevious)
                    {
                        MenuLine(FString::Printf(
                            TEXT("  LAP %d vs prior timed: %+.2fs"),
                            Row.CompletedLap, Row.DeltaToPreviousSeconds));
                        if (Row.HasAdjacentPenaltyComparison &&
                            Row.PenaltyDeltaToPreviousSeconds != 0.0)
                        {
                            MenuLine(FString::Printf(
                                TEXT("  CLOCK EXCL. FEE %+.2fs | FEE CHANGE %+.2fs"),
                                Row.ClockDeltaToPreviousSeconds,
                                Row.PenaltyDeltaToPreviousSeconds));
                        }
                    }
                }
            }
            if (LapReview.Valid && LapReview.CompletePenaltyAttribution)
            {
                MenuLine(FString::Printf(
                    TEXT("%s: %d FEE-FREE / %d PENALIZED LAPS"),
                    LapReview.CoversEntireRace ? TEXT("ALL LAPS") : TEXT("RETAINED LAPS"),
                    LapReview.PenaltyFreeLaps, LapReview.PenalizedLaps));
                if (LapReview.PenalizedLaps > 0)
                {
                    MenuLine(FString::Printf(
                        TEXT("RETAINED FEES +%.2fs | MOST ON LAP %d (+%.2fs)"),
                        LapReview.RetainedPenaltySeconds,
                        LapReview.LargestPenaltyLap,
                        LapReview.LargestPenaltySeconds));
                    MenuLine(TEXT("FEE-EXCLUDED CLOCK IS NOT A LAP RECORD."), true);
                }
            }
            // Compare the final physical lap against a prior, actually
            // completed fee-free reference. Never substitute mixed best splits.
            const auto& Measured = Race.GetMeasuredLapSplits().LastComparison();
            if (latebrake::CanAwardFinishedRace(Race) && Measured.Valid)
            {
                MenuLine(FString::Printf(
                    TEXT("FINAL LAP %d VS FEE-FREE LAP %d: %+.2fs"),
                    Measured.LapNumber, Measured.ReferenceLapNumber,
                    Measured.DeltaSeconds), true);
                MenuLine(FString::Printf(
                    TEXT("GATES FASTER %d | MATCHED %d | SLOWER %d"),
                    Measured.FasterGates, Measured.MatchedGates,
                    Measured.SlowerGates));
                if (Measured.LargestLossGate >= 0)
                    MenuLine(FString::Printf(TEXT("BIGGEST LOSS GATE %d: +%.2fs"),
                        Measured.LargestLossGate+1, Measured.LargestLossSeconds));
                if (Measured.LargestGainGate >= 0)
                    MenuLine(FString::Printf(TEXT("BIGGEST GAIN GATE %d: -%.2fs"),
                        Measured.LargestGainGate+1, Measured.LargestGainSeconds));
                if (!bCompact)
                {
                    for (int32 Index=0; Index<Measured.GateCount; ++Index)
                    {
                        const auto& Row = Measured.Rows[static_cast<std::size_t>(Index)];
                        MenuLine(FString::Printf(
                            TEXT("GATE %d: SPLIT %+.2fs | TOTAL %+.2fs | FEE +%.2fs"),
                            Index+1, Row.SectorDeltaSeconds,
                            Row.CumulativeDeltaSeconds, Row.IncludedFeeSeconds));
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

            // Fee-free physical laps, not adjusted versions of penalized
            // times. Keep the retained-window scope explicit in Results.
            const auto CleanPace = latebrake::BuildCleanLapPaceReview(Race);
            if (CleanPace.Valid)
            {
                MenuLine(FString::Printf(
                    TEXT("FEE-FREE LAPS: %d/%d RETAINED"),
                    CleanPace.CleanLaps, CleanPace.RetainedLaps), true);
                MenuLine(FString::Printf(
                    TEXT("Fastest fee-free lap %d: %.2fs"),
                    CleanPace.FastestLap, CleanPace.FastestSeconds));
                if (CleanPace.HasVariation && !bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Fee-free median %.2fs | median deviation %.2fs"),
                        CleanPace.MedianSeconds,
                        CleanPace.MedianAbsoluteDeviationSeconds));
                }
                if (CleanPace.HasAdjacentComparison && !bCompact)
                {
                    MenuLine(FString::Printf(
                        TEXT("Fee-free lap %d vs %d: %+.2fs"),
                        CleanPace.AdjacentNewerLap, CleanPace.AdjacentOlderLap,
                        CleanPace.AdjacentDeltaSeconds));
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
                    const auto& Distribution=DriveReview.SpeedDistribution;
                    if (Distribution.Valid)
                    {
                        MenuLine(FString::Printf(
                            TEXT("Average sampled speed %.1f km/h"),
                            Distribution.AverageSpeedKmH));
                        if (!bCompact)
                        {
                            const auto Median=static_cast<std::size_t>(Distribution.MedianBand);
                            const auto P90=static_cast<std::size_t>(Distribution.P90Band);
                            const auto& Edges=latebrake::SampledSpeedEdgesKmH;
                            MenuLine(FString::Printf(
                                TEXT("Time-weighted median %.0f-%.0f | P90 %.0f-%.0f km/h"),
                                Edges[Median],Edges[Median+1],Edges[P90],Edges[P90+1]));
                            for (std::size_t Index=0;Index<Distribution.Rows.size();++Index)
                            {
                                const auto& Row=Distribution.Rows[Index];
                                if (Row.Frames==0) continue;
                                MenuLine(FString::Printf(
                                    TEXT("%.0f-%.0f km/h | %.1fs | %.0f%% of sample"),
                                    Edges[Index],Edges[Index+1],Row.Seconds,
                                    100.0*Row.Seconds/Distribution.TotalSeconds));
                            }
                        }
                    }
                }
            }
            if (latebrake::CanAwardFinishedRace(Race))
            {
                const auto PitSummary = Car->GetPitJournal().Review(
                    Car->GetPitService().StopCount());
                if (PitSummary.Valid)
                {
                    MenuLine(FString::Printf(TEXT("PIT SERVICE | %d COMPLETED"),
                        PitSummary.Count), true);
                    MenuLine(FString::Printf(
                        TEXT("Damage -%.1f pp | wear -%.1f pp | boost +%.1f pp"),
                        PitSummary.DamageRestored*100.0,
                        PitSummary.WearRemoved*100.0,
                        PitSummary.ChargeRestored*100.0));
                    if (!bCompact)
                    {
                        const auto& Journal=Car->GetPitJournal();
                        for (int32 Index=0; Index<Journal.RetainedCount(); ++Index)
                        {
                            const auto Receipt=Journal.GetRecent(Index);
                            MenuLine(FString::Printf(
                                TEXT("STOP %d | LAP %d | CLOCK %.2fs"),
                                Receipt.Stop, Receipt.Lap, Receipt.RaceSeconds));
                            MenuLine(FString::Printf(
                                TEXT("Damage -%.1f pp | wear -%.1f pp | boost +%.1f pp"),
                                Receipt.DamageRestored*100.0, Receipt.WearRemoved*100.0,
                                Receipt.ChargeRestored*100.0));
                        }
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
            MenuLine(Mode->IsDuskLightingPreview()
                ? TEXT("LIGHTING: DUSK (SESSION PREVIEW ONLY)")
                : TEXT("LIGHTING: DAY (SESSION PREVIEW ONLY)"), true);
            MenuLine(TEXT("F3: TOGGLE DAY/DUSK | NOT SAVED | VISUAL ONLY"));
#if !UE_BUILD_SHIPPING
            // Real pawn component/material/light-request state. This is a
            // diagnostic of source commands, NOT proof of rendered pixels,
            // physically accurate light or adequate frame rate.
            const auto VisualReport = latebrake::ReportVisualPlaytest(
                true, Car->IsOriginalVehicleSurfaceCommitted(),
                !Car->GetOriginalVehicleTintParameter().IsNone(),
                Car->AreFrontLampsRequestedOn(),
                Mode->IsDuskLightingPreview(),
                Car->GetCommittedOriginalWheelParts());
            MenuLine(FString::Printf(TEXT("LIVE CAR MESH: %s"),
                UTF8_TO_TCHAR(latebrake::VisualPlaytestSurfaceLabel(
                    VisualReport.Surface))),true);
            MenuLine(FString::Printf(TEXT("PAINT PARAM: %s"),
                VisualReport.PaintParameterDiscovered
                    ? *Car->GetOriginalVehicleTintParameter().ToString()
                    : TEXT("NONE / FALLBACK")));
            const int32 FinishCount=Car->GetOriginalVehicleFinishParameterCount();
            MenuLine(FString::Printf(TEXT("FINISH PARAMETERS: %d/5 EXPOSED | %s"),
                FinishCount,FinishCount>0?TEXT("VERIFY SHADER"):TEXT("TINT ONLY")));
            MenuLine(FString::Printf(TEXT("FRONT BEAMS REQUESTED: %s"),
                VisualReport.FrontLampsRequested ? TEXT("ON")
                    : TEXT("OFF (PAUSED OR INACTIVE)")));
            MenuLine(FString::Printf(
                TEXT("3D WHEEL RIG: %d/4 COMPONENTS (NOT PIXELS)"),
                VisualReport.AnimatedWheelPartsCommitted));
            const auto BuildStats=Car->GetOriginalMeshBuildTelemetry();
            MenuLine(FString::Printf(
                TEXT("3D SOURCE BUILDS: %u ATTEMPTS / %u READY"),
                BuildStats.Attempts, BuildStats.Committed));
            const auto CacheStats=Car->GetOriginalRenderCacheStats();
            MenuLine(FString::Printf(
                TEXT("CPU MESH CACHE: %u HIT / %u MISS / %u PREPARED"),
                CacheStats.Hits, CacheStats.Misses, CacheStats.Prepared));
            MenuLine(FString::Printf(
                TEXT("CACHE: %d/%d RESIDENT | %u EVICTIONS | %u FAILED"),
                static_cast<int32>(CacheStats.ResidentEntries),
                static_cast<int32>(latebrake::vehiclemesh::OriginalRenderCacheCapacity),
                CacheStats.Evictions, CacheStats.Failed));
            MenuLine(FString::Printf(
                TEXT("CPU CACHE: %d VERTICES / %d INDICES (NOT VRAM)"),
                static_cast<int32>(CacheStats.ResidentVertices),
                static_cast<int32>(CacheStats.ResidentIndices)));
            const int32 CommittedRim=Car->GetCommittedOriginalRimStyle();
            MenuLine(FString::Printf(TEXT("LIVE 3D RIM: %s"),
                CommittedRim>=0 && CommittedRim<latebrake::GarageWheelOptions
                    ? UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(
                        latebrake::GarageLookLayer::Wheels,CommittedRim))
                    : TEXT("FALLBACK / NO SOURCE MESH")));
            MenuLine(TEXT("SOURCE DIAGNOSTIC ONLY - VERIFY PIXELS IN ENGINE"));
#endif
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
        // Desktop menu dock: shared enabled/disabled state also gates the
        // actual mouse click dispatcher. Empty, first and last detail
        // pages cannot advertise working scroll arrows.
        if (MouseMenu.Valid)
        {
            const auto MenuAvailability =
                latebrake::AvailableDesktopMenuActions(
                    Mode->GetMenuPage(), MenuPager,
                    Mode->CanEquipGarageViewed());
            float CursorX = 0.f;
            float CursorY = 0.f;
            const bool HasCursor = PlayerOwner->GetMousePosition(
                CursorX, CursorY);
            const latebrake::DesktopMenuMouseAction Hovered = HasCursor
                ? HitTestMenuMouse(CursorX, CursorY)
                : latebrake::DesktopMenuMouseAction::None;
            const auto& FirstButton = MouseMenu.Buttons[0].Bounds;
            DrawText(FString::Printf(TEXT("MENU PAGE %d/%d"),
                static_cast<int32>(Mode->GetMenuPage()),
                static_cast<int32>(latebrake::MenuPageNames.size()) - 1),
                Gold, static_cast<float>(FirstButton.X),
                static_cast<float>(FirstButton.Y - 24.0 * Layout.Scale),
                GEngine->GetSmallFont(), 0.88f * UIScale);
            // A discoverable mouse hint is kept below, not over,
            // the actual clickable rectangles in the right-hand dock.
            const auto& LastButton = MouseMenu.Buttons[5].Bounds;
            DrawText(TEXT("WHEEL: DETAILS"), White,
                static_cast<float>(MouseMenu.Left),
                static_cast<float>(LastButton.Y + LastButton.Height +
                    9.0 * Layout.Scale),
                GEngine->GetSmallFont(), 0.82f * UIScale);
            for (const auto& Button : MouseMenu.Buttons)
            {
                const float BX = static_cast<float>(Button.Bounds.X);
                const float BY = static_cast<float>(Button.Bounds.Y);
                const float BW = static_cast<float>(Button.Bounds.Width);
                const float BH = static_cast<float>(Button.Bounds.Height);
                const bool Enabled =
                    latebrake::DesktopMenuMouseActionEnabled(
                        Button.Action, MenuAvailability);
                const bool Selected = Enabled && Button.Action ==
                    latebrake::DesktopMenuMouseAction::Activate;
                const bool Hover = Enabled && Button.Action == Hovered;
                // Disabled detail controls stay visible, with readable
                // labels but no hover highlight or live hit target.
                const FLinearColor DisabledLabel(0.62f, 0.68f, 0.75f);
                DrawRect(!Enabled
                    ? FLinearColor(0.07f, 0.10f, 0.14f, 0.84f)
                    : Hover
                        ? FLinearColor(0.42f, 0.30f, 0.10f, 0.95f)
                        : (Selected
                            ? FLinearColor(0.27f, 0.20f, 0.09f, 0.9f)
                            : FLinearColor(0.08f, 0.13f, 0.20f, 0.9f)),
                    BX, BY, BW, BH);
                DrawRect(!Enabled ? DisabledLabel :
                    Selected ? Gold : White, BX, BY,
                    3.f * UIScale, BH);
                FString Caption(TEXT("CLOSE"));
                switch (Button.Action)
                {
                case latebrake::DesktopMenuMouseAction::PreviousPage:
                    Caption = TEXT("PREV PAGE"); break;
                case latebrake::DesktopMenuMouseAction::NextPage:
                    Caption = TEXT("NEXT PAGE"); break;
                case latebrake::DesktopMenuMouseAction::DetailsUp:
                    Caption = TEXT("DETAIL UP"); break;
                case latebrake::DesktopMenuMouseAction::DetailsDown:
                    Caption = TEXT("DETAIL DOWN"); break;
                case latebrake::DesktopMenuMouseAction::Activate:
                    Caption = Mode->GetMenuPage() == latebrake::MenuPage::Garage
                        ? FString(UTF8_TO_TCHAR(
                            latebrake::GarageEquipUiCaption(
                                Mode->GetGarageEquipUiState())))
                        : FString(TEXT("SELECT"));
                    break;
                case latebrake::DesktopMenuMouseAction::Close:
                    Caption = TEXT("CLOSE"); break;
                default: break;
                }
                DrawButtonCaption(Caption, FString(), Button.Bounds,
                    !Enabled ? DisabledLabel : Selected ? Gold : White,
                    0.82f * UIScale, 0.0);
            }
            // Only the Garage shows separate clickable cosmetic actions.
            // Uses exactly the same bounded rectangles as mouse hit tests;
            // these buttons do not have economy or race authority.
            if (GarageStyleMenu.Valid)
            {
                const bool bCanEditLook = Mode->CanCustomizeGarage();
                const bool bCanBrowse = Mode->CanBrowseGarage();
                const int32 PreviewVehicle = Mode->GetGaragePreviewVehicleId();
                const latebrake::GarageLook PreviewLook =
                    Mode->GetGaragePreviewLook();
                const latebrake::VehiclePaintRecipe StylePaint =
                    latebrake::PaintForGarageLook(PreviewVehicle, PreviewLook);
                const latebrake::DesktopMenuMouseAction StyleHover =
                    HasCursor ? HitTestMenuMouse(CursorX, CursorY)
                        : latebrake::DesktopMenuMouseAction::None;
                for (const auto& StyleButton : GarageStyleMenu.Buttons)
                {
                    const float BX = static_cast<float>(StyleButton.Bounds.X);
                    const float BY = static_cast<float>(StyleButton.Bounds.Y);
                    const float BW = static_cast<float>(StyleButton.Bounds.Width);
                    const float BH = static_cast<float>(StyleButton.Bounds.Height);
                    const bool bBrowseAction =
                        StyleButton.Action ==
                            latebrake::DesktopMenuMouseAction::GarageBrowsePrevious ||
                        StyleButton.Action ==
                            latebrake::DesktopMenuMouseAction::GarageBrowseNext;
                    const bool Enabled = bBrowseAction
                        ? bCanBrowse : bCanEditLook;
                    const bool Hover = Enabled &&
                        StyleButton.Action == StyleHover;
                    const FLinearColor TextColor = !Enabled
                        ? FLinearColor(0.61f, 0.66f, 0.73f)
                        : Hover ? Gold : White;
                    DrawRect(!Enabled
                        ? FLinearColor(0.07f, 0.10f, 0.14f, 0.84f)
                        : Hover
                            ? FLinearColor(0.42f, 0.30f, 0.10f, 0.95f)
                            : FLinearColor(0.08f, 0.13f, 0.20f, 0.9f),
                        BX, BY, BW, BH);
                    DrawRect(TextColor, BX, BY, 3.f * UIScale, BH);
                    const auto Indicator =
                        latebrake::DescribeGarageStyleButton(
                            StyleButton.Action, PreviewVehicle, PreviewLook);
                    // The 1-based option counter updates immediately on
                    // the viewed car, including an owned parked car.
                    // Factory state is explicit, not another cycle slot.
                    if (!Indicator.Valid) continue;
                    const FString Caption = Indicator.Total > 0
                        ? FString::Printf(TEXT("%s %d/%d"),
                            UTF8_TO_TCHAR(Indicator.Caption),
                            Indicator.Ordinal, Indicator.Total)
                        : FString(UTF8_TO_TCHAR(Indicator.Caption));
                    const bool bPaint = StyleButton.Action ==
                        latebrake::DesktopMenuMouseAction::GaragePaint;
                    const float Swatch = FMath::Clamp(
                        12.f * UIScale, 8.f, 14.f);
                    DrawButtonCaption(Caption, FString(), StyleButton.Bounds,
                        TextColor, 0.82f * UIScale,
                        bPaint ? Swatch + 6.f * UIScale : 0.0);
                    if (bPaint)
                    {
                        // A bounded preview swatch is visual-only: it is
                        // inside the existing hit rectangle, with no new
                        // pointer target, material allocation or input.
                        const auto Rgb = StylePaint.Body;
                        DrawRect(FLinearColor(
                            static_cast<float>(Rgb.R),
                            static_cast<float>(Rgb.G),
                            static_cast<float>(Rgb.B), 1.f),
                            BX + BW - Swatch - 6.f * UIScale,
                            BY + 0.5f * (BH - Swatch),
                            Swatch, Swatch);
                    }
                }

                // Grounded SCREEN-SPACE TOP VIEW of the browsed prototype.
                // It uses the same six vehicle geometry/paint recipes as
                // the source Pawn. Unlike a 3D car actor, this costs no
                // physics, asset loads, capture camera or saved ownership.
                const auto Diagram = latebrake::CalculateGarageSchematic(
                    static_cast<double>(Canvas->SizeX),
                    static_cast<double>(Canvas->SizeY),
                    Layout, GarageStyleMenu, PreviewVehicle, PreviewLook);
                if (Diagram.Valid)
                {
                    const auto& Frame = Diagram.Panel;
                    const float FrameX = static_cast<float>(Frame.X);
                    const float FrameY = static_cast<float>(Frame.Y);
                    const float FrameW = static_cast<float>(Frame.Width);
                    const float FrameH = static_cast<float>(Frame.Height);
                    DrawRect(FLinearColor(0.045f, 0.085f, 0.13f, 0.97f),
                        FrameX, FrameY, FrameW, FrameH);
                    DrawRect(Gold, FrameX, FrameY,
                        FrameW, 2.f * Layout.Scale);
                    // Read-only numbered vehicle selectors in the
                    // reserved 27px title region of the actual diagram.
                    const auto QuickView =
                        latebrake::CalculateGarageQuickViewLayout(Diagram);
                    const auto QuickSnapshot = Mode->GetGarageSnapshot();
                    float PickX=0.f,PickY=0.f;
                    const bool bHasPickCursor=PlayerOwner->GetMousePosition(
                        PickX,PickY);
                    const auto HoverPick=bHasPickCursor
                        ? HitTestMenuMouse(PickX, PickY)
                        : latebrake::DesktopMenuMouseAction::None;
                    if (QuickView.Valid)
                    {
                        for(std::size_t i=0;i<QuickView.Buttons.size();++i)
                        {
                            const auto& Button=QuickView.Buttons[i];
                            const float BX=static_cast<float>(Button.Bounds.X);
                            const float BY=static_cast<float>(Button.Bounds.Y);
                            const float BW=static_cast<float>(Button.Bounds.Width);
                            const float BH=static_cast<float>(Button.Bounds.Height);
                            const bool bViewed=static_cast<int32>(i)==PreviewVehicle;
                            const bool bEquipped=static_cast<int32>(i)==
                                Mode->GetSelectedVehicleId();
                            const bool bOwned=(QuickSnapshot.OwnedVehicleMask &
                                (1 << static_cast<int32>(i))) != 0;
                            const bool bHovered=HoverPick==Button.Action;
                            const FLinearColor Ink=bViewed ? Gold :
                                bEquipped ? FLinearColor(0.21f,0.85f,0.94f) :
                                bOwned ? White : FLinearColor(0.49f,0.55f,0.63f);
                            DrawRect(bViewed
                                ? FLinearColor(0.31f,0.25f,0.09f,1.f)
                                : bHovered
                                    ? FLinearColor(0.19f,0.31f,0.43f,1.f)
                                    : FLinearColor(0.08f,0.13f,0.19f,1.f),
                                BX,BY,BW,BH);
                            DrawRect(Ink,BX,BY,BW,2.f);
                            DrawButtonCaption(FString::FromInt(static_cast<int32>(i)+1),
                                FString(), Button.Bounds, Ink, 0.78f * UIScale, 0.0);
                        }
                    }
                    DrawText(TEXT("CAR TOP VIEW | 2D SCHEMATIC"), Gold,
                        FrameX + 8.f * Layout.Scale,
                        FrameY + FrameH - 14.f * Layout.Scale,
                        GEngine->GetSmallFont(), 0.63f * UIScale);

                    const latebrake::VehiclePaintRecipe Colors =
                        latebrake::PaintForGarageLook(
                            PreviewVehicle, PreviewLook);
                    const auto Convert = [](latebrake::VehiclePaintRgb rgb)
                    {
                        return FLinearColor(
                            static_cast<float>(rgb.R),
                            static_cast<float>(rgb.G),
                            static_cast<float>(rgb.B), 1.f);
                    };
                    // The schematic is deliberately reliable even when a
                    // built-in primitive's Unreal material has no editable
                    // "Color" parameter. Real 3D material QA is separate.
                    for (std::size_t i = 0; i < Diagram.Count; ++i)
                    {
                        const auto& Piece = Diagram.Pieces[i];
                        FLinearColor Ink(0.07f, 0.10f, 0.15f, 0.92f);
                        switch (Piece.Ink)
                        {
                        case latebrake::GarageSchematicInk::Shadow:
                            Ink = FLinearColor(0.f, 0.f, 0.f, 0.45f);
                            break;
                        case latebrake::GarageSchematicInk::Tire:
                            Ink = FLinearColor(0.06f, 0.07f, 0.09f, 1.f);
                            break;
                        case latebrake::GarageSchematicInk::Rim:
                            Ink = Convert(latebrake::RimTintForGarageLook(PreviewLook));
                            break;
                        case latebrake::GarageSchematicInk::Body:
                            Ink = Convert(Colors.Body);
                            break;
                        case latebrake::GarageSchematicInk::Accent:
                            Ink = Convert(Colors.Accent);
                            break;
                        case latebrake::GarageSchematicInk::Canopy:
                            Ink = Convert(Colors.Canopy);
                            break;
                        case latebrake::GarageSchematicInk::Stripe:
                            Ink = Convert(latebrake::StripeTintForGarageLook(PreviewLook));
                            break;
                        default:
                            continue;
                        }
                        DrawRect(Ink,
                            static_cast<float>(Piece.Bounds.X),
                            static_cast<float>(Piece.Bounds.Y),
                            static_cast<float>(Piece.Bounds.Width),
                            static_cast<float>(Piece.Bounds.Height));
                    }
                }
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
        StuckHint.Reset();
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
    const FString SpeedLine = Mode->UsesMilesPerHour()
        ? FString::Printf(TEXT("SPEED  %.0f mph"), Car->GetSpeedKmH() * 0.621371f)
        : FString::Printf(TEXT("SPEED  %.0f km/h"), Car->GetSpeedKmH());
    // One existing priority row; no extra line competes with Android controls
    // or authoritative time/checkpoint status on narrow viewports.
    // Keep boost meter in the speed row; compact touch layouts preserve
    // their existing gate/time priority with no new optional overlays.
    // The policy reads actual charge, heat, cooldown and signed forward speed
    // without touching boost physics, checkpoint timing, or player controls.
    // A held key/charged tank alone must never be displayed as immediately
    // usable when the game is paused, finished or track isn't ready.
    const latebrake::NitroHudReadout BoostStatus = latebrake::ReadNitroHud(
        Car->GetNitroBoost(), static_cast<double>(Car->GetForwardSpeedKmH()),
        !Mode->IsMenuOpen() && !Mode->IsManualRacePauseActive() &&
        !UGameplayStatics::IsGamePaused(this) &&
        Mode->IsCircuitReadyForGameplay() && !Race.IsFinished());
    FString BoostStatusText;
    switch (BoostStatus.Phase)
    {
    case latebrake::NitroHudPhase::Active:
        BoostStatusText = TEXT("ACTIVE"); break;
    case latebrake::NitroHudPhase::Cooldown:
        BoostStatusText = FString::Printf(
            TEXT("COOLDOWN %.1fs"), BoostStatus.CooldownSeconds); break;
    case latebrake::NitroHudPhase::Overheated:
        BoostStatusText = TEXT("OVERHEAT"); break;
    case latebrake::NitroHudPhase::Empty:
        BoostStatusText = TEXT("EMPTY"); break;
    case latebrake::NitroHudPhase::BuildSpeed:
        BoostStatusText = TEXT("BUILD SPEED"); break;
    case latebrake::NitroHudPhase::Stored:
        BoostStatusText = TEXT("STORED"); break;
    default:
        BoostStatusText = TEXT("UNAVAILABLE"); break;
    }
    const double RawBoostCharge = Car->GetNitroBoost().ChargeFraction();
    const double DisplayBoostCharge = FMath::IsFinite(RawBoostCharge)
        ? FMath::Clamp(RawBoostCharge, 0.0, 1.0) : 0.0;
    const FString NitroLabel = !bCompact
        ? FString::Printf(TEXT(" | NITRO %s %.0f%%"),
            *BoostStatusText, DisplayBoostCharge * 100.0)
        : FString();
    Line((Car->IsHandbrakeHeld() && !bCompact
        ? SpeedLine + TEXT(" | HANDBRAKE") : SpeedLine) + NitroLabel, Primary);

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
    // Optional, clearly ESTIMATED automatic gearbox telemetry on desktop.
    // Preserve gate/time rows and mobile touch controls as first priority.
    if (!bCompact && !bTouchHUD &&
        VerticalPlan.MaxTelemetryLines >= 24)
    {
        const latebrake::AutomaticTransmissionState Gear =
            Car->GetTransmissionReadout();
        if (Gear.Valid)
        {
            const FString GearLabel = Gear.Gear < 0
                ? FString(TEXT("R"))
                : FString::Printf(TEXT("%d"), Gear.Gear);
            Line(FString::Printf(
                TEXT("AUTO GEAR %s | RPM EST %d%s"),
                *GearLabel, Gear.EstimatedRpm,
                Gear.Shifting ? TEXT(" | SHIFT") : TEXT("")),
                Gear.Shifting ? Accent : Primary);
        }
    }
    // Saved solo PB is a deadline for official elapsed clock, not predicted
    // finish. No new touch HUD row may compete with steering/gate controls.
    if (!bCompact && !bTouchHUD)
    {
        const bool bRunning =
            Mode->IsCircuitReadyForGameplay() &&
            Mode->IsRestartPlacementReady() &&
            Mode->HasLocalPlayerDriver() && Mode->IsRaceStartGreen() &&
            !Mode->IsMenuOpen() && !Mode->IsFocusLossGuardActive() &&
            !Mode->IsManualRacePauseActive() &&
            !UGameplayStatics::IsGamePaused(this);
        const latebrake::TimeAttackPersonalBestTarget PbTarget =
            latebrake::MeasureTimeAttackPersonalBestTarget(
                Mode->GetSelectedEvent(), Mode->IsCareerRace(), bRunning,
                Race.IsFinished(), Race.GetElapsedSeconds(),
                Mode->GetEventBestSeconds());
        if (PbTarget.Valid)
        {
            Line(PbTarget.DeadlineReached
                ? FString::Printf(TEXT("PB CLOCK OVER  +%.2fs  |  TARGET %.2fs"),
                    PbTarget.SecondsOver, PbTarget.SavedBestSeconds)
                : FString::Printf(TEXT("PB CLOCK  %.2fs LEFT  |  TARGET %.2fs"),
                    PbTarget.SecondsRemaining, PbTarget.SavedBestSeconds),
                Accent);
        }
    }
    // Help a human recover from a possible obstacle deadlock: four seconds
    // of qualified forward throttle with almost no physical movement.
    // A separate read-only observation NEVER recovers or charges the car.
    // The real R/RECOVER action still enforces its placement/penalty rules.
    if (LastWayfindingPawn.Get() != Car ||
        LastWayfindingSession != Mode->GetCheckpointSessionRevision() ||
        LastWayfindingTeleport != Car->GetTeleportRevision())
    {
        StuckHint.Reset();
    }
    const FVector RecoveryHintPosition = Car->GetActorLocation();
    const bool bRecoveryHintEligible =
        Mode->IsCircuitReadyForGameplay() &&
        Mode->IsRestartPlacementReady() &&
        Mode->HasLocalPlayerDriver() && Mode->IsRaceStartGreen() &&
        Car->IsRacePhysicsReady() && !Race.IsFinished() &&
        !Mode->IsFocusLossGuardActive() && !Mode->IsManualRacePauseActive() &&
        !UGameplayStatics::IsGamePaused(this);
    const bool bPossiblyStuck = StuckHint.Observe({
        static_cast<double>(GetWorld()->GetDeltaSeconds()),
        static_cast<double>(RecoveryHintPosition.X),
        static_cast<double>(RecoveryHintPosition.Y),
        static_cast<double>(Car->GetSpeedKmH()) / 0.036,
        Car->IsAttemptingForwardDrive(), bRecoveryHintEligible});
    const auto RecoveryNotice=bRecoveryHintEligible
        ? Mode->GetRecoveryNotice() : latebrake::PlayerRecoveryNotice::None;
    switch (RecoveryNotice)
    {
    case latebrake::PlayerRecoveryNotice::Occupied:
        Line(TEXT("RECOVER BLOCKED: RIVAL AT LANDING | NO FEE"), Accent);
        break;
    case latebrake::PlayerRecoveryNotice::TrafficUnavailable:
        Line(TEXT("RECOVER BLOCKED: TRAFFIC UNKNOWN | NO FEE"), Accent);
        break;
    case latebrake::PlayerRecoveryNotice::LandingUnavailable:
        Line(TEXT("RECOVER BLOCKED: LANDING UNAVAILABLE | NO FEE"), Accent);
        break;
    case latebrake::PlayerRecoveryNotice::FeeUnavailable:
        Line(TEXT("RECOVER BLOCKED: CLOCK UNAVAILABLE | NO FEE"), Accent);
        break;
    case latebrake::PlayerRecoveryNotice::PlacementUnverified:
        Line(TEXT("RECOVER FAILED: PLACEMENT UNVERIFIED | NO FEE"), Accent);
        break;
    case latebrake::PlayerRecoveryNotice::VerifiedAndCharged:
        Line(TEXT("RECOVER VERIFIED | +5s INCLUDED IN TIME"), Primary);
        break;
    default:
        if (bPossiblyStuck)
        {
            Line(bTouchHUD ? TEXT("NO MOTION? TAP RECOVER (+5s)")
                           : TEXT("NO MOTION? PRESS R TO RECOVER (+5s)"), Accent);
        }
        break;
    }
    // A genuinely guarded race-start sequence. Countdowns take a bounded
    // priority row only when track, physical player and grid are ready.
    // The authoritative clock / gate / physics / rival policies all use
    // this same GameMode start light (not an independent HUD timer).
    if (Mode->IsExperimentalFleetRequested() && !Mode->IsFleetStartPresentationReady() &&
        !Race.IsFinished())
    {
        const auto Fleet=Mode->GetFleetAttemptReadout();
        const auto Startup=Mode->GetFleetStartupReadout();
        if (Fleet.Phase==latebrake::FleetAttemptPhase::Faulted)
            Line(FString::Printf(TEXT("FLEET: %s | RESTART"),
                UTF8_TO_TCHAR(latebrake::FleetAttemptFaultName(Fleet.Fault))),Accent,true);
        else if (Fleet.Phase!=latebrake::FleetAttemptPhase::Running)
        {
            const char* Detail=Startup.Cause==latebrake::FleetStartupCause::UnsafeGrid
                ? latebrake::FleetGridFailureName(Startup.Grid.Failure)
                : latebrake::FleetStartupCauseName(Startup.Cause);
            if (Startup.Phase==latebrake::FleetStartupPhase::Settling)
                Detail="verifying settled grid";
            Line(FString::Printf(TEXT("GRID: %s | %.0fs left"),
                UTF8_TO_TCHAR(Detail),Startup.RemainingSeconds),Accent,true);
            if (Startup.Grid.Slot>=0 && Startup.Cause==latebrake::FleetStartupCause::UnsafeGrid)
                Line(FString::Printf(TEXT("Check grid slot %d"),Startup.Grid.Slot+1),Primary);
        }
        else Line(TEXT("FLEET DRIVE SUSPENDED"),Accent,true);
    }
    else if (Mode->IsCircuitReadyForGameplay() &&
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

    // Measured in-game stopping projection, only while consecutive
    // validated chassis samples show sustained forward deceleration.
    // This is observational and never drives physics, rewards or timing.
    if (!bCompact && Car->GetObservedBrakeDistance().HasLiveEstimate())
    {
        const double BrakeMeters =
            Car->GetObservedBrakeDistance().GetDistanceMeters();
        Line(Mode->UsesMilesPerHour()
            ? FString::Printf(TEXT("BRAKE DIST EST %.0f ft (CURRENT RATE)"),
                BrakeMeters * 3.280839895)
            : FString::Printf(TEXT("BRAKE DIST EST %.0f m (CURRENT RATE)"),
                BrakeMeters), Primary);
    }

    // Read-only chassis traction diagnostic uses ACTUAL accepted WorldStatic
    // traces, not the XY road mask or assumed decorative wheel contacts.
    // Keep compact mobile and short screens focused on race/lap timing.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28)
    {
        const latebrake::ChassisContactReadout Contact =
            Car->GetGroundContactReadout();
        if (Contact.Live)
        {
            const TCHAR* GroundName =
                Contact.Status == latebrake::ChassisContactStatus::Full
                    ? TEXT("ROAD")
                    : Contact.Status == latebrake::ChassisContactStatus::Partial
                    ? TEXT("PARTIAL") : TEXT("AIRBORNE");
            Line(FString::Printf(
                TEXT("GROUND %s  %d/4 | TRACTION %d%%"),
                GroundName, Contact.QualifiedProbes,
                Contact.DriveTractionPercent),
                Contact.Status == latebrake::ChassisContactStatus::Full
                    ? Primary : Accent);
        }
    }

    // Prototype chassis-based tire condition: no tire sensor or certified
    // physical compound model. Preserve compact mobile gate/time priority.
    if (!bCompact && Car->GetTireCondition().HasValidSample())
    {
        Line(FString::Printf(TEXT("ARCADE GRIP %.0f%% | TIRE WEAR %.0f%%"),
            Car->GetTireCondition().GripMultiplier() * 100.0,
            Car->GetTireCondition().WearFraction() * 100.0), Primary);
    }
    // Optional human Arcade stability hint reports the actual bounded
    // throttle reduction from the last accepted live physical frame.
    // Keep compact and low-height HUDs reserved for lap/time/touch controls.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Car->GetSpinAssist().IsActive())
    {
        Line(FString::Printf(
            TEXT("SPIN ASSIST  %.0f%% POSITIVE THROTTLE TRIM"),
            Car->GetSpinAssist().CurrentReductionFraction() * 100.0),
            Accent);
    }
    // Optional arcade brake-temperature feedback uses measured service
    // brake demand and actual chassis velocity, not decorative wheel heat.
    // Keep compact mobile HUD space reserved for lap and input controls.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Car->GetBrakeHeat().HasValidSample() &&
        Car->GetBrakeHeat().HeatFraction() >= 0.60)
    {
        Line(FString::Printf(TEXT("BRAKE HEAT %.0f%% | SERVICE BRAKE %.0f%%"),
            Car->GetBrakeHeat().HeatFraction() * 100.0,
            Car->GetBrakeHeat().ServiceBrakeMultiplier() * 100.0), Accent);
    }
    // Sustained propulsion heat lowers normal, nitro and drafting motor
    // output. This is an arcade estimate, not a real engine sensor.
    // Compact and low-height layouts retain their race timing priority.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Car->GetEngineHeat().HasValidSample() &&
        Car->GetEngineHeat().HeatFraction() >= 0.64)
    {
        Line(FString::Printf(
            TEXT("ENGINE HEAT %.0f%% | MOTOR %.0f%%"),
            Car->GetEngineHeat().HeatFraction() * 100.0,
            Car->GetEngineHeat().MotorMultiplier() * 100.0), Accent);
    }
    // Actual high-speed hard chassis collisions can weaken engine and
    // lateral arcade performance until the next race reset. Damage is a
    // bounded race-only prototype; preserve limited mobile HUD space.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Car->GetCollisionDamage().DamageFraction() > 0.0)
    {
        Line(FString::Printf(
            TEXT("IMPACT DAMAGE %.0f%% | MOTOR %.0f%% | GRIP %.0f%%"),
            Car->GetCollisionDamage().DamageFraction() * 100.0,
            Car->GetCollisionDamage().MotorMultiplier() * 100.0,
            Car->GetCollisionDamage().LateralGripMultiplier() * 100.0),
            Accent);
    }
    // An optional desktop pit notice is derived from the ACTUAL player
    // vehicle and the shared deterministic service policy. The HUD may
    // offer wayfinding but cannot repair a car or advance race state.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Mode->IsRaceStartGreen() && !Race.IsFinished() &&
        !Mode->IsMenuOpen())
    {
        const latebrake::RacePitService& Pit = Car->GetPitService();
        if (Pit.IsInBay())
        {
            if (!Pit.IsArmed())
            {
                Line(FString::Printf(TEXT("PIT SERVICED %d | EXIT BAY TO REARM"),
                    Pit.StopCount()), Accent);
                const auto Summary=Car->GetPitJournal().Review(Pit.StopCount());
                if (Summary.Valid)
                {
                    const auto Receipt=Car->GetPitJournal().GetRecent(0);
                    Line(FString::Printf(
                        TEXT("RESTORED: DAMAGE %.0f pp | WEAR %.0f pp | BOOST %.0f pp"),
                        Receipt.DamageRestored*100.0, Receipt.WearRemoved*100.0,
                        Receipt.ChargeRestored*100.0), Primary);
                }
            }
            else
            {
                // Reflect actual last qualified RacePitService observation,
                // not HUD-estimated speed or independent scoring rules.
                switch (Pit.Guidance())
                {
                case latebrake::PitServiceGuidance::NoMaintenance:
                    Line(TEXT("PIT BAY | NO SERVICE NEEDED"), Primary);
                    break;
                case latebrake::PitServiceGuidance::SlowDown:
                    Line(TEXT("PIT: SLOW TO 4.3 KM/H OR LESS"), Accent);
                    break;
                case latebrake::PitServiceGuidance::ReleaseThrottle:
                    Line(TEXT("PIT: RELEASE THROTTLE"), Accent);
                    break;
                case latebrake::PitServiceGuidance::ReleaseNitro:
                    Line(TEXT("PIT: RELEASE NITRO"), Accent);
                    break;
                case latebrake::PitServiceGuidance::SettleChassis:
                    Line(TEXT("PIT: SETTLE CHASSIS ON ROAD"), Accent);
                    break;
                case latebrake::PitServiceGuidance::HoldButton:
                    Line(TEXT("PIT: HOLD P / Y FOR 2.5 SECONDS"), Accent);
                    break;
                case latebrake::PitServiceGuidance::Servicing:
                    Line(FString::Printf(
                        TEXT("PIT: SERVICING %.0f%% | STOP %d"),
                        Pit.ProgressFraction() * 100.0,
                        Pit.StopCount() + 1), Accent);
                    break;
                case latebrake::PitServiceGuidance::Completed:
                case latebrake::PitServiceGuidance::ExitToRearm:
                    Line(TEXT("PIT: EXIT THE BAY BEFORE ANOTHER STOP"), Accent);
                    break;
                default:
                    Line(TEXT("PIT: WAIT FOR RACE / PHYSICS READINESS"), Primary);
                    break;
                }
            }
        }
        else
        {
            const FVector Position = Car->GetActorLocation();
            const double DX = static_cast<double>(Position.X) -
                latebrake::RacePitBay.X;
            const double DY = static_cast<double>(Position.Y) -
                latebrake::RacePitBay.Y;
            const double DistanceCm = FMath::Sqrt(DX * DX + DY * DY);
            if (FMath::IsFinite(DistanceCm) && DistanceCm < 1500.0)
            {
                Line(FString::Printf(
                    TEXT("PIT: INNER SOUTH SHOULDER | %.0f m"),
                    DistanceCm / 100.0), Primary);
            }
        }
    }

    // The independent off-track direction hint may appear immediately;
    // this readout reports only CONFIRMED road-surface motor/grip effects.
    // Do not consume limited compact or low-height HUD timing lines.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        Car->GetSurfaceTraction().IsOffPavement())
    {
        Line(FString::Printf(
            TEXT("OFF-PAVEMENT TRACTION  MOTOR %.0f%% | GRIP %.0f%%"),
            latebrake::ArcadeSurfaceTraction::OffPavementMotorMultiplier * 100.0,
            latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier * 100.0),
            Accent);
    }

    // Optional, verified human-only aerodynamic *arcade* gameplay hint;
    // keep compact touch layouts focused on authoritative race status.
    if (!bCompact && Car->GetSlipstreamBoost().IsActive() &&
        Car->GetAppliedDraftBoostFraction() > 0.f)
    {
        Line(FString::Printf(TEXT("SLIPSTREAM +%.1f%% DRIVE"),
            Car->GetAppliedDraftBoostFraction() * 100.0), Primary);
    }
    else if (!bCompact && Mode->IsExperimentalFleetRequested() &&
        Car->GetAppliedDraftBoostFraction()>0.f)
    {
        const auto Draft=Car->GetFleetSlipstreamReadout();
        if (Draft.Active())
            Line(FString::Printf(TEXT("SLIPSTREAM AI %d | +%.1f%% DRIVE"),
                Draft.TargetSlot,Car->GetAppliedDraftBoostFraction()*100.0),Primary);
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
        StuckHint.Reset();
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
    const bool bExperimentalFleet=Mode->IsExperimentalFleetRequested();
    const auto FleetReview=bExperimentalFleet?Mode->GetFleetRaceReview():latebrake::FleetRaceReview{};
    const bool bRivalRace = !bExperimentalFleet && !bTimeAttack && Mode->HasRival();
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
    // Read-only discrete comparison of already accepted directional gate
    // events. Direct radar distance NEVER determines who is ahead; two
    // vehicles at the same gate count could still be far apart on the road.
    // Skip tight mobile/low-height views and never display an unsupported
    // official race position or award.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        bRivalPositionVerified)
    {
        latebrake::RivalGateGap GateGap;
        if (Mode->TryGetRivalCreditedGateGap(GateGap) &&
            (GateGap.PlayerCreditedGates > 0 ||
             GateGap.RivalCreditedGates > 0))
        {
            if (GateGap.Kind ==
                latebrake::RivalGateGapKind::PlayerMoreCreditedGates)
            {
                Line(FString::Printf(
                    TEXT("GATE GAP: YOU +%d VERIFIED (NOT RACE PLACE)"),
                    GateGap.CreditedGateDelta), Primary);
            }
            else if (GateGap.Kind ==
                     latebrake::RivalGateGapKind::RivalMoreCreditedGates)
            {
                Line(FString::Printf(
                    TEXT("GATE GAP: RIVAL +%d VERIFIED (NOT RACE PLACE)"),
                    -GateGap.CreditedGateDelta), Accent);
            }
            else if (GateGap.Kind ==
                     latebrake::RivalGateGapKind::SameCreditedGateCount)
            {
                // Equal credited gates alone cannot resolve the order.
                // Use measured chassis progress only on the SAME straight;
                // no inferred race rank, scoring authority or corner shortcut.
                const latebrake::RivalSameSegmentOrder Straight =
                    latebrake::CompareRivalSameSegment({
                        GateGap, Race.GetNextCheckpoint(),
                        {static_cast<double>(WayfindingPosition.X),
                         static_cast<double>(WayfindingPosition.Y)},
                        {static_cast<double>(RivalWorldPosition.X),
                         static_cast<double>(RivalWorldPosition.Y)},
                        bWayfindingEligible && Mode->IsRaceStartGreen()});
                if (Straight.Valid &&
                    Straight.Leader == latebrake::RivalSegmentLeader::Player)
                {
                    Line(FString::Printf(
                        TEXT("SAME GATES | YOU +%.1fm STRAIGHT (NOT RACE PLACE)"),
                        Straight.AlongRoadSeparationMeters), Primary);
                }
                else if (Straight.Valid &&
                         Straight.Leader == latebrake::RivalSegmentLeader::Rival)
                {
                    Line(FString::Printf(
                        TEXT("SAME GATES | RIVAL +%.1fm STRAIGHT (NOT RACE PLACE)"),
                        Straight.AlongRoadSeparationMeters), Accent);
                }
                else
                {
                    Line(TEXT("GATE GAP: SAME VERIFIED COUNT (NOT RACE PLACE)"),
                         Primary);
                }
            }
        }
    }
    // Optional SAME-GATE verified split timing on the true local clock.
    // It never claims official standing or updates race timing.
    if (!bCompact && VerticalPlan.MaxTelemetryLines >= 28 &&
        bRivalPositionVerified)
    {
        latebrake::RivalMatchedGateSplit Matched;
        if (Mode->TryGetLatestRivalMatchedGateSplit(Matched))
        {
            Line(FString::Printf(
                TEXT("SPLIT L%d G%d: RIVAL %+.2fs (NOT PLACE)"),
                Matched.LapNumber, Matched.GateNumber,
                Matched.RivalMinusPlayerSeconds),
                Matched.RivalMinusPlayerSeconds < 0.0 ? Accent : Primary);
        }
    }
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
    // The source steward observes projected decorative wheel overlap.
    // Time penalties are already included in the authoritative clock.
    if (Mode->IsExperimentalFleetRequested())
    {
        const auto Fleet=Mode->GetFleetAttemptReadout();
        Line(FString::Printf(TEXT("EXPERIMENTAL GRID %d/6 | NO SAVED REWARDS"),
            Mode->GetPhysicalFleetCount()),Accent);
        if (Fleet.Phase==latebrake::FleetAttemptPhase::Faulted)
            Line(TEXT("RESTART REQUIRED | FLEET UNAVAILABLE"),Accent,true);
        else if (!Fleet.RunningAllowed)
            Line(TEXT("WAITING FOR VERIFIED SIX-CAR GRID"),Primary);
        else if (FleetReview.ValidRoster && FleetReview.PlayerRouteValid)
        {
            Line(FString::Printf(TEXT("GATE COUNTS | MORE %d | SAME %d | FEWER %d"),
                FleetReview.MoreGates,FleetReview.SameGates,FleetReview.FewerGates),Primary);
            if (FleetReview.UnavailableRivals>0)
                Line(FString::Printf(TEXT("AI EVIDENCE UNAVAILABLE %d"),FleetReview.UnavailableRivals),Accent);
            if (FleetReview.SameGates>0 && !bCompact)
            {
                Line(FString::Printf(TEXT("SAME STRAIGHT | YOU AHEAD %d | AI AHEAD %d | CLOSE %d"),
                    FleetReview.StraightPlayerAhead,FleetReview.StraightRivalAhead,
                    FleetReview.StraightClose),Primary);
                if (FleetReview.StraightUnavailable>0)
                    Line(FString::Printf(TEXT("STRAIGHT COMPARISON UNAVAILABLE %d"),
                        FleetReview.StraightUnavailable),Accent);
            }
            const auto& Traffic=FleetReview.Traffic;
            if (Traffic.Valid && Traffic.StrongestSlot>=1 &&
                Traffic.StrongestSlot<latebrake::StartGridCapacity)
            {
                const auto& Threat=Traffic.Rivals[static_cast<std::size_t>(Traffic.StrongestSlot-1)];
                if (Threat.Slot==Traffic.StrongestSlot)
                    Line(FString::Printf(TEXT("AI %d | %s | %s %.1fm"),
                        Threat.Slot,UTF8_TO_TCHAR(latebrake::FleetTrafficDirectionName(Threat.Direction)),
                        UTF8_TO_TCHAR(latebrake::FleetTrafficSeverityName(Threat.Severity)),
                        Threat.CentreDistanceCm/100.0),Accent,true);
            }
            const auto& Timing=FleetReview.Timing;
            if (Timing.CommonCohortValid)
                Line(FString::Printf(TEXT("MATCHED G%d | AI EARLIER %d | CLOSE %d | LATER %d"),
                    Timing.CohortOrdinal,Timing.CohortEarlier,Timing.CohortUncertain,Timing.CohortLater),Primary);
            else if (Timing.ValidPlayer && Timing.MatchedRivals>0)
                Line(FString::Printf(TEXT("MATCHED ACTIVE-CLOCK SPLITS %d/5 | NOT RANK"),
                    Timing.MatchedRivals),Primary);
            if (!bCompact)
            {
                for(const auto& Row:FleetReview.Rivals)
                {
                    if (!Row.RouteEvidenceValid) continue;
                    const auto& Timing=FleetReview.Timing.Rivals[static_cast<std::size_t>(Row.Slot-1)];
                    if (Timing.Availability==latebrake::FleetTimingAvailability::Matched)
                    {
                        const FString Delta=Timing.Relation==latebrake::FleetTimingRelation::SameClockWindow
                            ? TEXT("CLOSE") : FString::Printf(TEXT("%+.1fs"),Timing.RivalMinusPlayerSeconds);
                        Line(FString::Printf(TEXT("AI %d | G%d %s | %d/%d LAPS"),
                            Row.Slot,Timing.CommonOrdinal,*Delta,Row.CompletedLaps,
                            FleetReview.RequiredLaps),Primary);
                    }
                    else if (Row.Straight.Valid)
                    {
                        if (Row.Straight.Leader==latebrake::RivalSegmentLeader::TooClose)
                            Line(FString::Printf(TEXT("AI %d | SAME STRAIGHT | CLOSE"),Row.Slot),Primary);
                        else
                            Line(FString::Printf(TEXT("AI %d | SAME STRAIGHT | %s %.1fm"),
                                Row.Slot,Row.Straight.Leader==latebrake::RivalSegmentLeader::Player
                                    ? TEXT("YOU AHEAD"):TEXT("AI AHEAD"),
                                Row.Straight.AlongRoadSeparationMeters),Primary);
                    }
                    else if (Row.RouteFinished)
                        Line(FString::Printf(TEXT("AI %d | ROUTE COMPLETE"),Row.Slot),Primary);
                    else
                        Line(FString::Printf(TEXT("AI %d | LAPS %d/%d | NEXT %d"),
                            Row.Slot,Row.CompletedLaps,FleetReview.RequiredLaps,Row.NextGate+1),Primary);
                }
            }
        }
        else Line(TEXT("FLEET ROUTE EVIDENCE UNAVAILABLE"),Accent);
    }
    const auto TrackReport=Mode->GetTrackLimits().Snapshot();
    if (bWayfindingEligible)
    {
        const auto Notice=Mode->GetTrackLimits().Notice(Race.GetElapsedSeconds());
        if (Notice.Event==latebrake::TrackLimitsEvent::PenaltyApplied)
            Line(TEXT("TRACK LIMITS | +5s INCLUDED IN TIME"), Accent);
        else if (Notice.Event==latebrake::TrackLimitsEvent::PenaltyRefused)
            Line(TEXT("TRACK LIMIT CHARGE REFUSED | NO FEE"), Accent);
        else if (Notice.Event==latebrake::TrackLimitsEvent::ViolationRecorded)
            Line(FString::Printf(TEXT("TRACK LIMITS | VIOLATION %d"),
                TrackReport.Violations), Accent);
        else if (TrackReport.Warning && !TrackReport.PendingCleanRejoin)
            Line(TEXT("TRACK LIMIT WARNING | RETURN TO PAVEMENT"), Accent);
        if (!bCompact && TrackReport.Violations>0)
            Line(FString::Printf(TEXT("TRACK LIMITS %d | CHARGES %d"),
                TrackReport.Violations,TrackReport.Penalties), Primary);
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
        const auto GatePace = latebrake::ProjectGateAwareRacePace(Race);
        if (GatePace.Status == latebrake::GatePaceStatus::Estimated)
        {
            Line(FString::Printf(
                TEXT("REF LAP %d | GATE %d | EST. FINISH %.1fs"),
                GatePace.ReferenceLapNumber, GatePace.NextGateIndex+1,
                GatePace.EstimatedFinishSeconds), Accent);
        }
        else if (GatePace.Status == latebrake::GatePaceStatus::CurrentSectorOverdue)
        {
            Line(FString::Printf(
                TEXT("GATE %d PAST REF PACE | FINISH EST. UNAVAILABLE"),
                GatePace.NextGateIndex+1), Accent);
        }
        else if (!Race.GetMeasuredLapSplits().BestReference().Complete)
        {
            // Older timed-lap fallback is only allowed before a complete
            // measured reference exists. An invalid current gate prefix
            // must not be hidden by reverting to the lap-average estimate.
            const latebrake::LiveRacePaceProjection Pace =
                latebrake::ProjectLiveRacePace(Race);
            if (Pace.Valid)
            {
                Line(FString::Printf(TEXT("PACE  %.1fs/lap (%d recent) | EST. FINISH  %.1fs"),
                    Pace.ReferenceLapSeconds, Pace.SampledLaps,
                    Pace.ProjectedFinishSeconds), Accent);
            }
        }
        // Exact gate clock delta to one completed reference lap.
        // It remains a discrete gate observation, not live positional gap.
        const auto GateReference = Race.GetMeasuredLapSplits().LatestGate();
        if (GateReference.Valid)
        {
            Line(FString::Printf(
                TEXT("GATE %d | VS FEE-FREE LAP %d  %+.2fs"),
                GateReference.GateIndex+1, GateReference.ReferenceLapNumber,
                GateReference.CumulativeDeltaSeconds),
                GateReference.CumulativeDeltaSeconds <= 0.0 ? Primary : Accent);
        }
        else if (Race.GetMeasuredLapSplits().LastComparison().Valid &&
                 Race.GetNextCheckpoint()==0)
        {
            const auto& Last = Race.GetMeasuredLapSplits().LastComparison();
            Line(FString::Printf(TEXT("LAST LAP VS FEE-FREE LAP %d  %+.2fs"),
                Last.ReferenceLapNumber, Last.DeltaSeconds),
                Last.DeltaSeconds <= 0.0 ? Primary : Accent);
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
        if (!bTimeAttack && !bExperimentalFleet)
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
        // Optional source-defined pit map marker: same world point
        // and north-up transform as the real chassis-service policy.
        // Do not infer leaderboard position, reward, pit eligibility or
        // modify checkpoint state from this decorative radar symbol.
        if (Mode->IsCircuitReadyForGameplay() &&
            latebrake::IsOnPavedSurface(latebrake::RacePitBay))
        {
            const FVector2D Pit = ToScreen(
                latebrake::NormalizeCircuitMap(latebrake::RacePitBay));
            const FLinearColor PitColor(0.23f, 0.96f, 0.70f);
            DrawRect(PitColor,
                Pit.X - 4.f * Scale, Pit.Y - 4.f * Scale,
                8.f * Scale, 8.f * Scale);
            DrawText(TEXT("PIT"), PitColor,
                Pit.X + 6.f * Scale, Pit.Y - 11.f * Scale,
                GEngine->GetSmallFont(), 0.73f * Scale);
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
        if (bExperimentalFleet && bWayfindingEligible)
        {
            const auto Clusters=latebrake::GroupFleetRadar(FleetReview,0.075);
            const FLinearColor FleetColor(1.f,0.39f,0.23f);
            for(int32 Index=0;Index<Clusters.Count;++Index)
            {
                const auto& Group=Clusters.Groups[static_cast<std::size_t>(Index)];
                const FVector2D Point=ScreenMarker(Group.Marker);
                DrawRect(FleetColor,Point.X-4.f*Scale,Point.Y-4.f*Scale,8.f*Scale,8.f*Scale);
                FString Label;
                if (Group.Count>1) Label=FString::Printf(TEXT("AI x%d"),Group.Count);
                else
                {
                    for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
                        if ((Group.SlotMask&(1u<<static_cast<unsigned>(Slot)))!=0u)
                            Label=FString::Printf(TEXT("AI%d"),Slot);
                }
                // Clamp the label into the radar's visual bounding box.
                DrawText(Label,FleetColor,
                    FMath::Clamp(static_cast<float>(Point.X+5.f*Scale),
                        MapLeft,MapLeft+MapSize-34.f*Scale),
                    FMath::Clamp(static_cast<float>(Point.Y-9.f*Scale),
                        MapTop,MapTop+MapSize-12.f*Scale),
                    GEngine->GetSmallFont(),0.65f*Scale);
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

    // Read-only instrument panel. Avoid the touch UI, menu, results and
    // compact windows entirely, and reserve a text lane rather than allowing
    // optional diagnostic strings to paint over the gauges.
    if (Instruments.Visible)
    {
        const float Gx = static_cast<float>(Instruments.Panel.X);
        const float Gy = static_cast<float>(Instruments.Panel.Y);
        const float S = static_cast<float>(Instruments.Scale);
        const float Gw = static_cast<float>(Instruments.Panel.Width);
        const float Gh = static_cast<float>(Instruments.Panel.Height);
        const FLinearColor Panel(0.018f, 0.034f, 0.062f, 0.82f);
        const FLinearColor Track(0.14f, 0.19f, 0.24f, 0.93f);
        const FLinearColor Gold(1.f, 0.73f, 0.17f, 1.f);
        const FLinearColor Mint(0.42f, 0.89f, 0.80f, 1.f);
        const FLinearColor Coral(1.f, 0.52f, 0.38f, 1.f);
        const FLinearColor Ink(0.95f, 0.96f, 1.f, 1.f);
        DrawRect(Panel, Gx, Gy, Gw, Gh);
        DrawRect(Gold, Gx, Gy, 4.f * S, Gh);
        DrawText(FString(TEXT("RACE / INSTRUMENTS")), Gold,
            Gx + 16.f * S, Gy + 8.f * S,
            GEngine->GetSmallFont(), 0.86f * S);
        const double DisplaySpeed = Mode->UsesMilesPerHour()
            ? Instruments.SpeedKmH * 0.621371
            : Instruments.SpeedKmH;
        const FString SpeedReadout = FString::Printf(
            Mode->UsesMilesPerHour()
                ? TEXT("%.0f mph") : TEXT("%.0f km/h"), DisplaySpeed);
        DrawText(SpeedReadout, Ink, Gx + 16.f * S, Gy + 30.f * S,
            GEngine->GetMediumFont(), 1.10f * S);

        // Native automatic gear *observation* shares the existing
        // right-hand gauge's speed row. Measure the actual proportional
        // fonts at their rendered scales so even a small desktop panel
        // keeps speed and shifting data in separate horizontal lanes.
        // Estimated RPM comes from the speed-based arcade transmission,
        // not a physical engine sensor or a gameplay controller.
        const latebrake::AutomaticTransmissionState DriveGear =
            Car->GetTransmissionReadout();
        if (DriveGear.Valid && DriveGear.Gear >= -1 &&
            DriveGear.Gear <= latebrake::AutomaticTransmission::ForwardGearCount &&
            DriveGear.Gear != 0 && DriveGear.EstimatedRpm >= 900 &&
            DriveGear.EstimatedRpm <= 7200 &&
            GEngine->GetMediumFont() && GEngine->GetSmallFont())
        {
            const FString GearShort = DriveGear.Gear < 0
                ? FString(TEXT("R"))
                : FString::Printf(TEXT("G%d"), DriveGear.Gear);
            const FString GearFull = DriveGear.Shifting
                ? GearShort + TEXT(" SHIFT")
                : FString::Printf(TEXT("%s  %d RPM EST"),
                    *GearShort, DriveGear.EstimatedRpm);
            const float GearFontScale = 0.83f * S;
            float SpeedWidth = 0.f;
            float SpeedHeight = 0.f;
            float ShortWidth = 0.f;
            float ShortHeight = 0.f;
            float FullWidth = 0.f;
            float FullHeight = 0.f;
            GetTextSize(SpeedReadout, SpeedWidth, SpeedHeight,
                GEngine->GetMediumFont(), 1.10f * S);
            GetTextSize(GearShort, ShortWidth, ShortHeight,
                GEngine->GetSmallFont(), GearFontScale);
            GetTextSize(GearFull, FullWidth, FullHeight,
                GEngine->GetSmallFont(), GearFontScale);
            const auto Badge = latebrake::CalculateHudInstrumentDriveBadge(
                Instruments.Panel, Instruments.Scale,
                static_cast<double>(SpeedWidth),
                static_cast<double>(ShortWidth),
                static_cast<double>(ShortHeight),
                static_cast<double>(FullWidth),
                static_cast<double>(FullHeight));
            if (Badge.Visible)
            {
                DrawRect(FLinearColor(0.08f, 0.16f, 0.22f, 0.96f),
                    static_cast<float>(Badge.X) - 5.f * S,
                    Gy + 29.f * S,
                    static_cast<float>(Badge.Width) + 10.f * S, 27.f * S);
                DrawText(Badge.ShowExtended ? GearFull : GearShort,
                    DriveGear.Shifting ? Gold : Mint,
                    static_cast<float>(Badge.X), Gy + 33.f * S,
                    GEngine->GetSmallFont(), GearFontScale);
            }
        }

        // Labeled percentage bars remain usable without color distinction.
        // Heat is an arcade estimate; chassis condition is derived solely
        // from current-session collision damage, not a safety rating.
        const auto DrawMeter = [&](const TCHAR* Label, double Fraction,
                                   float Row, const FLinearColor& Fill)
        {
            const float PosY = Gy + Row * S;
            // Anchor the measured percentage on the right even when
            // "ENGINE HEAT (ARCADE)" or "CHASSIS CONDITION" is longer
            // than the left part of a laptop-width instrument panel.
            const float FontScale = 0.83f * S;
            UFont* Font = GEngine->GetSmallFont();
            const FString Percentage =
                FString::Printf(TEXT("%.0f%%"), Fraction * 100.0);
            float PercentageWidth = 0.f;
            float PercentageHeight = 0.f;
            GetTextSize(Percentage, PercentageWidth, PercentageHeight,
                Font, FontScale);
            const auto MeterText = latebrake::CalculateHudInstrumentMeterText(
                Instruments.Panel, Instruments.Scale,
                static_cast<double>(PercentageWidth));
            if (MeterText.Valid)
            {
                const FString MeterName(Label);
                const FString Suffix(TEXT("..."));
                const auto Measure = [&](const FString& Value) -> double
                {
                    float MeasuredWidth = 0.f;
                    float MeasuredHeight = 0.f;
                    GetTextSize(Value, MeasuredWidth, MeasuredHeight,
                        Font, FontScale);
                    return static_cast<double>(MeasuredWidth);
                };
                const int32 Prefix = latebrake::ChooseHudTextPrefix(
                    MeterName.Len(), MeterText.LabelMaxWidth,
                    Measure(MeterName), Measure(Suffix),
                    [&](int32 Units) -> double
                    {
                        return Measure(MeterName.Left(Units) + Suffix);
                    });
                if (Prefix >= 0)
                {
                    const FString VisibleName =
                        Prefix == MeterName.Len() ? MeterName :
                        MeterName.Left(Prefix) + Suffix;
                    DrawText(VisibleName, Ink,
                        static_cast<float>(MeterText.LabelX), PosY,
                        Font, FontScale);
                }
                DrawText(Percentage, Ink,
                    static_cast<float>(MeterText.PercentX), PosY,
                    Font, FontScale);
            }
            const float BarX = Gx + 16.f * S;
            const float BarWidth = Gw - 32.f * S;
            const float BarY = PosY + 18.f * S;
            DrawRect(Track, BarX, BarY, BarWidth, 7.f * S);
            if (Fraction > 0.0)
            {
                DrawRect(Fill, BarX, BarY,
                    BarWidth * static_cast<float>(Fraction), 7.f * S);
            }
        };
        // An accepted-checkpoint count is progress, not distance or placement.
        // Read the SAME authoritative race object that drives LAP/GATE text;
        // never let a visual fill fraction grant timing, credit or a result.
        const latebrake::RaceGateProgressGauge GateGauge =
            latebrake::CalculateRaceGateProgressGauge(
                Race.GetCompletedLaps(), Race.GetNextCheckpoint(),
                Race.GetRequiredLaps(), Race.GetCheckpointCount(),
                Race.IsFinished());
        if (GateGauge.Valid)
        {
            const FString GateLabel = FString::Printf(TEXT("GATES %d/%d"),
                GateGauge.CreditedGates, GateGauge.TotalGates);
            DrawMeter(*GateLabel, GateGauge.Fraction, 64.f, Gold);
        }
        else
        {
            DrawText(TEXT("GATE STATUS UNAVAILABLE"), Ink,
                Gx + 16.f * S, Gy + 64.f * S,
                GEngine->GetSmallFont(), 0.83f * S);
        }
        // The charge bar alone does not tell a driver WHY boost is
        // unavailable. Classify an actual ArcadeNitroBoost snapshot:
        // no independent input state, cooldown clock or gameplay authority.
        // Do not claim "READY" from charge alone: throttle, held key and
        // service/handbrake gates still belong to the car's real Step().
        const bool bNitroDrivingEligible =
            Mode->IsCircuitReadyForGameplay() &&
            Mode->IsRestartPlacementReady() &&
            Mode->HasLocalPlayerDriver() &&
            Mode->IsRaceStartGreen() &&
            Car->IsRacePhysicsReady() &&
            !Mode->IsFocusLossGuardActive() &&
            !Mode->IsManualRacePauseActive() &&
            !UGameplayStatics::IsGamePaused(this);
        const latebrake::NitroHudReadout NitroState =
            latebrake::ReadNitroHud(Car->GetNitroBoost(),
                static_cast<double>(Car->GetForwardSpeedKmH()),
                bNitroDrivingEligible);
        FString NitroMeterName(TEXT("NITRO DATA N/A"));
        FLinearColor NitroMeterColor = Track;
        if (NitroState.Valid)
        {
            switch (NitroState.Phase)
            {
            case latebrake::NitroHudPhase::Active:
                NitroMeterName = TEXT("NITRO ACTIVE");
                NitroMeterColor = Gold;
                break;
            case latebrake::NitroHudPhase::Cooldown:
                NitroMeterName = FString::Printf(TEXT("NITRO COOL %.1fs"),
                    NitroState.CooldownSeconds);
                NitroMeterColor = Coral;
                break;
            case latebrake::NitroHudPhase::Overheated:
                NitroMeterName = TEXT("NITRO HOT");
                NitroMeterColor = Coral;
                break;
            case latebrake::NitroHudPhase::Empty:
                NitroMeterName = TEXT("NITRO EMPTY");
                NitroMeterColor = Coral;
                break;
            case latebrake::NitroHudPhase::BuildSpeed:
                NitroMeterName = TEXT("NITRO BUILD SPEED");
                NitroMeterColor = Mint;
                break;
            case latebrake::NitroHudPhase::Stored:
                NitroMeterName = TEXT("NITRO STORED");
                NitroMeterColor = Mint;
                break;
            default:
                NitroMeterName = TEXT("NITRO LOCKED");
                break;
            }
        }
        DrawMeter(*NitroMeterName, Instruments.NitroFraction, 99.f,
            NitroMeterColor);
        DrawMeter(TEXT("ENGINE HEAT (ARCADE)"),
            Instruments.EngineHeatFraction, 133.f, Coral);
        DrawMeter(TEXT("CHASSIS CONDITION"),
            Instruments.ChassisConditionFraction, 167.f, Gold);
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
        // The physical chassis PIT hold and this Android pad share the
        // same service authority. A touch is never interpreted as a tap
        // or a one-off repair command; a continuous original hold is
        // required by the portable finger-ownership and pit policies.
        const latebrake::RacePitService& MobilePit = Car->GetPitService();
        const latebrake::PitServiceGuidance PitGuidance =
            MobilePit.Guidance();
        const FString PitPadLabel =
            PitGuidance == latebrake::PitServiceGuidance::SlowDown
                ? FString(TEXT("PIT SLOW"))
            : PitGuidance == latebrake::PitServiceGuidance::ReleaseThrottle
                ? FString(TEXT("LIFT GAS"))
            : PitGuidance == latebrake::PitServiceGuidance::ReleaseNitro
                ? FString(TEXT("NO NITRO"))
            : PitGuidance == latebrake::PitServiceGuidance::SettleChassis
                ? FString(TEXT("SETTLE CAR"))
            : PitGuidance == latebrake::PitServiceGuidance::NoMaintenance
                ? FString(TEXT("PIT OK"))
            : Car->IsTouchPitHeld() && MobilePit.IsInBay() &&
              MobilePit.IsArmed() &&
              PitGuidance == latebrake::PitServiceGuidance::Servicing
                ? FString::Printf(TEXT("PIT %.0f%%"),
                    MobilePit.ProgressFraction() * 100.0)
                : FString(TEXT("PIT HOLD"));
        DrawPad(Pads.PitService, *PitPadLabel, Car->IsTouchPitHeld());
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
