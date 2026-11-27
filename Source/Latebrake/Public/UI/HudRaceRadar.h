#pragma once

#include "AI/FleetRaceReview.h"
#include "Track/CircuitCollisionPlan.h"
#include "UI/HudInstrumentCluster.h"
#include "UI/HudMenuMouseControls.h"
#include "UI/HudTextBounds.h"
#include "Racing/RacePitService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Shared north-up circuit projection for live and replay maps. These
// geometry functions do not grant position, gate or collision authority.
inline constexpr double HudRadarWorldHalfX=4200.0;
inline constexpr double HudRadarWorldHalfY=3000.0;
inline constexpr double HudRadarAspect=HudRadarWorldHalfX/HudRadarWorldHalfY;
inline constexpr int HudRadarMaximumMarkers=8;
inline constexpr int HudRadarMaximumLabels=8;

inline bool ValidHudRadarRect(const DesktopMenuMouseRect& r) noexcept
{
    return std::isfinite(r.X) && std::isfinite(r.Y) && std::isfinite(r.Width) &&
        std::isfinite(r.Height) && r.X>=0.0 && r.Y>=0.0 && r.Width>0.0 && r.Height>0.0 &&
        std::isfinite(r.X+r.Width) && std::isfinite(r.Y+r.Height) &&
        r.X+r.Width>r.X && r.Y+r.Height>r.Y;
}

inline bool HudRadarContains(const DesktopMenuMouseRect& outer,
    const DesktopMenuMouseRect& inner) noexcept
{
    return ValidHudRadarRect(outer) && ValidHudRadarRect(inner) &&
        inner.X>=outer.X && inner.Y>=outer.Y &&
        inner.X+inner.Width<=outer.X+outer.Width+1e-7 &&
        inner.Y+inner.Height<=outer.Y+outer.Height+1e-7;
}

inline bool HudRadarOverlaps(const DesktopMenuMouseRect& a,
    const DesktopMenuMouseRect& b) noexcept
{
    return ValidHudRadarRect(a) && ValidHudRadarRect(b) &&
        a.X<b.X+b.Width && b.X<a.X+a.Width &&
        a.Y<b.Y+b.Height && b.Y<a.Y+a.Height;
}

struct HudRadarPoint final
{
    bool Valid=false;
    double X=0.0;
    double Y=0.0;
};

inline HudRadarPoint ProjectHudRadarMarker(RadarMarker marker,
    const DesktopMenuMouseRect& map) noexcept
{
    if(!ValidHudRadarRect(map) || !marker.Valid ||
        !std::isfinite(marker.U) || !std::isfinite(marker.V) ||
        marker.U<0.0 || marker.U>1.0 || marker.V<0.0 || marker.V>1.0)
        return {};
    return {true,map.X+marker.U*map.Width,map.Y+(1.0-marker.V)*map.Height};
}

inline HudRadarPoint ProjectHudRadarWorld(CircuitPoint p,
    const DesktopMenuMouseRect& map) noexcept
{
    if(!std::isfinite(p.X) || !std::isfinite(p.Y) ||
        std::abs(p.X)>HudRadarWorldHalfX || std::abs(p.Y)>HudRadarWorldHalfY)
        return {};
    const auto uv=NormalizeCircuitMap(p);
    return ProjectHudRadarMarker({true,uv.U,uv.V},map);
}

// Preserve aspect ratio and explicit separation from the lower right
// instruments and left font-measured text lane. No arbitrary square
// projection distorts the starter circuit's 84m x 60m map region.
struct HudRaceRadarLayout final
{
    bool Valid=false;
    double Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Title{};
    DesktopMenuMouseRect Map{};
    DesktopMenuMouseRect Status{};
};

