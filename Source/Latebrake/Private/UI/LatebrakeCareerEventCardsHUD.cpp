#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool ALatebrakeRaceHUD::TryHandleCareerEventCardsPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Mode || !Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !Mode->IsMenuOpen())return false;
    const auto Current=Mode->GetCareerEventCardsSnapshot();
    const auto Hit=latebrake::HitPresentedCareerEventCards(PresentedCareerCards,
        Current,Canvas->SizeX,Canvas->SizeY,X,Y);
    if(!Hit.Consumed)return false;
    PresentedCareerCards={};
    if(Hit.EventId>=0)Mode->SelectCareerEventCard(Hit.EventId,Current);
    return true;
#endif
}

bool ALatebrakeRaceHUD::DrawCareerEventCards(const ALatebrakeRaceGameMode* Mode,
    const latebrake::CareerEventCardsLayout& Layout)
{
    PresentedCareerCards={};
    if(!Mode || !Canvas || !PlayerOwner || !GEngine || !GEngine->GetSmallFont() ||
        !latebrake::ValidCareerEventCardsLayout(Layout))return false;
    const auto Snapshot=Mode->GetCareerEventCardsSnapshot();
    if(!Snapshot.MenuOpen || Snapshot.Page!=Layout.Page)return false;
    const float S=static_cast<float>(Layout.Scale);
    UFont* Font=GEngine->GetSmallFont();
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.68f,0.76f,0.85f),Surface(0.04f,0.07f,0.11f,0.98f);
    const FLinearColor Mint(0.38f,0.89f,0.78f,1.f),Coral(1.f,0.52f,0.38f,1.f);
    struct FMeasured
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
                    return FMeasured{Text,{W,H},Scale};
            }
        }
        return FMeasured{};
    };
    const FString HeaderText=!Snapshot.CanBrowse()?TEXT("EVENT INPUT LOCKED"):
        Snapshot.ReadOnly?TEXT("READ-ONLY PROFILE | BROWSE ONLY"):
        TEXT("CHOOSE EVENT | SELECT TO START");
    const auto Header=Measure(HeaderText,TEXT("CAREER EVENTS"),Layout.Header,0.82f*S);
    if(Header.Scale<=0)return false;
    struct FPreparedCard
    {
        latebrake::CareerEventCardDescription Description{};
        FMeasured Name,Detail,State;
    };
    std::array<FPreparedCard,latebrake::CareerEventCount> Prepared{};
    for(int I=0;I<Layout.Count;++I)
    {
        const auto& C=Layout.Cards[static_cast<std::size_t>(I)];
        auto& P=Prepared[static_cast<std::size_t>(I)];
        P.Description=latebrake::DescribeCareerEventCard(Snapshot,C.EventId);
        const auto& D=P.Description;
        if(!D.Valid)return false;
        P.Name=Measure(UTF8_TO_TCHAR(D.Name),
            FString::Printf(TEXT("EVENT %02d"),D.EventId+1),C.Name,0.88f*S);
        P.Detail=Measure(FString::Printf(TEXT("%dL | %s | %s"),D.Laps,
            D.Rival?TEXT("RIVAL"):TEXT("SOLO"),
            UTF8_TO_TCHAR(latebrake::CareerCardMedalLabel(D.Medal))),
            FString::Printf(TEXT("%dL | %s"),D.Laps,D.Rival?TEXT("RIVAL"):TEXT("SOLO")),
            C.Detail,0.74f*S);
        FString State=UTF8_TO_TCHAR(latebrake::CareerCardReadinessLabel(D.Readiness));
        if(D.Readiness==latebrake::CareerCardReadiness::LockedPrerequisite &&
            latebrake::IsCareerId(D.Prerequisite))
            State+=FString::Printf(TEXT(" | EVENT %02d"),D.Prerequisite+1);
        else if(D.Readiness==latebrake::CareerCardReadiness::NeedsCar &&
            latebrake::GarageState::IsVehicle(D.RequiredVehicle))
            State+=FString::Printf(TEXT(" | %s"),
                UTF8_TO_TCHAR(latebrake::GarageState::Definition(D.RequiredVehicle).Name));
        else if(D.FirstCredits>0)
            State+=FString::Printf(TEXT(" | %d CR"),D.FirstCredits);
        P.State=Measure(State,UTF8_TO_TCHAR(latebrake::CareerCardReadinessLabel(D.Readiness)),
            C.State,0.72f*S);
        // Do not replace a complete catalogue with a misleading partial
        // one. All required names and statuses must fit before any draw.
        if(P.Name.Scale<=0 || P.Detail.Scale<=0 || P.State.Scale<=0)return false;
    }

    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Ink)
    {
        DrawRect(Ink,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FMeasured& Value,const latebrake::DesktopMenuMouseRect& R,
        FLinearColor Ink)
    {
        DrawText(Value.Text,Ink,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Value.Extent.Height)*0.5),Font,Value.Scale);
    };
    float MouseX=0,MouseY=0;
    const bool HasCursor=PlayerOwner->GetMousePosition(MouseX,MouseY);
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Text(Header,Layout.Header,Gold);
    latebrake::CareerEventCardsPresentation Presentation;
    Presentation.Valid=true;Presentation.Layout=Layout;Presentation.Snapshot=Snapshot;
    for(int I=0;I<Layout.Count;++I)
    {
        const auto& C=Layout.Cards[static_cast<std::size_t>(I)];
        const auto& P=Prepared[static_cast<std::size_t>(I)];
        const auto& D=P.Description;
        const bool Hover=D.Selectable && HasCursor && C.Bounds.Contains(MouseX,MouseY);
        const FLinearColor Border=D.Selected?Gold:Hover?White:Muted;
        Rect(C.Bounds,D.Selected?FLinearColor(0.27f,0.21f,0.11f):
            Hover?FLinearColor(0.18f,0.28f,0.39f):
            D.Selectable?FLinearColor(0.12f,0.19f,0.27f):FLinearColor(0.08f,0.11f,0.15f));
        Rect({C.Bounds.X,C.Bounds.Y,3*S,C.Bounds.Height},Border);
        Text(P.Name,C.Name,D.Selected?Gold:D.Selectable?White:Muted);
        Text(P.Detail,C.Detail,Muted);
        const FLinearColor StateInk=D.Readiness==latebrake::CareerCardReadiness::NeedsCar?Coral:
            D.Readiness==latebrake::CareerCardReadiness::LockedPrerequisite?Muted:
            D.Readiness==latebrake::CareerCardReadiness::Mastered?Gold:Mint;
        Text(P.State,C.State,StateInk);
        Presentation.Rendered[static_cast<std::size_t>(I)]=true;
    }
    PresentedCareerCards=Presentation;
    return true;
}
