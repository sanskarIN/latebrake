#pragma once

#include "Racing/PracticeGateReference.h"
#include "UI/SectorMotionResultsPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// A finished, exact-gate practice comparison is a different dataset from
// sampled sector motion. It has a single complete reference attempt and
// exact clock deltas, including fees. Never treat a missing row as zero.
inline constexpr int PracticePanelGates=4;
inline constexpr int PracticePanelRows=SectorPanelRowsPerPage;

inline bool SamePracticeGateDelta(const PracticeGateDelta& a,
    const PracticeGateDelta& b) noexcept
{
    return a.Valid==b.Valid && a.Ordinal==b.Ordinal && a.Lap==b.Lap && a.Gate==b.Gate &&
        a.CurrentTotalSeconds==b.CurrentTotalSeconds &&
        a.ReferenceTotalSeconds==b.ReferenceTotalSeconds &&
        a.TotalDeltaSeconds==b.TotalDeltaSeconds &&
        a.ActiveDeltaSeconds==b.ActiveDeltaSeconds &&
        a.FeeDeltaSeconds==b.FeeDeltaSeconds &&
        a.SectorDeltaSeconds==b.SectorDeltaSeconds &&
        a.SectorActiveDeltaSeconds==b.SectorActiveDeltaSeconds &&
        a.SectorFeeDeltaSeconds==b.SectorFeeDeltaSeconds;
}

inline bool ValidPracticeComparisonRow(const PracticeGateDelta& row,
    const PracticeGateDelta& previous,int ordinal) noexcept
{
    if(!row.Valid || row.Ordinal!=ordinal || ordinal<1 || ordinal>PracticeMaximumGates ||
        row.Lap!=(ordinal-1)/PracticePanelGates+1 ||
        row.Gate!=(ordinal-1)%PracticePanelGates ||
        !IsValidFinishedRaceTime(row.CurrentTotalSeconds) ||
        !IsValidFinishedRaceTime(row.ReferenceTotalSeconds) ||
        row.CurrentTotalSeconds<=previous.CurrentTotalSeconds ||
        row.ReferenceTotalSeconds<=previous.ReferenceTotalSeconds)
        return false;
    return PracticeGateEqual(row.TotalDeltaSeconds,row.CurrentTotalSeconds-row.ReferenceTotalSeconds) &&
        PracticeGateEqual(row.TotalDeltaSeconds,row.ActiveDeltaSeconds+row.FeeDeltaSeconds) &&
        PracticeGateEqual(row.SectorDeltaSeconds,row.TotalDeltaSeconds-previous.TotalDeltaSeconds) &&
        PracticeGateEqual(row.SectorActiveDeltaSeconds,row.ActiveDeltaSeconds-previous.ActiveDeltaSeconds) &&
        PracticeGateEqual(row.SectorFeeDeltaSeconds,row.FeeDeltaSeconds-previous.FeeDeltaSeconds) &&
        PracticeGateEqual(row.SectorDeltaSeconds,row.SectorActiveDeltaSeconds+row.SectorFeeDeltaSeconds);
}

inline bool ValidPracticeResultsComparison(const PracticeGateReview& review) noexcept
{
    if(!review.Valid || !review.Finished || !review.ReferenceAvailable ||
        review.EvidenceRejected || review.ExpectedGates!=review.ObservedGates ||
        (review.ExpectedGates!=12 && review.ExpectedGates!=20 && review.ExpectedGates!=40) ||
        !IsValidFinishedRaceTime(review.ReferenceFinishSeconds))
        return false;
    int faster=0,slower=0,matched=0;
    PracticeGateDelta previous{},gain{},loss{};
    for(int i=0;i<review.ObservedGates;++i)
    {
        const auto& row=review.Rows[static_cast<std::size_t>(i)];
        if(!ValidPracticeComparisonRow(row,previous,i+1))return false;
        if(row.SectorDeltaSeconds<-PracticeGateClockTolerance)
        {
            ++faster;
            if(!gain.Valid || row.SectorDeltaSeconds<gain.SectorDeltaSeconds)gain=row;
        }
        else if(row.SectorDeltaSeconds>PracticeGateClockTolerance)
        {
            ++slower;
            if(!loss.Valid || row.SectorDeltaSeconds>loss.SectorDeltaSeconds)loss=row;
        }
        else ++matched;
        previous=row;
    }
    return faster==review.FasterSectors && slower==review.SlowerSectors &&
        matched==review.MatchedSectors &&
        SamePracticeGateDelta(gain,review.LargestGain) &&
        SamePracticeGateDelta(loss,review.LargestLoss) &&
        SamePracticeGateDelta(previous,review.Latest) &&
        PracticeGateEqual(previous.ReferenceTotalSeconds,review.ReferenceFinishSeconds) &&
        review.NewReference==(previous.CurrentTotalSeconds<previous.ReferenceTotalSeconds);
}

