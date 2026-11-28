#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool ALatebrakeRaceHUD::TryHandleMainMenuDashboardPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Mode || !Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !Mode->IsMenuOpen())return false;
    const auto Current=Mode->GetMainMenuDashboardSnapshot();
    const auto Hit=latebrake::HitPresentedMainMenuDashboard(PresentedMainMenu,
        Current,Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    PresentedMainMenu={};
    if(latebrake::ValidMainMenuDestination(Hit.Destination))
        Mode->NavigateMainMenuDashboard(Hit.Destination,Current);
    return true;
#endif
}

bool ALatebrakeRaceHUD::DrawMainMenuDashboard(const ALatebrakeRaceGameMode* Mode,
    const latebrake::MainMenuDashboardLayout& Layout)
{
    PresentedMainMenu={};
    if(!Mode || !Canvas || !PlayerOwner || !GEngine ||
        !GEngine->GetSmallFont() || !latebrake::ValidMainMenuDashboardLayout(Layout))
        return false;
    const auto Snapshot=Mode->GetMainMenuDashboardSnapshot();
    if(!Snapshot.MenuOpen || Snapshot.Page!=latebrake::MenuPage::Main)return false;
    const float S=static_cast<float>(Layout.Scale);
    UFont* Font=GEngine->GetSmallFont();
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.68f,0.76f,0.85f),Surface(0.04f,0.07f,0.11f,0.98f);
    struct FMeasuredCaption
    {
        FString Text;
        latebrake::HudCaptionExtent Extent{};
        float Scale=0;
    };
    const auto Measure=[&](const FString& Primary,const FString& Short,
        const latebrake::DesktopMenuMouseRect& R,float Preferred)
    {
        for(const auto& Text:{Primary,Short})
        {
            if(Text.IsEmpty())continue;
            for(int I=0;I<4;++I)
            {
                const float Scale=Preferred*(1.f-0.08f*I);
                float W=0,H=0;
                GetTextSize(Text,W,H,Font,Scale);
                if(FMath::IsFinite(W) && FMath::IsFinite(H) && W>0 && H>0 &&
                    W<=R.Width && H<=R.Height)
                    return FMeasuredCaption{Text,{W,H},Scale};
            }
        }
        return FMeasuredCaption{};
    };
    const auto Header=Measure(Snapshot.AllowsNavigation()?
        TEXT("BROWSE TO MOVE | SELECT TO OPEN"):TEXT("NAVIGATION UNAVAILABLE"),
        Snapshot.AllowsNavigation()?TEXT("CHOOSE A SECTION"):TEXT("INPUT LOCKED"),
        Layout.Header,0.86f*S);
    if(Header.Scale<=0)return false;
    std::array<FMeasuredCaption,latebrake::MainMenuDestinationCount> Titles{},Descriptions{};
    // Measure the entire dashboard before painting. A failed title
    // means no partial or invisible navigation targets are created.
    for(std::size_t I=0;I<Layout.Cards.size();++I)
    {
        const auto& D=latebrake::MainMenuDestinations[I];
        const auto& C=Layout.Cards[I];
        Titles[I]=Measure(UTF8_TO_TCHAR(D.Title),UTF8_TO_TCHAR(D.ShortTitle),
            C.Title,0.94f*S);
        if(Titles[I].Scale<=0)return false;
        Descriptions[I]=Measure(UTF8_TO_TCHAR(D.Description),FString(),
            C.Description,0.73f*S);
    }
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Color)
    {
        DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FMeasuredCaption& Caption,
        const latebrake::DesktopMenuMouseRect& R,FLinearColor Color)
    {
        if(Caption.Scale<=0)return;
        DrawText(Caption.Text,Color,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Caption.Extent.Height)*0.5),Font,Caption.Scale);
    };
    float MouseX=0,MouseY=0;
    const bool HasCursor=PlayerOwner->GetMousePosition(MouseX,MouseY);
    const bool Enabled=Snapshot.AllowsNavigation();
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Text(Header,Layout.Header,Enabled?Gold:Muted);
    latebrake::MainMenuDashboardPresentation Presentation;
    Presentation.Valid=true;
    Presentation.Layout=Layout;
    Presentation.Snapshot=Snapshot;
    for(std::size_t I=0;I<Layout.Cards.size();++I)
    {
        const auto& C=Layout.Cards[I];
        const bool Hover=Enabled && HasCursor && C.Bounds.Contains(MouseX,MouseY);
        const bool Focused=Enabled && Snapshot.Focused==static_cast<int>(I);
        Rect(C.Bounds,!Enabled?FLinearColor(0.08f,0.11f,0.15f):
            Focused?FLinearColor(0.27f,0.21f,0.11f):
            Hover?FLinearColor(0.18f,0.28f,0.39f):FLinearColor(0.12f,0.19f,0.27f));
        Rect({C.Bounds.X,C.Bounds.Y,3*S,C.Bounds.Height},Focused?Gold:Hover?White:Muted);
        if(Focused)Rect({C.Bounds.X,C.Bounds.Y, C.Bounds.Width,2*S},Gold);
        Text(Titles[I],C.Title,Focused?Gold:Enabled?White:Muted);
        Text(Descriptions[I],C.Description,Muted);
        Presentation.Rendered[I]=true;
    }
    PresentedMainMenu=Presentation;
    return true;
}
