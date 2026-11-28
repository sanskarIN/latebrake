#pragma once

#include "Progression/RecordValidation.h"
#include "Racing/RaceProgress.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Read-only observations of actual comparable practice finishes. Active
// seconds and fee-free lap/finish evidence are separate from penalized
// total time. Nothing here grants rewards or changes a race clock.
inline constexpr std::size_t PracticeRecentAttemptCapacity=8;

struct PracticeAttemptSample final
{
    bool Valid=false;
    double FinishSeconds=0;
    double FeeSeconds=0;
    double BestLapSeconds=0;
    double BestFeeFreeLapSeconds=0;
    int CompletedLaps=0;
    int FeeFreeLaps=0;

    double ActiveSeconds() const noexcept {return FinishSeconds-FeeSeconds;}
    bool FeeFreeFinish() const noexcept {return Valid && FeeSeconds==0;}
};

inline bool ValidPracticeAttemptSample(const PracticeAttemptSample& sample) noexcept
{
    if(!sample.Valid || !IsValidFinishedRaceTime(sample.FinishSeconds) ||
        !IsValidFinishedRaceTime(sample.BestLapSeconds) ||
        !std::isfinite(sample.FeeSeconds) || sample.FeeSeconds<0 ||
        sample.FeeSeconds>=sample.FinishSeconds ||
        sample.BestLapSeconds>sample.FinishSeconds ||
        sample.CompletedLaps<1 || sample.CompletedLaps>10 ||
        sample.FeeFreeLaps<0 || sample.FeeFreeLaps>sample.CompletedLaps)
        return false;
    if((sample.FeeSeconds==0)!=(sample.FeeFreeLaps==sample.CompletedLaps))return false;
    if(sample.FeeFreeLaps==0)return sample.BestFeeFreeLapSeconds==0;
    return IsValidFinishedRaceTime(sample.BestFeeFreeLapSeconds) &&
        sample.BestFeeFreeLapSeconds>=sample.BestLapSeconds &&
        sample.BestFeeFreeLapSeconds<=sample.FinishSeconds;
}

inline PracticeAttemptSample MakePracticeAttemptSample(const RaceProgress& race) noexcept
{
    if(!race.IsFinished() || race.GetCompletedLaps()!=race.GetRequiredLaps() ||
        race.GetTimedLapCount()!=race.GetCompletedLaps() ||
        race.GetActiveTimedLapCount()!=race.GetCompletedLaps())return {};
    const auto full=race.GetWholeRaceLapSummary();
    if(!full.Valid || full.All.Count!=race.GetCompletedLaps())return {};
    PracticeAttemptSample sample{true,race.GetElapsedSeconds(),race.GetTotalPenaltySeconds(),
        race.GetBestLapSeconds(),full.FeeFree.FastestSeconds,full.All.Count,full.FeeFree.Count};
    if(!ValidPracticeAttemptSample(sample) ||
        !whole_lap_detail::Reconciles(full.All.TotalSeconds,sample.FinishSeconds) ||
        !whole_lap_detail::Reconciles(full.TotalPenaltySeconds,sample.FeeSeconds) ||
        !whole_lap_detail::Reconciles(full.All.FastestSeconds,sample.BestLapSeconds))return {};
    return sample;
}

struct PracticeAttemptMoments final
{
    std::uint32_t Count=0;
    double Mean=0;
    double PopulationDeviation=0;
    double Minimum=0;
    double Maximum=0;
};

namespace practice_attempt_detail
{
class Moments final
{
public:
    bool Add(double value) noexcept
    {
        if(!std::isfinite(value) || value<0 ||
            Count==std::numeric_limits<std::uint32_t>::max())return false;
        if(Count==0)Minimum=Maximum=value;
        else
        {
            Minimum=std::min(Minimum,value);
            Maximum=std::max(Maximum,value);
        }
        ++Count;
        const double delta=value-Mean;
        Mean+=delta/static_cast<double>(Count);
        M2+=delta*(value-Mean);
        return true;
    }

    PracticeAttemptMoments Read() const noexcept
    {
        return {Count,Mean,Count?std::sqrt(std::max(0.0,M2)/static_cast<double>(Count)):0,
            Minimum,Maximum};
    }

private:
    std::uint32_t Count=0;
    double Mean=0,M2=0,Minimum=0,Maximum=0;
};
}

struct PracticeRecentAttempt final
{
    std::uint32_t Number=0;
    PracticeAttemptSample Sample{};
};

