#include "UI/HudRaceRadar.h"
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
    if(!ok){std::cerr<<"HudRaceRadar: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<1e-7;}

HudRaceRadarLayout Layout(double w=1280,double h=720,bool touch=false,
    bool menu=false,bool finished=false)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto text=CalculateHudTextBounds(w,h,hud,touch);
    const auto instruments=CalculateHudInstrumentCluster(w,h,hud,touch,menu,finished,
        80,0.5,0.3,0.1);
    return CalculateHudRaceRadarLayout(w,h,hud,text,instruments,touch,menu,finished);
}

RaceMiniMapSnapshot Radar()
{
    RaceMiniMapSnapshot r;
    r.Player.Position={true,0.25,0.25};
    r.Player.DirectionX=0;r.Player.DirectionY=-1;
    r.Rival={true,0.65,0.7};
    r.NextGate={true,0.85,0.2};
    r.DirectRivalSeparationValid=true;r.DirectRivalSeparationMeters=40;
    return r;
}

void ResponsiveLayout()
{
    const auto l=Layout();
    Require(l.Valid && Near(l.Map.Width/l.Map.Height,HudRadarAspect),"original aspect");
    Require(HudRadarContains({0,0,1280,720},l.Panel) &&
        HudRadarContains(l.Panel,l.Map) && HudRadarContains(l.Panel,l.Status),
        "visible panel");
    const auto cluster=CalculateHudInstrumentCluster(1280,720,CalculateHudLayout(1280,720),
        false,false,false,100,0.5,0.5,0);
    Require(!HudRadarOverlaps(l.Panel,{cluster.Panel.X,cluster.Panel.Y,
        cluster.Panel.Width,cluster.Panel.Height}),"no gauge overlap");
    Require(!Layout(1000,700).Valid && !Layout(1280,720,true).Valid &&
        !Layout(1280,720,false,true).Valid && !Layout(1280,720,false,false,true).Valid,
        "hidden in compact, touch, menu and finished state");
    Require(!Layout(std::numeric_limits<double>::infinity(),720).Valid,"nonfinite viewport");
    const auto shortScreen=Layout(1120,600);
    Require(shortScreen.Valid && Near(shortScreen.Map.Width/shortScreen.Map.Height,HudRadarAspect),
        "short desktop retains aspect");
}

void ProjectionAndAuthoredRoads()
{
    const auto l=Layout();
    const auto north=ProjectHudRadarWorld({-4200,3000},l.Map);
    const auto south=ProjectHudRadarWorld({4200,-3000},l.Map);
    Require(north.Valid && south.Valid && north.X==l.Map.X && north.Y==l.Map.Y &&
        Near(south.X,l.Map.X+l.Map.Width) && Near(south.Y,l.Map.Y+l.Map.Height),
        "north-up corners");
    Require(!ProjectHudRadarWorld({4201,0},l.Map).Valid &&
        !ProjectHudRadarMarker({true,1.01,0.5},l.Map).Valid &&
        !ProjectHudRadarMarker({false,0.5,0.5},l.Map).Valid,"no invented edge markers");
    const auto roads=BuildHudRadarRoadGeometry(l.Map);
    Require(roads.AsphaltCount==4 && roads.BarrierCount==4,"actual original collision plan");
    for(const auto& r:roads.Asphalt)Require(HudRadarContains(l.Map,r),"asphalt bounded");
    for(const auto& r:roads.Barriers)Require(HudRadarContains(l.Map,r),"barrier clipped");
    const auto northRoad=ProjectHudRadarWorld({0,NorthY},l.Map);
    const auto southRoad=ProjectHudRadarWorld({0,SouthY},l.Map);
    Require(northRoad.Y<southRoad.Y,"north above south");
    const auto live=ProjectHudRadarWorld({1500,-600},l.Map);
    const auto replay=ProjectFleetReplayMap({1500,-600},l.Map);
    Require(live.Valid && replay.Valid && Near(live.X,replay.X) && Near(live.Y,replay.Y),
        "live and replay same projection");
    Require(!ValidHudRadarRect(ProjectHudRadarWorldRect({9000,0},100,100,l.Map)),
        "outside geometry not fabricated");
}

