#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
bool CanViewLapMotion(const ALatebrakeRaceGameMode* Mode)
{
    return Mode && Mode->IsMenuOpen() &&
        Mode->GetMenuPage()==latebrake::MenuPage::Results &&
        Mode->GetRace().IsFinished() && !Mode->IsExperimentalFleetRequested();
}
}

void ALatebrakeRaceHUD::ToggleLapMotionPanel()
{
#if !PLATFORM_ANDROID
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!CanViewLapMotion(Mode) || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        Mode->IsFocusLossGuardActive() || Mode->IsManualRacePauseActive())return;
    const auto Revision=Mode->GetCheckpointSessionRevision();
    if(!Revision || !latebrake::ValidLapPanelReview(Mode->GetPlayerLapMotionReview()))return;
    LapMotionPanelRevision=LapMotionPanelRevision==Revision?0:Revision;
    LapMotionViewer.Reset();
    PresentedLapMotion={};
    PresentedPractice={};
    PresentedSector={};
#endif
}

bool ALatebrakeRaceHUD::TryHandleLapMotionPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !CanViewLapMotion(Mode) || !PresentedLapMotion.Valid)return false;
    const auto Revision=Mode->GetCheckpointSessionRevision();
    if(LapMotionPanelRevision!=Revision || Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive())return PresentedLapMotion.Layout.Panel.Contains(X,Y);
    const auto Review=Mode->GetPlayerLapMotionReview();
    const auto Hit=latebrake::HitPresentedLapPanel(PresentedLapMotion,LapMotionViewer,
        Review,Revision,Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    PresentedLapMotion={};
    if(Hit.Action!=latebrake::SectorPanelAction::None)LapMotionViewer.Act(Hit.Action,Review,Revision);
    return true;
#endif
}

