#include "UI/PracticeGateResultsPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"PracticeGateResultsPanel: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

PracticeGateReview Fixture(int laps,bool tied=false)
{
    PracticeGateTrace current,reference;
    current.Expected=reference.Expected=laps*4;
    double total=0,refTotal=0,fee=0;
    PracticeGateReview r;
    r.Valid=true;r.Finished=true;r.ReferenceAvailable=true;
    r.ExpectedGates=r.ObservedGates=laps*4;
    for(int i=0;i<laps*4;++i)
    {
        const double difference=tied?0.0:static_cast<double>((i/4)%5-2);
        const double charged=!tied && i%7==0?1.0:0.0;
        refTotal+=10;
        total+=10+difference+charged;
        fee+=charged;
        Require(current.Append(i+1,{total,fee}) && reference.Append(i+1,{refTotal,0}),
            "fixture exact gates");
    }
    current.Complete=reference.Complete=true;
    r.ReferenceFinishSeconds=refTotal;r.NewReference=total<refTotal;
    for(int i=0;i<laps*4;++i)
    {
        const auto row=ComparePracticeGate(current,reference,i+1,4);
        Require(row.Valid,"fixture valid row");
        r.Rows[static_cast<std::size_t>(i)]=row;
        if(row.SectorDeltaSeconds<-PracticeGateClockTolerance)
        {
            ++r.FasterSectors;
            if(!r.LargestGain.Valid || row.SectorDeltaSeconds<r.LargestGain.SectorDeltaSeconds)
                r.LargestGain=row;
        }
        else if(row.SectorDeltaSeconds>PracticeGateClockTolerance)
        {
            ++r.SlowerSectors;
            if(!r.LargestLoss.Valid || row.SectorDeltaSeconds>r.LargestLoss.SectorDeltaSeconds)
                r.LargestLoss=row;
        }
        else ++r.MatchedSectors;
        r.Latest=row;
    }
    return r;
}

void ValidateAndRejectCorruption()
{
    auto r=Fixture(10);
    Require(ValidPracticeResultsComparison(r),"complete ten lap trace");
    for(int i=0;i<20;++i)
    {
        auto bad=r;
        switch(i)
        {
        case 0:bad.Valid=false;break;
        case 1:bad.Finished=false;break;
        case 2:bad.ReferenceAvailable=false;break;
        case 3:bad.EvidenceRejected=true;break;
        case 4:--bad.ExpectedGates;break;
        case 5:--bad.ObservedGates;break;
        case 6:bad.ReferenceFinishSeconds+=1;break;
        case 7:bad.Rows[5].Valid=false;break;
        case 8:++bad.Rows[5].Ordinal;break;
        case 9:++bad.Rows[5].Lap;break;
        case 10:++bad.Rows[5].Gate;break;
        case 11:bad.Rows[5].CurrentTotalSeconds=bad.Rows[4].CurrentTotalSeconds;break;
        case 12:bad.Rows[5].ReferenceTotalSeconds=std::numeric_limits<double>::infinity();break;
        case 13:bad.Rows[5].SectorFeeDeltaSeconds+=1;break;
        case 14:bad.Rows[5].ActiveDeltaSeconds+=1;break;
        case 15:++bad.FasterSectors;break;
        case 16:bad.LargestGain.Ordinal=0;break;
        case 17:bad.LargestLoss.Valid=false;break;
        case 18:bad.Latest.TotalDeltaSeconds+=1;break;
        case 19:bad.NewReference=!bad.NewReference;break;
        }
        Require(!ValidPracticeResultsComparison(bad) &&
            !SelectPracticePanelGate(bad,0).Valid,"corrupted comparison fully refused");
    }
    Require(!ValidPracticeResultsComparison(Fixture(4)),"unsupported practice shape");
    Require(!SelectPracticePanelGate(r,-1).Valid && !SelectPracticePanelGate(r,4).Valid,
        "invalid gate filter");
}

