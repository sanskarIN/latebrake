#pragma once

#include "Racing/PlayerSectorMotionReview.h"
#include "UI/FleetReplayPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A finished-race inspection surface for retained observed sectors.
// Every plotted bar is an actual accepted split. Missing/invalid rows
// remain visible as unavailable; never turn those into a zero duration.
inline constexpr int SectorPanelRowsPerPage=4;

inline bool ValidSectorMotionHistoryRow(const SectorMotionRow& row,
    int accepted) noexcept
{
    if(row.Ordinal<1 || row.Ordinal>accepted ||
        row.Gate!=(row.Ordinal-1)%CircuitGateCount ||
        row.Lap!=(row.Ordinal-1)/CircuitGateCount+1 ||
        row.StandingStart!=(row.Ordinal==1))
        return false;
    if(!row.ValidTiming) return row.Quality==SectorMotionQuality::Unavailable;
    if(!std::isfinite(row.ActiveSeconds) || row.ActiveSeconds<=0.0 ||
        row.ActiveSeconds>SectorMotionMaximumClock ||
        !std::isfinite(row.FeeSeconds) || row.FeeSeconds<0.0 ||
        !std::isfinite(row.OfficialSeconds) || row.OfficialSeconds<=0.0 ||
        std::abs(row.ActiveSeconds+row.FeeSeconds-row.OfficialSeconds)>
            SectorMotionClockTolerance ||
        !std::isfinite(row.Coverage) || row.Coverage<0.0 || row.Coverage>1.0 ||
        !std::isfinite(row.Motion.ObservedSeconds) ||
        row.Motion.ObservedSeconds<0.0 ||
        row.Motion.ObservedSeconds>row.ActiveSeconds+SectorMotionClockTolerance ||
        std::abs(row.Coverage-std::clamp(row.Motion.ObservedSeconds/row.ActiveSeconds,0.0,1.0))>1e-6 ||
        !std::isfinite(row.Motion.SpeedIntegral) || row.Motion.SpeedIntegral<0.0 ||
        !std::isfinite(row.Motion.PeakSpeed) || row.Motion.PeakSpeed<0.0 ||
        row.Motion.PeakSpeed>SectorMotionMaximumSpeed ||
        !std::isfinite(row.Motion.PlanarDistanceCm) || row.Motion.PlanarDistanceCm<0.0 ||
        !std::isfinite(row.Motion.HighSlipSeconds) || row.Motion.HighSlipSeconds<0.0 ||
        row.Motion.HighSlipSeconds>row.Motion.ObservedSeconds+SectorMotionClockTolerance ||
        !std::isfinite(row.Motion.LowSpeedSeconds) || row.Motion.LowSpeedSeconds<0.0 ||
        row.Motion.LowSpeedSeconds>row.Motion.ObservedSeconds+SectorMotionClockTolerance ||
        row.Motion.AverageKmh()>row.Motion.PeakKmh()+1e-6)
        return false;
    switch(row.Quality)
    {
        case SectorMotionQuality::Qualified:
            return row.FeeSeconds<=SectorMotionClockTolerance && !row.Motion.Breaks &&
                !row.Motion.Rejected && row.Motion.Intervals>0 &&
                row.ActiveSeconds>=SectorMotionMinimumDuration &&
                row.Coverage>=SectorMotionMinimumCoverage;
        case SectorMotionQuality::Penalized:
            return row.FeeSeconds>SectorMotionClockTolerance;
        case SectorMotionQuality::Interrupted:
            return row.FeeSeconds<=SectorMotionClockTolerance &&
                (row.Motion.Breaks || row.Motion.Rejected);
        case SectorMotionQuality::InsufficientCoverage:
            return row.FeeSeconds<=SectorMotionClockTolerance &&
                !row.Motion.Breaks && !row.Motion.Rejected &&
                (row.ActiveSeconds<SectorMotionMinimumDuration ||
                    row.Motion.Intervals==0 || row.Coverage<SectorMotionMinimumCoverage);
        default:return false;
    }
}

