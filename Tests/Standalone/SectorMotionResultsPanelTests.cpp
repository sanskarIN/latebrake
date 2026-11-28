#include "UI/SectorMotionResultsPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"SectorMotionResultsPanel: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<1e-7;}

SectorMotionHistory History(int laps=8)
{
    SectorMotionHistory h;
    h.Valid=true;
    h.AcceptedGates=laps*4;
    h.Count=std::min(h.AcceptedGates,SectorMotionHistoryCapacity);
    for(int i=0;i<h.Count;++i)
    {
        auto& r=h.Rows[static_cast<std::size_t>(i)];
        r.Ordinal=h.AcceptedGates-i;
        r.Lap=(r.Ordinal-1)/4+1;
        r.Gate=(r.Ordinal-1)%4;
        r.StandingStart=r.Ordinal==1;
        r.ValidTiming=true;
        r.ActiveSeconds=2.0+0.1*r.Lap;
        r.OfficialSeconds=r.ActiveSeconds;
        r.Coverage=0.9;
        r.Quality=SectorMotionQuality::Qualified;
        r.Motion.ObservedSeconds=r.ActiveSeconds*0.9;
        r.Motion.Samples=20;
        r.Motion.Intervals=19;
        r.Motion.SpeedIntegral=1000*r.Motion.ObservedSeconds;
        r.Motion.PeakSpeed=1200;
        h.ActiveSeconds+=r.ActiveSeconds;
    }
    return h;
}

SectorPanelLayout Layout(double w=1280,double h=900,bool touch=false)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,touch,true);
    const auto vert=CalculateHudMenuVerticalPlan(w,h,hud,touch);
    const auto bounds=CalculateHudTextBounds(w,h,hud,touch);
    const double safe=dock.Valid?std::min(bounds.MaxWidthPixels,
        dock.Left-bounds.Left-12.0*hud.Scale):bounds.MaxWidthPixels;
    const auto text=CalculateHudMenuReadablePanel(w,h,hud,vert,safe,true);
    return CalculateSectorPanelLayout(w,h,hud,dock,text,touch,true);
}

void HistoryValidation()
{
    const auto h=History();
    Require(ValidSectorMotionHistory(h),"valid exact retained chronological history");
    auto bad=h;bad.Valid=false;
    Require(!ValidSectorMotionHistory(bad),"no source");
    bad=h;bad.Count=33;Require(!ValidSectorMotionHistory(bad),"bounded count");
    bad=h;bad.AcceptedGates=31;Require(!ValidSectorMotionHistory(bad),"not finished");
    bad=h;bad.Rows[0].Ordinal=31;Require(!ValidSectorMotionHistory(bad),"wrong ordinal");
    bad=h;bad.Rows[0].Gate=0;Require(!ValidSectorMotionHistory(bad),"wrong gate");
    bad=h;bad.Rows[0].Lap=1;Require(!ValidSectorMotionHistory(bad),"wrong lap");
    bad=h;bad.Rows[0].StandingStart=true;Require(!ValidSectorMotionHistory(bad),"false standing");
    bad=h;bad.Rows[0].Coverage=0.1;Require(!ValidSectorMotionHistory(bad),"false coverage");
    bad=h;bad.Rows[0].OfficialSeconds=0;Require(!ValidSectorMotionHistory(bad),"invalid split");
    bad=h;bad.Rows[0].Motion.SpeedIntegral=1e20;Require(!ValidSectorMotionHistory(bad),"false speed");
    bad=h;bad.Rows[0].Motion.HighSlipSeconds=4;Require(!ValidSectorMotionHistory(bad),"false slip");
    bad=h;bad.Rows[0].Motion.Breaks=1;Require(!ValidSectorMotionHistory(bad),"false quality");
    bad=h;bad.ActiveSeconds=1;Require(!ValidSectorMotionHistory(bad),"clock insufficient");
    bad=h;bad.Rows[0].OfficialSeconds=std::numeric_limits<double>::quiet_NaN();
    Require(!ValidSectorMotionHistory(bad),"nonfinite");
}