void PagingAndSignedBars()
{
    const auto review=Fixture(10);
    PracticeResultsViewer viewer;
    Require(viewer.Synchronize(review,7,true),"real finish view");
    auto read=viewer.Read(review,7);
    Require(read.Valid && read.Gate==3 && read.Page==0 && read.PageCount==3 &&
        read.Count==4 && read.Selected.Ordinal==40 && read.Older && !read.Newer,
        "initial view newest final gate");
    for(int gate=0;gate<4;++gate)
    {
        Require(viewer.Act(static_cast<SectorPanelAction>(
            static_cast<int>(SectorPanelAction::Gate0)+gate),review,7),"select gate");
        const auto selected=SelectPracticePanelGate(review,gate);
        int covered=0;
        for(int page=0;page<3;++page)
        {
            read=viewer.Read(review,7);
            Require(read.Valid && read.Page==page && read.Gate==gate &&
                read.ScaleSeconds==selected.MaxAbsoluteSectorDelta,"stable gate scale");
            for(int i=0;i<read.Count;++i)
            {
                const auto& row=read.Rows[static_cast<std::size_t>(i)];
                Require(row.Present && row.Delta.Gate==gate &&
                    row.Delta.Lap==10-covered,"actual lap order");
                Require(viewer.Act(static_cast<SectorPanelAction>(
                    static_cast<int>(SectorPanelAction::Row0)+i),review,7),"select visible row");
                Require(viewer.Read(review,7).Selected.Ordinal==row.Delta.Ordinal,"selected exact ordinal");
                const auto geom=BuildPracticePanelBarGeometry({10,20,200,44},row,1);
                Require(geom.Valid && geom.Gain==(row.SignedFraction<0),
                    "signed bar direction");
                if(row.SignedFraction!=0)
                {
                    Require(FleetReplayContainsRect(geom.Track,geom.Fill),"bounded bar");
                    const auto center=geom.Track.X+geom.Track.Width/2;
                    Require(geom.Gain?PracticeGateEqual(geom.Fill.X+geom.Fill.Width,center):
                        PracticeGateEqual(geom.Fill.X,center),"shared zero anchor");
                }
                ++covered;
            }
            if(page<2)Require(viewer.Act(SectorPanelAction::Older,review,7),"older page");
        }
        Require(covered==10 && !viewer.Act(SectorPanelAction::Older,review,7) &&
            !viewer.Act(SectorPanelAction::Row3,review,7),"partial last page");
        Require(viewer.Act(SectorPanelAction::Newer,review,7) &&
            viewer.Read(review,7).Page==1,"newer page");
    }
    Require(!viewer.Act(SectorPanelAction::Gate0,review,8),"old revision no navigation");
    Require(viewer.Synchronize(review,8,true) && viewer.Read(review,8).Selected.Ordinal==40,
        "new results reset");
    Require(!viewer.Synchronize(review,8,false) && !viewer.Read(review,8).Valid,"hidden reset");
    const auto tie=Fixture(3,true);
    Require(viewer.Synchronize(tie,9,true),"ties are real results");
    read=viewer.Read(tie,9);
    Require(read.ScaleSeconds==0 && read.Rows[0].SignedFraction==0 &&
        BuildPracticePanelBarGeometry({0,0,200,44},read.Rows[0],1).Valid,
        "zero delta without fabricated bar");
}

SectorPanelLayout Layout()
{
    const auto hud=CalculateHudLayout(1280,720);
    const auto dock=CalculateDesktopMenuMouseLayout(1280,720,hud,false,true);
    const HudMenuReadablePanel text{true,hud.Left,hud.Top,dock.Left-hud.Left-12*hud.Scale,
        720-hud.Top-20,40};
    return CalculateSectorPanelLayout(1280,720,hud,dock,text,false,true);
}