inline bool ValidSectorMotionHistory(const SectorMotionHistory& history) noexcept
{
    if(!history.Valid || history.AcceptedGates<1 ||
        history.AcceptedGates>CircuitGateCount*SectorMotionMaximumLaps ||
        history.AcceptedGates%CircuitGateCount!=0 ||
        history.Count!=std::min(history.AcceptedGates,SectorMotionHistoryCapacity) ||
        !std::isfinite(history.ActiveSeconds) || history.ActiveSeconds<=0.0 ||
        history.ActiveSeconds>SectorMotionMaximumClock)
        return false;
    double activeSum=0.0;
    for(int i=0;i<history.Count;++i)
    {
        const auto& row=history.Rows[static_cast<std::size_t>(i)];
        if(row.Ordinal!=history.AcceptedGates-i ||
            !ValidSectorMotionHistoryRow(row,history.AcceptedGates))
            return false;
        if(row.ValidTiming) activeSum+=row.ActiveSeconds;
    }
    return activeSum<=history.ActiveSeconds+SectorMotionClockTolerance;
}

struct SectorPanelGateRows final
{
    bool Valid=false;
    int Gate=-1;
    int Count=0;
    double MaximumOfficialSeconds=0.0;
    int BestQualifiedOrdinal=0;
    double BestQualifiedSeconds=0.0;
    std::array<int,SectorMotionHistoryCapacity> Index{};
};

inline SectorPanelGateRows SelectSectorPanelGate(const SectorMotionHistory& history,
    int gate) noexcept
{
    SectorPanelGateRows out;
    if(!ValidSectorMotionHistory(history) || gate<0 || gate>=CircuitGateCount) return out;
    out.Valid=true;
    out.Gate=gate;
    for(int i=0;i<history.Count;++i)
    {
        const auto& row=history.Rows[static_cast<std::size_t>(i)];
        if(row.Gate!=gate) continue;
        out.Index[static_cast<std::size_t>(out.Count++)]=i;
        if(!row.ValidTiming) continue;
        out.MaximumOfficialSeconds=std::max(out.MaximumOfficialSeconds,row.OfficialSeconds);
        if(row.Comparable() && !row.StandingStart &&
            (!out.BestQualifiedOrdinal || row.ActiveSeconds<out.BestQualifiedSeconds))
        {
            out.BestQualifiedOrdinal=row.Ordinal;
            out.BestQualifiedSeconds=row.ActiveSeconds;
        }
    }
    return out;
}

enum class SectorPanelAction
{
    None,
    Gate0,
    Gate1,
    Gate2,
    Gate3,
    Older,
    Newer,
    Row0,
    Row1,
    Row2,
    Row3
};

struct SectorPanelState final
{
    bool Valid=false;
    std::uint64_t Revision=0;
    int AcceptedGates=0;
    int RetainedRows=0;
    double ActiveSeconds=0.0;
    int Gate=0;
    int Page=0;
    int SelectedOrdinal=0;
};

struct SectorPanelBar final
{
    bool Present=false;
    bool Timed=false;
    bool Qualified=false;
    bool StandingStart=false;
    bool BestRetained=false;
    bool Selected=false;
    int Ordinal=0;
    int Lap=0;
    double OfficialSeconds=0.0;
    double ActiveFraction=0.0;
    double FeeFraction=0.0;
    double Coverage=0.0;
    SectorMotionQuality Quality=SectorMotionQuality::Unavailable;
};

struct SectorPanelReadout final
{
    bool Valid=false;
    int Gate=-1;
    int Page=0;
    int PageCount=0;
    int Rows=0;
    bool Older=false;
    bool Newer=false;
    double MaximumOfficialSeconds=0.0;
    std::array<SectorPanelBar,SectorPanelRowsPerPage> Bars{};
    SectorMotionRow Selected{};
};

class SectorPanelViewer final
{
public:
    void Reset() noexcept {State={};}

    bool Matches(const SectorMotionHistory& history,std::uint64_t revision) const noexcept
    {
        return State.Valid && ValidSectorMotionHistory(history) &&
            State.Revision==revision && State.AcceptedGates==history.AcceptedGates &&
            State.RetainedRows==history.Count && State.ActiveSeconds==history.ActiveSeconds;
    }

