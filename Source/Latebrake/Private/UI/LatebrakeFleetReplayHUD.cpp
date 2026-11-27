#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

namespace
{
FString ReplayButtonCaption(latebrake::FleetReplayAction action,
    const latebrake::FleetReplayViewerState& state,double speed)
{
    using latebrake::FleetReplayAction;
    switch(action)
    {
    case FleetReplayAction::Previous: return TEXT("-0.5s");
    case FleetReplayAction::Next: return TEXT("+0.5s");
    case FleetReplayAction::PlayPause: return state.Playing?TEXT("PAUSE"):TEXT("PLAY");
    case FleetReplayAction::NextHighlight: return TEXT("EVENT");
    case FleetReplayAction::NextFocus: return TEXT("FOCUS");
    case FleetReplayAction::Speed: return FString::Printf(TEXT("%.1fx"),speed);
    default: return {};
    }
}

bool CanReviewFleetReplay(const ALatebrakeRaceGameMode* Mode)
{
    return Mode && Mode->IsMenuOpen() &&
        Mode->GetMenuPage()==latebrake::MenuPage::Results &&
        Mode->GetRace().IsFinished() && Mode->IsExperimentalFleetRequested();
}
}

bool ALatebrakeRaceHUD::HandleFleetReplayAction(latebrake::FleetReplayAction Action)
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !CanReviewFleetReplay(Mode) || Mode->IsFocusLossGuardActive() ||
        !PresentedReplay.Valid || PresentedReplay.ViewportWidth!=Canvas->SizeX ||
        PresentedReplay.ViewportHeight!=Canvas->SizeY)
        return false;
    const auto Review=Mode->GetFleetReplayReview();
    if(Review.Attempt!=PresentedReplayAttempt || !ReplayViewer.Matches(Review))
        return false;
    return ReplayViewer.Act(Action,Review);
#endif
}

bool ALatebrakeRaceHUD::TryHandleFleetReplayPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !CanReviewFleetReplay(Mode) || Mode->IsFocusLossGuardActive() ||
        !PresentedReplay.Valid || PresentedReplay.ViewportWidth!=Canvas->SizeX ||
        PresentedReplay.ViewportHeight!=Canvas->SizeY)
        return false;
    const auto Review=Mode->GetFleetReplayReview();
    if(Review.Attempt!=PresentedReplayAttempt || !ReplayViewer.Matches(Review))
        return false;
    const auto Hit=latebrake::HitFleetReplayPanel(PresentedReplay,X,Y);
    if(!Hit.Consumed) return false;
    if(Hit.Seek) ReplayViewer.SeekFraction(Hit.Fraction,Review);
    else if(Hit.Action!=latebrake::FleetReplayAction::None)
        ReplayViewer.Act(Hit.Action,Review);
    return true;
#endif
}

