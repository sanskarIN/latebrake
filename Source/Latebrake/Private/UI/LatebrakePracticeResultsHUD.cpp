#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
bool CanReviewPractice(const ALatebrakeRaceGameMode* Mode)
{
    return Mode && Mode->IsMenuOpen() &&
        Mode->GetMenuPage()==latebrake::MenuPage::Results &&
        Mode->IsPracticeRace() && Mode->GetRace().IsFinished();
}
}

bool ALatebrakeRaceHUD::TryHandlePracticeResultsPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !CanReviewPractice(Mode) || !PresentedPractice.Valid)return false;
    if(Mode->IsFocusLossGuardActive())
        return PresentedPractice.Layout.Panel.Contains(X,Y);
    const auto Review=Mode->GetPracticeGateReview();
    const auto Revision=Mode->GetCheckpointSessionRevision();
    const auto Hit=latebrake::HitPresentedPracticePanel(PresentedPractice,PracticeViewer,
        Review,Revision,Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    PresentedPractice={};
    if(Hit.Action!=latebrake::SectorPanelAction::None)
        PracticeViewer.Act(Hit.Action,Review,Revision);
    return true;
#endif
}

bool ALatebrakeRaceHUD::DrawPracticeResultsPanel(const ALatebrakeRaceGameMode* Mode,
    latebrake::HudLayout Hud,const latebrake::DesktopMenuMouseLayout& Dock,
    const latebrake::HudMenuReadablePanel& Text,bool bTouch)
{
    PresentedPractice={};
    if(!Canvas || !GEngine || !GEngine->GetSmallFont() || !CanReviewPractice(Mode))
    {
        PracticeViewer.Reset();
        return false;
    }
    const auto Review=Mode->GetPracticeGateReview();
    const auto Layout=latebrake::CalculateSectorPanelLayout(Canvas->SizeX,Canvas->SizeY,
        Hud,Dock,Text,bTouch,true);
    const auto Revision=Mode->GetCheckpointSessionRevision();
    if(!PracticeViewer.Synchronize(Review,Revision,Layout.Valid))return false;
    const auto Read=PracticeViewer.Read(Review,Revision);
    if(!Read.Valid)return false;
    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Gain(0.29f,0.77f,0.63f),Loss(0.96f,0.49f,0.32f);
    const FLinearColor Muted(0.58f,0.65f,0.73f),Surface(0.04f,0.07f,0.11f,0.98f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color)
    {
        if(!latebrake::ValidFleetReplayRect(R))return;
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
                if(!FMath::IsFinite(W) || !FMath::IsFinite(H) ||
                    W<=0 || H<=0 || W>R.Width-2.0 || H>R.Height)continue;
                DrawText(Caption,Color,static_cast<float>(R.X+(R.Width-W)*0.5),
                    static_cast<float>(R.Y+(R.Height-H)*0.5),Font,Scale);
                return true;
            }
            return false;
        };
        return Try(Value) || Try(Short);
    };
    latebrake::PracticePanelPresentation Presentation;
    Presentation.Valid=true;
    Presentation.Layout=Layout;
    Presentation.State=PracticeViewer.ReadState();
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Label(TEXT("PRACTICE VS PRIOR REFERENCE"),TEXT("PRACTICE DELTAS"),
        Layout.Header,Gold,0.83f*S);
    for(int Gate=0;Gate<latebrake::PracticePanelGates;++Gate)
    {
        const auto I=static_cast<std::size_t>(Gate);
        const auto& Tab=Layout.Tabs[I];
        const bool Selected=Gate==Read.Gate;
        Rect(Tab.Bounds,Selected?FLinearColor(0.35f,0.26f,0.10f):
            FLinearColor(0.13f,0.20f,0.28f));
        Presentation.TabsRendered[I]=Label(FString::Printf(TEXT("G%d"),Gate+1),
            FString(),Tab.Bounds,Selected?Gold:White,0.84f*S);
    }
    for(int Index=0;Index<latebrake::PracticePanelRows;++Index)
    {
        const auto I=static_cast<std::size_t>(Index);
        const auto& Bar=Read.Rows[I];
        const auto& Row=Layout.Rows[I].Bounds;
        if(!Bar.Present)
        {
            Rect(Row,FLinearColor(0.07f,0.10f,0.14f));
            continue;
        }
        const auto Geometry=latebrake::BuildPracticePanelBarGeometry(Row,Bar,Layout.Scale);
        if(!Geometry.Valid)continue;
        Rect(Row,Bar.Selected?FLinearColor(0.19f,0.27f,0.34f):FLinearColor(0.09f,0.14f,0.20f));
        const TCHAR* Note=Bar.LargestGain?TEXT("BEST GAIN"):Bar.LargestLoss?TEXT("BIGGEST LOSS"):TEXT("");
        const auto Caption=FString::Printf(TEXT("L%d | %+.2fs %s"),
            Bar.Delta.Lap,Bar.Delta.SectorDeltaSeconds,Note);
        const auto Short=FString::Printf(TEXT("L%d | %+.2fs"),
            Bar.Delta.Lap,Bar.Delta.SectorDeltaSeconds);
        Presentation.RowsRendered[I]=Label(Caption,Short,Geometry.Label,
            Bar.Selected?Gold:White,0.78f*S);
        Rect(Geometry.Track,FLinearColor(0.17f,0.22f,0.27f));
        Rect(Geometry.Fill,Geometry.Gain?Gain:Loss);
        Rect(Geometry.Zero,Muted);
    }
    for(int I=0;I<2;++I)
    {
        const auto Index=static_cast<std::size_t>(I);
        const bool Enabled=I==0?Read.Newer:Read.Older;
        const auto& Button=Layout.Pager[Index];
        Rect(Button.Bounds,Enabled?FLinearColor(0.14f,0.24f,0.34f):Surface);
        Presentation.PagerRendered[Index]=Label(I==0?TEXT("NEWER"):TEXT("OLDER"),FString(),
            Button.Bounds,Enabled?White:Muted,0.76f*S);
    }
    Label(FString::Printf(TEXT("%d/%d"),Read.Page+1,Read.PageCount),FString(),
        Layout.PageLabel,White,0.80f*S);
    const auto& Selected=Read.Selected;
    Label(FString::Printf(TEXT("L%d G%d | CUMULATIVE %+.2fs"),Selected.Lap,
        Selected.Gate+1,Selected.TotalDeltaSeconds),FString::Printf(TEXT("CUM %+.2fs"),
        Selected.TotalDeltaSeconds),Layout.SelectedDetail,White,0.74f*S);
    Label(FString::Printf(TEXT("SECTOR ACTIVE %+.2f | FEE %+.2f"),
        Selected.SectorActiveDeltaSeconds,Selected.SectorFeeDeltaSeconds),
        FString::Printf(TEXT("A %+.2f | F %+.2f"),Selected.SectorActiveDeltaSeconds,
            Selected.SectorFeeDeltaSeconds),Layout.SelectedSpeed,White,0.74f*S);
    Label(FString::Printf(TEXT("FASTER < 0 > SLOWER | %.2fs SCALE"),Read.ScaleSeconds),
        TEXT("FASTER LEFT | SLOWER RIGHT"),Layout.Legend,Muted,0.68f*S);
    PresentedPractice=Presentation;
    return true;
}