    bool Synchronize(const SectorMotionHistory& history,std::uint64_t revision,
        bool visible) noexcept
    {
        if(!visible || !ValidSectorMotionHistory(history))
        {
            Reset();
            return false;
        }
        if(Matches(history,revision)) return true;
        Reset();
        State.Valid=true;
        State.Revision=revision;
        State.AcceptedGates=history.AcceptedGates;
        State.RetainedRows=history.Count;
        State.ActiveSeconds=history.ActiveSeconds;
        State.Gate=history.Rows[0].Gate;
        State.SelectedOrdinal=history.Rows[0].Ordinal;
        return true;
    }

    SectorPanelState ReadState() const noexcept {return State;}

    SectorPanelReadout Read(const SectorMotionHistory& history,
        std::uint64_t revision) const noexcept
    {
        SectorPanelReadout out;
        if(!Matches(history,revision)) return out;
        const auto gate=SelectSectorPanelGate(history,State.Gate);
        if(!gate.Valid || gate.Count<=0) return out;
        const int pages=(gate.Count+SectorPanelRowsPerPage-1)/SectorPanelRowsPerPage;
        if(State.Page<0 || State.Page>=pages) return out;
        out.Valid=true;
        out.Gate=State.Gate;
        out.Page=State.Page;
        out.PageCount=pages;
        out.Older=State.Page+1<pages;
        out.Newer=State.Page>0;
        out.MaximumOfficialSeconds=gate.MaximumOfficialSeconds;
        const int begin=State.Page*SectorPanelRowsPerPage;
        const int end=std::min(begin+SectorPanelRowsPerPage,gate.Count);
        for(int i=begin;i<end;++i)
        {
            const auto& row=history.Rows[static_cast<std::size_t>(gate.Index[static_cast<std::size_t>(i)])];
            auto& bar=out.Bars[static_cast<std::size_t>(i-begin)];
            bar.Present=true;
            bar.Ordinal=row.Ordinal;
            bar.Lap=row.Lap;
            bar.Timed=row.ValidTiming;
            bar.Quality=row.Quality;
            bar.StandingStart=row.StandingStart;
            bar.Selected=row.Ordinal==State.SelectedOrdinal;
            bar.Qualified=row.Comparable();
            bar.BestRetained=gate.BestQualifiedOrdinal==row.Ordinal;
            if(row.ValidTiming && gate.MaximumOfficialSeconds>0.0)
            {
                bar.OfficialSeconds=row.OfficialSeconds;
                bar.ActiveFraction=std::clamp(row.ActiveSeconds/gate.MaximumOfficialSeconds,0.0,1.0);
                bar.FeeFraction=std::clamp(row.FeeSeconds/gate.MaximumOfficialSeconds,0.0,
                    1.0-bar.ActiveFraction);
                bar.Coverage=row.Coverage;
            }
            if(bar.Selected) out.Selected=row;
            ++out.Rows;
        }
        return out;
    }

    bool Act(SectorPanelAction action,const SectorMotionHistory& history,
        std::uint64_t revision) noexcept
    {
        if(!Matches(history,revision)) return false;
        const int value=static_cast<int>(action);
        const int firstGate=static_cast<int>(SectorPanelAction::Gate0);
        const int firstRow=static_cast<int>(SectorPanelAction::Row0);
        if(value>=firstGate && value<firstGate+CircuitGateCount)
        {
            const auto gate=SelectSectorPanelGate(history,value-firstGate);
            if(!gate.Valid || !gate.Count) return false;
            State.Gate=gate.Gate;
            State.Page=0;
            State.SelectedOrdinal=history.Rows[static_cast<std::size_t>(gate.Index[0])].Ordinal;
            return true;
        }
        const auto read=Read(history,revision);
        if(!read.Valid) return false;
        if(action==SectorPanelAction::Older || action==SectorPanelAction::Newer)
        {
            if((action==SectorPanelAction::Older && !read.Older) ||
                (action==SectorPanelAction::Newer && !read.Newer)) return false;
            State.Page+=action==SectorPanelAction::Older?1:-1;
            const auto gate=SelectSectorPanelGate(history,State.Gate);
            State.SelectedOrdinal=history.Rows[static_cast<std::size_t>(
                gate.Index[static_cast<std::size_t>(State.Page*SectorPanelRowsPerPage)])].Ordinal;
            return true;
        }
        if(value>=firstRow && value<firstRow+SectorPanelRowsPerPage)
        {
            const auto& row=read.Bars[static_cast<std::size_t>(value-firstRow)];
            if(!row.Present) return false;
            State.SelectedOrdinal=row.Ordinal;
            return true;
        }
        return false;
    }

private:
    SectorPanelState State{};
};

