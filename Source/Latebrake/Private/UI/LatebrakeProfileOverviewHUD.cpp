#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

bool ALatebrakeRaceHUD::DrawProfileOverview(
    const latebrake::ProfileOverviewLayout& Layout,
    const latebrake::ProfileOverviewReadout& Data)
{
    if(!Canvas || !GEngine || !Layout.Valid || !Data.Valid ||
        !latebrake::GarageState::IsVehicle(Data.EquippedCar))return false;
    UFont* Small=GEngine->GetSmallFont();
    UFont* Medium=GEngine->GetMediumFont();
    if(!Small || !Medium)return false;
    const auto Bars=latebrake::BuildProfileOverviewBars(Layout,Data);
    if(!Bars.Valid)return false;
    const float S=static_cast<float>(Layout.Scale),LabelScale=0.76f*S;
    const auto Clock=[](bool Valid,double Seconds)
    {
        if(!Valid)return FString(TEXT("N/A"));
        const auto C=latebrake::SplitHudRaceStatusClock(Seconds);
        return FString::Printf(TEXT("%02d:%02d.%02d"),C.Minutes,C.Seconds,C.Hundredths);
    };
    const FString Header=UTF8_TO_TCHAR(latebrake::ProfileOverviewStatusLabel(Data.Status));
    const FString Medals=FString::Printf(TEXT("MEDALS %d/%d | B %d | S %d | G %d"),
        Data.CareerCompleted,latebrake::CareerEventCount,Data.Bronze,Data.Silver,Data.Gold);
    const FString Cars=FString::Printf(TEXT("OWNED CARS %d/%d | ENGINE %d/3 | GRIP %d/3"),
        Data.OwnedCars,latebrake::VehicleCount,Data.EngineLevel,Data.GripLevel);
    const std::array<FString,latebrake::ProfileOverviewTiles> Labels{{
        TEXT("RACES FINISHED"),TEXT("BANKED CREDITS"),TEXT("EQUIPPED CAR"),
        TEXT("CIRCUIT BEST"),TEXT("BEST LAP"),TEXT("CAREER MEDALS")
    }};
    const std::array<FString,latebrake::ProfileOverviewTiles> Values{{
        FString::FromInt(Data.CompletedRaces),FString::FromInt(Data.Credits),
        FString(UTF8_TO_TCHAR(latebrake::GarageState::Definition(Data.EquippedCar).Name)),
        Clock(Data.CircuitBestValid,Data.CircuitBest),Clock(Data.BestLapValid,Data.BestLap),
        FString::Printf(TEXT("%d / %d"),Data.CareerCompleted,latebrake::CareerEventCount)
    }};
    const auto Measure=[&](const FString& Text,UFont* Font,float Scale)
    {
        float W=0,H=0;
        GetTextSize(Text,W,H,Font,Scale);
        return latebrake::HudRaceStatusTextSize{W,H};
    };
    latebrake::ProfileOverviewMeasurements Sizes;
    Sizes.Header=Measure(Header,Small,0.86f*S);
    Sizes.Medals=Measure(Medals,Small,LabelScale);
    Sizes.Vehicles=Measure(Cars,Small,LabelScale);
    for(std::size_t I=0;I<Labels.size();++I)
        Sizes.Labels[I]=Measure(Labels[I],Small,LabelScale);
    float ValueScale=0;
    for(const float Candidate:{1.08f,0.96f,0.84f,0.72f})
    {
        for(std::size_t I=0;I<Values.size();++I)
            Sizes.Values[I]=Measure(Values[I],Medium,Candidate*S);
        if(latebrake::CanDrawProfileOverview(Layout,Sizes))
        {
            ValueScale=Candidate*S;
            break;
        }
    }
    if(ValueScale<=0)return false;
    const FLinearColor Panel(0.04f,0.07f,0.11f,0.98f);
    const FLinearColor Tile(0.075f,0.12f,0.18f,0.96f);
    const FLinearColor Track(0.18f,0.24f,0.31f,1.f);
    const FLinearColor Gold(1.f,0.73f,0.17f,1.f);
    const FLinearColor Silver(0.75f,0.82f,0.9f,1.f);
    const FLinearColor Bronze(0.76f,0.43f,0.24f,1.f);
    const FLinearColor Ink(0.95f,0.96f,1.f,1.f);
    const FLinearColor Muted(0.68f,0.76f,0.85f,1.f);
    const FLinearColor Mint(0.38f,0.89f,0.78f,1.f);
    const FLinearColor Coral(1.f,0.52f,0.38f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Color)
    {
        if(R.Width>0 && R.Height>0)
            DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FString& Value,const latebrake::DesktopMenuMouseRect& R,
        latebrake::HudRaceStatusTextSize Size,UFont* Font,float Scale,FLinearColor Color)
    {
        DrawText(Value,Color,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Size.Height)*0.5),Font,Scale);
    };
    const FLinearColor HeaderInk=Data.Status==latebrake::ProfileOverviewStatus::Current?Gold:Coral;
    Rect(Layout.Panel,Panel);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},HeaderInk);
    Text(Header,Layout.Header,Sizes.Header,Small,0.86f*S,HeaderInk);
    for(std::size_t I=0;I<Layout.Tiles.size();++I)
    {
        const auto& T=Layout.Tiles[I];
        Rect(T.Bounds,Tile);
        Text(Labels[I],T.Label,Sizes.Labels[I],Small,LabelScale,Muted);
        Text(Values[I],T.Value,Sizes.Values[I],Medium,ValueScale,Ink);
    }
    Text(Medals,Layout.MedalLabel,Sizes.Medals,Small,LabelScale,Muted);
    Rect(Layout.MedalTrack,Track);
    Rect(Bars.Medals[0],Bronze);
    Rect(Bars.Medals[1],Silver);
    Rect(Bars.Medals[2],Gold);
    Text(Cars,Layout.VehicleLabel,Sizes.Vehicles,Small,LabelScale,Muted);
    Rect(Layout.VehicleTrack,Track);
    Rect(Bars.Vehicles,Mint);
    return true;
}
