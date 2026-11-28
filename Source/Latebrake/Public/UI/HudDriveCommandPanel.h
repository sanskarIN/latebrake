#pragma once

#include "Racing/DriveCommandTrace.h"
#include "UI/HudRaceRadar.h"

namespace latebrake
{
// Optional desktop detail lane; the standard gauges and radar always
// keep their own positions. A narrow/short/touch viewport gets text only.
struct HudDriveCommandPanel final
{
    bool Valid=false;
    double Scale=1.0;
    DesktopMenuMouseRect Panel{};
    DesktopMenuMouseRect Header{};
    DesktopMenuMouseRect ThrottleTitle{};
    DesktopMenuMouseRect ThrottleGraph{};
    DesktopMenuMouseRect SteeringTitle{};
    DesktopMenuMouseRect SteeringGraph{};
    DesktopMenuMouseRect BrakeTitle{};
    DesktopMenuMouseRect BrakeGraph{};
    DesktopMenuMouseRect SupportGraph{};
    DesktopMenuMouseRect Footer{};
};

inline HudDriveCommandPanel CalculateHudDriveCommandPanel(
    double width,double height,const HudInstrumentCluster& gauges,
    const HudRaceRadarLayout& radar,bool requested) noexcept
{
    if(!requested || !gauges.Visible || !radar.Valid ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width<1120.0 || height<600.0 || width>20000.0 || height>20000.0 ||
        !std::isfinite(gauges.Scale) || gauges.Scale<0.85 || gauges.Scale>1.2)
        return {};
    const DesktopMenuMouseRect gauge{gauges.Panel.X,gauges.Panel.Y,
        gauges.Panel.Width,gauges.Panel.Height};
    if(!HudRadarContains({0,0,width,height},gauge) ||
        !HudRadarContains({0,0,width,height},radar.Panel))
        return {};
    const double s=gauges.Scale;
    const double top=radar.Panel.Y+radar.Panel.Height+8.0*s;
    const double bottom=gauge.Y-8.0*s;
    const double available=bottom-top;
    // The compact three-track chart needs explicit height; never shrink
    // the speedometer, radar or labels to make room.
    if(!std::isfinite(available) || available<152.0*s)
        return {};
    HudDriveCommandPanel out;
    out.Scale=s;
    out.Panel={gauge.X,top,gauge.Width,152.0*s};
    if(!HudRadarContains({0,0,width,height},out.Panel) ||
        HudRadarOverlaps(out.Panel,gauge) || HudRadarOverlaps(out.Panel,radar.Panel))
        return {};
    const double pad=10.0*s;
    const double x=out.Panel.X+pad;
    const double w=out.Panel.Width-2.0*pad;
    double y=top+5.0*s;
    const auto row=[&](double h,double gap)
    {
        const DesktopMenuMouseRect r{x,y,w,h*s};
        y+=(h+gap)*s;
        return r;
    };
    out.Header=row(14,3);
    out.ThrottleTitle=row(13,2);
    out.ThrottleGraph=row(19,3);
    out.SteeringTitle=row(13,2);
    out.SteeringGraph=row(19,3);
    out.BrakeTitle=row(13,2);
    out.BrakeGraph=row(10,3);
    out.SupportGraph=row(6,3);
    out.Footer=row(14,0);
    const std::array<DesktopMenuMouseRect,9> parts{{out.Header,out.ThrottleTitle,
        out.ThrottleGraph,out.SteeringTitle,out.SteeringGraph,out.BrakeTitle,
        out.BrakeGraph,out.SupportGraph,out.Footer}};
    for(std::size_t i=0;i<parts.size();++i)
    {
        if(!HudRadarContains(out.Panel,parts[i]))return {};
        for(std::size_t j=0;j<i;++j)
            if(HudRadarOverlaps(parts[i],parts[j]))return {};
    }
    out.Valid=true;
    return out;
}

struct DriveCommandColumn final
{
    bool Visible=false;
    DesktopMenuMouseRect Throttle{};
    DesktopMenuMouseRect Steering{};
    DesktopMenuMouseRect ServiceBrake{};
    DesktopMenuMouseRect Handbrake{};
    DesktopMenuMouseRect GroundSupport{};
    double FilteredThrottleY=0.0;
    double ModeratedThrottleY=0.0;
    double FilteredSteeringY=0.0;
    double ModeratedSteeringY=0.0;
    double CenterX=0.0;
    double ThrottleRangeTop=0.0;
    double ThrottleRangeBottom=0.0;
    double SteeringRangeTop=0.0;
    double SteeringRangeBottom=0.0;
};

struct HudDriveCommandPlot final
{
    bool Valid=false;
    std::array<DriveCommandColumn,DriveTraceBinCount> Columns{};
};

inline bool ValidDriveTraceAxisRange(DriveTraceAxisRange range,double mean) noexcept
{
    return ValidDriveCommandAxis(range.Minimum) &&
        ValidDriveCommandAxis(range.Maximum) &&
        range.Minimum<=range.Maximum &&
        std::isfinite(mean) &&
        mean>=range.Minimum-1e-9 &&
        mean<=range.Maximum+1e-9;
}

inline bool ValidDriveTraceBin(const DriveTraceBin& b) noexcept
{
    const auto fraction=[](double f)
    {
        return std::isfinite(f) && f>=-1e-9 && f<=1.0+1e-9;
    };
    return std::isfinite(b.ObservedSeconds) && b.ObservedSeconds>=0.0 &&
        b.ObservedSeconds<=DriveTraceBinSeconds+1e-9 &&
        std::isfinite(b.FilteredThrottle) && std::abs(b.FilteredThrottle)<=1.0+1e-9 &&
        std::isfinite(b.ModeratedThrottle) && std::abs(b.ModeratedThrottle)<=1.0+1e-9 &&
        std::isfinite(b.FilteredSteering) && std::abs(b.FilteredSteering)<=1.0+1e-9 &&
        std::isfinite(b.ModeratedSteering) && std::abs(b.ModeratedSteering)<=1.0+1e-9 &&
        fraction(b.ServiceBrakeFraction) && fraction(b.HandbrakeFraction) &&
        fraction(b.GroundFraction) &&
        ValidDriveTraceAxisRange(b.ModeratedThrottleRange,b.ModeratedThrottle) &&
        ValidDriveTraceAxisRange(b.ModeratedSteeringRange,b.ModeratedSteering);
}

inline double DriveTraceAxisY(DesktopMenuMouseRect graph,double axis) noexcept
{
    // Signed command: positive at top, negative at bottom. No axis
    // rescaling between frames, so the visual comparison stays honest.
    return graph.Y+(1.0-std::clamp(axis,-1.0,1.0))*graph.Height*0.5;
}

inline HudDriveCommandPlot BuildHudDriveCommandPlot(
    const HudDriveCommandPanel& panel,const DriveCommandReview& review) noexcept
{
    HudDriveCommandPlot out;
    if(!panel.Valid || !review.Valid || !ValidDriveCommandSample(review.Latest) ||
        !std::isfinite(review.ObservedSeconds) || review.ObservedSeconds<=0.0 ||
        review.ObservedSeconds>DriveTraceWindowSeconds+1e-9)
        return out;
    const std::array<DesktopMenuMouseRect,4> graphs{{panel.ThrottleGraph,
        panel.SteeringGraph,panel.BrakeGraph,panel.SupportGraph}};
    for(const auto& graph:graphs)
        if(!HudRadarContains(panel.Panel,graph) ||
            graph.X!=panel.ThrottleGraph.X || graph.Width!=panel.ThrottleGraph.Width)
            return {};
    double total=0.0;
    bool seenObserved=false;
    for(std::size_t i=0;i<review.Bins.size();++i)
    {
        const auto& bin=review.Bins[i];
        if(!ValidDriveTraceBin(bin))return {};
        total+=bin.ObservedSeconds;
        if(bin.ObservedSeconds<=1e-12)
        {
            if(seenObserved)return {}; // history must be contiguous
            continue;
        }
        if(seenObserved && bin.ObservedSeconds<DriveTraceBinSeconds-1e-9)
            return {};
        seenObserved=true;
        auto& col=out.Columns[i];
        const double step=panel.ThrottleGraph.Width/DriveTraceBinCount;
        const double left=panel.ThrottleGraph.X+i*step;
        const double right=i+1==DriveTraceBinCount?
            panel.ThrottleGraph.X+panel.ThrottleGraph.Width:
            panel.ThrottleGraph.X+(i+1)*step;
        // The oldest partial bin uses only its observed width.
        const double coverage=std::clamp(bin.ObservedSeconds/DriveTraceBinSeconds,0.0,1.0);
        const double x=std::max(left,right-(right-left)*coverage);
        const double width=right-x;
        col.Visible=true;
        col.CenterX=x+width*0.5;
        col.Throttle={x,panel.ThrottleGraph.Y,width,panel.ThrottleGraph.Height};
        col.Steering={x,panel.SteeringGraph.Y,width,panel.SteeringGraph.Height};
        col.FilteredThrottleY=DriveTraceAxisY(panel.ThrottleGraph,bin.FilteredThrottle);
        col.ModeratedThrottleY=DriveTraceAxisY(panel.ThrottleGraph,bin.ModeratedThrottle);
        col.FilteredSteeringY=DriveTraceAxisY(panel.SteeringGraph,bin.FilteredSteering);
        col.ModeratedSteeringY=DriveTraceAxisY(panel.SteeringGraph,bin.ModeratedSteering);
        col.ThrottleRangeTop=DriveTraceAxisY(panel.ThrottleGraph,bin.ModeratedThrottleRange.Maximum);
        col.ThrottleRangeBottom=DriveTraceAxisY(panel.ThrottleGraph,bin.ModeratedThrottleRange.Minimum);
        col.SteeringRangeTop=DriveTraceAxisY(panel.SteeringGraph,bin.ModeratedSteeringRange.Maximum);
        col.SteeringRangeBottom=DriveTraceAxisY(panel.SteeringGraph,bin.ModeratedSteeringRange.Minimum);
        const double half=panel.BrakeGraph.Height*0.5;
        col.ServiceBrake={x,panel.BrakeGraph.Y,width*std::clamp(bin.ServiceBrakeFraction,0.0,1.0),half};
        col.Handbrake={x,panel.BrakeGraph.Y+half,width*std::clamp(bin.HandbrakeFraction,0.0,1.0),half};
        col.GroundSupport={x,panel.SupportGraph.Y,width*std::clamp(bin.GroundFraction,0.0,1.0),
            panel.SupportGraph.Height};
        if(!HudRadarContains(panel.ThrottleGraph,col.Throttle) ||
            !HudRadarContains(panel.SteeringGraph,col.Steering))
            return {};
        for(const auto& rect:{col.ServiceBrake,col.Handbrake,col.GroundSupport})
            if(rect.Width>0.0 && !HudRadarContains(panel.Panel,rect))return {};
    }
    if(std::abs(total-review.ObservedSeconds)>1e-8)return {};
    out.Valid=seenObserved;
    return out;
}
} // namespace latebrake