inline HudRaceRadarLayout CalculateHudRaceRadarLayout(double width,double height,
    HudLayout hud,HudTextBounds text,const HudInstrumentCluster& instruments,
    bool touch,bool menuOpen,bool finished) noexcept
{
    HudRaceRadarLayout out;
    if(touch || menuOpen || finished || hud.Compact || !text.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<1120 || height<600 || width>20000 || height>20000 ||
        !std::isfinite(hud.Scale) || hud.Scale<0.25 || hud.Scale>4 ||
        !std::isfinite(hud.Left) || hud.Left<0 || !std::isfinite(text.Left) ||
        !std::isfinite(text.Right) || text.Right<=text.Left)
        return out;
    const double s=std::clamp(hud.Scale,0.65,1.2);
    const double right=instruments.Visible?
        instruments.Panel.X+instruments.Panel.Width:
        width-std::max(18.0*s,width*0.018);
    const double top=std::max(20.0,22.0*s);
    double lower=height-72.0*s;
    if(instruments.Visible)
    {
        const DesktopMenuMouseRect instrument{instruments.Panel.X,instruments.Panel.Y,
            instruments.Panel.Width,instruments.Panel.Height};
        if(!HudRadarContains({0,0,width,height},instrument))return out;
        lower=std::min(lower,instrument.Y-12.0*s);
    }
    const double pad=10.0*s;
    const double title=23.0*s;
    const double status=22.0*s;
    const double gap=6.0*s;
    const double fixedHeight=2.0*pad+title+status+2.0*gap;
    const double preferredMapWidth=240.0*s;
    const double mapWidth=std::min(preferredMapWidth,(lower-top-fixedHeight)*HudRadarAspect);
    if(!std::isfinite(mapWidth) || mapWidth<146.0)return out;
    const double mapHeight=mapWidth/HudRadarAspect;
    const double panelWidth=mapWidth+2.0*pad;
    const double left=right-panelWidth;
    out.Panel={left,top,panelWidth,fixedHeight+mapHeight};
    if(!HudRadarContains({0,0,width,height},out.Panel) ||
        left<std::max(width*0.53,text.Left+280.0*s))
        return {};
    out.Scale=s;
    const double x=left+pad;
    double y=top+pad;
    out.Title={x,y,mapWidth,title};
    y+=title+gap;
    out.Map={x,y,mapWidth,mapHeight};
    y+=mapHeight+gap;
    out.Status={x,y,mapWidth,status};
    if(!HudRadarContains(out.Panel,out.Title) ||
        !HudRadarContains(out.Panel,out.Map) ||
        !HudRadarContains(out.Panel,out.Status))
        return {};
    out.Valid=true;
    return out;
}

inline DesktopMenuMouseRect ProjectHudRadarWorldRect(CircuitPoint center,
    double widthCm,double heightCm,const DesktopMenuMouseRect& map) noexcept
{
    if(!ValidHudRadarRect(map) || !std::isfinite(center.X) || !std::isfinite(center.Y) ||
        !std::isfinite(widthCm) || !std::isfinite(heightCm) || widthCm<=0 || heightCm<=0)
        return {};
    const double left=std::max(-HudRadarWorldHalfX,center.X-widthCm*0.5);
    const double right=std::min(HudRadarWorldHalfX,center.X+widthCm*0.5);
    const double bottom=std::max(-HudRadarWorldHalfY,center.Y-heightCm*0.5);
    const double top=std::min(HudRadarWorldHalfY,center.Y+heightCm*0.5);
    if(right<=left || top<=bottom)return {};
    const auto a=ProjectHudRadarWorld({left,top},map);
    const auto b=ProjectHudRadarWorld({right,bottom},map);
    if(!a.Valid || !b.Valid)return {};
    const DesktopMenuMouseRect r{a.X,a.Y,b.X-a.X,b.Y-a.Y};
    return HudRadarContains(map,r)?r:DesktopMenuMouseRect{};
}

struct HudRadarRoadGeometry final
{
    int AsphaltCount=0;
    int BarrierCount=0;
    std::array<DesktopMenuMouseRect,4> Asphalt{};
    std::array<DesktopMenuMouseRect,4> Barriers{};
};

inline HudRadarRoadGeometry BuildHudRadarRoadGeometry(const DesktopMenuMouseRect& map) noexcept
{
    HudRadarRoadGeometry out;
    if(!ValidHudRadarRect(map))return out;
    for(const auto& block:CircuitCollisionBlocks)
    {
        if(!IsValidCollisionBlock(block) || block.Style==CollisionBlockStyle::Ground)
            continue;
        const auto r=ProjectHudRadarWorldRect(block.Center,block.ScaleX*100.0,
            block.ScaleY*100.0,map);
        if(!ValidHudRadarRect(r))continue;
        if(block.Style==CollisionBlockStyle::Asphalt && out.AsphaltCount<4)
            out.Asphalt[static_cast<std::size_t>(out.AsphaltCount++)]=r;
        if(block.Style==CollisionBlockStyle::Barrier && out.BarrierCount<4)
            out.Barriers[static_cast<std::size_t>(out.BarrierCount++)]=r;
    }
    return out;
}

enum class HudRadarMarkerKind
{
    NextGate,
    Pit,
    Rival,
    Fleet,
    Player
};

struct HudRadarDrawMarker final
{
    bool Valid=false;
    HudRadarMarkerKind Kind=HudRadarMarkerKind::Player;
    int Count=0;
    std::uint32_t SlotMask=0;
    HudRadarPoint Center{};
    DesktopMenuMouseRect Body{};
    HudRadarPoint Nose{};
};

