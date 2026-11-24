#pragma once
#include "Track/WorldScaleVariantRacePace.h"

namespace latebrake
{
// Read-only estimate derived from geometric pace, never official timing.
inline constexpr unsigned VariantReferenceSectorCount=4u;
struct VariantReferenceInterval final
{
    bool Valid=false;
    double StartStationCm=0.0;
    double DistanceCm=0.0;
    double EstimatedSeconds=0.0;
    double MeanSpeedCmps=0.0;
    double EntrySpeedCmps=0.0;
    double ExitSpeedCmps=0.0;
    bool OfficialRaceTime=false;
};
inline double VariantReferenceSegmentSeconds(
    double distance,double entry,double exit) noexcept
{
    if(!std::isfinite(distance)||distance<0.0||
       !std::isfinite(entry)||!std::isfinite(exit)||
       entry<WorldScalePaceMinSpeedCmps-1e-6||
       exit<WorldScalePaceMinSpeedCmps-1e-6||
       entry>WorldScalePaceMaxSpeedCmps+1e-6||
       exit>WorldScalePaceMaxSpeedCmps+1e-6)return -1.0;
    // Exact integral for linear squared-speed interpolation.
    return 2.0*distance/(entry+exit);
}
inline VariantReferenceInterval EstimateVariantReferenceInterval(
    const WorldScaleVariantPacePlan& plan,
    double startStationCm,double distanceCm) noexcept
{
    if(!ValidWorldScaleVariantPaceMetadata(plan)||
       !std::isfinite(startStationCm)||std::abs(startStationCm)>1e9||
       !std::isfinite(distanceCm)||distanceCm<=0.0||
       distanceCm>plan.Track.LapLengthCm)return {};
    const double start=WorldScaleVariantWrapDistance(plan.Track,startStationCm);
    const auto entry=SampleWorldScaleVariantPace(plan,start);
    if(!entry.Valid)return {};
    const double begin=start/plan.StepCm;
    const double end=begin+distanceCm/plan.StepCm;
    if(!std::isfinite(begin)||!std::isfinite(end)||
       begin<0.0||begin>=WorldScalePaceStations||end<=begin||
       end>2.0*WorldScalePaceStations)return {};
    const unsigned first=static_cast<unsigned>(begin);
    double total=0.0,covered=0.0;
    for(unsigned offset=0u;offset<=WorldScalePaceStations;++offset)
    {
        const unsigned unwrapped=first+offset;
        const double lo=std::max(begin,static_cast<double>(unwrapped));
        const double hi=std::min(end,static_cast<double>(unwrapped+1u));
        if(hi<=lo)break;
        const unsigned i=unwrapped%WorldScalePaceStations;
        const unsigned next=(i+1u)%WorldScalePaceStations;
        if(!ValidWorldScaleVariantPaceEdge(plan,i))return {};
        const double a=plan.Nodes[i].TargetCmps;
        const double b=plan.Nodes[next].TargetCmps;
        const double delta=b*b-a*a;
        const double v0=std::sqrt(a*a+(lo-unwrapped)*delta);
        const double v1=std::sqrt(a*a+(hi-unwrapped)*delta);
        const double length=(hi-lo)*plan.StepCm;
        const double time=VariantReferenceSegmentSeconds(length,v0,v1);
        if(!std::isfinite(time)||time<=0.0)return {};
        covered+=length;
        total+=time;
        if(hi==end)break;
    }
    if(!std::isfinite(total)||total<=0.0||
       std::abs(covered-distanceCm)>std::max(1e-7,distanceCm*1e-10))
        return {};
    const auto exit=SampleWorldScaleVariantPace(plan,start+distanceCm);
    const double mean=distanceCm/total;
    if(!exit.Valid||!std::isfinite(mean)||
       mean<WorldScalePaceMinSpeedCmps-1e-6||
       mean>WorldScalePaceMaxSpeedCmps+1e-6)return {};
    return {true,start,distanceCm,total,mean,
        entry.TargetSpeedCmps,exit.TargetSpeedCmps,false};
}
struct VariantReferenceSectorTiming final
{
    bool Valid=false;
    WorldScaleVariantId Track=WorldScaleVariantId::OriginalLoop;
    std::array<VariantReferenceInterval,VariantReferenceSectorCount> Sectors{};
    std::array<double,VariantReferenceSectorCount> CumulativeSeconds{};
    double EstimatedLapSeconds=0.0;
    unsigned LongestSector=0u;
    bool AdvisoryOnly=true;
};
inline VariantReferenceSectorTiming BuildVariantReferenceSectorTiming(
    const WorldScaleVariantPacePlan& plan) noexcept
{
    if(!ValidWorldScaleVariantPaceMetadata(plan))return {};
    VariantReferenceSectorTiming result{};
    result.Track=plan.Track.Id;
    const double span=plan.Track.LapLengthCm/VariantReferenceSectorCount;
    double total=0.0;
    for(unsigned i=0;i<VariantReferenceSectorCount;++i)
    {
        const auto interval=EstimateVariantReferenceInterval(plan,i*span,span);
        if(!interval.Valid)return {};
        result.Sectors[i]=interval;
        total+=interval.EstimatedSeconds;
        result.CumulativeSeconds[i]=total;
        if(interval.EstimatedSeconds>
           result.Sectors[result.LongestSector].EstimatedSeconds)
            result.LongestSector=i;
    }
    if(!std::isfinite(total)||std::abs(total-plan.EstimatedLapSeconds)>
       std::max(1e-8,plan.EstimatedLapSeconds*1e-10))return {};
    result.EstimatedLapSeconds=total;
    result.Valid=true;
    return result;
}
} // namespace latebrake
