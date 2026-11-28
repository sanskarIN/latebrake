#pragma once

#include "Racing/DrivingCoachInsights.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// On-screen digest of a *run-local*, possibly interrupted sample archive.
// The engine's circuit gates, race clock, tire loads and records are not
// represented by this output, and the viewer cannot submit an action.
enum class DrivingCoachPaceTrend : std::uint8_t
{
    Unavailable,
    NearlyEqual,
    FasterRecentSamples,
    SlowerRecentSamples
};

inline const char* DrivingCoachPaceTrendLabel(DrivingCoachPaceTrend trend) noexcept
{
    switch(trend)
    {
    case DrivingCoachPaceTrend::NearlyEqual:return "SIMILAR SAMPLE SPEED";
    case DrivingCoachPaceTrend::FasterRecentSamples:return "FASTER RECENT SAMPLES";
    case DrivingCoachPaceTrend::SlowerRecentSamples:return "SLOWER RECENT SAMPLES";
    default:return "SAMPLE TREND UNAVAILABLE";
    }
}

struct DrivingCoachPhaseSummary final
{
    bool Valid=false;
    std::uint64_t FirstSequence=0;
    std::uint64_t LastSequence=0;
    unsigned WindowCount=0;
    double ObservedSeconds=0.0;
    double DistanceKm=0.0;
    double MeanSpeedKmh=0.0;
    double BrakeFraction=0.0;
    double SlipFraction=0.0;
};

struct DrivingCoachReview final
{
    bool Valid=false;
    DrivingCoachEvidence Evidence=DrivingCoachEvidence::Unavailable;
    DrivingCoachMessage Message=DrivingCoachMessage::CollectMoreSamples;
    DrivingCoachPaceTrend Trend=DrivingCoachPaceTrend::Unavailable;
    DrivingCoachPhaseSummary Early{};
    DrivingCoachPhaseSummary Recent{};
    int DominantSpeedBand=-1;
    double DominantSpeedSeconds=0.0;
    double MeanObservedSpeedKmh=0.0;
    double EarlyToRecentSpeedKmh=0.0;
    double EarlyToRecentSpeedFraction=0.0;
    bool HasLongestBrake=false;
    bool HasLongestSlip=false;
    DrivingCoachEpisode LongestBrake{};
    DrivingCoachEpisode LongestSlip{};
    unsigned AvailableCompleteWindows=0;
    std::uint64_t RetainedBrakeEpisodeCount=0;
    std::uint64_t RetainedSlipEpisodeCount=0;
    bool SameContinuityPeriod=false;
    bool AdvisoryOnly=true;
};

inline bool ValidDrivingCoachWindow(DrivingCoachWindow window) noexcept
{
    return window.Valid && window.Sequence>0 &&
        window.ContinuityEpoch>0 && window.Samples>0 &&
        std::isfinite(window.StartedSeconds) &&
        std::isfinite(window.EndedSeconds) &&
        window.StartedSeconds>=0.0 &&
        window.EndedSeconds>window.StartedSeconds &&
        std::isfinite(window.ObservedSeconds) &&
        window.ObservedSeconds>0.0 &&
        window.ObservedSeconds<=DrivingCoachWindowSeconds+1e-7 &&
        window.EndedSeconds-window.StartedSeconds<=
            window.ObservedSeconds+1e-7 &&
        std::isfinite(window.DistanceKm) &&
        window.DistanceKm>=0.0 &&
        window.DistanceKm<=window.ObservedSeconds*0.1+1e-7 &&
        std::isfinite(window.MeanSpeedKmh) &&
        window.MeanSpeedKmh>=0.0 &&
        window.MeanSpeedKmh<=360.0+1e-7 &&
        std::abs(window.DistanceKm*3600.0/window.ObservedSeconds-
            window.MeanSpeedKmh)<=1.0e-6 &&
        std::isfinite(window.BrakingFraction) &&
        window.BrakingFraction>=0.0 && window.BrakingFraction<=1.0 &&
        std::isfinite(window.SlipFraction) &&
        window.SlipFraction>=0.0 && window.SlipFraction<=1.0;
}