struct SectorPanelTarget final
{
    SectorPanelAction Action=SectorPanelAction::None;
    DesktopMenuMouseRect Bounds{};
};

struct SectorPanelLayout final
{
    bool Valid=false;
    double ViewportWidth=0.0;
    double ViewportHeight=0.0;
    double Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    DesktopMenuMouseRect PageLabel{};
    DesktopMenuMouseRect SelectedDetail{};
    DesktopMenuMouseRect SelectedSpeed{};
    DesktopMenuMouseRect Legend{};
    std::array<SectorPanelTarget,CircuitGateCount> Tabs{};
    std::array<SectorPanelTarget,SectorPanelRowsPerPage> Rows{};
    std::array<SectorPanelTarget,2> Pager{};
};

inline SectorPanelLayout CalculateSectorPanelLayout(double width,double height,
    HudLayout hud,const DesktopMenuMouseLayout& dock,const HudMenuReadablePanel& text,
    bool touch,bool visible) noexcept
{
    SectorPanelLayout out;
    if(!visible || touch || hud.Compact || !dock.Valid || !text.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<900.0 || height<600.0 || width>20000.0 || height>20000.0 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.25 || hud.Scale>4.0)
        return out;
    const double s=std::clamp(hud.Scale,0.6,1.2);
    double left=width,right=0,bottom=0;
    for(const auto& button:dock.Buttons)
    {
        if(!FleetReplayContainsRect({0,0,width,height},button.Bounds)) return out;
        left=std::min(left,button.Bounds.X);
        right=std::max(right,button.Bounds.X+button.Bounds.Width);
        bottom=std::max(bottom,button.Bounds.Y+button.Bounds.Height);
    }
    const double top=bottom+34.0*s;
    const double panelWidth=right-left;
    const double inset=8.0*s;
    const double gap=5.0*s;
    const double header=23.0*s;
    const double tabHeight=std::max(30.0,30.0*s);
    const double rowHeight=std::max(36.0,40.0*s);
    const double pagerHeight=std::max(30.0,30.0*s);
    const double detailHeight=20.0*s;
    const double panelHeight=2.0*inset+header+tabHeight+4.0*rowHeight+
        pagerHeight+3.0*detailHeight+10.0*gap;
    out.Panel={left,top,panelWidth,panelHeight};
    if(panelWidth<216.0 ||
        !FleetReplayContainsRect({0,0,width,height-12.0*s},out.Panel) ||
        !ValidFleetReplayRect({text.X,text.Y,text.Width,text.Height}) ||
        FleetReplayRectsOverlap(out.Panel,{text.X,text.Y,text.Width,text.Height}))
        return {};
    out.ViewportWidth=width;
    out.ViewportHeight=height;
    out.Scale=s;
    const double x=left+inset;
    const double w=panelWidth-2.0*inset;
    double y=top+inset;
    out.Header={x,y,w,header};
    y+=header+gap;
    const double tabWidth=(w-3.0*gap)/4.0;
    for(int i=0;i<CircuitGateCount;++i)
        out.Tabs[static_cast<std::size_t>(i)]={
            static_cast<SectorPanelAction>(static_cast<int>(SectorPanelAction::Gate0)+i),
            {x+i*(tabWidth+gap),y,tabWidth,tabHeight}};
    y+=tabHeight+gap;
    for(int i=0;i<SectorPanelRowsPerPage;++i)
    {
        out.Rows[static_cast<std::size_t>(i)]={
            static_cast<SectorPanelAction>(static_cast<int>(SectorPanelAction::Row0)+i),
            {x,y,w,rowHeight}};
        y+=rowHeight+gap;
    }
    const double pagerWidth=w*0.28;
    out.Pager[0]={SectorPanelAction::Newer,{x,y,pagerWidth,pagerHeight}};
    out.Pager[1]={SectorPanelAction::Older,{x+w-pagerWidth,y,pagerWidth,pagerHeight}};
    out.PageLabel={x+pagerWidth+gap,y,w-2.0*(pagerWidth+gap),pagerHeight};
    y+=pagerHeight+gap;
    out.SelectedDetail={x,y,w,detailHeight};y+=detailHeight+gap;
    out.SelectedSpeed={x,y,w,detailHeight};y+=detailHeight+gap;
    out.Legend={x,y,w,detailHeight};
    for(const auto& tab:out.Tabs) if(!FleetReplayContainsRect(out.Panel,tab.Bounds))return {};
    for(const auto& row:out.Rows) if(!FleetReplayContainsRect(out.Panel,row.Bounds))return {};
    for(const auto& pager:out.Pager) if(!FleetReplayContainsRect(out.Panel,pager.Bounds))return {};
    if(!FleetReplayContainsRect(out.Panel,out.Legend) ||
        !FleetReplayContainsRect(out.Panel,out.SelectedSpeed) ||
        !FleetReplayContainsRect(out.Panel,out.SelectedDetail) ||
        !FleetReplayContainsRect(out.Panel,out.PageLabel) ||
        !FleetReplayContainsRect(out.Panel,out.Header))
        return {};
    out.Valid=true;
    return out;
}

