#include "UI/FleetReplayPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"FleetReplayPanel: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<1e-8;}

FleetReplayReview Review()
{
    FleetReplayReview r;
    r.Valid=true;
    r.Attempt=7;
    r.Coverage.Frozen=true;
    r.Coverage.HasReplay=true;
    r.Coverage.RecordedFrames=240;
    r.Coverage.RetainedFrames=240;
    r.Coverage.FirstClockSeconds=10;
    r.Coverage.LastClockSeconds=20;
    r.HighlightCount=3;
    r.Highlights[0]={true,1,ReplayBroadcastEventKind::CloseTraffic,12,11,13,60,1};
    r.Highlights[1]={true,4,ReplayBroadcastEventKind::SideBySide,17,16,18,75,1};
    r.Highlights[2]={true,2,ReplayBroadcastEventKind::StrongMeasuredBraking,19,18,20,90,1};
    return r;
}

FleetReplayPanelLayout Layout(double width=1280,double height=800,bool touch=false)
{
    const auto hud=CalculateHudLayout(width,height);
    const auto dock=CalculateDesktopMenuMouseLayout(width,height,hud,touch,true);
    const auto vertical=CalculateHudMenuVerticalPlan(width,height,hud,touch);
    const auto horizontal=CalculateHudTextBounds(width,height,hud,touch);
    const double safe=dock.Valid?std::min(horizontal.MaxWidthPixels,
        dock.Left-horizontal.Left-12.0*hud.Scale):horizontal.MaxWidthPixels;
    const auto text=CalculateHudMenuReadablePanel(width,height,hud,vertical,safe,true);
    return CalculateFleetReplayPanel(width,height,hud,dock,text,touch,true);
}

void ViewerAdmission()
{
    FleetReplayViewer viewer;
    auto r=Review();
    Require(!viewer.Act(FleetReplayAction::PlayPause,r),"no input before visible review");
    Require(viewer.Synchronize(r,true),"valid frozen window");
    auto state=viewer.Readout();
    Require(state.Valid && !state.Playing && state.CursorSeconds==10 &&
        state.FocusSlot==0 && viewer.Speed()==1,"initial paused at first sample");
    Require(!viewer.CanAct(FleetReplayAction::Previous,r) &&
        viewer.CanAct(FleetReplayAction::Next,r),"edge-aware seek");
    auto bad=r;bad.Attempt=0;
    Require(!viewer.Matches(bad) && !viewer.Act(FleetReplayAction::Next,bad),"zero attempt");
    bad=r;bad.Coverage.Frozen=false;
    Require(!viewer.Synchronize(bad,true) && !viewer.Readout().Valid,"live data cannot play");
    bad=r;bad.Coverage.RetainedFrames=481;
    Require(!viewer.Synchronize(bad,true),"invalid frame count");
    bad=r;bad.Coverage.LastClockSeconds=std::numeric_limits<double>::infinity();
    Require(!viewer.Synchronize(bad,true),"nonfinite clock");
    Require(viewer.Synchronize(r,true) && !viewer.Synchronize(r,false),"hidden resets");
    Require(!viewer.Readout().Valid,"no stale playback");
}