bool ALatebrakeRaceHUD::DrawLapMotionPanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedLapMotion={};
    if(!CanViewLapMotion(Mode) || LapMotionPanelRevision!=Mode->GetCheckpointSessionRevision())
    {
        LapMotionPanelRevision=0;
        LapMotionViewer.Reset();
        return false;
    }
    if(!Canvas || !GEngine || !GEngine->GetSmallFont())return false;
    const auto Review=Mode->GetPlayerLapMotionReview();
    const auto Layout=latebrake::CalculateSectorPanelLayout(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,true);
    const auto Revision=Mode->GetCheckpointSessionRevision();
    if(!LapMotionViewer.Synchronize(Review,Revision,Layout.Valid))return false;
    const auto Read=LapMotionViewer.Read(Review,Revision);
    if(!Read.Valid)return false;
    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Blue(0.28f,0.67f,0.92f),Fee(0.92f,0.38f,0.22f);
    const FLinearColor Muted(0.55f,0.64f,0.73f),Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color)
    {
        if(latebrake::ValidFleetReplayRect(R))
            DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Label=[&](const FString& Value,const FString& Short,
        const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color,float Preferred)
    {
        UFont* Font=GEngine->GetSmallFont();
        const auto Try=[&](const FString& Caption)
        {
            if(Caption.IsEmpty())return false;
            for(int I=0;I<=4;++I)
            {
                const float Scale=Preferred*(1.f-0.07f*I);
                float W=0.f,H=0.f;
                GetTextSize(Caption,W,H,Font,Scale);
                if(!FMath::IsFinite(W) || !FMath::IsFinite(H) || W<=0 || H<=0 ||
                    W>R.Width-2.0 || H>R.Height)continue;
                DrawText(Caption,Color,static_cast<float>(R.X+(R.Width-W)*0.5),
                    static_cast<float>(R.Y+(R.Height-H)*0.5),Font,Scale);
                return true;
            }
            return false;
        };
        return Try(Value) || Try(Short);
    };
    latebrake::LapPanelPresentation Presentation;
    Presentation.Valid=true;
    Presentation.Layout=Layout;
    Presentation.State=LapMotionViewer.ReadState();
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Label(TEXT("FULL LAP REVIEW | F11 BACK"),TEXT("LAP REVIEW"),Layout.Header,Gold,0.82f*S);
    const TCHAR* Tabs[]={TEXT("TIME"),TEXT("SPEED"),TEXT("COVER"),TEXT("SLIP")};
    for(std::size_t I=0;I<Layout.Tabs.size();++I)
    {
        const auto& Tab=Layout.Tabs[I];
        const bool Selected=I==static_cast<std::size_t>(Read.Metric);
        Rect(Tab.Bounds,Selected?FLinearColor(0.35f,0.26f,0.10f):FLinearColor(0.13f,0.20f,0.28f));
        Presentation.TabsRendered[I]=Label(Tabs[I],FString(),Tab.Bounds,Selected?Gold:White,0.78f*S);
    }
    for(std::size_t I=0;I<Read.Rows.size();++I)
    {
        const auto& Bar=Read.Rows[I];
        const auto& Row=Layout.Rows[I].Bounds;
        if(!Bar.Row.Present)
        {
            Rect(Row,FLinearColor(0.07f,0.10f,0.14f));
            continue;
        }
        const auto Geometry=latebrake::BuildLapPanelGeometry(Row,Bar,Layout.Scale);
        if(!Geometry.Valid)continue;
        Rect(Row,Bar.Selected?FLinearColor(0.19f,0.27f,0.34f):FLinearColor(0.09f,0.14f,0.20f));
        FString Caption;
        if(!Bar.Value.Available)Caption=FString::Printf(TEXT("L%d | UNAVAILABLE"),Bar.Row.Lap);
        else if(Read.Metric==latebrake::LapPanelMetric::Time)
            Caption=FString::Printf(TEXT("L%d | %.2fs%s"),Bar.Row.Lap,
                Bar.Value.Value+Bar.Value.Fee,Bar.Row.StandingStart?TEXT(" START"):TEXT(""));
        else if(Read.Metric==latebrake::LapPanelMetric::Speed)
            Caption=FString::Printf(TEXT("L%d | %.0f km/h"),Bar.Row.Lap,Bar.Value.Value);
        else Caption=FString::Printf(TEXT("L%d | %.0f%%"),Bar.Row.Lap,Bar.Value.Value);
        Presentation.RowsRendered[I]=Label(Caption,FString(),Geometry.Label,Bar.Selected?Gold:White,0.78f*S);
        Rect(Geometry.Track,FLinearColor(0.17f,0.22f,0.27f));
        Rect(Geometry.Active,Bar.Row.Comparable()?Blue:Muted);
        Rect(Geometry.Fee,Fee);
        Rect(Geometry.Coverage,Gold);
    }
    for(std::size_t I=0;I<Layout.Pager.size();++I)
    {
        const bool Enabled=I==0?Read.Newer:Read.Older;
        const auto& Button=Layout.Pager[I];
        Rect(Button.Bounds,Enabled?FLinearColor(0.14f,0.24f,0.34f):Surface);
        Presentation.PagerRendered[I]=Label(I==0?TEXT("NEWER"):TEXT("OLDER"),FString(),
            Button.Bounds,Enabled?White:Muted,0.76f*S);
    }
    Label(FString::Printf(TEXT("%d/%d"),Read.Page+1,Read.Pages),FString(),Layout.PageLabel,White,0.8f*S);
    const auto& Selected=Read.Selected;
    Label(FString::Printf(TEXT("L%d | %s"),Selected.Lap,
        UTF8_TO_TCHAR(latebrake::LapMotionQualityName(Selected.Quality))),
        FString(),Layout.SelectedDetail,White,0.74f*S);
    if(Selected.Totals.ObservedSeconds>0)
        Label(FString::Printf(TEXT("%.0f%% OBSERVED | %.0fm PATH"),
            Selected.Totals.Coverage()*100,Selected.Totals.DistanceMeters()),
            FString(),Layout.SelectedSpeed,White,0.74f*S);
    else Label(TEXT("NO SAMPLED MOVEMENT"),FString(),Layout.SelectedSpeed,Muted,0.74f*S);
    const TCHAR* Legend=Read.Metric==latebrake::LapPanelMetric::Time?TEXT("ACTIVE + RED FEES | GOLD COVERAGE"):
        Read.Metric==latebrake::LapPanelMetric::Speed?TEXT("MEAN OVER OBSERVED CLOCK"):
        Read.Metric==latebrake::LapPanelMetric::Coverage?TEXT("OBSERVED / ACTIVE CLOCK"):
        TEXT("SLIP / OBSERVED CLOCK");
    Label(Legend,FString(),Layout.Legend,Muted,0.68f*S);
    PresentedLapMotion=Presentation;
    return true;
}
