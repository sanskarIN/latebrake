#include "UI/HudDriveCommandPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"HudDriveCommandPanel FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
HudDriveCommandPanel Layout(double w,double h,bool touch=false,bool menu=false,
    bool finished=false,bool requested=true)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto bounds=CalculateHudTextBounds(w,h,hud,touch);
    const auto gauges=CalculateHudInstrumentCluster(w,h,hud,touch,menu,finished,80,0.5,0.2,0.1);
    const auto radar=CalculateHudRaceRadarLayout(w,h,hud,bounds,gauges,touch,menu,finished);
    return CalculateHudDriveCommandPanel(w,h,gauges,radar,requested);
}
DriveCommandReview Review(int count,double delta=0.05)
{
    DriveCommandTrace trace;
    for(int i=1;i<=count;++i)
    {
        const auto frame=static_cast<std::uint64_t>(i);
        trace.BeginFrame(frame);
        Require(trace.Observe({frame,4,5,6,delta,0.8,0.4,-0.8,-0.4,
            i%2==0,false,true,i%2!=0}),"valid fixture");
    }
    return trace.Read(static_cast<std::uint64_t>(count),4,5,6,true);
}
void LayoutSafety()
{
    Require(Layout(1920,1080).Valid,"ordinary large desktop panel");
    Require(!Layout(800,600).Valid,"compact viewport");
    Require(!Layout(1920,1080,true).Valid,"touch viewport");
    Require(!Layout(1920,1080,false,true).Valid,"menu");
    Require(!Layout(1920,1080,false,false,true).Valid,"finished");
    Require(!Layout(1920,1080,false,false,false,false).Valid,"non-detailed mode");
    Require(!Layout(std::numeric_limits<double>::infinity(),1080).Valid,"invalid viewport");
    int accepted=0;
    for(int w=900;w<=3840;w+=59)
        for(int h=600;h<=2160;h+=37)
        {
            const auto hud=CalculateHudLayout(w,h);
            const auto bounds=CalculateHudTextBounds(w,h,hud,false);
            const auto gauges=CalculateHudInstrumentCluster(w,h,hud,false,false,false,80,0.5,0.2,0.1);
            const auto radar=CalculateHudRaceRadarLayout(w,h,hud,bounds,gauges,false,false,false);
            const auto p=CalculateHudDriveCommandPanel(w,h,gauges,radar,true);
            if(!p.Valid)continue;
            ++accepted;
            Require(HudRadarContains({0,0,static_cast<double>(w),static_cast<double>(h)},p.Panel),
                "panel in viewport");
            Require(!HudRadarOverlaps(p.Panel,radar.Panel),"no radar overlap");
            const DesktopMenuMouseRect g{gauges.Panel.X,gauges.Panel.Y,
                gauges.Panel.Width,gauges.Panel.Height};
            Require(!HudRadarOverlaps(p.Panel,g),"no gauge overlap");
            Require(p.Panel.X==g.X,"same protected text lane");
            const auto plot=BuildHudDriveCommandPlot(p,Review(12));
            Require(plot.Valid,"partial history in every admitted layout");
        }
    Require(accepted>100,"substantial valid layout sweep");
}
void TransientEnvelopeGeometry()
{
    DriveCommandTrace trace;
    trace.BeginFrame(1);
    Require(trace.Observe({1,4,5,6,0.06,1,1,-1,-1,false,false,true,true}),
        "positive command sample");
    trace.BeginFrame(2);
    Require(trace.Observe({2,4,5,6,0.06,-1,-1,1,1,false,false,true,true}),
        "negative command sample");
    auto review=trace.Read(2,4,5,6,true);
    const auto panel=Layout(1920,1080);
    auto plot=BuildHudDriveCommandPlot(panel,review);
    Require(plot.Valid,"transient range plot");
    const auto& col=plot.Columns.back();
    Require(std::abs(col.ThrottleRangeTop-panel.ThrottleGraph.Y)<1e-8 &&
        std::abs(col.ThrottleRangeBottom-panel.ThrottleGraph.Y-panel.ThrottleGraph.Height)<1e-8,
        "full signed throttle envelope bounded by fixed graph");
    Require(std::abs(col.SteeringRangeTop-panel.SteeringGraph.Y)<1e-8 &&
        std::abs(col.SteeringRangeBottom-panel.SteeringGraph.Y-panel.SteeringGraph.Height)<1e-8,
        "full signed steering envelope bounded by fixed graph");
    Require(col.ModeratedThrottleY>col.ThrottleRangeTop &&
        col.ModeratedThrottleY<col.ThrottleRangeBottom,
        "average line inside actual command range");
    for(int variant=0;variant<4;++variant)
    {
        auto invalid=review;
        auto& range=invalid.Bins.back().ModeratedThrottleRange;
        switch(variant)
        {
        case 0:range.Minimum=2;break;
        case 1:range.Maximum=std::numeric_limits<double>::infinity();break;
        case 2:range.Minimum=1;range.Maximum=-1;break;
        case 3:range.Minimum=0.5;range.Maximum=0.9;break;
        default:break;
        }
        Require(!BuildHudDriveCommandPlot(panel,invalid).Valid,
            "untrustworthy envelope rejected before draw");
    }
}
void PlotSafety()
{
    const auto panel=Layout(1920,1080);
    const auto review=Review(80);
    auto plot=BuildHudDriveCommandPlot(panel,review);
    Require(plot.Valid,"complete history");
    for(const auto& col:plot.Columns)
    {
        Require(col.Visible,"complete coverage");
        Require(HudRadarContains(panel.ThrottleGraph,col.Throttle) &&
            HudRadarContains(panel.SteeringGraph,col.Steering),"bounded graph columns");
        Require(col.FilteredThrottleY<col.ModeratedThrottleY &&
            col.FilteredSteeringY>col.ModeratedSteeringY,"signed axes preserve direction");
        Require(col.ModeratedThrottleY<panel.ThrottleGraph.Y+panel.ThrottleGraph.Height*0.5,
            "positive throttle above zero");
        Require(col.ModeratedSteeringY>panel.SteeringGraph.Y+panel.SteeringGraph.Height*0.5,
            "negative steering below zero");
        Require(col.ServiceBrake.Width>=0 && col.ServiceBrake.Width<=col.Throttle.Width+1e-8,
            "brake fraction bounded");
    }
    plot=BuildHudDriveCommandPlot(panel,Review(1));
    Require(plot.Valid,"single valid observation");
    for(std::size_t i=0;i<DriveTraceBinCount-1;++i)
        Require(!plot.Columns[i].Visible,"no invented earlier columns");
    Require(plot.Columns.back().Visible &&
        plot.Columns.back().Throttle.Width<panel.ThrottleGraph.Width/DriveTraceBinCount,
        "partial observation width");
    auto bad=review;bad.Valid=false;
    Require(!BuildHudDriveCommandPlot(panel,bad).Valid,"invalid review");
    bad=review;bad.Bins[10].ObservedSeconds=0;
    Require(!BuildHudDriveCommandPlot(panel,bad).Valid,"interior missing history");
    bad=review;bad.Bins[0].FilteredThrottle=2;
    Require(!BuildHudDriveCommandPlot(panel,bad).Valid,"malformed axis");
    bad=review;bad.Bins[0].GroundFraction=-1;
    Require(!BuildHudDriveCommandPlot(panel,bad).Valid,"malformed support fraction");
    bad=review;bad.ObservedSeconds=2;
    Require(!BuildHudDriveCommandPlot(panel,bad).Valid,"duration mismatch");
    auto corrupt=panel;corrupt.BrakeGraph.X+=1;
    Require(!BuildHudDriveCommandPlot(corrupt,review).Valid,"misaligned graph geometry");
}
}
int main()
{
    LayoutSafety();
    PlotSafety();
    TransientEnvelopeGeometry();
    std::cout<<"HudDriveCommandPanel passed\n";
}