struct PracticePanelGateSelection final
{
    bool Valid=false;
    int Gate=-1;
    int Count=0;
    double MaxAbsoluteSectorDelta=0.0;
    int LargestGainOrdinal=0;
    int LargestLossOrdinal=0;
    std::array<int,PracticeMaximumGates/PracticePanelGates> Indices{};
};

inline PracticePanelGateSelection SelectPracticePanelGate(const PracticeGateReview& review,
    int gate) noexcept
{
    if(!ValidPracticeResultsComparison(review) || gate<0 || gate>=PracticePanelGates)
        return {};
    PracticePanelGateSelection out;
    out.Valid=true;
    out.Gate=gate;
    double gain=0.0,loss=0.0;
    // Most recent physical lap first, without mixing different gates.
    for(int i=review.ObservedGates-1;i>=0;--i)
    {
        const auto& row=review.Rows[static_cast<std::size_t>(i)];
        if(row.Gate!=gate)continue;
        out.Indices[static_cast<std::size_t>(out.Count++)]=i;
        out.MaxAbsoluteSectorDelta=std::max(out.MaxAbsoluteSectorDelta,
            std::abs(row.SectorDeltaSeconds));
        if(row.SectorDeltaSeconds<gain)
        {
            gain=row.SectorDeltaSeconds;
            out.LargestGainOrdinal=row.Ordinal;
        }
        if(row.SectorDeltaSeconds>loss)
        {
            loss=row.SectorDeltaSeconds;
            out.LargestLossOrdinal=row.Ordinal;
        }
    }
    return out;
}

struct PracticePanelState final
{
    bool Valid=false;
    std::uint64_t Revision=0;
    int Gates=0;
    double CurrentFinish=0.0;
    double ReferenceFinish=0.0;
    int Gate=0;
    int Page=0;
    int SelectedOrdinal=0;
    std::uint64_t ViewRevision=0;

    bool operator==(const PracticePanelState&) const noexcept=default;
};

struct PracticePanelRow final
{
    bool Present=false;
    bool Selected=false;
    bool LargestGain=false;
    bool LargestLoss=false;
    double SignedFraction=0.0;
    PracticeGateDelta Delta{};
};

struct PracticePanelReadout final
{
    bool Valid=false;
    int Gate=-1;
    int Page=0;
    int PageCount=0;
    int Count=0;
    bool Older=false;
    bool Newer=false;
    double ScaleSeconds=0.0;
    std::array<PracticePanelRow,PracticePanelRows> Rows{};
    PracticeGateDelta Selected{};
};

class PracticeResultsViewer final
{
public:
    void Reset() noexcept {State={};}

    bool Matches(const PracticeGateReview& review,std::uint64_t revision) const noexcept
    {
        return State.Valid && revision!=0 && State.Revision==revision &&
            ValidPracticeResultsComparison(review) &&
            State.Gates==review.ObservedGates &&
            State.CurrentFinish==review.Latest.CurrentTotalSeconds &&
            State.ReferenceFinish==review.ReferenceFinishSeconds;
    }