void PlayPauseAndSpeed()
{
    FleetReplayViewer v;const auto r=Review();
    Require(v.Synchronize(r,true),"valid");
    Require(v.Act(FleetReplayAction::PlayPause,r),"play");
    v.Tick(100,true);v.Tick(100.1,true);
    Require(Near(v.Readout().CursorSeconds,10.1),"real UI clock");
    Require(v.Act(FleetReplayAction::Speed,r) && v.Speed()==2,"double speed");
    v.Tick(100.2,true);v.Tick(100.3,true);
    Require(Near(v.Readout().CursorSeconds,10.3),"speed changes only replay");
    v.Tick(101,true);
    Require(!v.Readout().Playing && Near(v.Readout().CursorSeconds,10.3),"hitch pauses");
    Require(v.Act(FleetReplayAction::PlayPause,r),"resume");
    v.Tick(102,true);v.Tick(102.1,false);
    Require(!v.Readout().Playing && Near(v.Readout().CursorSeconds,10.3),"focus loss no time");
    Require(v.Act(FleetReplayAction::Speed,r) && v.Speed()==0.5,"half speed");
    Require(v.Act(FleetReplayAction::PlayPause,r),"play half");
    v.Tick(110,true);v.Tick(110.2,true);
    Require(Near(v.Readout().CursorSeconds,10.4),"half rate");
    v.Tick(109,true);
    Require(!v.Readout().Playing,"clock rollback pauses");
    Require(v.SeekFraction(0.99,r),"seek near end");
    Require(v.Act(FleetReplayAction::PlayPause,r),"play near end");
    v.Tick(120,true);v.Tick(120.25,true);
    Require(v.Readout().CursorSeconds==20 && !v.Readout().Playing,"stop exact endpoint");
    Require(v.Act(FleetReplayAction::PlayPause,r) &&
        v.Readout().CursorSeconds==10,"play from end restarts window");
}

void SeekAndHighlight()
{
    FleetReplayViewer v;auto r=Review();v.Synchronize(r,true);
    Require(v.Act(FleetReplayAction::Next,r) && Near(v.Readout().CursorSeconds,10.5),"step forward");
    Require(v.Act(FleetReplayAction::Previous,r) && v.Readout().CursorSeconds==10,"step back");
    Require(v.SeekFraction(0.25,r) && v.Readout().CursorSeconds==12.5 &&
        Near(v.Fraction(),0.25),"normalized scrub");
    Require(!v.SeekFraction(-0.1,r) && !v.SeekFraction(1.1,r) &&
        !v.SeekFraction(std::numeric_limits<double>::quiet_NaN(),r),"invalid scrub");
    Require(v.Act(FleetReplayAction::NextHighlight,r) &&
        v.Readout().SelectedHighlight==0 && v.Readout().FocusSlot==1 &&
        v.Readout().CursorSeconds==12,"first event");
    r.Highlights[1].EndSeconds=21;
    Require(v.Act(FleetReplayAction::NextHighlight,r) &&
        v.Readout().SelectedHighlight==2 && v.Readout().FocusSlot==2 &&
        v.Readout().CursorSeconds==19,"skip invalid event");
    Require(v.Act(FleetReplayAction::NextHighlight,r) &&
        v.Readout().SelectedHighlight==0,"event wraps");
    Require(v.Act(FleetReplayAction::NextFocus,r) && v.Readout().FocusSlot==2,"focus cycle");
    for(int i=0;i<6;++i) Require(v.Act(FleetReplayAction::NextFocus,r),"cycle physical slots");
    Require(v.Readout().FocusSlot==2,"six slots only");
    auto next=r;++next.Attempt;
    Require(!v.Act(FleetReplayAction::Next,next),"old presentation cannot operate new race");
    Require(v.Synchronize(next,true) && v.Readout().CursorSeconds==10 &&
        v.Readout().SelectedHighlight==-1,"new attempt reset");
}