void GateFilteringAndComparisonScale()
{
    auto h=History();
    auto gate=SelectSectorPanelGate(h,0);
    Require(gate.Valid && gate.Count==8 && gate.Gate==0 &&
        gate.BestQualifiedOrdinal==5 && Near(gate.BestQualifiedSeconds,2.2),
        "standing start excluded from rolling best");
    Require(Near(gate.MaximumOfficialSeconds,2.8),"shared gate scale");
    Require(h.Rows[static_cast<std::size_t>(gate.Index[0])].Ordinal==29 &&
        h.Rows[static_cast<std::size_t>(gate.Index[7])].Ordinal==1,"chronological gate rows");
    Require(!SelectSectorPanelGate(h,4).Valid,"bad gate");
    auto& r=h.Rows[3];r.FeeSeconds=2;r.OfficialSeconds=r.ActiveSeconds+2;
    r.Quality=SectorMotionQuality::Penalized;
    gate=SelectSectorPanelGate(h,0);
    Require(gate.Valid && Near(gate.MaximumOfficialSeconds,4.8) &&
        gate.BestQualifiedOrdinal==5,"penalty remains in bar scale not best");
}

void SelectionAndPaging()
{
    auto h=History();
    SectorPanelViewer v;
    Require(!v.Act(SectorPanelAction::Gate0,h,5),"hidden input rejected");
    Require(v.Synchronize(h,5,true),"finished source");
    auto r=v.Read(h,5);
    Require(r.Valid && r.Gate==3 && r.Page==0 && r.PageCount==2 &&
        r.Rows==4 && r.Selected.Ordinal==32 && r.Older && !r.Newer,
        "latest gate selected by default");
    Require(!v.Act(SectorPanelAction::Newer,h,5),"no page underflow");
    Require(v.Act(SectorPanelAction::Gate0,h,5),"gate click");
    r=v.Read(h,5);
    Require(r.Gate==0 && r.Selected.Ordinal==29 && r.Bars[0].Selected,"same gate latest");
    Require(v.Act(SectorPanelAction::Row2,h,5) && v.Read(h,5).Selected.Ordinal==21,
        "select visible lap");
    Require(v.Act(SectorPanelAction::Older,h,5),"older page");
    r=v.Read(h,5);
    Require(r.Page==1 && r.Selected.Ordinal==13 && r.Bars[2].BestRetained &&
        r.Bars[3].StandingStart && r.Newer && !r.Older,"older retained rows");
    Require(!v.Act(SectorPanelAction::Older,h,5),"no overflow");
    Require(v.Act(SectorPanelAction::Newer,h,5),"back to current");
    Require(!v.Act(SectorPanelAction::Row0,h,6),"stale revision");
    Require(v.Synchronize(h,6,true) && v.Read(h,6).Gate==3,"restart revision reset");
    Require(!v.Synchronize(h,6,false) && !v.Read(h,6).Valid,"menu hidden resets");
}

void UntimedAndPenalizedRows()
{
    auto h=History(2);
    auto& latest=h.Rows[0];latest.ValidTiming=false;
    latest.Quality=SectorMotionQuality::Unavailable;
    auto& prior=h.Rows[4];prior.FeeSeconds=3;
    prior.OfficialSeconds=prior.ActiveSeconds+3;
    prior.Quality=SectorMotionQuality::Penalized;
    SectorPanelViewer v;Require(v.Synchronize(h,1,true),"incomplete evidence remains inspectable");
    auto r=v.Read(h,1);
    Require(r.Valid && r.Rows==2 && !r.Bars[0].Timed && r.Bars[0].Present &&
        r.Bars[0].ActiveFraction==0 && !r.Selected.ValidTiming,"untimed not fabricated");
    Require(r.Bars[1].Timed && !r.Bars[1].Qualified &&
        Near(r.Bars[1].ActiveFraction+r.Bars[1].FeeFraction,1.0),"stacked actual fee");
    Require(v.Act(SectorPanelAction::Row1,h,1) &&
        v.Read(h,1).Selected.Quality==SectorMotionQuality::Penalized,"fee row inspection");
    Require(!v.Act(SectorPanelAction::Row2,h,1),"empty row disabled");
    Require(!v.Act(SectorPanelAction::Older,h,1),"short history one page");
}