struct SectorPanelPointerHit final
{
    bool Consumed=false;
    SectorPanelAction Action=SectorPanelAction::None;
};

inline SectorPanelPointerHit HitSectorPanel(const SectorPanelLayout& layout,
    double x,double y) noexcept
{
    if(!layout.Valid || !layout.Panel.Contains(x,y)) return {};
    for(const auto& tab:layout.Tabs)
        if(tab.Bounds.Contains(x,y)) return {true,tab.Action};
    for(const auto& row:layout.Rows)
        if(row.Bounds.Contains(x,y)) return {true,row.Action};
    for(const auto& pager:layout.Pager)
        if(pager.Bounds.Contains(x,y)) return {true,pager.Action};
    return {true,SectorPanelAction::None};
}

struct SectorPanelBarGeometry final
{
    bool Valid=false;
    DesktopMenuMouseRect Track{};
    DesktopMenuMouseRect Active{};
    DesktopMenuMouseRect Fee{};
    DesktopMenuMouseRect Coverage{};
    DesktopMenuMouseRect Label{};
};

inline SectorPanelBarGeometry BuildSectorPanelBarGeometry(const DesktopMenuMouseRect& row,
    const SectorPanelBar& bar,double scale) noexcept
{
    SectorPanelBarGeometry out;
    if(!bar.Present || !ValidFleetReplayRect(row) || !std::isfinite(scale) ||
        scale<0.6 || scale>1.2 || row.Width<100.0 || row.Height<30.0 ||
        !std::isfinite(bar.ActiveFraction) || bar.ActiveFraction<0.0 || bar.ActiveFraction>1.0 ||
        !std::isfinite(bar.FeeFraction) || bar.FeeFraction<0.0 ||
        bar.ActiveFraction+bar.FeeFraction>1.0+1e-8 ||
        !std::isfinite(bar.Coverage) || bar.Coverage<0.0 || bar.Coverage>1.0)
        return out;
    const double pad=4.0*scale;
    const double width=row.Width-2.0*pad;
    const double labelHeight=row.Height*0.48;
    out.Label={row.X+pad,row.Y+pad,width,labelHeight-pad};
    out.Track={row.X+pad,row.Y+labelHeight,width,row.Height*0.24};
    if(bar.Timed)
    {
        if(bar.ActiveFraction>0.0)
            out.Active={out.Track.X,out.Track.Y,width*bar.ActiveFraction,out.Track.Height};
        if(bar.FeeFraction>0.0)
            out.Fee={out.Track.X+width*bar.ActiveFraction,out.Track.Y,
                width*bar.FeeFraction,out.Track.Height};
        if(bar.Coverage>0.0)
            out.Coverage={row.X+pad,row.Y+row.Height-pad-2.0*scale,
                width*bar.Coverage,2.0*scale};
    }
    out.Valid=FleetReplayContainsRect(row,out.Label) && FleetReplayContainsRect(row,out.Track);
    if(!out.Valid) return {};
    return out;
}
} // namespace latebrake