void RealMarkerAdmission()
{
    const auto l=Layout();
    auto r=Radar();
    auto p=BuildHudRadarPresentation(l,r,{},false,true);
    Require(p.Valid && p.Count==4 && p.ObservedRivals==1 && p.PlayerVisible &&
        p.RivalDistanceValid && p.RivalDistanceMeters==40,"real player, gate, pit, rival");
    Require(p.Markers[0].Kind==HudRadarMarkerKind::NextGate &&
        p.Markers[1].Kind==HudRadarMarkerKind::Pit &&
        p.Markers[2].Kind==HudRadarMarkerKind::Rival &&
        p.Markers[3].Kind==HudRadarMarkerKind::Player,"deterministic layering");
    const auto& player=p.Markers[3];
    Require(player.Nose.Valid && player.Nose.Y<player.Center.Y,"north heading");
    r.Player.Position={true,0,1};
    p=BuildHudRadarPresentation(l,r,{},false,false);
    Require(p.PlayerVisible && p.Count==3 &&
        HudRadarContains(l.Map,p.Markers[2].Body) &&
        p.Markers[2].Nose.Y==l.Map.Y,"edge marker and heading clipped");
    r.Player.DirectionX=0;r.Player.DirectionY=0;
    p=BuildHudRadarPresentation(l,r,{},false,false);
    Require(!p.PlayerVisible && !p.RivalDistanceValid,"invalid heading does not fabricate player");
    r=Radar();r.Rival.Valid=false;
    p=BuildHudRadarPresentation(l,r,{},false,false);
    Require(p.ObservedRivals==0 && !p.RivalDistanceValid,"missing rival omitted");
}

void FleetGroupsAndDuplicateRejection()
{
    const auto l=Layout();
    FleetRadarClusters c;
    c.Count=3;
    c.Groups[0]={{true,0.4,0.5},0x06,2};
    c.Groups[1]={{true,0.5,0.5},0x08,1};
    c.Groups[2]={{true,0.7,0.3},0x30,2};
    auto p=BuildHudRadarPresentation(l,Radar(),c,true,true);
    Require(p.Valid && p.ObservedRivals==5 && p.Count==6 &&
        !p.RivalDistanceValid && p.PlayerVisible,"only physical groups, no single-rival fallback");
    auto bad=c;bad.Groups[2].SlotMask=0x0c;
    p=BuildHudRadarPresentation(l,Radar(),bad,true,true);
    Require(p.Valid && p.ObservedRivals==0 && p.Count==3,"duplicate roster refuses entire rival set");
    bad=c;bad.Groups[0].SlotMask=0x07;
    Require(BuildHudRadarPresentation(l,Radar(),bad,true,true).ObservedRivals==0,
        "human slot cannot masquerade as rival");
    bad=c;bad.Groups[0].Count=1;
    Require(BuildHudRadarPresentation(l,Radar(),bad,true,true).ObservedRivals==0,
        "mask count mismatch");
    bad=c;bad.Groups[0].Marker.U=-1;
    Require(BuildHudRadarPresentation(l,Radar(),bad,true,true).ObservedRivals==0,
        "malformed group position");
}