struct PracticeAttemptSeriesReview final
{
    bool Valid=false;
    std::uint32_t Finishes=0;
    std::uint32_t FeeFreeFinishes=0;
    std::uint64_t CompletedLaps=0;
    std::uint64_t FeeFreeLaps=0;
    PracticeAttemptMoments Finish{};
    PracticeAttemptMoments Active{};
    PracticeAttemptMoments Fees{};
    PracticeAttemptMoments BestLap{};
    PracticeAttemptMoments FeeFreeFinish{};
    double BestFeeFreeLap=0;
    std::uint32_t BestFinishAttempt=0;
    std::uint32_t BestFeeFreeFinishAttempt=0;
    std::uint32_t BestFeeFreeLapAttempt=0;
    int RecentCount=0;
    std::array<PracticeRecentAttempt,PracticeRecentAttemptCapacity> Recent{};

    double FeeFreeFinishPercent() const noexcept
    {
        return Finishes?100.0*FeeFreeFinishes/static_cast<double>(Finishes):0;
    }

    double FeeFreeLapPercent() const noexcept
    {
        return CompletedLaps?100.0*static_cast<double>(FeeFreeLaps)/static_cast<double>(CompletedLaps):0;
    }
};

class PracticeAttemptSeries final
{
public:
    void Reset() noexcept {*this={};}

    // The caller binds this series to one exact practice configuration.
    // Every sample is fully validated before any observation is committed.
    bool Observe(PracticeAttemptSample sample) noexcept
    {
        if(!ValidPracticeAttemptSample(sample) ||
            Finish.Read().Count==std::numeric_limits<std::uint32_t>::max() ||
            (LapsPerAttempt && LapsPerAttempt!=sample.CompletedLaps))
            return false;
        if(!LapsPerAttempt)LapsPerAttempt=sample.CompletedLaps;
        const auto prior=Finish.Read();
        const auto priorClean=FeeFreeFinish.Read();
        Finish.Add(sample.FinishSeconds);
        Active.Add(sample.ActiveSeconds());
        Fees.Add(sample.FeeSeconds);
        BestLap.Add(sample.BestLapSeconds);
        const auto number=prior.Count+1u;
        if(prior.Count==0 || sample.FinishSeconds<prior.Minimum)BestFinishAttempt=number;
        if(sample.FeeFreeFinish())
        {
            FeeFreeFinish.Add(sample.FinishSeconds);
            if(priorClean.Count==0 || sample.FinishSeconds<priorClean.Minimum)
                BestFeeFreeFinishAttempt=number;
        }
        if(sample.BestFeeFreeLapSeconds>0 &&
            (BestFeeFreeLap==0 || sample.BestFeeFreeLapSeconds<BestFeeFreeLap))
        {
            BestFeeFreeLap=sample.BestFeeFreeLapSeconds;
            BestFeeFreeLapAttempt=number;
        }
        CompletedLaps+=static_cast<std::uint64_t>(sample.CompletedLaps);
        FeeFreeLaps+=static_cast<std::uint64_t>(sample.FeeFreeLaps);
        Ring[Next]={number,sample};
        Next=(Next+1u)%PracticeRecentAttemptCapacity;
        Retained=std::min(Retained+1u,PracticeRecentAttemptCapacity);
        return true;
    }

    PracticeAttemptSeriesReview Read() const noexcept
    {
        const auto total=Finish.Read();
        if(total.Count==0)return {};
        PracticeAttemptSeriesReview out;
        out.Valid=true;
        out.Finishes=total.Count;
        out.FeeFreeFinishes=FeeFreeFinish.Read().Count;
        out.CompletedLaps=CompletedLaps;
        out.FeeFreeLaps=FeeFreeLaps;
        out.Finish=total;
        out.Active=Active.Read();
        out.Fees=Fees.Read();
        out.BestLap=BestLap.Read();
        out.FeeFreeFinish=FeeFreeFinish.Read();
        out.BestFeeFreeLap=BestFeeFreeLap;
        out.BestFinishAttempt=BestFinishAttempt;
        out.BestFeeFreeFinishAttempt=BestFeeFreeFinishAttempt;
        out.BestFeeFreeLapAttempt=BestFeeFreeLapAttempt;
        out.RecentCount=static_cast<int>(Retained);
        for(std::size_t i=0;i<Retained;++i)
            out.Recent[i]=Ring[(Next+PracticeRecentAttemptCapacity-1u-i)%PracticeRecentAttemptCapacity];
        return out;
    }

private:
    int LapsPerAttempt=0;
    std::uint64_t CompletedLaps=0,FeeFreeLaps=0;
    practice_attempt_detail::Moments Finish,Active,Fees,BestLap,FeeFreeFinish;
    double BestFeeFreeLap=0;
    std::uint32_t BestFinishAttempt=0,BestFeeFreeFinishAttempt=0,BestFeeFreeLapAttempt=0;
    std::array<PracticeRecentAttempt,PracticeRecentAttemptCapacity> Ring{};
    std::size_t Next=0,Retained=0;
};
} // namespace latebrake
