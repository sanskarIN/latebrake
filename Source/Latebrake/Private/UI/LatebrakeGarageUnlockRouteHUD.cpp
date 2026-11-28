#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

bool ALatebrakeRaceHUD::DrawGarageUnlockRoute(
    const latebrake::GarageUnlockRouteLayout& Layout,
    const latebrake::GarageUnlockRouteReadout& Data)
{
    if(!Canvas || !GEngine || !GEngine->GetSmallFont() || !Layout.Valid ||
        !Data.Valid || Layout.Count!=Data.Count)return false;
    const auto Bar=latebrake::BuildGarageUnlockRouteBar(Layout.Track,Data);
    if(!Bar.Valid)return false;
    UFont* Font=GEngine->GetSmallFont();
    const float S=static_cast<float>(Layout.Scale);
    struct FCaption {FString Text;latebrake::HudRaceStatusTextSize Size{};float Scale=0;};
    const auto Fit=[&](const FString& Primary,const FString& Short,
        const latebrake::DesktopMenuMouseRect& Bounds,float Preferred)
    {
        for(const auto& Text:{Primary,Short})
        {
            if(Text.IsEmpty())continue;
            for(int I=0;I<4;++I)
            {
                const float Scale=Preferred*(1.f-0.08f*I);
                float W=0,H=0;
                GetTextSize(Text,W,H,Font,Scale);
                const latebrake::HudRaceStatusTextSize Size{W,H};
                if(latebrake::HudRaceStatusTextFits(Bounds,Size))
                    return FCaption{Text,Size,Scale};
            }
        }
        return FCaption{};
    };
    const auto Header=Fit(FString::Printf(TEXT("UNLOCK ROUTE / %s"),
        UTF8_TO_TCHAR(latebrake::GarageState::Definition(Data.Viewed).Name)),
        TEXT("VIEWED CAR ROUTE"),Layout.Header,0.86f*S);
    const auto Summary=Fit(FString::Printf(TEXT("%d CR TOTAL | NEED %d | %d/%d FUNDED"),
        Data.TotalCost,Data.Shortfall,Data.FundedCount,Data.Count),
        FString::Printf(TEXT("%d CR / NEED %d"),Data.TotalCost,Data.Shortfall),
        Layout.Summary,0.72f*S);
    std::array<FCaption,latebrake::VehicleCount-1> Steps{};
    latebrake::GarageUnlockRouteMeasurements Sizes;
    Sizes.Header=Header.Size;Sizes.Summary=Summary.Size;
    for(int I=0;I<Data.Count;++I)
    {
        const auto& Step=Data.Steps[static_cast<std::size_t>(I)];
        const FString Name=UTF8_TO_TCHAR(latebrake::GarageState::Definition(Step.Vehicle).Name);
        const FString Status=Step.Funded?TEXT("FUNDED"):
            FString::Printf(TEXT("NEED %d"),Step.CumulativeShortfall);
        Steps[static_cast<std::size_t>(I)]=Fit(
            FString::Printf(TEXT("%s%s | %d CR | %s"),Step.Next?TEXT("NEXT: "):TEXT(""),
                *Name,Step.Cost,*Status),
            FString::Printf(TEXT("%s %d | %s"),*Name,Step.Cost,*Status),
            Layout.Steps[static_cast<std::size_t>(I)],0.72f*S);
        Sizes.Steps[static_cast<std::size_t>(I)]=Steps[static_cast<std::size_t>(I)].Size;
    }
    if(!latebrake::CanDrawGarageUnlockRoute(Layout,Sizes))return false;
    const FLinearColor Panel(0.04f,0.07f,0.11f,0.98f),Gold(1.f,0.73f,0.17f,1.f);
    const FLinearColor Mint(0.38f,0.89f,0.78f,1.f),Muted(0.68f,0.76f,0.85f,1.f);
    const FLinearColor Coral(1.f,0.52f,0.38f,1.f),Track(0.18f,0.24f,0.31f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Ink)
    {
        if(R.Width>0 && R.Height>0)
            DrawRect(Ink,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FCaption& Caption,const latebrake::DesktopMenuMouseRect& R,FLinearColor Ink)
    {
        DrawText(Caption.Text,Ink,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Caption.Size.Height)*0.5),Font,Caption.Scale);
    };
    Rect(Layout.Panel,Panel);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},Gold);
    Text(Header,Layout.Header,Gold);
    Text(Summary,Layout.Summary,Data.Shortfall>0?Coral:Mint);
    Rect(Layout.Track,Track);
    Rect(Bar.Funded,Mint);
    for(int I=0;I<Bar.BoundaryCount;++I)
    {
        const double X=Bar.Boundaries[static_cast<std::size_t>(I)];
        const double Left=std::max(Layout.Track.X,X-S*0.5);
        const double Right=std::min(Layout.Track.X+Layout.Track.Width,X+S*0.5);
        Rect({Left,Layout.Track.Y,Right-Left,Layout.Track.Height},Panel);
    }
    for(int I=0;I<Data.Count;++I)
    {
        const auto& Step=Data.Steps[static_cast<std::size_t>(I)];
        Text(Steps[static_cast<std::size_t>(I)],Layout.Steps[static_cast<std::size_t>(I)],
            Step.Next?Gold:Step.Funded?Mint:Muted);
    }
    return true;
}
