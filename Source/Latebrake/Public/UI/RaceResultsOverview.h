#pragma once

#include "Racing/FinishedRaceInsights.h"
#include "Racing/QuickRaceRecordReview.h"
#include "UI/HudRaceStatusPanel.h"
#include "UI/HudMenuReadablePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
inline constexpr int ResultsOverviewMetricCount=4;
inline constexpr int ResultsOverviewMinimumDetails=5;
enum class ResultsOverviewOutcome : std::uint8_t
{
    Incomplete, InvalidFinish, Complete, PracticeComplete, ExperimentalComplete
};
enum class ResultsOverviewPersistence : std::uint8_t
{
    NotApplicable, SessionOnly, ReadOnly, SaveFailed, FirstBest, ImprovedBest,
    MatchedBest, ExistingBest, NotVerified
};

struct ResultsOverviewContext final
{
    EventKind Event=EventKind::Circuit;
    bool Career=false;
    bool Practice=false;
    bool Experimental=false;
    bool ReadOnly=false;
    bool SaveFailed=false;
    double PreviousBest=0;
    double SavedBest=0;
};

struct ResultsOverviewReadout final
{
    bool Valid=false;
    ResultsOverviewOutcome Outcome=ResultsOverviewOutcome::Incomplete;
    ResultsOverviewPersistence Persistence=ResultsOverviewPersistence::NotApplicable;
    FinishTimingEligibility Eligibility=FinishTimingEligibility::RaceIncomplete;
    int CompletedLaps=0;
    int RequiredLaps=0;
    int TimedLaps=0;
    bool ClockValid=false;
    double ElapsedSeconds=0;
    bool BreakdownValid=false;
    double ActiveSeconds=0;
    double PenaltySeconds=0;
    bool BestLapValid=false;
    double BestLapSeconds=0;
    double SavedBestSeconds=0;
    double RecordDeltaSeconds=0;
};

inline const char* ResultsOverviewOutcomeLabel(ResultsOverviewOutcome outcome) noexcept
{
    switch(outcome)
    {
    case ResultsOverviewOutcome::Incomplete:return "ATTEMPT IN PROGRESS";
    case ResultsOverviewOutcome::InvalidFinish:return "FINISH NOT RECORDABLE";
    case ResultsOverviewOutcome::Complete:return "RACE COMPLETE";
    case ResultsOverviewOutcome::PracticeComplete:return "PRACTICE COMPLETE";
    case ResultsOverviewOutcome::ExperimentalComplete:return "EXPERIMENTAL FINISH";
    default:return "RESULT UNAVAILABLE";
    }
}
inline const char* ResultsOverviewPersistenceLabel(ResultsOverviewPersistence status) noexcept
{
    switch(status)
    {
    case ResultsOverviewPersistence::NotApplicable:return "NO FINISH RECORD";
    case ResultsOverviewPersistence::SessionOnly:return "SESSION ONLY / NO SAVED REWARDS";
    case ResultsOverviewPersistence::ReadOnly:return "READ-ONLY PROFILE";
    case ResultsOverviewPersistence::SaveFailed:return "SAVE FAILED / NOT PERSISTED";
    case ResultsOverviewPersistence::FirstBest:return "FIRST SAVED PERSONAL BEST";
    case ResultsOverviewPersistence::ImprovedBest:return "NEW SAVED PERSONAL BEST";
    case ResultsOverviewPersistence::MatchedBest:return "MATCHED SAVED PERSONAL BEST";
    case ResultsOverviewPersistence::ExistingBest:return "PREVIOUS PERSONAL BEST RETAINED";
    case ResultsOverviewPersistence::NotVerified:return "CHECK DETAILS FOR SAVE STATUS";
    default:return "SAVE STATUS UNAVAILABLE";
    }
}