inline DesktopMenuMouseRect ClipHudRadarMarker(HudRadarPoint center,double radius,
    const DesktopMenuMouseRect& map) noexcept
{
    if(!center.Valid || !ValidHudRadarRect(map) || !std::isfinite(center.X) ||
        !std::isfinite(center.Y) || !std::isfinite(radius) || radius<=0.0)
        return {};
    const double x=std::max(map.X,center.X-radius);
    const double y=std::max(map.Y,center.Y-radius);
    const double r=std::min(map.X+map.Width,center.X+radius);
    const double b=std::min(map.Y+map.Height,center.Y+radius);
    const DesktopMenuMouseRect out{x,y,r-x,b-y};
    return HudRadarContains(map,out)?out:DesktopMenuMouseRect{};
}

inline int HudRadarPopcount(std::uint32_t mask) noexcept
{
    int out=0;
    while(mask){out+=static_cast<int>(mask&1u);mask>>=1;}
    return out;
}

struct HudRadarPresentation final
{
    bool Valid=false;
    HudRadarRoadGeometry Roads{};
    int Count=0;
    int ObservedRivals=0;
    bool PlayerVisible=false;
    bool RivalDistanceValid=false;
    double RivalDistanceMeters=0.0;
    std::array<HudRadarDrawMarker,HudRadarMaximumMarkers> Markers{};
};

inline HudRadarPresentation BuildHudRadarPresentation(const HudRaceRadarLayout& layout,
    const RaceMiniMapSnapshot& radar,const FleetRadarClusters& clusters,
    bool experimental,bool pitReady) noexcept
{
    HudRadarPresentation out;
    if(!layout.Valid || !HudRadarContains(layout.Panel,layout.Map) ||
        !std::isfinite(layout.Scale) || layout.Scale<0.25 || layout.Scale>4.0)
        return out;
    out.Roads=BuildHudRadarRoadGeometry(layout.Map);
    if(out.Roads.AsphaltCount!=4 || out.Roads.BarrierCount!=4)return {};
    const auto add=[&](HudRadarMarkerKind kind,RadarMarker source,double radius,
        int count,std::uint32_t mask) -> HudRadarDrawMarker*
    {
        if(out.Count>=HudRadarMaximumMarkers)return nullptr;
        const auto center=ProjectHudRadarMarker(source,layout.Map);
        const auto body=ClipHudRadarMarker(center,radius*layout.Scale,layout.Map);
        if(!center.Valid || !ValidHudRadarRect(body))return nullptr;
        auto& marker=out.Markers[static_cast<std::size_t>(out.Count++)];
        marker={true,kind,count,mask,center,body,{}};
        return &marker;
    };
    if(radar.NextGate.Valid)add(HudRadarMarkerKind::NextGate,radar.NextGate,5.0,1,0);
    if(pitReady && IsOnPavedSurface(RacePitBay))
    {
        const auto uv=NormalizeCircuitMap(RacePitBay);
        add(HudRadarMarkerKind::Pit,{true,uv.U,uv.V},3.5,1,0);
    }
    if(experimental)
    {
        // Use only the previously verified physical group masks. If the
        // same rival appears twice or a malformed group slips through,
        // refuse the rival set rather than render duplicate cars.
        bool valid=clusters.Count>=0 && clusters.Count<=StartGridCapacity-1;
        std::uint32_t seen=0;
        for(int i=0;valid && i<clusters.Count;++i)
        {
            const auto& g=clusters.Groups[static_cast<std::size_t>(i)];
            valid=g.Count>=1 && g.Count<StartGridCapacity &&
                (g.SlotMask&~0x3eu)==0 && (g.SlotMask&seen)==0 &&
                HudRadarPopcount(g.SlotMask)==g.Count &&
                ProjectHudRadarMarker(g.Marker,layout.Map).Valid;
            seen|=g.SlotMask;
        }
        if(valid)
        {
            for(int i=0;i<clusters.Count;++i)
            {
                const auto& g=clusters.Groups[static_cast<std::size_t>(i)];
                if(add(HudRadarMarkerKind::Fleet,g.Marker,g.Count>1?5.0:4.0,
                    g.Count,g.SlotMask))
                    out.ObservedRivals+=g.Count;
            }
        }
    }
    else if(radar.Rival.Valid)
    {
        if(add(HudRadarMarkerKind::Rival,radar.Rival,4.0,1,0))
            out.ObservedRivals=1;
    }
    if(radar.Player.Position.Valid &&
        std::isfinite(radar.Player.DirectionX) && std::isfinite(radar.Player.DirectionY))
    {
        const double length=std::hypot(radar.Player.DirectionX,radar.Player.DirectionY);
        if(std::abs(length-1.0)<=0.002)
        {
            auto* player=add(HudRadarMarkerKind::Player,radar.Player.Position,4.5,1,1);
            if(player)
            {
                out.PlayerVisible=true;
                const double l=11.0*layout.Scale;
                player->Nose={true,std::clamp(player->Center.X+l*radar.Player.DirectionX,
                    layout.Map.X,layout.Map.X+layout.Map.Width),
                    std::clamp(player->Center.Y+l*radar.Player.DirectionY,
                    layout.Map.Y,layout.Map.Y+layout.Map.Height)};
            }
        }
    }
    if(!experimental && out.PlayerVisible && out.ObservedRivals==1 &&
        radar.DirectRivalSeparationValid && std::isfinite(radar.DirectRivalSeparationMeters) &&
        radar.DirectRivalSeparationMeters>=0 && radar.DirectRivalSeparationMeters<=104.0)
    {
        out.RivalDistanceValid=true;
        out.RivalDistanceMeters=radar.DirectRivalSeparationMeters;
    }
    out.Valid=true;
    return out;
}