inline bool ValidDrivingCoachEpisode(DrivingCoachEpisode episode) noexcept
{
    return episode.Valid && episode.Samples>0 &&
        std::isfinite(episode.StartedSeconds) &&
        std::isfinite(episode.EndedSeconds) &&
        std::isfinite(episode.DurationSeconds) &&
        episode.StartedSeconds>=0.0 &&
        episode.EndedSeconds>episode.StartedSeconds &&
        episode.DurationSeconds>0.0 &&
        std::abs(episode.EndedSeconds-episode.StartedSeconds-
            episode.DurationSeconds)<=1e-5 &&
        std::isfinite(episode.DistanceKm) &&
        episode.DistanceKm>=0.0 &&
        episode.DistanceKm<=episode.DurationSeconds*0.1+1e-7 &&
        std::isfinite(episode.EntrySpeedKmh) &&
        episode.EntrySpeedKmh>=0.0 && episode.EntrySpeedKmh<=360.0 &&
        std::isfinite(episode.ExitSpeedKmh) &&
        episode.ExitSpeedKmh>=0.0 && episode.ExitSpeedKmh<=360.0 &&
        std::isfinite(episode.MinimumSpeedKmh) &&
        episode.MinimumSpeedKmh>=0.0 &&
        episode.MinimumSpeedKmh<=episode.EntrySpeedKmh &&
        episode.MinimumSpeedKmh<=episode.ExitSpeedKmh &&
        std::isfinite(episode.PeakSlipFraction) &&
        episode.PeakSlipFraction>=0.0 &&
        episode.PeakSlipFraction<=10000.0;
}

inline bool ValidDrivingCoachSpeedBand(DrivingCoachSpeedBand band) noexcept
{
    return std::isfinite(band.LowKmh) && std::isfinite(band.HighKmh) &&
        band.LowKmh>=0.0 && band.LowKmh<band.HighKmh &&
        band.HighKmh<=360.0 &&
        std::isfinite(band.Seconds) && band.Seconds>=0.0 &&
        std::isfinite(band.DistanceKm) && band.DistanceKm>=0.0 &&
        std::isfinite(band.BrakingSeconds) && band.BrakingSeconds>=0.0 &&
        std::isfinite(band.HighSlipSeconds) && band.HighSlipSeconds>=0.0 &&
        band.BrakingSeconds<=band.Seconds+1e-8 &&
        band.HighSlipSeconds<=band.Seconds+1e-8 &&
        band.DistanceKm<=band.Seconds*0.1+1e-7 &&
        ((band.Samples==0)==(band.Seconds==0.0));
}