// The result card is deliberately separate from award decisions.
// Do not infer a save from a valid clock, a career result, or a positive
// credit count. Practice and experimental fleet finishes are session-only.
inline ResultsOverviewReadout ReadResultsOverview(const RaceProgress& race,
    ResultsOverviewContext context) noexcept
{
    ResultsOverviewReadout out;
    const int completed=race.GetCompletedLaps(),required=race.GetRequiredLaps();
    if(!IsValidEvent(context.Event) || required<=0 || completed<0 || completed>required ||
        (context.Career && context.Practice) ||
        (context.Experimental && context.Practice))
        return out;
    out.Valid=true;
    out.CompletedLaps=completed;
    out.RequiredLaps=required;
    out.TimedLaps=race.GetTimedLapCount();
    out.Eligibility=ValidateFinishTiming(race);
    const double elapsed=race.GetElapsedSeconds();
    const double fee=race.GetTotalPenaltySeconds();
    out.ClockValid=std::isfinite(elapsed) && elapsed>=0.0 &&
        elapsed<=HudRaceStatusMaximumElapsed;
    if(out.ClockValid)
    {
        out.ElapsedSeconds=elapsed;
        if(std::isfinite(fee) && fee>=0.0 && fee<=elapsed)
        {
            out.BreakdownValid=true;
            out.ActiveSeconds=elapsed-fee;
            out.PenaltySeconds=fee;
        }
    }
    if(!race.IsFinished())
        return out;
    if(out.Eligibility!=FinishTimingEligibility::Eligible)
    {
        out.Outcome=ResultsOverviewOutcome::InvalidFinish;
        return out;
    }
    const auto insights=BuildFinishedRaceInsights(race);
    if(insights.Valid && IsValidFinishedRaceTime(insights.BestLapSeconds))
    {
        out.BestLapValid=true;
        out.BestLapSeconds=insights.BestLapSeconds;
    }
    if(context.Practice || context.Experimental)
    {
        out.Outcome=context.Practice?ResultsOverviewOutcome::PracticeComplete:
            ResultsOverviewOutcome::ExperimentalComplete;
        out.Persistence=ResultsOverviewPersistence::SessionOnly;
        return out;
    }
    out.Outcome=ResultsOverviewOutcome::Complete;
    if(context.SaveFailed)
    {
        out.Persistence=ResultsOverviewPersistence::SaveFailed;
        return out;
    }
    if(context.ReadOnly)
    {
        out.Persistence=ResultsOverviewPersistence::ReadOnly;
        return out;
    }
    out.Persistence=ResultsOverviewPersistence::NotVerified;
    if(context.Career)return out;
    const auto record=ReviewQuickRaceRecord(context.Event,false,true,false,
        elapsed,context.PreviousBest,context.SavedBest);
    switch(record.State)
    {
    case QuickRaceRecordState::FirstRecord:out.Persistence=ResultsOverviewPersistence::FirstBest;break;
    case QuickRaceRecordState::Improved:out.Persistence=ResultsOverviewPersistence::ImprovedBest;break;
    case QuickRaceRecordState::Matched:out.Persistence=ResultsOverviewPersistence::MatchedBest;break;
    case QuickRaceRecordState::Slower:out.Persistence=ResultsOverviewPersistence::ExistingBest;break;
    default:return out;
    }
    out.SavedBestSeconds=record.SavedBestSeconds;
    out.RecordDeltaSeconds=record.DeltaSeconds;
    return out;
}

struct ResultsOverviewTile final
{
    DesktopMenuMouseRect Background{};
    DesktopMenuMouseRect Label{};
    DesktopMenuMouseRect Value{};
};
struct ResultsOverviewLayout final
{
    bool Valid=false;
    double Scale=1;
    int ReservedRows=0;
    double NextDetailY=0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Outcome{};
    DesktopMenuMouseRect Persistence{};
    std::array<ResultsOverviewTile,ResultsOverviewMetricCount> Metrics{};
    DesktopMenuMouseRect TimingLabel{};
    DesktopMenuMouseRect TimingTrack{};
};

