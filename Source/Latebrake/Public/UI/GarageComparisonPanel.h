#pragma once

#include "Garage/GarageComparisonView.h"
#include "UI/HudRaceStatusPanel.h"
#include "UI/HudMenuReadablePanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Pinned, read-only garage comparison. No purchase, equip, preview,
// tuning or input authority. Performance figures are the game's
// configured arcade multipliers under the current shared upgrades.
inline constexpr int GarageComparisonMetricCount=3;
inline constexpr int GarageComparisonMinimumDetailRows=6;
inline constexpr double GarageComparisonMinimumAxisPercent=25.0;
inline constexpr double GarageComparisonMaximumAxisPercent=500.0;
inline constexpr double GarageComparisonFlatEpsilon=0.05;

enum class GarageComparisonMetric : std::uint8_t
{
    Acceleration, Grip, TopSpeed
};

enum class GarageComparisonDirection : std::uint8_t
{
    Equal, Lower, Higher
};

inline const char* GarageComparisonMetricLabel(GarageComparisonMetric metric) noexcept
{
    switch(metric)
    {
    case GarageComparisonMetric::Acceleration:return "ACCEL";
    case GarageComparisonMetric::Grip:return "GRIP";
    case GarageComparisonMetric::TopSpeed:return "TOP SPEED";
    default:return "";
    }
}

struct GarageComparisonMetricValue final
{
    GarageComparisonMetric Metric=GarageComparisonMetric::Acceleration;
    double Equipped=0.0;
    double Viewed=0.0;
    double DeltaPercent=0.0;
    GarageComparisonDirection Direction=GarageComparisonDirection::Equal;
};

struct GarageComparisonPanelReadout final
{
    bool Valid=false;
    int EquippedId=-1;
    int ViewedId=-1;
    GarageCatalogOwnership ViewedOwnership=GarageCatalogOwnership::LockedInsufficientCredits;
    double AxisPercent=GarageComparisonMinimumAxisPercent;
    std::array<GarageComparisonMetricValue,GarageComparisonMetricCount> Metrics{};
};

inline bool ValidGarageComparisonOwnership(GarageCatalogOwnership ownership) noexcept
{
    switch(ownership)
    {
    case GarageCatalogOwnership::Equipped:
    case GarageCatalogOwnership::Owned:
    case GarageCatalogOwnership::LockedAffordable:
    case GarageCatalogOwnership::LockedInsufficientCredits:return true;
    default:return false;
    }
}

inline const char* GarageComparisonOwnershipLabel(GarageCatalogOwnership ownership) noexcept
{
    switch(ownership)
    {
    case GarageCatalogOwnership::Equipped:return "EQUIPPED";
    case GarageCatalogOwnership::Owned:return "OWNED";
    case GarageCatalogOwnership::LockedAffordable:
    case GarageCatalogOwnership::LockedInsufficientCredits:return "LOCKED PREVIEW";
    default:return "UNAVAILABLE";
    }
}

inline GarageComparisonDirection GarageComparisonDeltaDirection(double percent) noexcept
{
    if(std::abs(percent)<GarageComparisonFlatEpsilon)
        return GarageComparisonDirection::Equal;
    return percent>0.0?GarageComparisonDirection::Higher:GarageComparisonDirection::Lower;
}