    bool Synchronize(const PracticeGateReview& review,std::uint64_t revision,
        bool visible) noexcept
    {
        if(!visible || revision==0 || !ValidPracticeResultsComparison(review))
        {
            Reset();
            return false;
        }
        if(Matches(review,revision))return true;
        State={true,revision,review.ObservedGates,review.Latest.CurrentTotalSeconds,
            review.ReferenceFinishSeconds,review.Latest.Gate,0,review.Latest.Ordinal,1};
        return true;
    }

    PracticePanelState ReadState() const noexcept {return State;}

    PracticePanelReadout Read(const PracticeGateReview& review,std::uint64_t revision) const noexcept
    {
        if(!Matches(review,revision))return {};
        const auto gate=SelectPracticePanelGate(review,State.Gate);
        if(!gate.Valid || gate.Count<=0)return {};
        const int pages=(gate.Count+PracticePanelRows-1)/PracticePanelRows;
        if(State.Page<0 || State.Page>=pages)return {};
        PracticePanelReadout out;
        out.Valid=true;
        out.Gate=State.Gate;
        out.Page=State.Page;
        out.PageCount=pages;
        out.Older=State.Page+1<pages;
        out.Newer=State.Page>0;
        out.ScaleSeconds=gate.MaxAbsoluteSectorDelta;
        const int begin=State.Page*PracticePanelRows;
        const int end=std::min(begin+PracticePanelRows,gate.Count);
        for(int i=begin;i<end;++i)
        {
            auto& bar=out.Rows[static_cast<std::size_t>(i-begin)];
            const auto& delta=review.Rows[static_cast<std::size_t>(
                gate.Indices[static_cast<std::size_t>(i)])];
            bar.Present=true;
            bar.Delta=delta;
            bar.Selected=delta.Ordinal==State.SelectedOrdinal;
            bar.LargestGain=delta.Ordinal==gate.LargestGainOrdinal;
            bar.LargestLoss=delta.Ordinal==gate.LargestLossOrdinal;
            bar.SignedFraction=gate.MaxAbsoluteSectorDelta>0.0
                ?std::clamp(delta.SectorDeltaSeconds/gate.MaxAbsoluteSectorDelta,-1.0,1.0):0.0;
            if(bar.Selected)out.Selected=delta;
            ++out.Count;
        }
        if(!out.Selected.Valid)return {};
        return out;
    }

    bool Act(SectorPanelAction action,const PracticeGateReview& review,
        std::uint64_t revision) noexcept
    {
        if(!Matches(review,revision))return false;
        const int value=static_cast<int>(action);
        const int firstGate=static_cast<int>(SectorPanelAction::Gate0);
        const int firstRow=static_cast<int>(SectorPanelAction::Row0);
        if(value>=firstGate && value<firstGate+PracticePanelGates)
        {
            const auto gate=SelectPracticePanelGate(review,value-firstGate);
            if(!gate.Valid || !gate.Count)return false;
            State.Gate=gate.Gate;
            State.Page=0;
            State.SelectedOrdinal=review.Rows[static_cast<std::size_t>(gate.Indices[0])].Ordinal;
            Changed();
            return true;
        }
        const auto read=Read(review,revision);
        if(!read.Valid)return false;
        if(action==SectorPanelAction::Older || action==SectorPanelAction::Newer)
        {
            if((action==SectorPanelAction::Older && !read.Older) ||
                (action==SectorPanelAction::Newer && !read.Newer))return false;
            const auto gate=SelectPracticePanelGate(review,State.Gate);
            State.Page+=action==SectorPanelAction::Older?1:-1;
            State.SelectedOrdinal=review.Rows[static_cast<std::size_t>(
                gate.Indices[static_cast<std::size_t>(State.Page*PracticePanelRows)])].Ordinal;
            Changed();
            return true;
        }
        if(value>=firstRow && value<firstRow+PracticePanelRows)
        {
            const auto& row=read.Rows[static_cast<std::size_t>(value-firstRow)];
            if(!row.Present)return false;
            State.SelectedOrdinal=row.Delta.Ordinal;
            Changed();
            return true;
        }
        return false;
    }

private:
    void Changed() noexcept {if(++State.ViewRevision==0)++State.ViewRevision;}
    PracticePanelState State{};
};