inline bool ValidDrivingCoachReadout(const DrivingCoachReadout& readout) noexcept
{
    if(!readout.Valid || !readout.NoGameplayAuthority ||
        !readout.NoPersistentData || !readout.NoOfficialTiming ||
        readout.AcceptedSamples==0 ||
        readout.RetainedWindows>DrivingCoachWindowCapacity ||
        readout.RetainedBrakeEpisodes>DrivingCoachEpisodeCapacity ||
        readout.RetainedSlipEpisodes>DrivingCoachEpisodeCapacity ||
        !std::isfinite(readout.ObservedSeconds) ||
        readout.ObservedSeconds<=0.0 ||
        readout.ObservedSeconds>DrivingCoachMaximumActiveSeconds ||
        !std::isfinite(readout.DistanceKm) || readout.DistanceKm<0.0 ||
        !std::isfinite(readout.MeanSpeedKmh) ||
        !std::isfinite(readout.PeakSpeedKmh) ||
        readout.MeanSpeedKmh<0.0 ||
        readout.MeanSpeedKmh>readout.PeakSpeedKmh+1e-5 ||
        readout.PeakSpeedKmh>360.0 ||
        !std::isfinite(readout.BrakeFraction) ||
        !std::isfinite(readout.HighSlipFraction) ||
        readout.BrakeFraction<0.0 || readout.BrakeFraction>1.0 ||
        readout.HighSlipFraction<0.0 || readout.HighSlipFraction>1.0 ||
        !std::isfinite(readout.MovingSeconds) ||
        !std::isfinite(readout.StationarySeconds) ||
        !std::isfinite(readout.IntermediateSpeedSeconds) ||
        readout.MovingSeconds<0.0 || readout.StationarySeconds<0.0 ||
        readout.IntermediateSpeedSeconds<0.0 ||
        !std::isfinite(readout.MovingFraction) ||
        readout.MovingFraction<0.0 || readout.MovingFraction>1.0 ||
        std::abs(readout.MovingSeconds+readout.StationarySeconds+
            readout.IntermediateSpeedSeconds-readout.ObservedSeconds)>
            1e-6*std::max(1.0,readout.ObservedSeconds) ||
        std::abs(readout.MovingSeconds/readout.ObservedSeconds-
            readout.MovingFraction)>1e-6 ||
        !std::isfinite(readout.PeakObservedSpeedChangeCmps2) ||
        !std::isfinite(readout.MeanObservedAbsoluteSpeedChangeCmps2) ||
        readout.PeakObservedSpeedChangeCmps2<0.0 ||
        readout.MeanObservedAbsoluteSpeedChangeCmps2<0.0 ||
        readout.MeanObservedAbsoluteSpeedChangeCmps2>
            readout.PeakObservedSpeedChangeCmps2+1e-6 ||
        readout.AcceptedBrakeEpisodes<readout.CompletedBrakeEpisodes ||
        readout.AcceptedSlipEpisodes<readout.CompletedSlipEpisodes ||
        readout.AcceptedBrakeEpisodes>readout.AcceptedSamples ||
        readout.AcceptedSlipEpisodes>readout.AcceptedSamples ||
        readout.RetainedBrakeEpisodes>readout.AcceptedBrakeEpisodes ||
        readout.RetainedSlipEpisodes>readout.AcceptedSlipEpisodes)
        return false;
    double totalSeconds=0.0,totalDistance=0.0;
    std::uint64_t totalSamples=0;
    for(const auto& band:readout.SpeedBands)
    {
        if(!ValidDrivingCoachSpeedBand(band))return false;
        totalSeconds+=band.Seconds;
        totalDistance+=band.DistanceKm;
        if(band.Samples>readout.AcceptedSamples-totalSamples)return false;
        totalSamples+=band.Samples;
    }
    if(totalSamples!=readout.AcceptedSamples ||
        std::abs(totalSeconds-readout.ObservedSeconds)>
            1e-6*std::max(1.0,readout.ObservedSeconds) ||
        std::abs(totalDistance-readout.DistanceKm)>
            1e-6*std::max(1.0,readout.DistanceKm))
        return false;
    std::uint64_t priorSequence=std::numeric_limits<std::uint64_t>::max();
    for(std::size_t i=0;i<readout.RetainedWindows;++i)
    {
        const auto& window=readout.Windows[i];
        if(!ValidDrivingCoachWindow(window) || window.Sequence>=priorSequence)
            return false;
        priorSequence=window.Sequence;
        if(i>0 && window.EndedSeconds>
            readout.Windows[i-1].StartedSeconds+1e-6)
            return false;
    }
    for(std::size_t i=0;i<readout.RetainedBrakeEpisodes;++i)
    {
        const auto& entry=readout.Brakes[i];
        if(!ValidDrivingCoachEpisode(entry) ||
            (i>0 && entry.EndedSeconds>
                readout.Brakes[i-1].StartedSeconds+1e-6))return false;
    }
    for(std::size_t i=0;i<readout.RetainedSlipEpisodes;++i)
    {
        const auto& entry=readout.Slips[i];
        if(!ValidDrivingCoachEpisode(entry) ||
            (i>0 && entry.EndedSeconds>
                readout.Slips[i-1].StartedSeconds+1e-6))return false;
    }
    return true;
}

inline DrivingCoachPhaseSummary SummarizeDrivingCoachPhase(
    const DrivingCoachReadout& readout,std::size_t start,unsigned count) noexcept
{
    DrivingCoachPhaseSummary result;
    if(!ValidDrivingCoachReadout(readout) || !count ||
        start>=readout.RetainedWindows ||
        count>readout.RetainedWindows-start)return result;
    double brake=0.0,slip=0.0;
    for(unsigned i=0;i<count;++i)
    {
        const auto& window=readout.Windows[start+i];
        if(!window.Complete)return {};
        if(i==0)result.LastSequence=window.Sequence;
        if(i+1==count)result.FirstSequence=window.Sequence;
        result.ObservedSeconds+=window.ObservedSeconds;
        result.DistanceKm+=window.DistanceKm;
        brake+=window.ObservedSeconds*window.BrakingFraction;
        slip+=window.ObservedSeconds*window.SlipFraction;
    }
    if(result.ObservedSeconds<=0.0)return {};
    result.WindowCount=count;
    result.MeanSpeedKmh=result.DistanceKm*3600.0/result.ObservedSeconds;
    result.BrakeFraction=std::clamp(brake/result.ObservedSeconds,0.0,1.0);
    result.SlipFraction=std::clamp(slip/result.ObservedSeconds,0.0,1.0);
    result.Valid=true;
    return result;
}

