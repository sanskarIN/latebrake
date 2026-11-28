#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

bool ALatebrakeRaceHUD::DrawRaceStatusPanel(
    const latebrake::HudRaceStatusLayout& Layout,
    const latebrake::HudRaceStatusReadout& Status)
{
    if(!Canvas || !GEngine || !Layout.Valid || !Status.Valid)
        return false;
    UFont* Small=GEngine->GetSmallFont();
    UFont* Medium=GEngine->GetMediumFont();
    if(!Small || !Medium)return false;
    const float S=static_cast<float>(Layout.Scale);
    const float LabelScale=0.76f*S;
    const float ProgressScale=0.78f*S;
    const std::array<FString,latebrake::HudRaceStatusTileCount> Labels{{
        TEXT("RACE TIME"),TEXT("LAP"),TEXT("NEXT GATE"),
        Status.MilesPerHour?TEXT("SPEED / MPH"):TEXT("SPEED / KM/H")
    }};
    const std::array<FString,latebrake::HudRaceStatusTileCount> Values{{
        FString::Printf(TEXT("%02d:%02d.%02d"),Status.Clock.Minutes,
            Status.Clock.Seconds,Status.Clock.Hundredths),
        FString::Printf(TEXT("%d / %d"),Status.CurrentLap,Status.RequiredLaps),
        FString::Printf(TEXT("%d / %d"),Status.NextGate,Status.GateCount),
        FString::Printf(TEXT("%d"),Status.DisplaySpeed)
    }};
    const FString Progress=FString::Printf(TEXT("CHECKPOINTS CLEARED  %d / %d"),
        Status.Progress.CreditedGates,Status.Progress.TotalGates);
    const auto Measure=[&](const FString& Text,UFont* Font,float Scale)
    {
        float W=0.f,H=0.f;
        GetTextSize(Text,W,H,Font,Scale);
        return latebrake::HudRaceStatusTextSize{W,H};
    };
    latebrake::HudRaceStatusMeasurements Sizes;
    for(std::size_t I=0;I<Labels.size();++I)
        Sizes.Labels[I]=Measure(Labels[I],Small,LabelScale);
    Sizes.Progress=Measure(Progress,Small,ProgressScale);
    // Choose one consistent value scale rather than allowing each tile
    // to shrink independently as numbers change. Do not ellipsize a clock,
    // gate count or unit. If none fits, retain the original essential rows.
    float ValueScale=0.f;
    for(const float Candidate:{1.10f,0.98f,0.86f,0.74f})
    {
        for(std::size_t I=0;I<Values.size();++I)
            Sizes.Values[I]=Measure(Values[I],Medium,Candidate*S);
        if(latebrake::CanDrawHudRaceStatus(Layout,Sizes))
        {
            ValueScale=Candidate*S;
            break;
        }
    }
    if(ValueScale<=0.f)return false;
    const auto Bar=latebrake::BuildHudRaceStatusProgress(Layout,Status.Progress);
    if(!Bar.Valid)return false;

    // All geometry and text were checked before the first draw call.
    // These colors match the established LateBrake dark/gold dashboard.
    const FLinearColor Panel(0.018f,0.034f,0.062f,0.90f);
    const FLinearColor Tile(0.075f,0.12f,0.18f,0.96f);
    const FLinearColor Track(0.18f,0.23f,0.30f,1.f);
    const FLinearColor Gold(1.f,0.73f,0.17f,1.f);
    const FLinearColor Ink(0.95f,0.96f,1.f,1.f);
    const FLinearColor Muted(0.68f,0.76f,0.85f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,
        const FLinearColor& Color)
    {
        if(R.Width>0.0 && R.Height>0.0)
            DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FString& Value,
        const latebrake::DesktopMenuMouseRect& R,
        latebrake::HudRaceStatusTextSize Size,UFont* Font,
        float Scale,const FLinearColor& Color)
    {
        DrawText(Value,Color,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Size.Height)*0.5),
            Font,Scale);
    };
    Rect(Layout.Panel,Panel);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2.0*S},Gold);
    for(std::size_t I=0;I<Layout.Tiles.size();++I)
    {
        const auto& T=Layout.Tiles[I];
        Rect(T.Background,Tile);
        Text(Labels[I],T.Label,Sizes.Labels[I],Small,LabelScale,Muted);
        Text(Values[I],T.Value,Sizes.Values[I],Medium,ValueScale,I==0?Gold:Ink);
    }
    Text(Progress,Layout.ProgressLabel,Sizes.Progress,Small,ProgressScale,Muted);
    for(int I=0;I<Bar.Count;++I)
    {
        const auto& Segment=Bar.Segments[static_cast<std::size_t>(I)];
        Rect(Segment.Track,Track);
        Rect(Segment.Fill,Gold);
    }
    return true;
}