void ClipPlayback()
{
    auto r=Review();
    FleetReplayViewer v;
    Require(v.Synchronize(r,true),"valid clip review");
    Require(v.Act(FleetReplayAction::NextHighlight,r),"select first clip");
    auto s=v.Readout();
    Require(s.CursorSeconds==12 && s.ClipStartSeconds==11 &&
        s.ClipEndSeconds==13 && s.ClipPrimed,"event preview retains exact clip");
    Require(v.Act(FleetReplayAction::PlayPause,r) &&
        v.Readout().CursorSeconds==11 && !v.Readout().ClipPrimed,
        "playing selected event begins at pre-event footage");
    v.Tick(100,true);v.Tick(100.25,true);
    Require(Near(v.Readout().CursorSeconds,11.25),"clip advances normally");
    Require(v.Act(FleetReplayAction::PlayPause,r) &&
        v.Act(FleetReplayAction::PlayPause,r) &&
        Near(v.Readout().CursorSeconds,11.25),"pause resumes within clip");
    Require(v.Act(FleetReplayAction::Speed,r),"clip supports double speed");
    v.Tick(101,true);
    for(int i=1;i<=4;++i) v.Tick(101+0.25*i,true);
    Require(!v.Readout().Playing && v.Readout().CursorSeconds==13,
        "clip stops exactly at its own end instead of full window");
    Require(v.Act(FleetReplayAction::PlayPause,r) &&
        v.Readout().CursorSeconds==11,"replay restarts selected clip");
    Require(v.SeekFraction(0.8,r) && v.Readout().SelectedHighlight==-1 &&
        v.Readout().CursorSeconds==18 && !v.Readout().Playing,
        "manual scrub exits clip mode");
    Require(v.Act(FleetReplayAction::NextHighlight,r) &&
        v.Act(FleetReplayAction::NextFocus,r) &&
        v.Readout().SelectedHighlight==-1,"manual focus exits event selection");

    Require(v.Act(FleetReplayAction::NextHighlight,r),"select clip again");
    r.Highlights[0].EndSeconds=13.5;
    Require(!v.CanAct(FleetReplayAction::PlayPause,r),"changed clip blocks stale play");
    Require(v.Synchronize(r,true) && v.Readout().SelectedHighlight==-1 &&
        !v.Readout().Playing,"changed bounds clear selected clip");
    r.Highlights[0].Kind=static_cast<ReplayBroadcastEventKind>(255);
    r.Highlights[1].SampledSpeedKmh=std::numeric_limits<double>::quiet_NaN();
    r.Highlights[2].Observations=0;
    Require(!v.CanAct(FleetReplayAction::NextHighlight,r),
        "invalid event enum, speed and observation count are not playable");
}

void LayoutAndPointer()
{
    const auto l=Layout();
    Require(l.Valid,"desktop replay layout");
    Require(Near(l.Map.Width/l.Map.Height,8400.0/6000.0),"preserved track aspect");
    Require(FleetReplayContainsRect({0,0,1280,800},l.Panel),"visible panel");
    Require(!FleetReplayRectsOverlap(l.Map,l.Scrub),"map and scrub independent");
    for(const auto& b:l.Buttons)
    {
        Require(FleetReplayContainsRect(l.Panel,b.Bounds),"button inside panel");
        const auto hit=HitFleetReplayPanel(l,b.Bounds.X+b.Bounds.Width/2,
            b.Bounds.Y+b.Bounds.Height/2);
        Require(hit.Consumed && !hit.Seek && hit.Action==b.Action,"same rendered and hit geometry");
        Require(!FleetReplayRectsOverlap(b.Bounds,l.Map) &&
            !FleetReplayRectsOverlap(b.Bounds,l.Scrub),"no map hit overlap");
    }
    for(std::size_t i=0;i<l.Buttons.size();++i)
        for(std::size_t j=0;j<i;++j)
            Require(!FleetReplayRectsOverlap(l.Buttons[i].Bounds,l.Buttons[j].Bounds),
                "distinct targets");
    auto h=HitFleetReplayPanel(l,l.Scrub.X+l.Scrub.Width*0.4,l.Scrub.Y+1);
    Require(h.Consumed && h.Seek && Near(h.Fraction,0.4),"scrub exact position");
    Require(!HitFleetReplayPanel(l,0,0).Consumed,"outside does not steal menu");
    Require(!HitFleetReplayPanel(l,std::numeric_limits<double>::quiet_NaN(),0).Consumed,
        "malformed pointer");
    Require(!Layout(800,600).Valid && !Layout(1280,800,true).Valid,
        "compact or touch keeps existing text layout");
    Require(!Layout(std::numeric_limits<double>::infinity(),800).Valid,
        "nonfinite viewport");
}

PhysicalReplayPlaybackFrame Frame(double clock=10)
{
    PhysicalReplayPlaybackFrame f;
    f.Valid=true;f.Session=7;f.ActiveClockSeconds=clock;
    for(std::size_t i=0;i<f.Cars.size();++i)
    {
        auto& c=f.Cars[i];
        c.Actor=100+i;c.ProvenanceMatched=true;
        c.Xcm=-2000+static_cast<double>(i)*800;
        c.Ycm=1000;
        c.HeadingRadians=0;
        c.ObservedSpeedCmps=1000;
    }
    return f;
}

