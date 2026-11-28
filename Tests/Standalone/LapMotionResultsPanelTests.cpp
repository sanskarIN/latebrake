#include "UI/LapMotionResultsPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"LapMotionResultsPanel: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
PlayerLapMotionReview Fixture(int laps)
{
    PlayerLapMotionJournal journal;
    journal.Reset(4,laps);
    double elapsed=0,penalty=0;
    for(int i=1;i<=laps*4;++i)
    {
        const int lap=(i-1)/4+1;
        const double active=lap==3?8:10;
        const double fee=lap==2 && i%4==1?2:0;
        const double observed=lap==4?0:lap==5?active/2:active;
        elapsed+=active+fee;penalty+=fee;
        SectorMotionRow row;
        row.ValidTiming=true;
        row.Ordinal=i;row.Lap=lap;row.Gate=(i-1)%4;
        row.StandingStart=i==1;
        row.ActiveSeconds=active;row.OfficialSeconds=active+fee;row.FeeSeconds=fee;
        row.Coverage=observed/active;
        row.Quality=fee?SectorMotionQuality::Penalized:
            observed<active?SectorMotionQuality::InsufficientCoverage:SectorMotionQuality::Qualified;
        if(observed>0)
        {
            row.Motion.Samples=101;row.Motion.Intervals=100;
            row.Motion.ObservedSeconds=observed;
            row.Motion.SpeedIntegral=observed*1000;
            row.Motion.PeakSpeed=1200;
            row.Motion.PlanarDistanceCm=observed*1000;
            row.Motion.HighSlipSeconds=lap==3?0:observed/4;
        }
        Require(journal.Accept({i,row.Gate,elapsed,penalty,active+fee},row),"real gate receipt");
    }
    return journal.Read(laps*4,elapsed,penalty,true);
}
void MetricsAndAvailability()
{
    const auto review=Fixture(10);
    Require(ValidLapPanelReview(review),"full review validation");
    Require(LapPanelMetricMaximum(review,LapPanelMetric::Time)==42 &&
        LapPanelMetricMaximum(review,LapPanelMetric::Speed)==36 &&
        LapPanelMetricMaximum(review,LapPanelMetric::Coverage)==100 &&
        LapPanelMetricMaximum(review,LapPanelMetric::Slip)==100,"stable whole-run metric scales");
    auto fee=ReadLapPanelValue(review.At(2),LapPanelMetric::Time);
    Require(fee.Available && fee.Value==40 && fee.Fee==2,"split fees");
    Require(!ReadLapPanelValue(review.At(4),LapPanelMetric::Speed).Available &&
        !ReadLapPanelValue(review.At(4),LapPanelMetric::Slip).Available,"no samples not zero speed/slip");
    auto cover=ReadLapPanelValue(review.At(4),LapPanelMetric::Coverage);
    Require(cover.Available && cover.Value==0,"zero coverage is known");
    auto slip=ReadLapPanelValue(review.At(3),LapPanelMetric::Slip);
    Require(slip.Available && slip.Value==0,"observed zero slip is known");
    Require(ReadLapPanelValue(review.At(5),LapPanelMetric::Coverage).Value==50,
        "actual half coverage");
    Require(!ReadLapPanelValue(review.At(1),static_cast<LapPanelMetric>(255)).Available,"bad metric");
}
void PagingAndSelection()
{
    const auto review=Fixture(10);
    LapMotionPanelViewer viewer;
    Require(viewer.Synchronize(review,9,true),"finished view");
    auto view=viewer.Read(review,9);
    Require(view.Valid && view.Count==4 && view.Pages==3 && view.Selected.Lap==10 &&
        view.Older && !view.Newer,"first page newest");
    Require(viewer.Act(SectorPanelAction::Row2,review,9) &&
        viewer.Read(review,9).Selected.Lap==8,"select actual row");
    Require(viewer.Act(SectorPanelAction::Gate1,review,9) &&
        viewer.Read(review,9).Metric==LapPanelMetric::Speed &&
        viewer.Read(review,9).Selected.Lap==8,"metric preserves selection");
    Require(viewer.Act(SectorPanelAction::Older,review,9),"older");
    view=viewer.Read(review,9);
    Require(view.Selected.Lap==6 && view.Rows[2].Row.Lap==4 &&
        !view.Rows[2].Value.Available,"middle page missing speed");
    Require(viewer.Act(SectorPanelAction::Older,review,9),"oldest");
    view=viewer.Read(review,9);
    Require(view.Count==2 && view.Selected.Lap==2 && !view.Older &&
        !viewer.Act(SectorPanelAction::Row3,review,9),"no invisible row action");
    Require(!viewer.Act(SectorPanelAction::Older,review,9),"no wrap");
    Require(viewer.Act(SectorPanelAction::Newer,review,9),"newer");
    Require(!viewer.Act(SectorPanelAction::Gate0,review,10),"stale race revision");
    Require(viewer.Synchronize(review,10,true) && viewer.Read(review,10).Selected.Lap==10 &&
        viewer.Read(review,10).Metric==LapPanelMetric::Time,"new race resets");
    Require(!viewer.Synchronize(review,10,false),"hidden reset");
}
void MalformedEvidence()
{
    const auto review=Fixture(10);
    for(int fault=0;fault<12;++fault)
    {
        auto bad=review;
        switch(fault)
        {
        case 0:bad.Finished=false;break;
        case 1:--bad.CompletedLaps;break;
        case 2:bad.Rows[0].StandingStart=false;break;
        case 3:bad.Rows[1].Lap=99;break;
        case 4:bad.Rows[1].Totals.FeeSeconds=0;break;
        case 5:bad.Rows[3].Totals.SpeedIntegral=5;break;
        case 6:bad.Rows[5].Totals.HighSlipSeconds=100;break;
        case 7:bad.Completed.DistanceCm+=1;break;
        case 8:++bad.QualifiedLaps;break;
        case 9:bad.BestRolling.Lap=99;break;
        case 10:bad.Rows[0].Quality=LapMotionQuality::Interrupted;break;
        case 11:bad.Rows[0].Totals.PeakSpeed=std::numeric_limits<double>::quiet_NaN();break;
        }
        Require(!ValidLapPanelReview(bad),"malformed summary rejected");
        LapMotionPanelViewer viewer;
        Require(!viewer.Synchronize(bad,9,true),"no malformed view");
    }
}
void PointerAndGeometry()
{
    const auto review=Fixture(10);
    LapMotionPanelViewer viewer;
    viewer.Synchronize(review,11,true);
    const auto hud=CalculateHudLayout(1280,720);
    const auto dock=CalculateDesktopMenuMouseLayout(1280,720,hud,false,true);
    const HudMenuReadablePanel text{true,hud.Left,hud.Top,dock.Left-hud.Left-12*hud.Scale,
        720-hud.Top-20,40};
    const auto layout=CalculateSectorPanelLayout(1280,720,hud,dock,text,false,true);
    Require(layout.Valid,"actual lane");
    LapPanelPresentation shown{true,layout,viewer.ReadState(),{},{},{}};
    shown.TabsRendered.fill(true);shown.RowsRendered.fill(true);shown.PagerRendered.fill(true);
    const auto hit=[&](DesktopMenuMouseRect rect)
    {
        return HitPresentedLapPanel(shown,viewer,review,11,1280,720,
            rect.X+rect.Width/2,rect.Y+rect.Height/2);
    };
    Require(hit(layout.Tabs[1].Bounds).Action==SectorPanelAction::Gate1,"metric tab");
    Require(hit(layout.Pager[0].Bounds).Action==SectorPanelAction::None &&
        hit(layout.Pager[1].Bounds).Action==SectorPanelAction::Older,"only actual pages");
    shown.RowsRendered[1]=false;
    Require(hit(layout.Rows[1].Bounds).Consumed &&
        hit(layout.Rows[1].Bounds).Action==SectorPanelAction::None,"hidden row blocked");
    viewer.Act(SectorPanelAction::Older,review,11);
    Require(hit(layout.Rows[0].Bounds).Action==SectorPanelAction::None,"stale view blocked");
    shown.State=viewer.ReadState();
    Require(hit(layout.Rows[0].Bounds).Action==SectorPanelAction::Row0,"redrawn row");
    const auto read=viewer.Read(review,11);
    for(std::size_t i=0;i<read.Rows.size();++i)
    {
        const auto geom=BuildLapPanelGeometry(layout.Rows[i].Bounds,read.Rows[i],layout.Scale);
        Require(geom.Valid && FleetReplayContainsRect(layout.Rows[i].Bounds,geom.Track),"real bounded bars");
    }
    Require(!HitPresentedLapPanel(shown,viewer,review,11,1280,720,0,0).Consumed,"outside panel");
}
}
int main()
{
    MetricsAndAvailability();PagingAndSelection();MalformedEvidence();PointerAndGeometry();
    std::cout<<"Lap motion results panel tests passed\n";
}
