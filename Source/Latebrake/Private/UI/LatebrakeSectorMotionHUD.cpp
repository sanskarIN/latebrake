#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
bool CanPresentSectorMotion(const ALatebrakeRaceGameMode* Mode)
{
    // The experimental fleet uses the same right lane for its replay.
    // Keep ordinary race, solo and career sector inspection independent.
    return Mode && Mode->IsMenuOpen() &&
        Mode->GetMenuPage()==latebrake::MenuPage::Results &&
        Mode->GetRace().IsFinished() && !Mode->IsExperimentalFleetRequested();
}
}

bool ALatebrakeRaceHUD::TryHandleSectorMotionPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !CanPresentSectorMotion(Mode) || Mode->IsFocusLossGuardActive() ||
        !PresentedSector.Valid || PresentedSector.ViewportWidth!=Canvas->SizeX ||
        PresentedSector.ViewportHeight!=Canvas->SizeY ||
        PresentedSectorRevision!=Mode->GetCheckpointSessionRevision())
        return false;
    const auto History=Mode->GetPlayerSectorMotionHistory();
    if(!SectorViewer.Matches(History,PresentedSectorRevision)) return false;
    const auto Hit=latebrake::HitSectorPanel(PresentedSector,X,Y);
    if(!Hit.Consumed) return false;
    if(Hit.Action!=latebrake::SectorPanelAction::None)
        SectorViewer.Act(Hit.Action,History,PresentedSectorRevision);
    return true;
#endif
}

void ALatebrakeRaceHUD::DrawSectorMotionPanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedSector={};
    PresentedSectorRevision=0;
    if(!Canvas || !GEngine || !GEngine->GetSmallFont() || !CanPresentSectorMotion(Mode))
    {
        SectorViewer.Reset();
        return;
    }
    const auto History=Mode->GetPlayerSectorMotionHistory();
    const auto Layout=latebrake::CalculateSectorPanelLayout(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,true);
    const auto Revision=Mode->GetCheckpointSessionRevision();
    if(!SectorViewer.Synchronize(History,Revision,Layout.Valid)) return;
    const auto Read=SectorViewer.Read(History,Revision);
    if(!Read.Valid) return;

    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f);
    const FLinearColor Gold(1.f,0.73f,0.17f);
    const FLinearColor Blue(0.28f,0.67f,0.92f);
    const FLinearColor Muted(0.55f,0.64f,0.73f);
    const FLinearColor Fee(0.92f,0.38f,0.22f);
    const FLinearColor Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color)
    {
        if(!latebrake::ValidFleetReplayRect(R)) return;
        DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
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
    Label(TEXT("SECTOR HISTORY"),Layout.Header,Gold,0.88f*S);
    for(int Gate=0;Gate<latebrake::CircuitGateCount;++Gate)
    {
        const auto& Tab=Layout.Tabs[static_cast<std::size_t>(Gate)];
        const bool Selected=Gate==Read.Gate;
        Rect(Tab.Bounds,Selected?FLinearColor(0.35f,0.26f,0.10f):FLinearColor(0.13f,0.20f,0.28f));
        Label(FString::Printf(TEXT("G%d"),Gate+1),Tab.Bounds,Selected?Gold:White,0.84f*S);
    }
    for(int Index=0;Index<latebrake::SectorPanelRowsPerPage;++Index)
    {
        const auto I=static_cast<std::size_t>(Index);
        const auto& Bar=Read.Bars[I];
        const auto& Row=Layout.Rows[I].Bounds;
        if(!Bar.Present)
        {
            Rect(Row,FLinearColor(0.07f,0.10f,0.14f));
            continue;
        }
        const auto Geometry=latebrake::BuildSectorPanelBarGeometry(Row,Bar,Layout.Scale);
        if(!Geometry.Valid) continue;
        Rect(Row,Bar.Selected?FLinearColor(0.19f,0.27f,0.34f):FLinearColor(0.09f,0.14f,0.20f));
        FString Caption=Bar.Timed
            ? FString::Printf(TEXT("L%d  %.2fs  %s"),Bar.Lap,Bar.OfficialSeconds,
                Bar.StandingStart?TEXT("START"):Bar.BestRetained?TEXT("BEST RETAINED"):
                Bar.Qualified?TEXT("OK"):TEXT("PARTIAL"))
            : FString::Printf(TEXT("L%d  UNTIMED"),Bar.Lap);
        Label(Caption,Geometry.Label,Bar.Selected?Gold:White,0.76f*S);
        Rect(Geometry.Track,FLinearColor(0.17f,0.22f,0.27f));
        Rect(Geometry.Active,Bar.Qualified?Blue:Muted);
        Rect(Geometry.Fee,Fee);
        Rect(Geometry.Coverage,Gold);
    }
    for(int i=0;i<2;++i)
    {
        const auto& Button=Layout.Pager[static_cast<std::size_t>(i)];
        const bool Enabled=i==0?Read.Newer:Read.Older;
        Rect(Button.Bounds,Enabled?FLinearColor(0.14f,0.24f,0.34f):Surface);
        Label(i==0?TEXT("NEWER"):TEXT("OLDER"),Button.Bounds,Enabled?White:Muted,0.76f*S);
    }
    Label(FString::Printf(TEXT("%d/%d"),Read.Page+1,Read.PageCount),Layout.PageLabel,White,0.80f*S);
    const auto& Selected=Read.Selected;
    if(Selected.Ordinal>0 && Selected.ValidTiming)
    {
        Label(FString::Printf(TEXT("L%d | %.0f%% | %s"),Selected.Lap,Selected.Coverage*100,
            UTF8_TO_TCHAR(latebrake::SectorMotionQualityName(Selected.Quality))),
            Layout.SelectedDetail,White,0.74f*S);
        if(Selected.Motion.ObservedSeconds>0.0)
            Label(FString::Printf(TEXT("%.0f km/h mean | %.0f peak"),
                Selected.Motion.AverageKmh(),Selected.Motion.PeakKmh()),
                Layout.SelectedSpeed,White,0.76f*S);
        else
            Label(TEXT("No sampled motion"),Layout.SelectedSpeed,Muted,0.76f*S);
    }
    else
    {
        Label(TEXT("No qualified split time"),Layout.SelectedDetail,Muted,0.76f*S);
        Label(TEXT("Select another retained lap"),Layout.SelectedSpeed,Muted,0.76f*S);
    }
    Label(TEXT("BLUE active | RED fees | GOLD coverage"),Layout.Legend,Muted,0.69f*S);
    PresentedSector=Layout;
    PresentedSectorRevision=Revision;
}
