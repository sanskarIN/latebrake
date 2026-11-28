#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

bool ALatebrakeRaceHUD::DrawGarageComparisonPanel(
    const latebrake::GarageComparisonPanelLayout& Layout,
    const latebrake::GarageComparisonPanelReadout& Readout)
{
    if(!Canvas || !GEngine || !Layout.Valid || !Readout.Valid ||
        !latebrake::GarageState::IsVehicle(Readout.EquippedId) ||
        !latebrake::GarageState::IsVehicle(Readout.ViewedId))
        return false;
    UFont* Font=GEngine->GetSmallFont();
    if(!Font)return false;
    const float S=static_cast<float>(Layout.Scale);
    const float FontScale=0.78f*S;
    FString Title=FString::Printf(TEXT("%s vs %s"),
        UTF8_TO_TCHAR(latebrake::GarageState::Definition(Readout.ViewedId).Name),
        UTF8_TO_TCHAR(latebrake::GarageState::Definition(Readout.EquippedId).Name));
    const FString Context=FString::Printf(TEXT("%s | CONFIGURED EST."),
        UTF8_TO_TCHAR(latebrake::GarageComparisonOwnershipLabel(Readout.ViewedOwnership)));
    std::array<FString,latebrake::GarageComparisonMetricCount> Labels{};
    std::array<FString,latebrake::GarageComparisonMetricCount> Deltas{};
    std::array<latebrake::GarageComparisonBar,latebrake::GarageComparisonMetricCount> Bars{};
    const auto Measure=[&](const FString& Text)
    {
        float W=0.f,H=0.f;
        GetTextSize(Text,W,H,Font,FontScale);
        return latebrake::HudRaceStatusTextSize{W,H};
    };
    latebrake::GarageComparisonTextMeasurements Sizes;
    Sizes.Title=Measure(Title);
    if(!latebrake::HudRaceStatusTextFits(Layout.Title,Sizes.Title))
    {
        Title=FString::Printf(TEXT("VIEWED #%d vs EQUIPPED #%d"),
            Readout.ViewedId+1,Readout.EquippedId+1);
        Sizes.Title=Measure(Title);
    }
    Sizes.Context=Measure(Context);
    for(std::size_t I=0;I<Readout.Metrics.size();++I)
    {
        const auto& Metric=Readout.Metrics[I];
        Labels[I]=UTF8_TO_TCHAR(latebrake::GarageComparisonMetricLabel(Metric.Metric));
        // Suppress negative zero at the displayed one-decimal precision.
        Deltas[I]=Metric.Direction==latebrake::GarageComparisonDirection::Equal?
            FString(TEXT("0.0%")):FString::Printf(TEXT("%+.1f%%"),Metric.DeltaPercent);
        Sizes.Labels[I]=Measure(Labels[I]);
        Sizes.Deltas[I]=Measure(Deltas[I]);
        Bars[I]=latebrake::BuildGarageComparisonBar(Layout.Metrics[I].Track,
            Metric.DeltaPercent,Readout.AxisPercent);
        if(!Bars[I].Valid)return false;
    }
    if(!latebrake::CanDrawGarageComparisonPanel(Layout,Sizes))return false;

    const FLinearColor Background(0.065f,0.11f,0.17f,0.98f);
    const FLinearColor Track(0.18f,0.24f,0.31f,1.f);
    const FLinearColor Gold(1.f,0.73f,0.17f,1.f);
    const FLinearColor Ink(0.95f,0.96f,1.f,1.f);
    const FLinearColor Muted(0.68f,0.76f,0.85f,1.f);
    const FLinearColor Higher(0.38f,0.89f,0.78f,1.f);
    const FLinearColor Lower(1.f,0.52f,0.38f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,const FLinearColor& Color)
    {
        if(R.Width>0.0 && R.Height>0.0)
            DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FString& Value,const latebrake::DesktopMenuMouseRect& R,
        latebrake::HudRaceStatusTextSize Size,const FLinearColor& Color,bool Right=false)
    {
        DrawText(Value,Color,static_cast<float>(Right?R.X+R.Width-Size.Width:R.X),
            static_cast<float>(R.Y+(R.Height-Size.Height)*0.5),Font,FontScale);
    };
    Rect(Layout.Panel,Background);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2.0*S},Gold);
    Text(Title,Layout.Title,Sizes.Title,Gold);
    Text(Context,Layout.Context,Sizes.Context,Muted);
    for(std::size_t I=0;I<Bars.size();++I)
    {
        const auto& Metric=Layout.Metrics[I];
        const auto& Bar=Bars[I];
        const FLinearColor Color=Bar.Direction==latebrake::GarageComparisonDirection::Higher?
            Higher:Bar.Direction==latebrake::GarageComparisonDirection::Lower?Lower:Ink;
        Text(Labels[I],Metric.Label,Sizes.Labels[I],Ink);
        Text(Deltas[I],Metric.Delta,Sizes.Deltas[I],Color,true);
        Rect(Bar.Track,Track);
        Rect(Bar.Fill,Color);
        Rect({Bar.ZeroX-0.5*S,Bar.Track.Y,1.0*S,Bar.Track.Height},Muted);
    }
    return true;
}