// Font-measured label placement. Try eight relative positions, keep
// complete labels within the map, and never overlap vehicle glyphs or
// earlier labels. Omit impossible text, not the actual vehicle marker.
struct HudRadarLabelMeasure final
{
    bool Requested=false;
    double Width=0.0;
    double Height=0.0;
};

struct HudRadarLabel final
{
    bool Visible=false;
    int MarkerIndex=-1;
    DesktopMenuMouseRect Bounds{};
};

struct HudRadarLabelPlan final
{
    bool Valid=false;
    int Count=0;
    std::array<HudRadarLabel,HudRadarMaximumLabels> Labels{};
};

inline int HudRadarLabelPriority(HudRadarMarkerKind kind) noexcept
{
    switch(kind)
    {
        case HudRadarMarkerKind::Player:return 0;
        case HudRadarMarkerKind::NextGate:return 1;
        case HudRadarMarkerKind::Fleet:return 2;
        case HudRadarMarkerKind::Rival:return 2;
        case HudRadarMarkerKind::Pit:return 3;
        default:return 4;
    }
}

inline HudRadarLabelPlan PlaceHudRadarLabels(const HudRaceRadarLayout& layout,
    const HudRadarPresentation& source,
    const std::array<HudRadarLabelMeasure,HudRadarMaximumMarkers>& measured) noexcept
{
    HudRadarLabelPlan out;
    if(!layout.Valid || !source.Valid || source.Count<0 ||
        source.Count>HudRadarMaximumMarkers || !HudRadarContains(layout.Panel,layout.Map))
        return out;
    const double gap=3.0*layout.Scale;
    for(int i=0;i<source.Count;++i)
    {
        const auto& m=source.Markers[static_cast<std::size_t>(i)];
        if(!m.Valid || !m.Center.Valid || !HudRadarContains(layout.Map,m.Body))
            return {};
    }
    for(int priority=0;priority<=3;++priority)
    {
        for(int i=0;i<source.Count;++i)
        {
            const auto& marker=source.Markers[static_cast<std::size_t>(i)];
            const auto& size=measured[static_cast<std::size_t>(i)];
            if(HudRadarLabelPriority(marker.Kind)!=priority || !size.Requested ||
                !std::isfinite(size.Width) || !std::isfinite(size.Height) ||
                size.Width<=0.0 || size.Height<=0.0 ||
                size.Width>layout.Map.Width || size.Height>layout.Map.Height)
                continue;
            const auto& b=marker.Body;
            const double x=marker.Center.X,y=marker.Center.Y,w=size.Width,h=size.Height;
            const std::array<DesktopMenuMouseRect,8> trials{{
                {b.X+b.Width+gap,y-h*0.5,w,h},
                {b.X-gap-w,y-h*0.5,w,h},
                {x-w*0.5,b.Y-gap-h,w,h},
                {x-w*0.5,b.Y+b.Height+gap,w,h},
                {b.X+b.Width+gap,b.Y-gap-h,w,h},
                {b.X-gap-w,b.Y-gap-h,w,h},
                {b.X+b.Width+gap,b.Y+b.Height+gap,w,h},
                {b.X-gap-w,b.Y+b.Height+gap,w,h}
            }};
            for(const auto& r:trials)
            {
                if(!HudRadarContains(layout.Map,r))continue;
                bool clear=true;
                for(int j=0;clear && j<source.Count;++j)
                    clear=!HudRadarOverlaps(r,source.Markers[static_cast<std::size_t>(j)].Body);
                for(int j=0;clear && j<out.Count;++j)
                    clear=!HudRadarOverlaps(r,out.Labels[static_cast<std::size_t>(j)].Bounds);
                if(!clear)continue;
                out.Labels[static_cast<std::size_t>(out.Count++)]={true,i,r};
                break;
            }
        }
    }
    out.Valid=true;
    return out;
}
} // namespace latebrake
