#include "UI/LatebrakeRaceHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ALatebrakeRaceHUD::DrawDriveCommandPanel(
    const latebrake::HudDriveCommandPanel& Layout,
    const latebrake::DriveCommandReview& Review)
{
    if(!Canvas || !GEngine || !GEngine->GetSmallFont())return;
    const auto Plot=latebrake::BuildHudDriveCommandPlot(Layout,Review);
    if(!Plot.Valid)return;
    const float S=static_cast<float>(Layout.Scale);
    const FLinearColor Background(0.018f,0.034f,0.062f,0.88f);
    const FLinearColor Graph(0.11f,0.15f,0.20f,0.9f);
    const FLinearColor Midline(0.31f,0.36f,0.42f,1.f);
    const FLinearColor Filtered(0.63f,0.72f,0.86f,1.f);
    const FLinearColor Moderated(0.37f,0.91f,0.78f,1.f);
    const FLinearColor Range(0.37f,0.91f,0.78f,0.33f);
    const FLinearColor Service(1.f,0.39f,0.32f,1.f);
    const FLinearColor Handbrake(1.f,0.72f,0.21f,1.f);
    const FLinearColor Support(0.45f,0.64f,0.95f,1.f);
    const FLinearColor Ink(0.95f,0.96f,1.f,1.f);
    const auto Rect=[&](const latebrake::DesktopMenuMouseRect& R,FLinearColor C)
    {
        if(R.Width>0.0 && R.Height>0.0)
            DrawRect(C,R.X,R.Y,R.Width,R.Height);
    };
    const auto Label=[&](const latebrake::DesktopMenuMouseRect& R,
        const FString& Full,const FString& Short,FLinearColor Color)
    {
        for(const auto& Text:{Full,Short})
        {
            if(Text.IsEmpty())continue;
            float W=0,H=0;
            GetTextSize(Text,W,H,GEngine->GetSmallFont(),0.72f*S);
            if(FMath::IsFinite(W) && FMath::IsFinite(H) &&
                W<=R.Width && H<=R.Height)
            {
                DrawText(Text,Color,R.X,R.Y,GEngine->GetSmallFont(),0.72f*S);
                return;
            }
        }
    };
    Rect(Layout.Panel,Background);
    Label(Layout.Header,TEXT("FILTERED (GRAY) / COMMAND (MINT)"),
        TEXT("COMMAND TRACE"),Ink);
    const auto& D=Review.Latest;
    Label(Layout.ThrottleTitle,FString::Printf(TEXT("THROTTLE  %+.0f > %+.0f%%"),
        D.FilteredThrottle*100.0,D.ModeratedThrottle*100.0),TEXT("THROTTLE"),Moderated);
    Label(Layout.SteeringTitle,FString::Printf(TEXT("STEERING  %+.0f > %+.0f%%"),
        D.FilteredSteering*100.0,D.ModeratedSteering*100.0),TEXT("STEERING"),Moderated);
    Label(Layout.BrakeTitle,TEXT("BRAKE (RED) / HAND (GOLD) / SUPPORT (BLUE)"),
        TEXT("BRAKE / HAND / SUPPORT"),Ink);
    Label(Layout.Footer,FString::Printf(TEXT("%s | %.1fs observed"),
        UTF8_TO_TCHAR(latebrake::DriveCommandPathName(Review.Path)),
        Review.ObservedSeconds),FString::Printf(TEXT("%.1fs observed / 4s"),
        Review.ObservedSeconds),Ink);
    for(const auto& R:{Layout.ThrottleGraph,Layout.SteeringGraph,
        Layout.BrakeGraph,Layout.SupportGraph})
        Rect(R,Graph);
    for(const auto& R:{Layout.ThrottleGraph,Layout.SteeringGraph})
        DrawLine(R.X,R.Y+R.Height*0.5,R.X+R.Width,R.Y+R.Height*0.5,Midline,1.f);
    const latebrake::DriveCommandColumn* Previous=nullptr;
    for(const auto& C:Plot.Columns)
    {
        if(!C.Visible)
        {
            Previous=nullptr;
            continue;
        }
        // The faint envelope shows the real extreme moderated commands
        // observed within this bin, including rapid direction changes
        // that cancel in the weighted average.
        DrawLine(C.CenterX,C.ThrottleRangeTop,C.CenterX,C.ThrottleRangeBottom,Range,2.f*S);
        DrawLine(C.CenterX,C.SteeringRangeTop,C.CenterX,C.SteeringRangeBottom,Range,2.f*S);
        // A short visible stroke even if only one physics sample exists.
        DrawLine(C.Throttle.X,C.ModeratedThrottleY,
            C.Throttle.X+C.Throttle.Width,C.ModeratedThrottleY,Moderated,1.2f*S);
        DrawLine(C.Steering.X,C.ModeratedSteeringY,
            C.Steering.X+C.Steering.Width,C.ModeratedSteeringY,Moderated,1.2f*S);
        DrawLine(C.Throttle.X,C.FilteredThrottleY,
            C.Throttle.X+C.Throttle.Width,C.FilteredThrottleY,Filtered,0.8f*S);
        DrawLine(C.Steering.X,C.FilteredSteeringY,
            C.Steering.X+C.Steering.Width,C.FilteredSteeringY,Filtered,0.8f*S);
        if(Previous)
        {
            DrawLine(Previous->CenterX,Previous->FilteredThrottleY,
                C.CenterX,C.FilteredThrottleY,Filtered,0.8f*S);
            DrawLine(Previous->CenterX,Previous->FilteredSteeringY,
                C.CenterX,C.FilteredSteeringY,Filtered,0.8f*S);
            DrawLine(Previous->CenterX,Previous->ModeratedThrottleY,
                C.CenterX,C.ModeratedThrottleY,Moderated,1.2f*S);
            DrawLine(Previous->CenterX,Previous->ModeratedSteeringY,
                C.CenterX,C.ModeratedSteeringY,Moderated,1.2f*S);
        }
        Rect(C.ServiceBrake,Service);
        Rect(C.Handbrake,Handbrake);
        Rect(C.GroundSupport,Support);
        Previous=&C;
    }
}
