#pragma once
#include "Track/WorldScaleCircuitVariants.h"
#include "Track/WorldScaleRacePacePlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace latebrake
{
// Closed-lap pace for each independently authored candidate road.
// No original race timestamps, official checkpoint or native UE car
// state are accessed or modified here.
struct WorldScaleVariantPacePlan final
{
    bool Valid=false;
    WorldScaleVariantRecipe Track{};
    double StepCm=0.0;
    double EstimatedLapSeconds=0.0;
    unsigned RelaxationPasses=0;
    std::array<WorldScalePaceNode,WorldScalePaceStations> Nodes{};
    bool OriginalRaceResultsUnchanged=true;
};
inline double WorldScaleVariantCurvatureAt(
    const WorldScaleVariantRecipe& p,double station,
    double halfWindow) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p)||
       !std::isfinite(station)||
       !std::isfinite(halfWindow)||
       halfWindow<=0.0||halfWindow>1000.0)
        return std::numeric_limits<double>::quiet_NaN();
    const auto before=WorldScaleVariantCenterlineAt(p,station-halfWindow);
    const auto after=WorldScaleVariantCenterlineAt(p,station+halfWindow);
    if(!before.Valid||!after.Valid)
        return std::numeric_limits<double>::quiet_NaN();
    const double turn=std::remainder(
        after.Pose.HeadingDegrees-before.Pose.HeadingDegrees,360.0);
    return turn*WorldScalePaceRadPerDegree/(2.0*halfWindow);
}
inline WorldScaleVariantPacePlan BuildWorldScaleVariantPace(
    const WorldScaleVariantRecipe& p) noexcept
{
    if(!ValidWorldScaleVariantRecipe(p))return {};
    WorldScaleVariantPacePlan result{};
    result.Track=p;
    result.StepCm=p.LapLengthCm/WorldScalePaceStations;
    std::array<double,WorldScalePaceStations> squared{};
    for(unsigned i=0;i<WorldScalePaceStations;++i)
    {
        const double s=i*result.StepCm;
        const auto source=WorldScaleVariantCenterlineAt(p,s);
        const double curve=std::abs(WorldScaleVariantCurvatureAt(
            p,s,result.StepCm*0.5));
        if(!source.Valid||!std::isfinite(curve)||
           curve>0.002||
           !IsOnWorldScaleVariantRoad(p,source.Pose.Position))
            return {};
        const double limit=curve<=1e-12 ?
            WorldScalePaceMaxSpeedCmps :
            std::min(WorldScalePaceMaxSpeedCmps,
                std::sqrt(WorldScalePaceLateralAccelCmps2/curve));
        if(!std::isfinite(limit)||
           limit<WorldScalePaceMinSpeedCmps)
            return {};
        result.Nodes[i]={source.Pose.Position,s,curve,limit,limit};
        squared[i]=limit*limit;
    }
    const double brake=2.0*WorldScalePaceBrakeAccelCmps2*result.StepCm;
    const double drive=2.0*WorldScalePaceDriveAccelCmps2*result.StepCm;
    bool settled=false;
    for(unsigned pass=0;pass<WorldScalePaceStations;++pass)
    {
        bool changed=false;
        for(unsigned i=WorldScalePaceStations;i-- >0u;)
        {
            const unsigned next=(i+1u)%WorldScalePaceStations;
            if(squared[i]>squared[next]+brake)
            {
                squared[i]=squared[next]+brake;
                changed=true;
            }
        }
        for(unsigned i=0;i<WorldScalePaceStations;++i)
        {
            const unsigned next=(i+1u)%WorldScalePaceStations;
            if(squared[next]>squared[i]+drive)
            {
                squared[next]=squared[i]+drive;
                changed=true;
            }
        }
        result.RelaxationPasses=pass+1u;
        if(!changed){settled=true;break;}
    }
    if(!settled)return {};
    double seconds=0.0;
    for(unsigned i=0;i<WorldScalePaceStations;++i)
    {
        const unsigned next=(i+1u)%WorldScalePaceStations;
        const double v=std::sqrt(squared[i]);
        const double ahead=std::sqrt(squared[next]);
        if(!std::isfinite(v)||!std::isfinite(ahead)||
           v<WorldScalePaceMinSpeedCmps||
           v>result.Nodes[i].LocalLimitCmps+1e-6||
           squared[i]>squared[next]+brake+0.01||
           squared[next]>squared[i]+drive+0.01)
            return {};
        result.Nodes[i].TargetCmps=v;
        seconds+=2.0*result.StepCm/(v+ahead);
    }
    if(!std::isfinite(seconds)||seconds<=1.0||seconds>1800.0)
        return {};
    result.EstimatedLapSeconds=seconds;
    result.Valid=true;
    return result;
}
inline WorldScalePaceReference SampleWorldScaleVariantPace(
    const WorldScaleVariantPacePlan& plan,double station) noexcept
{
    if(!plan.Valid||!std::isfinite(station))return {};
    const double wrapped=WorldScaleVariantWrapDistance(plan.Track,station);
    if(!std::isfinite(wrapped))return {};
    const unsigned i=std::min(
        static_cast<unsigned>(wrapped/plan.StepCm),
        WorldScalePaceStations-1u);
    const unsigned next=(i+1u)%WorldScalePaceStations;
    const double fraction=std::clamp(
        (wrapped-plan.Nodes[i].DistanceCm)/plan.StepCm,0.0,1.0);
    const double a=plan.Nodes[i].TargetCmps;
    const double b=plan.Nodes[next].TargetCmps;
    const double v=std::sqrt(a*a+fraction*(b*b-a*a));
    const auto pose=WorldScaleVariantCenterlineAt(plan.Track,wrapped);
    if(!pose.Valid||!std::isfinite(v)||
       v<WorldScalePaceMinSpeedCmps-1e-6)return {};
    return {true,pose.Pose,wrapped,v};
}
inline WorldScaleRouteProjection ProjectWorldScaleVariantPace(
    const WorldScaleVariantPacePlan& plan,CircuitPoint point) noexcept
{
    if(!plan.Valid||!std::isfinite(point.X)||
       !std::isfinite(point.Y)||
       std::abs(point.X)>1000000.0||
       std::abs(point.Y)>1000000.0)return {};
    double best=std::numeric_limits<double>::infinity();
    unsigned segment=0;
    double fraction=0.0;
    double side=0.0;
    for(unsigned i=0;i<WorldScalePaceStations;++i)
    {
        const auto a=plan.Nodes[i].Position;
        const auto b=plan.Nodes[(i+1u)%WorldScalePaceStations].Position;
        const double dx=b.X-a.X,dy=b.Y-a.Y;
        const double norm=dx*dx+dy*dy;
        if(!std::isfinite(norm)||norm<=0.0)return {};
        const double t=std::clamp(
            ((point.X-a.X)*dx+(point.Y-a.Y)*dy)/norm,0.0,1.0);
        const double ex=point.X-(a.X+t*dx);
        const double ey=point.Y-(a.Y+t*dy);
        const double squared=ex*ex+ey*ey;
        if(!std::isfinite(squared))return {};
        if(squared<best)
        {
            best=squared;segment=i;fraction=t;
            side=(dx*(point.Y-a.Y)-dy*(point.X-a.X))/
                std::sqrt(norm);
        }
    }
    if(!std::isfinite(best)||
       best>WorldScaleProjectionMaxDistanceCm*
            WorldScaleProjectionMaxDistanceCm)return {};
    double station=(segment+fraction)*plan.StepCm;
    if(station>=plan.Track.LapLengthCm)
        station-=plan.Track.LapLengthCm;
    const auto target=SampleWorldScaleVariantPace(plan,station);
    if(!target.Valid||!std::isfinite(side))return {};
    return {true,station,side,std::sqrt(best),
            IsOnWorldScaleVariantRoad(plan.Track,point),
            target.TargetSpeedCmps};
}
} // namespace latebrake