inline GarageComparisonPanelReadout ReadGarageComparisonPanel(
    const GarageCatalogView& catalogue,int viewedId) noexcept
{
    const auto comparison=CompareGaragePerformance(catalogue,viewedId);
    if(!comparison.Valid)return {};
    const auto& equipped=catalogue.Cars[static_cast<std::size_t>(catalogue.SelectedVehicle)];
    const auto& viewed=catalogue.Cars[static_cast<std::size_t>(viewedId)];
    if(equipped.Ownership!=GarageCatalogOwnership::Equipped ||
        !ValidGarageComparisonOwnership(viewed.Ownership) ||
        ((viewedId==catalogue.SelectedVehicle)!=
            (viewed.Ownership==GarageCatalogOwnership::Equipped)))
        return {};
    GarageComparisonPanelReadout out;
    out.EquippedId=catalogue.SelectedVehicle;
    out.ViewedId=viewedId;
    out.ViewedOwnership=viewed.Ownership;
    const std::array<double,GarageComparisonMetricCount> base{{
        equipped.Effective.Acceleration,equipped.Effective.Grip,equipped.Effective.TopSpeed}};
    const std::array<double,GarageComparisonMetricCount> next{{
        viewed.Effective.Acceleration,viewed.Effective.Grip,viewed.Effective.TopSpeed}};
    const std::array<double,GarageComparisonMetricCount> delta{{
        comparison.AccelerationPercent,comparison.GripPercent,comparison.TopSpeedPercent}};
    double extent=GarageComparisonMinimumAxisPercent;
    for(std::size_t i=0;i<out.Metrics.size();++i)
    {
        if(!std::isfinite(delta[i]) || std::abs(delta[i])>GarageComparisonMaximumAxisPercent)
            return {};
        extent=std::max(extent,std::abs(delta[i]));
        out.Metrics[i]={static_cast<GarageComparisonMetric>(i),base[i],next[i],delta[i],
            GarageComparisonDeltaDirection(delta[i])};
    }
    // Keep a consistent scale for typical vehicles; expand in 25-point
    // steps only if an actual configured comparison exceeds it.
    out.AxisPercent=25.0*std::ceil(extent/25.0);
    out.Valid=true;
    return out;
}

struct GarageComparisonMetricLayout final
{
    DesktopMenuMouseRect Label{};
    DesktopMenuMouseRect Delta{};
    DesktopMenuMouseRect Track{};
};

struct GarageComparisonPanelLayout final
{
    bool Valid=false;
    double Scale=1.0;
    int ReservedRows=0;
    double NextDetailY=0.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Title{};
    DesktopMenuMouseRect Context{};
    std::array<GarageComparisonMetricLayout,GarageComparisonMetricCount> Metrics{};
};

// The purchase feedback and menu heading remain pinned above this
// optional card. All other text remains available through MenuPager;
// only its visible page size shrinks, not its logical content.
inline GarageComparisonPanelLayout CalculateGarageComparisonPanelLayout(
    HudLayout hud,HudMenuVerticalPlan vertical,const HudMenuReadablePanel& readable,
    double safeWidth,double currentY,int drawnRows,bool garage,bool touch) noexcept
{
    if(!garage || touch || hud.Compact || !vertical.Valid || !readable.Valid ||
        !std::isfinite(hud.Scale) || hud.Scale<0.6 || hud.Scale>1.2 ||
        !std::isfinite(hud.Left) || !std::isfinite(hud.Top) ||
        !std::isfinite(safeWidth) || safeWidth<=0.0 ||
        !std::isfinite(currentY) || drawnRows<0 || drawnRows>vertical.MaxDetailsRows ||
        vertical.MaxDetailsRows<GarageComparisonMinimumDetailRows ||
        vertical.MaxDetailsRows>90 || !std::isfinite(vertical.OverflowNoticeY) ||
        !std::isfinite(vertical.SaveWarningY) ||
        vertical.OverflowNoticeY>=vertical.SaveWarningY)
        return {};
    const double s=hud.Scale,step=28.0*s;
    if(std::abs(currentY-(hud.Top+drawnRows*step))>0.02)return {};
    constexpr int rows=5;
    const int remaining=vertical.MaxDetailsRows-drawnRows-rows;
    if(remaining<GarageComparisonMinimumDetailRows)return {};
    const double width=std::min(safeWidth,readable.X+readable.Width-hud.Left);
    if(width<242.0*s)return {};
    GarageComparisonPanelLayout out;
    out.Scale=s;
    out.ReservedRows=rows;
    out.NextDetailY=currentY+rows*step;
    out.Panel={hud.Left,currentY,width,130.0*s};
    const DesktopMenuMouseRect owner{readable.X,readable.Y,readable.Width,readable.Height};
    if(!HudRadarContains(owner,out.Panel) ||
        out.NextDetailY<out.Panel.Y+out.Panel.Height ||
        out.NextDetailY+(remaining-1)*step>vertical.OverflowNoticeY-1.25*step+1e-7)
        return {};
    const double pad=8.0*s;
    const double x=out.Panel.X+pad;
    const double w=out.Panel.Width-2*pad;
    out.Title={x,currentY+6.0*s,w,15.0*s};
    out.Context={x,currentY+23.0*s,w,14.0*s};
    for(std::size_t i=0;i<out.Metrics.size();++i)
    {
        const double y=currentY+(42.0+27.0*static_cast<double>(i))*s;
        auto& metric=out.Metrics[i];
        metric.Label={x,y,w*0.48,14.0*s};
        metric.Delta={x+w*0.50,y,w*0.50,14.0*s};
        metric.Track={x,y+17.0*s,w,5.0*s};
    }
    std::array<DesktopMenuMouseRect,2+GarageComparisonMetricCount*3> parts{};
    parts[0]=out.Title;
    parts[1]=out.Context;
    for(std::size_t i=0;i<out.Metrics.size();++i)
    {
        parts[2+3*i]=out.Metrics[i].Label;
        parts[3+3*i]=out.Metrics[i].Delta;
        parts[4+3*i]=out.Metrics[i].Track;
    }
    for(std::size_t i=0;i<parts.size();++i)
    {
        if(!HudRadarContains(out.Panel,parts[i]))return {};
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(parts[i],parts[j]))return {};
    }
    out.Valid=true;
    return out;
}