struct PracticePanelBarGeometry final
{
    bool Valid=false;
    DesktopMenuMouseRect Label{};
    DesktopMenuMouseRect Track{};
    DesktopMenuMouseRect Zero{};
    DesktopMenuMouseRect Fill{};
    bool Gain=false;
};

inline PracticePanelBarGeometry BuildPracticePanelBarGeometry(DesktopMenuMouseRect row,
    const PracticePanelRow& bar,double scale) noexcept
{
    if(!bar.Present || !bar.Delta.Valid || !ValidFleetReplayRect(row) ||
        row.Width<100 || row.Height<30 || !std::isfinite(scale) || scale<0.6 || scale>1.2 ||
        !std::isfinite(bar.SignedFraction) || bar.SignedFraction<-1.0 || bar.SignedFraction>1.0 ||
        (bar.Delta.SectorDeltaSeconds<0 && bar.SignedFraction>0) ||
        (bar.Delta.SectorDeltaSeconds>0 && bar.SignedFraction<0) ||
        (bar.Delta.SectorDeltaSeconds==0 && bar.SignedFraction!=0))
        return {};
    PracticePanelBarGeometry out;
    const double pad=4*scale;
    const double width=row.Width-2*pad;
    out.Label={row.X+pad,row.Y+pad,width,row.Height*0.48-pad};
    out.Track={row.X+pad,row.Y+row.Height*0.56,width,row.Height*0.24};
    const double center=out.Track.X+width*0.5;
    out.Zero={center-scale*0.5,out.Track.Y,scale,out.Track.Height};
    const double extent=std::abs(bar.SignedFraction)*width*0.5;
    out.Gain=bar.SignedFraction<0;
    if(extent>0)
        out.Fill={out.Gain?center-extent:center,out.Track.Y,extent,out.Track.Height};
    if(!FleetReplayContainsRect(row,out.Label) || !FleetReplayContainsRect(row,out.Track) ||
        !FleetReplayContainsRect(out.Track,out.Zero) ||
        (extent>0 && !FleetReplayContainsRect(out.Track,out.Fill)))
        return {};
    out.Valid=true;
    return out;
}

struct PracticePanelPresentation final
{
    bool Valid=false;
    SectorPanelLayout Layout{};
    PracticePanelState State{};
    std::array<bool,PracticePanelGates> TabsRendered{};
    std::array<bool,PracticePanelRows> RowsRendered{};
    std::array<bool,2> PagerRendered{};
};

inline SectorPanelPointerHit HitPresentedPracticePanel(const PracticePanelPresentation& shown,
    const PracticeResultsViewer& viewer,const PracticeGateReview& review,std::uint64_t revision,
    double width,double height,double x,double y) noexcept
{
    if(!shown.Valid || !shown.Layout.Panel.Contains(x,y))return {};
    if(!shown.Layout.Valid || width!=shown.Layout.ViewportWidth ||
        height!=shown.Layout.ViewportHeight || shown.State!=viewer.ReadState() ||
        !viewer.Matches(review,revision))
        return {true,SectorPanelAction::None};
    const auto hit=HitSectorPanel(shown.Layout,x,y);
    const auto read=viewer.Read(review,revision);
    if(!hit.Consumed || !read.Valid)return {true,SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Tabs.size();++i)
        if(hit.Action==shown.Layout.Tabs[i].Action)
            return {true,shown.TabsRendered[i]?hit.Action:SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Rows.size();++i)
        if(hit.Action==shown.Layout.Rows[i].Action)
            return {true,shown.RowsRendered[i] && read.Rows[i].Present?hit.Action:SectorPanelAction::None};
    for(std::size_t i=0;i<shown.Layout.Pager.size();++i)
        if(hit.Action==shown.Layout.Pager[i].Action)
            return {true,shown.PagerRendered[i] && (i==0?read.Newer:read.Older)?
                hit.Action:SectorPanelAction::None};
    return {true,SectorPanelAction::None};
}
} // namespace latebrake
