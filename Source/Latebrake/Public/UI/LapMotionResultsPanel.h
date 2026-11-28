#pragma once

#include "Racing/PlayerLapMotionJournal.h"
#include "UI/SectorMotionResultsPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A separate full-lap view of the same Results lane. No metric mixes
// official clock values with observed movement or unobserved time.
enum class LapPanelMetric : std::uint8_t { Time, Speed, Coverage, Slip };

inline bool ValidLapPanelMetric(LapPanelMetric metric) noexcept
{
    return static_cast<unsigned>(metric)<4;
}

inline bool ValidLapPanelTotals(const LapMotionTotals& t) noexcept
{
    if(t.Gates!=CircuitGateCount || t.TimedGates<0 || t.TimedGates>t.Gates ||
        t.SampledGates<0 || t.SampledGates>t.TimedGates ||
        t.QualifiedGates<0 || t.QualifiedGates>t.SampledGates ||
        t.InterruptedGates<0 || t.InterruptedGates>t.TimedGates ||
        t.PenalizedGates<0 || t.PenalizedGates>t.Gates ||
        !ValidSectorMotionClock(t.OfficialSeconds,t.FeeSeconds) ||
        !LapMotionClose(t.ActiveSeconds+t.FeeSeconds,t.OfficialSeconds) ||
        !std::isfinite(t.ActiveSeconds) || t.ActiveSeconds<0 ||
        !std::isfinite(t.ObservedSeconds) || t.ObservedSeconds<0 ||
        t.ObservedSeconds>t.ActiveSeconds+SectorMotionClockTolerance ||
        !std::isfinite(t.PeakSpeed) || t.PeakSpeed<0 ||
        t.PeakSpeed>SectorMotionMaximumSpeed ||
        !std::isfinite(t.SpeedIntegral) || t.SpeedIntegral<0 ||
        t.SpeedIntegral>t.ObservedSeconds*t.PeakSpeed+SectorMotionClockTolerance ||
        !std::isfinite(t.DistanceCm) || t.DistanceCm<0 ||
        (t.SampledGates==0)!=(t.ObservedSeconds==0) ||
        (t.ObservedSeconds==0 && (t.SpeedIntegral!=0 || t.DistanceCm!=0)))
        return false;
    const double durations[]={t.LowSpeedSeconds,t.HighSlipSeconds,t.ReverseSeconds,
        t.PartialRoadSeconds,t.FullOffRoadSeconds};
    for(double seconds:durations)
        if(!std::isfinite(seconds) || seconds<0 ||
            seconds>t.ObservedSeconds+SectorMotionClockTolerance)return false;
    return t.FullOffRoadSeconds<=t.PartialRoadSeconds+SectorMotionClockTolerance;
}

inline bool SameLapPanelTotals(const LapMotionTotals& a,const LapMotionTotals& b) noexcept
{
    return a.Gates==b.Gates && a.TimedGates==b.TimedGates && a.SampledGates==b.SampledGates &&
        a.QualifiedGates==b.QualifiedGates && a.InterruptedGates==b.InterruptedGates &&
        a.PenalizedGates==b.PenalizedGates &&
        LapMotionClose(a.OfficialSeconds,b.OfficialSeconds) &&
        LapMotionClose(a.ActiveSeconds,b.ActiveSeconds) && LapMotionClose(a.FeeSeconds,b.FeeSeconds) &&
        LapMotionClose(a.ObservedSeconds,b.ObservedSeconds) && LapMotionClose(a.SpeedIntegral,b.SpeedIntegral) &&
        LapMotionClose(a.PeakSpeed,b.PeakSpeed) && LapMotionClose(a.DistanceCm,b.DistanceCm) &&
        LapMotionClose(a.LowSpeedSeconds,b.LowSpeedSeconds) && LapMotionClose(a.HighSlipSeconds,b.HighSlipSeconds) &&
        LapMotionClose(a.ReverseSeconds,b.ReverseSeconds) &&
        LapMotionClose(a.PartialRoadSeconds,b.PartialRoadSeconds) &&
        LapMotionClose(a.FullOffRoadSeconds,b.FullOffRoadSeconds);
}

inline bool SameLapPanelRow(const PlayerLapMotionRow& a,const PlayerLapMotionRow& b) noexcept
{
    return a.Present==b.Present && a.Lap==b.Lap && a.StandingStart==b.StandingStart &&
        a.Quality==b.Quality && SameLapPanelTotals(a.Totals,b.Totals);
}