struct GarageComparisonBar final
{
    bool Valid=false;
    GarageComparisonDirection Direction=GarageComparisonDirection::Equal;
    DesktopMenuMouseRect Track{};
    DesktopMenuMouseRect Fill{};
    double ZeroX=0.0;
};

inline GarageComparisonBar BuildGarageComparisonBar(const DesktopMenuMouseRect& track,
    double percent,double axis) noexcept
{
    if(!ValidHudRadarRect(track) || !std::isfinite(percent) ||
        !std::isfinite(axis) || axis<GarageComparisonMinimumAxisPercent ||
        axis>GarageComparisonMaximumAxisPercent || std::abs(percent)>axis)
        return {};
    GarageComparisonBar out;
    out.Track=track;
    out.ZeroX=track.X+track.Width*0.5;
    out.Direction=GarageComparisonDeltaDirection(percent);
    const double span=std::abs(percent)/axis*track.Width*0.5;
    if(out.Direction!=GarageComparisonDirection::Equal)
        out.Fill={percent<0.0?out.ZeroX-span:out.ZeroX,track.Y,span,track.Height};
    if(out.Fill.Width>0.0 && !HudRadarContains(track,out.Fill))return {};
    out.Valid=true;
    return out;
}

struct GarageComparisonTextMeasurements final
{
    HudRaceStatusTextSize Title{};
    HudRaceStatusTextSize Context{};
    std::array<HudRaceStatusTextSize,GarageComparisonMetricCount> Labels{};
    std::array<HudRaceStatusTextSize,GarageComparisonMetricCount> Deltas{};
};

inline bool CanDrawGarageComparisonPanel(const GarageComparisonPanelLayout& layout,
    const GarageComparisonTextMeasurements& measures) noexcept
{
    if(!layout.Valid || !HudRaceStatusTextFits(layout.Title,measures.Title) ||
        !HudRaceStatusTextFits(layout.Context,measures.Context))
        return false;
    for(std::size_t i=0;i<layout.Metrics.size();++i)
        if(!HudRaceStatusTextFits(layout.Metrics[i].Label,measures.Labels[i]) ||
            !HudRaceStatusTextFits(layout.Metrics[i].Delta,measures.Deltas[i]))
            return false;
    return true;
}
} // namespace latebrake
