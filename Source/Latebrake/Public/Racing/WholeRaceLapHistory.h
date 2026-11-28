#pragma once

#include "Progression/RecordValidation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// Constant-memory whole-attempt timing statistics. The recent-lap ring
// remains unchanged, but its eight retained samples are not the entire
// history of a long practice. These aggregates observe actual lap gates,
// never a predicted lap, and do not influence eligibility or rewards.
struct WholeRaceLapGroup final
{
    int Count=0;
    int FastestLap=0;
    int SlowestLap=0;
    double FastestSeconds=0.0;
    double SlowestSeconds=0.0;
    double MeanSeconds=0.0;
    double PopulationDeviationSeconds=0.0;
    double TotalSeconds=0.0;
};

struct WholeRaceLapSummary final
{
    bool Valid=false;
    WholeRaceLapGroup All{};
    WholeRaceLapGroup FeeFree{};
    WholeRaceLapGroup Penalized{};
    double TotalPenaltySeconds=0.0;
    int LongestFeeFreeStreak=0;
    int CurrentFeeFreeStreak=0;
    int LongestImprovementStreak=0;
    int CurrentImprovementStreak=0;
    bool HasOpeningClosingComparison=false;
    double OpeningLapSeconds=0.0;
    double ClosingLapSeconds=0.0;
    double ClosingVsOpeningSeconds=0.0;
};

namespace whole_lap_detail
{
// Kahan total plus Welford variance: bounded finite recordable times,
// constant memory, numerically stable even for near-identical lap times.
class Moments final
{
public:
    void Add(int lap,double seconds) noexcept
    {
        if(Group.Count==0)
        {
            Group.FastestLap=Group.SlowestLap=lap;
            Group.FastestSeconds=Group.SlowestSeconds=seconds;
        }
        if(seconds<Group.FastestSeconds)
        {
            Group.FastestSeconds=seconds;
            Group.FastestLap=lap;
        }
        if(seconds>Group.SlowestSeconds)
        {
            Group.SlowestSeconds=seconds;
            Group.SlowestLap=lap;
        }
        ++Group.Count;
        const double delta=seconds-Group.MeanSeconds;
        Group.MeanSeconds+=delta/static_cast<double>(Group.Count);
        M2+=delta*(seconds-Group.MeanSeconds);
        const double corrected=seconds-Correction;
        const double next=Group.TotalSeconds+corrected;
        Correction=(next-Group.TotalSeconds)-corrected;
        Group.TotalSeconds=next;
    }

    WholeRaceLapGroup Read() const noexcept
    {
        auto out=Group;
        if(out.Count>0)
            out.PopulationDeviationSeconds=std::sqrt(std::max(0.0,M2)/out.Count);
        return out;
    }

private:
    WholeRaceLapGroup Group{};
    double M2=0.0;
    double Correction=0.0;
};

inline bool Reconciles(double a,double b) noexcept
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a-b)<=std::max(1e-7,1e-12*std::max(std::abs(a),std::abs(b)));
}
} // namespace whole_lap_detail

class WholeRaceLapHistory final
{
public:
    void Reset() noexcept { *this={}; }

    // Call once for every completed physical lap, including malformed
    // and untimed ones. Missing/invalid evidence revokes whole-run
    // completeness, rather than silently reporting a partial total.
    bool Observe(int lap,double seconds,bool activeClockEvidence,
        double penaltySeconds) noexcept
    {
        if(!Complete) return false;
        if(LastLap==std::numeric_limits<int>::max() || lap!=LastLap+1 ||
            !activeClockEvidence || !IsValidFinishedRaceTime(seconds) ||
            !std::isfinite(penaltySeconds) || penaltySeconds<0.0 ||
            penaltySeconds>seconds)
        {
            Complete=false;
            return false;
        }
        const auto before=All.Read();
        if(!IsValidFinishedRaceTime(before.TotalSeconds+seconds))
        {
            Complete=false;
            return false;
        }
        All.Add(lap,seconds);
        if(penaltySeconds==0.0)
        {
            FeeFree.Add(lap,seconds);
            ++FeeFreeStreak;
            LongestFeeFree=std::max(LongestFeeFree,FeeFreeStreak);
        }
        else
        {
            Penalized.Add(lap,seconds);
            FeeFreeStreak=0;
        }
        // Improvement streak is adjacent physical measured laps only.
        // A tie or slower lap breaks the streak, irrespective of fees.
        if(LastLap>0 && seconds<LastSeconds) ++ImprovementStreak;
        else ImprovementStreak=0;
        LongestImprovement=std::max(LongestImprovement,ImprovementStreak);
        if(lap==1) FirstSeconds=seconds;
        LastSeconds=seconds;
        LastLap=lap;
        const double corrected=penaltySeconds-PenaltyCorrection;
        const double next=PenaltyTotal+corrected;
        PenaltyCorrection=(next-PenaltyTotal)-corrected;
        PenaltyTotal=next;
        return true;
    }

    WholeRaceLapSummary Review(int completedLaps,double completedLapSeconds,
        double completedPenaltySeconds) const noexcept
    {
        if(!Complete || LastLap<=0 || completedLaps!=LastLap ||
            !IsValidFinishedRaceTime(completedLapSeconds) ||
            !std::isfinite(completedPenaltySeconds) || completedPenaltySeconds<0.0 ||
            completedPenaltySeconds>completedLapSeconds)
            return {};
        WholeRaceLapSummary out;
        out.All=All.Read();
        out.FeeFree=FeeFree.Read();
        out.Penalized=Penalized.Read();
        if(out.All.Count!=completedLaps ||
            out.FeeFree.Count+out.Penalized.Count!=out.All.Count ||
            !whole_lap_detail::Reconciles(out.All.TotalSeconds,completedLapSeconds) ||
            !whole_lap_detail::Reconciles(out.FeeFree.TotalSeconds+out.Penalized.TotalSeconds,
                out.All.TotalSeconds) ||
            !whole_lap_detail::Reconciles(PenaltyTotal,completedPenaltySeconds))
            return {};
        out.TotalPenaltySeconds=PenaltyTotal;
        out.LongestFeeFreeStreak=LongestFeeFree;
        out.CurrentFeeFreeStreak=FeeFreeStreak;
        out.LongestImprovementStreak=LongestImprovement;
        out.CurrentImprovementStreak=ImprovementStreak;
        out.HasOpeningClosingComparison=LastLap>=2;
        out.OpeningLapSeconds=FirstSeconds;
        out.ClosingLapSeconds=LastSeconds;
        if(out.HasOpeningClosingComparison)
            out.ClosingVsOpeningSeconds=LastSeconds-FirstSeconds;
        out.Valid=true;
        return out;
    }

private:
    bool Complete=true;
    int LastLap=0;
    int FeeFreeStreak=0;
    int LongestFeeFree=0;
    int ImprovementStreak=0;
    int LongestImprovement=0;
    double FirstSeconds=0.0;
    double LastSeconds=0.0;
    double PenaltyTotal=0.0;
    double PenaltyCorrection=0.0;
    whole_lap_detail::Moments All{};
    whole_lap_detail::Moments FeeFree{};
    whole_lap_detail::Moments Penalized{};
};
} // namespace latebrake