inline bool ValidLapPanelReview(const PlayerLapMotionReview& review) noexcept
{
    if(!review.Valid || !review.Finished || review.CompletedLaps<1 ||
        review.CompletedLaps>SectorMotionMaximumLaps ||
        review.RequiredLaps!=review.CompletedLaps ||
        review.AcceptedGates!=review.CompletedLaps*CircuitGateCount ||
        review.Current.Gates!=0)return false;
    LapMotionTotals sum;
    PlayerLapMotionRow best{},slowest{};
    int qualified=0,comparable=0,penalized=0,interrupted=0,low=0,missing=0;
    for(int lap=1;lap<=review.CompletedLaps;++lap)
    {
        const auto row=review.At(lap);
        if(!row.Present || row.Lap!=lap || row.StandingStart!=(lap==1) ||
            !ValidLapPanelTotals(row.Totals) || row.Quality!=ClassifyLapMotion(row.Totals))
            return false;
        sum.Include(row.Totals);
        switch(row.Quality)
        {
        case LapMotionQuality::Qualified:++qualified;break;
        case LapMotionQuality::Penalized:++penalized;break;
        case LapMotionQuality::Interrupted:++interrupted;break;
        case LapMotionQuality::InsufficientCoverage:++low;break;
        default:++missing;break;
        }
        if(row.Comparable())
        {
            ++comparable;
            if(!best.Present || row.Totals.ActiveSeconds<best.Totals.ActiveSeconds)best=row;
            if(!slowest.Present || row.Totals.ActiveSeconds>slowest.Totals.ActiveSeconds)slowest=row;
        }
    }
    return SameLapPanelTotals(sum,review.Completed) && qualified==review.QualifiedLaps &&
        comparable==review.ComparableLaps && penalized==review.PenalizedLaps &&
        interrupted==review.InterruptedLaps && low==review.LowCoverageLaps &&
        missing==review.MissingTimingLaps && SameLapPanelRow(best,review.BestRolling) &&
        SameLapPanelRow(slowest,review.SlowestRolling);
}

struct LapPanelValue final
{
    bool Available=false;
    double Value=0;
    double Fee=0;
};

inline LapPanelValue ReadLapPanelValue(const PlayerLapMotionRow& row,LapPanelMetric metric) noexcept
{
    if(!row.Present || !ValidLapPanelTotals(row.Totals))return {};
    const auto& t=row.Totals;
    switch(metric)
    {
    case LapPanelMetric::Time:
        return t.TimedGates==CircuitGateCount?LapPanelValue{true,t.ActiveSeconds,t.FeeSeconds}:LapPanelValue{};
    case LapPanelMetric::Speed:
        return t.ObservedSeconds>0?LapPanelValue{true,t.MeanKmh(),0}:LapPanelValue{};
    case LapPanelMetric::Coverage:
        return t.ActiveSeconds>0?LapPanelValue{true,t.Coverage()*100,0}:LapPanelValue{};
    case LapPanelMetric::Slip:
        return t.ObservedSeconds>0?LapPanelValue{true,100*t.HighSlipSeconds/t.ObservedSeconds,0}:LapPanelValue{};
    default:return {};
    }
}

inline double LapPanelMetricMaximum(const PlayerLapMotionReview& review,LapPanelMetric metric) noexcept
{
    if(metric==LapPanelMetric::Coverage || metric==LapPanelMetric::Slip)return 100;
    double result=0;
    for(int lap=1;lap<=review.CompletedLaps;++lap)
    {
        const auto value=ReadLapPanelValue(review.At(lap),metric);
        if(value.Available)result=std::max(result,value.Value+value.Fee);
    }
    return result;
}

struct LapPanelState final
{
    bool Valid=false;
    std::uint64_t Revision=0;
    int CompletedLaps=0;
    double OfficialSeconds=0;
    double ObservedSeconds=0;
    LapPanelMetric Metric=LapPanelMetric::Time;
    int Page=0;
    int SelectedLap=0;
    std::uint64_t ViewRevision=0;

    bool operator==(const LapPanelState&) const noexcept=default;
};

struct LapPanelBar final
{
    PlayerLapMotionRow Row{};
    LapPanelValue Value{};
    bool Selected=false;
    double Fraction=0;
    double FeeFraction=0;
};

struct LapPanelReadout final
{
    bool Valid=false;
    LapPanelMetric Metric=LapPanelMetric::Time;
    int Page=0;
    int Pages=0;
    int Count=0;
    bool Newer=false;
    bool Older=false;
    double Maximum=0;
    PlayerLapMotionRow Selected{};
    std::array<LapPanelBar,SectorPanelRowsPerPage> Rows{};
};

class LapMotionPanelViewer final
{
public:
    void Reset() noexcept {State={};}
    bool Matches(const PlayerLapMotionReview& review,std::uint64_t revision) const noexcept
    {
        return State.Valid && revision!=0 && State.Revision==revision &&
            ValidLapPanelReview(review) && State.CompletedLaps==review.CompletedLaps &&
            State.OfficialSeconds==review.Completed.OfficialSeconds &&
            State.ObservedSeconds==review.Completed.ObservedSeconds;
    }

    bool Synchronize(const PlayerLapMotionReview& review,std::uint64_t revision,bool visible) noexcept
    {
        if(!visible || !revision || !ValidLapPanelReview(review))
        {
            Reset();
            return false;
        }
        if(Matches(review,revision))return true;
        State={true,revision,review.CompletedLaps,review.Completed.OfficialSeconds,
            review.Completed.ObservedSeconds,LapPanelMetric::Time,0,review.CompletedLaps,1};
        return true;
    }