void MapProjection()
{
    const auto l=Layout();
    auto r=Review();FleetReplayViewer v;v.Synchronize(r,true);
    auto f=Frame();
    auto m=BuildFleetReplayMap(f,l,v.Readout());
    Require(m.Valid && m.VisibleCars==6,"all six physical markers");
    for(const auto& c:m.Cars)
        Require(c.Visible && c.SpeedKmh==36 && c.Nose.X>=c.Position.X &&
            c.Position.Valid && c.Nose.Valid,"observed speed and heading");
    const auto left=ProjectFleetReplayMap({-4200,-3000},l.Map);
    const auto right=ProjectFleetReplayMap({4200,3000},l.Map);
    Require(left.Valid && right.Valid && left.X==l.Map.X && Near(left.Y,l.Map.Y+l.Map.Height) &&
        Near(right.X,l.Map.X+l.Map.Width) && Near(right.Y,l.Map.Y),"map endpoints");
    f.Cars[4].Xcm=4300;
    m=BuildFleetReplayMap(f,l,v.Readout());
    Require(m.Valid && m.VisibleCars==5 && !m.Cars[4].Visible,"off-map omitted, not clamped");
    f.Cars[4].Actor=f.Cars[0].Actor;
    Require(!BuildFleetReplayMap(f,l,v.Readout()).Valid,"duplicate identity");
    f=Frame();f.Session=8;
    Require(!BuildFleetReplayMap(f,l,v.Readout()).Valid,"different run");
    f=Frame(10.2);
    Require(!BuildFleetReplayMap(f,l,v.Readout()).Valid,"different cursor clock");
    f=Frame();f.NoOfficialTimingAuthority=false;
    Require(!BuildFleetReplayMap(f,l,v.Readout()).Valid,"wrong playback authority");
    f=Frame();f.Cars[5].ProvenanceMatched=false;
    Require(!BuildFleetReplayMap(f,l,v.Readout()).Valid,"unqualified sixth sample");
}

void RandomViewportsAndControls()
{
    std::mt19937 rng(682);
    int visible=0;
    for(int i=0;i<5000;++i)
    {
        const double w=240+rng()%3600,h=240+rng()%2200;
        const auto l=Layout(w,h);
        if(!l.Valid) continue;
        ++visible;
        Require(FleetReplayContainsRect({0,0,w,h},l.Panel),"random panel inside viewport");
        const auto dock=CalculateDesktopMenuMouseLayout(w,h,CalculateHudLayout(w,h),false,true);
        for(const auto& b:dock.Buttons)
            Require(!FleetReplayRectsOverlap(l.Panel,b.Bounds),"no base menu overlap");
        for(const auto& b:l.Buttons)
            Require(FleetReplayContainsRect(l.Panel,b.Bounds),"control inside panel");
    }
    Require(visible>100,"many real desktop layouts");

    auto r=Review();FleetReplayViewer v;v.Synchronize(r,true);
    double time=100;
    for(int i=0;i<10000;++i)
    {
        const unsigned action=rng()%9;
        if(action<7) v.Act(static_cast<FleetReplayAction>(action),r);
        else if(action==7) v.SeekFraction((rng()%101)/100.0,r);
        else {time+=0.01;v.Tick(time,true);}
        const auto state=v.Readout();
        Require(state.Valid && state.Attempt==r.Attempt &&
            state.CursorSeconds>=10 && state.CursorSeconds<=20 &&
            state.FocusSlot>=0 && state.FocusSlot<6 &&
            v.Fraction()>=0 && v.Fraction()<=1,"bounded viewer state");
    }
}
}

int main()
{
    ViewerAdmission();PlayPauseAndSpeed();SeekAndHighlight();
    ClipPlayback();LayoutAndPointer();MapProjection();RandomViewportsAndControls();
    std::cout<<"Fleet replay viewer, map projection, safe layout and input tests passed\n";
}