inline DrivingCoachReview BuildDrivingCoachReview(
    const DrivingCoachReadout& observation) noexcept
{
    DrivingCoachReview out;
    if(!ValidDrivingCoachReadout(observation))return out;
    out.Valid=true;
    out.Evidence=observation.Evidence;
    out.Message=observation.Message;
    out.MeanObservedSpeedKmh=observation.MeanSpeedKmh;
    out.RetainedBrakeEpisodeCount=observation.RetainedBrakeEpisodes;
    out.RetainedSlipEpisodeCount=observation.RetainedSlipEpisodes;
    for(std::size_t i=0;i<observation.SpeedBands.size();++i)
        if(observation.SpeedBands[i].Seconds>out.DominantSpeedSeconds)
        {
            out.DominantSpeedBand=static_cast<int>(i);
            out.DominantSpeedSeconds=observation.SpeedBands[i].Seconds;
        }
    for(std::size_t i=0;i<observation.RetainedBrakeEpisodes;++i)
        if(observation.Brakes[i].DurationSeconds>out.LongestBrake.DurationSeconds)
        {
            out.LongestBrake=observation.Brakes[i];
            out.HasLongestBrake=true;
        }
    for(std::size_t i=0;i<observation.RetainedSlipEpisodes;++i)
        if(observation.Slips[i].DurationSeconds>out.LongestSlip.DurationSeconds)
        {
            out.LongestSlip=observation.Slips[i];
            out.HasLongestSlip=true;
        }
    // Windows are stored newest first. Noncomplete windows do not count
    // toward a trend, and a pause can create a gap in sequence numbers.
    std::array<std::size_t,DrivingCoachWindowCapacity> complete{};
    for(std::size_t i=0;i<observation.RetainedWindows;++i)
        if(observation.Windows[i].Complete)
            complete[out.AvailableCompleteWindows++]=i;
    if(out.AvailableCompleteWindows<4)return out;
    const unsigned perPhase=std::min(3u,out.AvailableCompleteWindows/2);
    // Require consecutive windows within each phase, not a collection of
    // unrelated fragments treated as one uninterrupted source interval.
    const auto consecutive=[&](unsigned first,unsigned count)
    {
        for(unsigned i=1;i<count;++i)
            if(observation.Windows[complete[first+i-1]].Sequence!=
                observation.Windows[complete[first+i]].Sequence+1)
                return false;
        return true;
    };
    const unsigned older=out.AvailableCompleteWindows-perPhase;
    if(!consecutive(0,perPhase) || !consecutive(older,perPhase))
        return out;
    // Read through the indices rather than a fabricated contiguous array.
    auto aggregate=[&](unsigned first)
    {
        DrivingCoachPhaseSummary phase;
        double braking=0.0,slip=0.0;
        for(unsigned i=0;i<perPhase;++i)
        {
            const auto& w=observation.Windows[complete[first+i]];
            if(i==0)phase.LastSequence=w.Sequence;
            if(i+1==perPhase)phase.FirstSequence=w.Sequence;
            phase.ObservedSeconds+=w.ObservedSeconds;
            phase.DistanceKm+=w.DistanceKm;
            braking+=w.BrakingFraction*w.ObservedSeconds;
            slip+=w.SlipFraction*w.ObservedSeconds;
        }
        phase.WindowCount=perPhase;
        phase.MeanSpeedKmh=phase.DistanceKm*3600.0/phase.ObservedSeconds;
        phase.BrakeFraction=braking/phase.ObservedSeconds;
        phase.SlipFraction=slip/phase.ObservedSeconds;
        phase.Valid=true;
        return phase;
    };
    out.Recent=aggregate(0);
    out.Early=aggregate(older);
    out.SameContinuityPeriod=true;
    const auto epoch=observation.Windows[complete[0]].ContinuityEpoch;
    for(unsigned i=0;i<perPhase;++i)
        if(observation.Windows[complete[i]].ContinuityEpoch!=epoch ||
            observation.Windows[complete[older+i]].ContinuityEpoch!=epoch)
            out.SameContinuityPeriod=false;
    // Comparing interrupted periods as though they formed one sampled
    // stint would be misleading. Retain both descriptive phase summaries.
    if(!out.SameContinuityPeriod)return out;
    out.EarlyToRecentSpeedKmh=out.Recent.MeanSpeedKmh-out.Early.MeanSpeedKmh;
    if(out.Early.MeanSpeedKmh>1.0)
        out.EarlyToRecentSpeedFraction=
            out.EarlyToRecentSpeedKmh/out.Early.MeanSpeedKmh;
    if(std::abs(out.EarlyToRecentSpeedKmh)<=1.0)
        out.Trend=DrivingCoachPaceTrend::NearlyEqual;
    else
        out.Trend=out.EarlyToRecentSpeedKmh>0.0 ?
            DrivingCoachPaceTrend::FasterRecentSamples :
            DrivingCoachPaceTrend::SlowerRecentSamples;
    return out;
}
} // namespace latebrake