void FontMeasuredLabels()
{
    const auto l=Layout();
    auto r=Radar();
    const auto p=BuildHudRadarPresentation(l,r,{},false,true);
    std::array<HudRadarLabelMeasure,HudRadarMaximumMarkers> sizes{};
    for(int i=0;i<p.Count;++i)sizes[static_cast<std::size_t>(i)]={true,22,10};
    const auto labels=PlaceHudRadarLabels(l,p,sizes);
    Require(labels.Valid && labels.Count>=3 &&
        p.Markers[static_cast<std::size_t>(labels.Labels[0].MarkerIndex)].Kind==
            HudRadarMarkerKind::Player,"player label first");
    for(int i=0;i<labels.Count;++i)
    {
        const auto& label=labels.Labels[static_cast<std::size_t>(i)];
        Require(label.Visible && HudRadarContains(l.Map,label.Bounds),"all labels within map");
        for(int j=0;j<p.Count;++j)
            Require(!HudRadarOverlaps(label.Bounds,p.Markers[static_cast<std::size_t>(j)].Body),
                "labels avoid marker bodies");
        for(int j=0;j<i;++j)
            Require(!HudRadarOverlaps(label.Bounds,labels.Labels[static_cast<std::size_t>(j)].Bounds),
                "labels avoid one another");
    }
    sizes[3].Width=l.Map.Width+1;
    const auto fit=PlaceHudRadarLabels(l,p,sizes);
    Require(fit.Valid,"unfit font does not remove map");
    for(int i=0;i<fit.Count;++i)
        Require(fit.Labels[static_cast<std::size_t>(i)].MarkerIndex!=3,"oversized text omitted");
    sizes[3].Width=std::numeric_limits<double>::quiet_NaN();
    Require(PlaceHudRadarLabels(l,p,sizes).Valid,"malformed measurement omitted");
}

void RandomViewportsAndCrowding()
{
    std::mt19937 rng(20838);
    int visible=0;
    for(int i=0;i<3000;++i)
    {
        const double w=900+rng()%3000,h=500+rng()%1400;
        const auto l=Layout(w,h);
        if(!l.Valid)continue;
        ++visible;
        Require(HudRadarContains({0,0,w,h},l.Panel),"random viewport bounded");
        const auto instruments=CalculateHudInstrumentCluster(w,h,CalculateHudLayout(w,h),
            false,false,false,100,0.5,0.5,0);
        if(instruments.Visible)
            Require(!HudRadarOverlaps(l.Panel,{instruments.Panel.X,instruments.Panel.Y,
                instruments.Panel.Width,instruments.Panel.Height}),"instrument separation");
        FleetRadarClusters c;c.Count=5;
        for(int j=0;j<5;++j)
        {
            const double u=static_cast<double>(rng()%101)/100.0;
            const double v=static_cast<double>(rng()%101)/100.0;
            c.Groups[static_cast<std::size_t>(j)]={{true,u,v},
                1u<<static_cast<unsigned>(j+1),1};
        }
        const auto p=BuildHudRadarPresentation(l,Radar(),c,true,true);
        Require(p.Valid && p.Count==8 && p.ObservedRivals==5,"full fleet capacity");
        std::array<HudRadarLabelMeasure,HudRadarMaximumMarkers> sizes{};
        for(auto& size:sizes)size={true,10.0+rng()%40,8.0+rng()%10};
        const auto labels=PlaceHudRadarLabels(l,p,sizes);
        Require(labels.Valid && labels.Count<=p.Count,"bounded collision-free labels");
        for(int j=0;j<labels.Count;++j)
        {
            const auto& r=labels.Labels[static_cast<std::size_t>(j)].Bounds;
            Require(HudRadarContains(l.Map,r),"crowded map labels bounded");
            for(int k=0;k<j;++k)
                Require(!HudRadarOverlaps(r,labels.Labels[static_cast<std::size_t>(k)].Bounds),
                    "crowded labels independent");
        }
    }
    Require(visible>100,"usable desktop coverage");
}
}

int main()
{
    ResponsiveLayout();ProjectionAndAuthoredRoads();RealMarkerAdmission();
    FleetGroupsAndDuplicateRejection();FontMeasuredLabels();RandomViewportsAndCrowding();
    std::cout<<"Live race radar geometry, identity, projection and measured label tests passed\n";
}