void ALatebrakeRaceHUD::DrawFleetReplayPanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedReplay={};
    PresentedReplayAttempt=0;
    if(!Canvas || !GEngine || !GEngine->GetSmallFont() || !CanReviewFleetReplay(Mode))
    {
        ReplayViewer.Reset();
        return;
    }
    const auto Review=Mode->GetFleetReplayReview();
    const auto Layout=latebrake::CalculateFleetReplayPanel(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,true);
    if(!ReplayViewer.Synchronize(Review,Layout.Valid)) return;
    ReplayViewer.Tick(FPlatformTime::Seconds(),!Mode->IsFocusLossGuardActive());
    const auto State=ReplayViewer.Readout();
    const auto Frame=Mode->EvaluateFleetReplay(State.CursorSeconds);
    const auto Map=latebrake::BuildFleetReplayMap(Frame,Layout,State);
    if(!Map.Valid)
    {
        ReplayViewer.Pause();
        return;
    }

    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f);
    const FLinearColor Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.55f,0.64f,0.73f);
    const FLinearColor Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color)
    {
        DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    // Every string is measured with the actual font; no hard-coded
    // character limit or clipping into neighbouring menu controls.
    const auto Label=[&](const FString& Value,const latebrake::DesktopMenuMouseRect& R,
        const FLinearColor& Color,float Scale)
    {
        UFont* Font=GEngine->GetSmallFont();
        float W=0.f,H=0.f;
        GetTextSize(Value,W,H,Font,Scale);
        if(!FMath::IsFinite(W) || !FMath::IsFinite(H) || W<=0 || H<=0 ||
            W>R.Width-4.0 || H>R.Height) return;
        DrawText(Value,Color,static_cast<float>(R.X+(R.Width-W)*0.5),
            static_cast<float>(R.Y+(R.Height-H)*0.5),Font,Scale);
    };

    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2.0*S},Gold);
    Label(TEXT("RECENT RACE REPLAY"),Layout.Header,Gold,0.85f*S);
    Rect(Layout.Map,FLinearColor(0.10f,0.16f,0.21f,1.f));
    // Fixed original circuit only; position projections outside the
    // visible map are omitted, not pinned to a misleading edge.
    for(int Gate=0;Gate<latebrake::CircuitGateCount;++Gate)
    {
        const auto A=latebrake::ProjectFleetReplayMap(latebrake::CircuitGate(Gate),Layout.Map);
        const auto B=latebrake::ProjectFleetReplayMap(latebrake::CircuitGate(
            (Gate+1)%latebrake::CircuitGateCount),Layout.Map);
        if(A.Valid && B.Valid)
            DrawLine(A.X,A.Y,B.X,B.Y,FLinearColor(0.36f,0.43f,0.48f),4.0f*S);
    }
    // Draw all real sampled cars first and the selected focus last.
    const auto Marker=[&](const latebrake::FleetReplayMapCar& Car)
    {
        if(!Car.Visible) return;
        const bool bFocus=Car.Slot==State.FocusSlot;
        const FLinearColor Ink=bFocus?Gold:Car.Slot==0?White:FLinearColor(0.30f,0.74f,0.92f);
        const double Radius=bFocus?3.5*S:2.5*S;
        const double Left=FMath::Max(Layout.Map.X,Car.Position.X-Radius);
        const double Top=FMath::Max(Layout.Map.Y,Car.Position.Y-Radius);
        const double Right=FMath::Min(Layout.Map.X+Layout.Map.Width,Car.Position.X+Radius);
        const double Bottom=FMath::Min(Layout.Map.Y+Layout.Map.Height,Car.Position.Y+Radius);
        if(Right>Left && Bottom>Top) Rect({Left,Top,Right-Left,Bottom-Top},Ink);
        if(Car.Nose.Valid)
            DrawLine(Car.Position.X,Car.Position.Y,Car.Nose.X,Car.Nose.Y,Ink,1.5f*S);
    };
    for(const auto& Car:Map.Cars) if(Car.Slot!=State.FocusSlot) Marker(Car);
    if(State.FocusSlot>=0 && State.FocusSlot<static_cast<int>(Map.Cars.size()))
        Marker(Map.Cars[static_cast<std::size_t>(State.FocusSlot)]);

    const FString Focus=State.FocusSlot==0?FString(TEXT("YOU")):
        FString::Printf(TEXT("AI %d"),State.FocusSlot);
    Label(FString::Printf(TEXT("%.1fs | %s | %d/6 visible"),State.CursorSeconds,*Focus,
        Map.VisibleCars),Layout.Status,White,0.78f*S);
    Rect(Layout.Scrub,FLinearColor(0.16f,0.23f,0.29f));
    const double Fill=Layout.Scrub.Width*ReplayViewer.Fraction();
    if(Fill>0.0)
        Rect({Layout.Scrub.X,Layout.Scrub.Y,Fill,Layout.Scrub.Height},Gold);
    const double Handle=std::clamp(Layout.Scrub.X+Fill-1.0*S,
        Layout.Scrub.X,Layout.Scrub.X+Layout.Scrub.Width-2.0*S);
    Rect({Handle,Layout.Scrub.Y,2.0*S,Layout.Scrub.Height},White);
    for(const auto& Button:Layout.Buttons)
    {
        const bool Enabled=ReplayViewer.CanAct(Button.Action,Review);
        Rect(Button.Bounds,Enabled?FLinearColor(0.14f,0.24f,0.34f):Surface);
        Label(ReplayButtonCaption(Button.Action,State,ReplayViewer.Speed()),
            Button.Bounds,Enabled?White:Muted,0.78f*S);
    }
    PresentedReplay=Layout;
    PresentedReplayAttempt=Review.Attempt;
}