inline ResultsOverviewLayout CalculateResultsOverviewLayout(HudLayout hud,
    HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double currentY,int drawnRows,bool results,bool touch) noexcept
{
    if(!results || touch || hud.Compact || !vertical.Valid || !readable.Valid ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0 ||
        !std::isfinite(currentY) || drawnRows<0 || drawnRows>vertical.MaxDetailsRows ||
        vertical.MaxDetailsRows>90 ||
        !std::isfinite(vertical.OverflowNoticeY) ||
        !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)
        return {};
    const double s=hud.Scale,step=28*s;
    if(std::abs(currentY-hud.Top-drawnRows*step)>0.02)return {};
    const double width=std::min({safeWidth,readable.X+readable.Width-hud.Left,680*s});
    if(width<300*s)return {};
    constexpr int reserve=7;
    const int remaining=vertical.MaxDetailsRows-drawnRows-reserve;
    if(remaining<ResultsOverviewMinimumDetails)return {};
    ResultsOverviewLayout out;
    out.Scale=s;
    out.ReservedRows=reserve;
    out.NextDetailY=currentY+reserve*step;
    out.Panel={hud.Left,currentY,width,182*s};
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    if(!HudRadarContains(owner,out.Panel) ||
        out.NextDetailY<out.Panel.Y+out.Panel.Height ||
        out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
        return {};
    const double pad=8*s,gap=6*s,x=hud.Left+pad,w=width-2*pad;
    out.Outcome={x,currentY+5*s,w,17*s};
    out.Persistence={x,currentY+25*s,w,15*s};
    const double tileW=(w-gap)*0.5,tileH=48*s;
    for(int i=0;i<ResultsOverviewMetricCount;++i)
    {
        auto& tile=out.Metrics[static_cast<std::size_t>(i)];
        tile.Background={x+(i%2)*(tileW+gap),currentY+44*s+(i/2)*(tileH+gap),tileW,tileH};
        tile.Label={tile.Background.X+5*s,tile.Background.Y+3*s,tileW-10*s,13*s};
        tile.Value={tile.Background.X+5*s,tile.Background.Y+18*s,tileW-10*s,25*s};
        if(!HudRadarContains(out.Panel,tile.Background) ||
            !HudRadarContains(tile.Background,tile.Label) ||
            !HudRadarContains(tile.Background,tile.Value) ||
            HudRadarOverlaps(tile.Label,tile.Value))
            return {};
    }
    out.TimingLabel={x,currentY+149*s,w,15*s};
    out.TimingTrack={x,currentY+168*s,w,6*s};
    const std::array<DesktopMenuMouseRect,4> regions{{out.Outcome,out.Persistence,
        out.TimingLabel,out.TimingTrack}};
    for(std::size_t i=0;i<regions.size();++i)
    {
        if(!HudRadarContains(out.Panel,regions[i]))return {};
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(regions[i],regions[j]))return {};
        for(const auto& tile:out.Metrics)
            if(HudRadarOverlaps(regions[i],tile.Background))return {};
    }
    for(std::size_t i=0;i<out.Metrics.size();++i)
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(out.Metrics[i].Background,out.Metrics[j].Background))return {};
    out.Valid=true;
    return out;
}

struct ResultsOverviewMeasurements final
{
    HudRaceStatusTextSize Outcome{};
    HudRaceStatusTextSize Persistence{};
    HudRaceStatusTextSize Timing{};
    std::array<HudRaceStatusTextSize,ResultsOverviewMetricCount> Labels{};
    std::array<HudRaceStatusTextSize,ResultsOverviewMetricCount> Values{};
};

inline bool CanDrawResultsOverview(const ResultsOverviewLayout& layout,
    const ResultsOverviewMeasurements& m) noexcept
{
    if(!layout.Valid || !HudRaceStatusTextFits(layout.Outcome,m.Outcome) ||
        !HudRaceStatusTextFits(layout.Persistence,m.Persistence) ||
        !HudRaceStatusTextFits(layout.TimingLabel,m.Timing))
        return false;
    for(std::size_t i=0;i<layout.Metrics.size();++i)
        if(!HudRaceStatusTextFits(layout.Metrics[i].Label,m.Labels[i]) ||
            !HudRaceStatusTextFits(layout.Metrics[i].Value,m.Values[i]))return false;
    return true;
}

struct ResultsOverviewTimingBar final
{
    bool Valid=false;
    DesktopMenuMouseRect Active{};
    DesktopMenuMouseRect Fees{};
};

// The full official clock already contains all fees. This is a visual
// partition, not an additional penalty, reward or a separate race clock.
inline ResultsOverviewTimingBar BuildResultsOverviewTimingBar(
    const DesktopMenuMouseRect& track,const ResultsOverviewReadout& read) noexcept
{
    if(!ValidHudRadarRect(track) || !read.Valid || !read.ClockValid ||
        !read.BreakdownValid || !std::isfinite(read.ElapsedSeconds) ||
        !std::isfinite(read.ActiveSeconds) || !std::isfinite(read.PenaltySeconds) ||
        read.ElapsedSeconds<=0 || read.ActiveSeconds<0 || read.PenaltySeconds<0 ||
        read.PenaltySeconds>read.ElapsedSeconds || read.ActiveSeconds>read.ElapsedSeconds ||
        std::abs(read.ActiveSeconds+read.PenaltySeconds-read.ElapsedSeconds)>1e-7)
        return {};
    const double activeWidth=track.Width*(read.ActiveSeconds/read.ElapsedSeconds);
    ResultsOverviewTimingBar out;
    out.Active={track.X,track.Y,activeWidth,track.Height};
    out.Fees={track.X+activeWidth,track.Y,track.Width-activeWidth,track.Height};
    if((out.Active.Width>0 && !HudRadarContains(track,out.Active)) ||
        (out.Fees.Width>0 && !HudRadarContains(track,out.Fees)) ||
        HudRadarOverlaps(out.Active,out.Fees))
        return {};
    out.Valid=true;
    return out;
}
} // namespace latebrake