void GeometryAndPointer()
{
    const auto l=Layout();
    Require(l.Valid && FleetReplayContainsRect({0,0,1280,900},l.Panel),"real desktop geometry");
    for(const auto& t:l.Tabs)
    {
        const auto hit=HitSectorPanel(l,t.Bounds.X+t.Bounds.Width/2,t.Bounds.Y+t.Bounds.Height/2);
        Require(hit.Consumed && hit.Action==t.Action,"exact tab geometry");
    }
    for(const auto& row:l.Rows)
    {
        const auto hit=HitSectorPanel(l,row.Bounds.X+1,row.Bounds.Y+1);
        Require(hit.Consumed && hit.Action==row.Action,"exact row geometry");
    }
    for(const auto& t:l.Pager)
        Require(HitSectorPanel(l,t.Bounds.X+1,t.Bounds.Y+1).Action==t.Action,
            "exact pager geometry");
    Require(!HitSectorPanel(l,0,0).Consumed &&
        !HitSectorPanel(l,std::numeric_limits<double>::quiet_NaN(),0).Consumed,
        "outside and invalid pointer untouched");
    Require(!Layout(800,600).Valid && !Layout(1280,900,true).Valid,
        "compact and touch retain ordinary text");
    SectorPanelBar bar;
    bar.Present=true;bar.Timed=true;bar.ActiveFraction=0.6;bar.FeeFraction=0.3;bar.Coverage=0.85;
    const auto g=BuildSectorPanelBarGeometry(l.Rows[0].Bounds,bar,l.Scale);
    Require(g.Valid && FleetReplayContainsRect(g.Track,g.Active) &&
        FleetReplayContainsRect(g.Track,g.Fee) &&
        !FleetReplayRectsOverlap(g.Active,g.Fee) &&
        FleetReplayContainsRect(l.Rows[0].Bounds,g.Coverage),"bounded stacked bars");
    Require(Near(g.Active.Width/g.Track.Width,0.6) &&
        Near(g.Fee.Width/g.Track.Width,0.3),"actual fractions");
    bar.FeeFraction=0.5;
    Require(!BuildSectorPanelBarGeometry(l.Rows[0].Bounds,bar,l.Scale).Valid,"overfull bar");
    bar.FeeFraction=0;bar.Timed=false;
    const auto missing=BuildSectorPanelBarGeometry(l.Rows[0].Bounds,bar,l.Scale);
    Require(missing.Valid && missing.Active.Width==0 && missing.Fee.Width==0 &&
        missing.Coverage.Width==0,"untimed no misleading bar");
}

void JournalEndToEndAndRandomViewports()
{
    PlayerSectorMotionJournal journal;
    journal.Reset(4,10);
    SectorMotionFrame f;
    f.Actor=11;f.PhysicalBodyObserved=true;f.ClockAdvanced=true;
    f.AdvancedSeconds=0.05;f.VelocityX=1000;f.Position={-2500,SouthY};
    double boundary=0;
    for(int sector=0;sector<40;++sector)
    {
        f.Ordinal=sector;
        f.Position={-2500,SouthY};
        for(int i=0;i<30;++i)
        {
            ++f.EngineFrame;f.ElapsedSeconds+=0.05;f.Position.X+=50;
            Require(journal.Observe(f),"real journal sample");
        }
        Require(journal.AcceptGate({sector+1,sector%4,f.ElapsedSeconds,0,
            f.ElapsedSeconds-boundary}),"real receipt");
        boundary=f.ElapsedSeconds;
        if(sector<39)
            Require(!journal.ReadFinishedHistory(sector+1,f.ElapsedSeconds,0).Valid,"not yet finished");
    }
    const auto h=journal.ReadFinishedHistory(40,f.ElapsedSeconds,0);
    Require(ValidSectorMotionHistory(h) && h.Count==32 && h.Rows[31].Ordinal==9,
        "actual frozen history can be rendered");
    SectorPanelViewer v;v.Synchronize(h,3,true);
    std::mt19937 rng(7071);
    int visible=0;
    for(int i=0;i<3500;++i)
    {
        const double w=240+rng()%4000,hh=240+rng()%2400;
        const auto l=Layout(w,hh);
        if(l.Valid)
        {
            ++visible;
            const auto dock=CalculateDesktopMenuMouseLayout(w,hh,CalculateHudLayout(w,hh),false,true);
            for(const auto& t:dock.Buttons)
                Require(!FleetReplayRectsOverlap(l.Panel,t.Bounds),"no navigation overlap");
            for(const auto& t:l.Rows)
                Require(FleetReplayContainsRect(l.Panel,t.Bounds),"rows bounded");
        }
        v.Act(static_cast<SectorPanelAction>(rng()%13),h,3);
        const auto r=v.Read(h,3);
        Require(r.Valid && r.Rows>=1 && r.Rows<=4 &&
            r.Selected.Ordinal>0 && r.Selected.Gate==r.Gate,"selection remains same gate");
    }
    Require(visible>100,"sufficient desktop viewport coverage");
}
}

int main()
{
    HistoryValidation();GateFilteringAndComparisonScale();SelectionAndPaging();
    UntimedAndPenalizedRows();GeometryAndPointer();JournalEndToEndAndRandomViewports();
    std::cout<<"Sector motion history panel, actual journal and pointer tests passed\n";
}