void PointerPresentation()
{
    auto review=Fixture(10);
    PracticeResultsViewer viewer;
    Require(viewer.Synchronize(review,21,true),"visible result");
    const auto layout=Layout();
    Require(layout.Valid,"real result layout");
    PracticePanelPresentation shown;
    shown.Valid=true;shown.Layout=layout;shown.State=viewer.ReadState();
    shown.TabsRendered.fill(true);shown.RowsRendered.fill(true);shown.PagerRendered.fill(true);
    const auto hit=[&](DesktopMenuMouseRect b)
    {
        return HitPresentedPracticePanel(shown,viewer,review,21,1280,720,
            b.X+b.Width/2,b.Y+b.Height/2);
    };
    Require(hit(layout.Tabs[0].Bounds).Action==SectorPanelAction::Gate0 &&
        hit(layout.Rows[2].Bounds).Action==SectorPanelAction::Row2,"visible card actions");
    Require(hit(layout.Pager[0].Bounds).Action==SectorPanelAction::None &&
        hit(layout.Pager[1].Bounds).Action==SectorPanelAction::Older,"only enabled page action");
    shown.RowsRendered[2]=false;
    Require(hit(layout.Rows[2].Bounds).Consumed &&
        hit(layout.Rows[2].Bounds).Action==SectorPanelAction::None,"unrendered row consumed inert");
    shown.RowsRendered[2]=true;
    shown.TabsRendered[0]=false;
    Require(hit(layout.Tabs[0].Bounds).Action==SectorPanelAction::None,"unrendered tab inert");
    shown.TabsRendered[0]=true;
    Require(viewer.Act(SectorPanelAction::Older,review,21),"page changes");
    Require(hit(layout.Rows[0].Bounds).Action==SectorPanelAction::None,"stale displayed page inert");
    shown.State=viewer.ReadState();
    Require(hit(layout.Rows[0].Bounds).Action==SectorPanelAction::Row0,"redrawn page actionable");
    auto stale=HitPresentedPracticePanel(shown,viewer,review,22,1280,720,
        layout.Rows[0].Bounds.X+1,layout.Rows[0].Bounds.Y+1);
    Require(stale.Consumed && stale.Action==SectorPanelAction::None,"new result revision");
    stale=HitPresentedPracticePanel(shown,viewer,review,21,1281,720,
        layout.Rows[0].Bounds.X+1,layout.Rows[0].Bounds.Y+1);
    Require(stale.Consumed && stale.Action==SectorPanelAction::None,"viewport changed");
    Require(!HitPresentedPracticePanel(shown,viewer,review,21,1280,720,0,0).Consumed,
        "outside panel left for other controls");
}

void BarBoundsSweep()
{
    std::mt19937 rng(48301);
    for(int i=0;i<10000;++i)
    {
        PracticePanelRow row;
        row.Present=true;row.Delta.Valid=true;
        row.SignedFraction=static_cast<double>(static_cast<int>(rng()%2001)-1000)/1000.0;
        row.Delta.SectorDeltaSeconds=row.SignedFraction*5;
        const double scale=0.6+static_cast<double>(rng()%601)/1000;
        const DesktopMenuMouseRect bounds{20,30,100+static_cast<double>(rng()%300),
            30+static_cast<double>(rng()%50)};
        const auto geom=BuildPracticePanelBarGeometry(bounds,row,scale);
        Require(geom.Valid && FleetReplayContainsRect(bounds,geom.Track) &&
            FleetReplayContainsRect(geom.Track,geom.Zero),"bounded geometry");
        if(row.SignedFraction!=0)
            Require(FleetReplayContainsRect(geom.Track,geom.Fill) &&
                PracticeGateEqual(geom.Fill.Width,
                    std::abs(row.SignedFraction)*geom.Track.Width/2),"accurate symmetric scale");
    }
    PracticePanelRow row;
    row.Present=true;row.Delta.Valid=true;row.Delta.SectorDeltaSeconds=1;
    row.SignedFraction=-1;
    Require(!BuildPracticePanelBarGeometry({0,0,200,44},row,1).Valid,"wrong sign denied");
    row.SignedFraction=std::numeric_limits<double>::quiet_NaN();
    Require(!BuildPracticePanelBarGeometry({0,0,200,44},row,1).Valid,"invalid fraction denied");
}
}
int main()
{
    ValidateAndRejectCorruption();
    PagingAndSignedBars();
    PointerPresentation();
    BarBoundsSweep();
    std::cout<<"Practice gate results panel tests passed\n";
}
