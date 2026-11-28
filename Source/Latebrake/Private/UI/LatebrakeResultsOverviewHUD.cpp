#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

bool ALatebrakeRaceHUD::DrawResultsOverview(
    const latebrake::ResultsOverviewLayout& Layout,
    const latebrake::ResultsOverviewReadout& Readout)
{
    if(!Canvas || !GEngine || !Layout.Valid || !Readout.Valid)return false;
    UFont* Small=GEngine->GetSmallFont();
    UFont* Medium=GEngine->GetMediumFont();
    if(!Small || !Medium)return false;
    const float S=static_cast<float>(Layout.Scale);
    const float LabelScale=0.76f*S;
    const auto FormatClock=[](double Seconds)
    {
        const auto Clock=latebrake::SplitHudRaceStatusClock(Seconds);
        return FString::Printf(TEXT("%02d:%02d.%02d"),Clock.Minutes,Clock.Seconds,Clock.Hundredths);
    };
    const FString Outcome=UTF8_TO_TCHAR(latebrake::ResultsOverviewOutcomeLabel(Readout.Outcome));
    const FString Persistence=UTF8_TO_TCHAR(latebrake::ResultsOverviewPersistenceLabel(Readout.Persistence));
    const std::array<FString,latebrake::ResultsOverviewMetricCount> Labels{{
        TEXT("ELAPSED TIME"),TEXT("BEST LAP"),TEXT("COMPLETED LAPS"),TEXT("TIMED LAPS")
    }};
    const std::array<FString,latebrake::ResultsOverviewMetricCount> Values{{
        Readout.ClockValid?FormatClock(Readout.ElapsedSeconds):FString(TEXT("N/A")),
        Readout.BestLapValid?FormatClock(Readout.BestLapSeconds):FString(TEXT("N/A")),
        FString::Printf(TEXT("%d / %d"),Readout.CompletedLaps,Readout.RequiredLaps),
        FString::Printf(TEXT("%d / %d"),Readout.TimedLaps,Readout.RequiredLaps)
    }};
    const auto Bar=latebrake::BuildResultsOverviewTimingBar(Layout.TimingTrack,Readout);
    const FString Timing=Bar.Valid?
        FString::Printf(TEXT("ACTIVE %.2fs | FEES %.2fs"),Readout.ActiveSeconds,Readout.PenaltySeconds):
        FString(TEXT("TIMING BREAKDOWN UNAVAILABLE"));
    const auto Measure=[&](const FString& Text,UFont* Font,float FontScale)
    {
        float W=0,H=0;
        GetTextSize(Text,W,H,Font,FontScale);
        return latebrake::HudRaceStatusTextSize{W,H};
    };
    latebrake::ResultsOverviewMeasurements Sizes;
    Sizes.Outcome=Measure(Outcome,Small,0.9f*S);
    Sizes.Persistence=Measure(Persistence,Small,LabelScale);
    Sizes.Timing=Measure(Timing,Small,LabelScale);
    for(std::size_t I=0;I<Labels.size();++I)
        Sizes.Labels[I]=Measure(Labels[I],Small,LabelScale);
    float ValueScale=0;
    for(const float Candidate:{1.10f,0.98f,0.86f,0.74f})
    {
        for(std::size_t I=0;I<Values.size();++I)
            Sizes.Values[I]=Measure(Values[I],Medium,Candidate*S);
        if(latebrake::CanDrawResultsOverview(Layout,Sizes))
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
    const FLinearColor Ink(0.95f,0.96f,1.f,1.f);
    const FLinearColor Muted(0.68f,0.76f,0.85f,1.f);
    const FLinearColor Mint(0.38f,0.89f,0.78f,1.f);
    const FLinearColor Coral(1.f,0.52f,0.38f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor Color)
    {
        if(R.Width>0.0 && R.Height>0.0)
            DrawRect(Color,static_cast<float>(R.X),static_cast<float>(R.Y),
                static_cast<float>(R.Width),static_cast<float>(R.Height));
    };
    const auto Text=[&](const FString& Value,const latebrake::DesktopMenuMouseRect& R,
        latebrake::HudRaceStatusTextSize Size,UFont* Font,float Scale,FLinearColor Color)
    {
        DrawText(Value,Color,static_cast<float>(R.X),
            static_cast<float>(R.Y+(R.Height-Size.Height)*0.5),Font,Scale);
    };
    using OutcomeKind=latebrake::ResultsOverviewOutcome;
    using SaveKind=latebrake::ResultsOverviewPersistence;
    const FLinearColor OutcomeInk=Readout.Outcome==OutcomeKind::InvalidFinish?Coral:
        Readout.Outcome==OutcomeKind::Incomplete?Muted:Gold;
    const FLinearColor SaveInk=Readout.Persistence==SaveKind::SaveFailed?Coral:
        Readout.Persistence==SaveKind::FirstBest || Readout.Persistence==SaveKind::ImprovedBest?
            Mint:Muted;
    Rect(Layout.Panel,Panel);
    Rect({Layout.Panel.X,Layout.Panel.Y,Layout.Panel.Width,2*S},OutcomeInk);
    Text(Outcome,Layout.Outcome,Sizes.Outcome,Small,0.9f*S,OutcomeInk);
    Text(Persistence,Layout.Persistence,Sizes.Persistence,Small,LabelScale,SaveInk);
    for(std::size_t I=0;I<Layout.Metrics.size();++I)
    {
        const auto& T=Layout.Metrics[I];
        Rect(T.Background,Tile);
        Text(Labels[I],T.Label,Sizes.Labels[I],Small,LabelScale,Muted);
        Text(Values[I],T.Value,Sizes.Values[I],Medium,ValueScale,Ink);
    }
    Text(Timing,Layout.TimingLabel,Sizes.Timing,Small,LabelScale,Muted);
    Rect(Layout.TimingTrack,Track);
    if(Bar.Valid)
    {
        Rect(Bar.Active,Mint);
        Rect(Bar.Fees,Coral);
    }
    return true;
}