    LapPanelState ReadState() const noexcept {return State;}

    LapPanelReadout Read(const PlayerLapMotionReview& review,std::uint64_t revision) const noexcept
    {
        if(!Matches(review,revision) || !ValidLapPanelMetric(State.Metric))return {};
        LapPanelReadout out;
        out.Metric=State.Metric;
        out.Pages=(review.CompletedLaps+SectorPanelRowsPerPage-1)/SectorPanelRowsPerPage;
        if(State.Page<0 || State.Page>=out.Pages)return {};
        out.Page=State.Page;
        out.Newer=out.Page>0;
        out.Older=out.Page+1<out.Pages;
        out.Maximum=LapPanelMetricMaximum(review,State.Metric);
        const int start=review.CompletedLaps-out.Page*SectorPanelRowsPerPage;
        for(int i=0;i<SectorPanelRowsPerPage && start-i>=1;++i)
        {
            auto& bar=out.Rows[static_cast<std::size_t>(i)];
            bar.Row=review.At(start-i);
            bar.Selected=bar.Row.Lap==State.SelectedLap;
            bar.Value=ReadLapPanelValue(bar.Row,State.Metric);
            if(bar.Value.Available && out.Maximum>0)
            {
                bar.Fraction=std::clamp(bar.Value.Value/out.Maximum,0.0,1.0);
                bar.FeeFraction=std::clamp(bar.Value.Fee/out.Maximum,0.0,1.0-bar.Fraction);
            }
            if(bar.Selected)out.Selected=bar.Row;
            ++out.Count;
        }
        out.Valid=out.Selected.Present;
        return out;
    }

    bool Act(SectorPanelAction action,const PlayerLapMotionReview& review,std::uint64_t revision) noexcept
    {
        const auto view=Read(review,revision);
        if(!view.Valid)return false;
        const int value=static_cast<int>(action);
        const int first=static_cast<int>(SectorPanelAction::Gate0);
        const int firstRow=static_cast<int>(SectorPanelAction::Row0);
        if(value>=first && value<first+4)
            State.Metric=static_cast<LapPanelMetric>(value-first);
        else if(action==SectorPanelAction::Newer || action==SectorPanelAction::Older)
        {
            if((action==SectorPanelAction::Newer && !view.Newer) ||
                (action==SectorPanelAction::Older && !view.Older))return false;
            State.Page+=action==SectorPanelAction::Older?1:-1;
            State.SelectedLap=review.CompletedLaps-State.Page*SectorPanelRowsPerPage;
        }
        else if(value>=firstRow && value<firstRow+SectorPanelRowsPerPage)
        {
            const auto& row=view.Rows[static_cast<std::size_t>(value-firstRow)].Row;
            if(!row.Present)return false;
            State.SelectedLap=row.Lap;
        }
        else return false;
        if(++State.ViewRevision==0)++State.ViewRevision;
        return true;
    }

private:
    LapPanelState State{};
};

inline SectorPanelBarGeometry BuildLapPanelGeometry(DesktopMenuMouseRect bounds,
    const LapPanelBar& bar,double scale) noexcept
{
    if(!bar.Row.Present)return {};
    SectorPanelBar base;
    base.Present=true;
    base.Timed=bar.Value.Available;
    base.ActiveFraction=bar.Fraction;
    base.FeeFraction=bar.FeeFraction;
    base.Coverage=bar.Row.Totals.Coverage();
    return BuildSectorPanelBarGeometry(bounds,base,scale);
}

struct LapPanelPresentation final
{
    bool Valid=false;
    SectorPanelLayout Layout{};
    LapPanelState State{};
    std::array<bool,4> TabsRendered{};
    std::array<bool,SectorPanelRowsPerPage> RowsRendered{};
    std::array<bool,2> PagerRendered{};
};

inline SectorPanelPointerHit HitPresentedLapPanel(const LapPanelPresentation& shown,
    const LapMotionPanelViewer& viewer,const PlayerLapMotionReview& review,std::uint64_t revision,
    double width,double height,double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(!shown.Layout.Valid || shown.Layout.ViewportWidth!=width ||
        shown.Layout.ViewportHeight!=height || shown.State!=viewer.ReadState() ||
        !viewer.Matches(review,revision))
        return {true,SectorPanelAction::None};
    const auto hit=HitSectorPanel(shown.Layout,x,y);
    const auto view=viewer.Read(review,revision);
    if(!view.Valid)return {true,SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Tabs.size();++i)
        if(hit.Action==shown.Layout.Tabs[i].Action)
            return {true,shown.TabsRendered[i]?hit.Action:SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Rows.size();++i)
        if(hit.Action==shown.Layout.Rows[i].Action)
            return {true,shown.RowsRendered[i] && view.Rows[i].Row.Present?
                hit.Action:SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Pager.size();++i)
        if(hit.Action==shown.Layout.Pager[i].Action)
            return {true,shown.PagerRendered[i] && (i==0?view.Newer:view.Older)?
                hit.Action:SectorPanelAction::None};
    return {true,SectorPanelAction::None};
}
} // namespace latebrake
