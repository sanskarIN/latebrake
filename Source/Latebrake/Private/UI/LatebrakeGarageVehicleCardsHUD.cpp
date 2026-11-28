#include "UI/LatebrakeRaceHUD.h"

#include "Game/LatebrakeRaceGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

bool ALatebrakeRaceHUD::TryHandleGarageVehicleCardsPointer(double X,double Y)
{
#if PLATFORM_ANDROID
    return false;
#else
    auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    if(!Mode || !Canvas || !PlayerOwner || !PlayerOwner->IsLocalController() ||
        !Mode->IsMenuOpen())return false;
    const auto Current=Mode->GetGarageVehicleCardsSnapshot();
    const auto Hit=latebrake::HitPresentedGarageVehicleCards(PresentedGarageCards,
        Current,Canvas->SizeX,Canvas->SizeY,X,Y,GaragePalette.Open);
    if(!Hit.Consumed)return false;
    PresentedGarageCards={};
    if(Hit.Vehicle>=0)Mode->BrowseGarageVehicleCard(Hit.Vehicle,Current);
    return true;
#endif
}

bool ALatebrakeRaceHUD::DrawGarageVehicleCards(const ALatebrakeRaceGameMode* Mode,
    const latebrake::GarageVehicleCardsLayout& Layout)
{
    PresentedGarageCards={};
    if(!Mode || !Canvas || !PlayerOwner || !GEngine || !GEngine->GetSmallFont() ||
        !latebrake::ValidGarageVehicleCardsLayout(Layout))return false;
    const auto Snapshot=Mode->GetGarageVehicleCardsSnapshot();
    if(!Snapshot.Open)return false;
    const auto Cards=latebrake::DescribeGarageVehicleCards(Snapshot,&GarageCatalogue);
    const bool Enabled=Snapshot.CanBrowse() && !GaragePalette.Open;
    const float S=static_cast<float>(Layout.Scale);
    UFont* Font=GEngine->GetSmallFont();
    const FLinearColor White(0.94f,0.96f,1.f),Gold(1.f,0.73f,0.17f);
    const FLinearColor Muted(0.68f,0.76f,0.85f),Surface(0.04f,0.07f,0.11f,0.98f);
    const FLinearColor Mint(0.38f,0.89f,0.78f,1.f),Coral(1.f,0.52f,0.38f,1.f);
    struct FMeasured
    {
        FString Text;
        float Width=0,Height=0,Scale=0;
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
                    return FMeasured{Text,W,H,Scale};
            }
        }
        return FMeasured{};
    };
    const auto Header=Measure(GaragePalette.Open?TEXT("CLOSE PALETTE TO BROWSE"):
        !Enabled?TEXT("SHOWROOM INPUT LOCKED"):
        Snapshot.ReadOnly?TEXT("READ-ONLY | CLICK TO VIEW"):
        TEXT("CLICK TO VIEW | SELECT TO EQUIP"),TEXT("VEHICLE COLLECTION"),
        Layout.Header,0.82f*S);
    if(Header.Scale<=0)return false;
    struct FPrepared {FMeasured Name,State,Cost;};
    std::array<FPrepared,latebrake::VehicleCount> Prepared{};
    for(std::size_t I=0;I<Cards.size();++I)
    {
        const auto& D=Cards[I];
        const auto& C=Layout.Cards[I];
        auto& P=Prepared[I];
        if(!D.Valid)return false;
        P.Name=Measure(UTF8_TO_TCHAR(D.Name),FString(),C.Name,0.91f*S);
        const FString State=UTF8_TO_TCHAR(latebrake::GarageCardStatusName(D.Status));
        using Status=latebrake::GarageCardStatus;
        P.State=Measure(State,D.Status==Status::RouteRequired?TEXT("ROUTE FIRST"):
            D.Status==Status::CreditsRequired?TEXT("NEED CREDITS"):State,C.State,0.72f*S);
        FString Cost;
        switch(D.Status)
        {
        case Status::Equipped:
        case Status::Owned:
            Cost=D.Customized?TEXT("CUSTOM LOOK"):TEXT("FACTORY LOOK");break;
        case Status::UnlockReady:
            Cost=FString::Printf(TEXT("%d CR"),D.RouteCost);break;
        case Status::RouteRequired:
            Cost=FString::Printf(TEXT("ROUTE %d CR"),D.RouteCost);break;
        case Status::CreditsRequired:
            Cost=FString::Printf(TEXT("NEED %d CR"),D.RouteShortfall);break;
        }
        P.Cost=Measure(Cost,FString(),C.Cost,0.72f*S);
        if(P.Name.Scale<=0 || P.State.Scale<=0 || P.Cost.Scale<=0)return false;
    }
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Ink)
    {
        DrawRect(Ink,static_cast<float>(R.X),static_cast<float>(R.Y),
            static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FMeasured& V,const latebrake::DesktopMenuMouseRect& R,FLinearColor Ink)
    {
        DrawText(V.Text,Ink,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-V.Height)*0.5),Font,V.Scale);
    };
    float MouseX=0,MouseY=0;
    const bool HasCursor=PlayerOwner->GetMousePosition(MouseX,MouseY);
    Rect(Layout.Panel,Surface);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Text(Header,Layout.Header,Enabled?Gold:Muted);
    latebrake::GarageVehicleCardsPresentation Presentation;
    Presentation.Valid=true;Presentation.Layout=Layout;Presentation.Snapshot=Snapshot;
    for(std::size_t I=0;I<Cards.size();++I)
    {
        const auto& D=Cards[I];const auto& C=Layout.Cards[I];const auto& P=Prepared[I];
        const bool Hover=Enabled && HasCursor && C.Bounds.Contains(MouseX,MouseY);
        Rect(C.Bounds,D.Viewed?FLinearColor(0.27f,0.21f,0.11f):
            Hover?FLinearColor(0.18f,0.28f,0.39f):FLinearColor(0.12f,0.19f,0.27f));
        Rect({C.Bounds.X,C.Bounds.Y,3*S,C.Bounds.Height},D.Viewed?Gold:Hover?White:Muted);
        Text(P.Name,C.Name,D.Viewed?Gold:White);
        Rect(C.Swatch,FLinearColor(D.Swatch.R,D.Swatch.G,D.Swatch.B,1.f));
        using Status=latebrake::GarageCardStatus;
        const auto Ink=D.Status==Status::Equipped?Gold:
            D.Status==Status::Owned || D.Status==Status::UnlockReady?Mint:
            D.Status==Status::CreditsRequired?Coral:Muted;
        Text(P.State,C.State,Ink);
        Text(P.Cost,C.Cost,Muted);
        Presentation.Rendered[I]=true;
    }
    PresentedGarageCards=Presentation;
    return true;
}
