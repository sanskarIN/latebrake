#pragma once

#include "Racing/PracticeSetupCatalogue.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace latebrake
{
// This is a test coverage planner, NOT a car-performance predictor.
// It chooses an observable next configuration without inventing times.
// Each group is one fixed practice event and one handling mode.
inline constexpr unsigned PracticeRecommendedRepeats=3;

enum class PracticeExperimentReason : std::uint8_t
{
    None, Untested, NoFeeFreeFinish, NeedsRepeats
};

struct PracticeExperimentCandidate final
{
    bool Valid=false;
    PracticeConfiguration Key{};
    PracticeExperimentReason Reason=PracticeExperimentReason::None;
    std::uint32_t Finishes=0;
    std::uint32_t FeeFreeFinishes=0;
    int ChangedAxes=0;
    int TuningDistance=0;
    bool Selected=false;
};

inline bool ValidPracticeExperimentReason(PracticeExperimentReason reason) noexcept
{
    return reason==PracticeExperimentReason::Untested ||
        reason==PracticeExperimentReason::NoFeeFreeFinish ||
        reason==PracticeExperimentReason::NeedsRepeats;
}

inline int PracticeExperimentAxisDistance(GarageTuningKey a,GarageTuningKey b) noexcept
{
    return static_cast<int>(a.Selected!=b.Selected)+
        static_cast<int>(a.Engine!=b.Engine)+static_cast<int>(a.Grip!=b.Grip);
}

inline int PracticeExperimentTuningDistance(GarageTuningKey a,GarageTuningKey b) noexcept
{
    return static_cast<int>(a.Selected!=b.Selected)+std::abs(a.Engine-b.Engine)+
        std::abs(a.Grip-b.Grip);
}

inline PracticeExperimentCandidate DescribePracticeExperiment(
    const PracticeSetupCatalogue& catalogue,PracticeConfiguration selected,
    PracticeConfiguration candidate) noexcept
{
    if(!catalogue.Valid || PracticeConfigurationIndex(selected)<0 ||
        PracticeConfigurationIndex(candidate)<0 ||
        selected.Preset!=catalogue.Preset || selected.Handling!=catalogue.Handling ||
        candidate.Preset!=selected.Preset || candidate.Handling!=selected.Handling)
        return {};
    const auto row=catalogue.At(candidate.Tuning.Selected,candidate.Tuning.Engine,candidate.Tuning.Grip);
    if(row.Valid && !(row.Key==candidate))return {};
    PracticeExperimentCandidate out;
    out.Key=candidate;
    out.Finishes=row.Finishes;
    out.FeeFreeFinishes=row.FeeFreeFinishes;
    out.ChangedAxes=PracticeExperimentAxisDistance(selected.Tuning,candidate.Tuning);
    out.TuningDistance=PracticeExperimentTuningDistance(selected.Tuning,candidate.Tuning);
    out.Selected=selected==candidate;
    if(!row.Valid)out.Reason=PracticeExperimentReason::Untested;
    else if(!row.FeeFreeFinishes)out.Reason=PracticeExperimentReason::NoFeeFreeFinish;
    else if(row.Finishes<PracticeRecommendedRepeats)out.Reason=PracticeExperimentReason::NeedsRepeats;
    else return {};
    out.Valid=true;
    return out;
}

inline int PracticeExperimentPriority(PracticeExperimentReason reason) noexcept
{
    switch(reason)
    {
    case PracticeExperimentReason::Untested:return 0;
    case PracticeExperimentReason::NoFeeFreeFinish:return 1;
    case PracticeExperimentReason::NeedsRepeats:return 2;
    default:return 3;
    }
}

inline bool BetterPracticeExperiment(const PracticeExperimentCandidate& a,
    const PracticeExperimentCandidate& b) noexcept
{
    if(!a.Valid || !ValidPracticeExperimentReason(a.Reason))return false;
    if(!b.Valid)return true;
    const auto ap=PracticeExperimentPriority(a.Reason),bp=PracticeExperimentPriority(b.Reason);
    if(ap!=bp)return ap<bp;
    // Prefer a controlled one-axis setup change over a multi-axis change
    // when coverage priority is equal. This is not a causal guarantee.
    if(a.ChangedAxes!=b.ChangedAxes)return a.ChangedAxes<b.ChangedAxes;
    if(a.TuningDistance!=b.TuningDistance)return a.TuningDistance<b.TuningDistance;
    if(a.Finishes!=b.Finishes)return a.Finishes<b.Finishes;
    return PracticeConfigurationIndex(a.Key)<PracticeConfigurationIndex(b.Key);
}

struct PracticeExperimentPlan final
{
    bool Valid=false;
    PracticeConfiguration Selected{};
    int Untested=0;
    int NoFeeFreeFinish=0;
    int NeedsRepeats=0;
    int Sufficient=0;
    std::uint64_t ObservedFinishes=0;
    PracticeExperimentCandidate Current{};
    PracticeExperimentCandidate NextAlternative{};
    PracticeExperimentCandidate NextVehicleOnly{};
    PracticeExperimentCandidate NextEngineOnly{};
    PracticeExperimentCandidate NextGripOnly{};
    PracticeSetupEntry FastestObserved{};
    PracticeSetupEntry FastestFeeFree{};
    int Count=0;
    std::array<PracticeExperimentCandidate,PracticeSetupsPerGroup> Candidates{};

    bool CoverageComplete() const noexcept
    {
        return Valid && Sufficient==PracticeSetupsPerGroup;
    }

    int Remaining() const noexcept
    {
        return Valid?Untested+NoFeeFreeFinish+NeedsRepeats:0;
    }
};

inline PracticeExperimentPlan BuildPracticeExperimentPlan(
    const PracticeSetupCatalogue& catalogue,PracticeConfiguration selected) noexcept
{
    if(!catalogue.Valid || PracticeConfigurationIndex(selected)<0 ||
        catalogue.Preset!=selected.Preset || catalogue.Handling!=selected.Handling)
        return {};
    PracticeExperimentPlan out;
    out.Valid=true;
    out.Selected=selected;
    out.ObservedFinishes=catalogue.All.Finishes;
    out.FastestObserved=catalogue.All.FastestFinish;
    out.FastestFeeFree=catalogue.All.FastestFeeFreeFinish;
    for(int car=0;car<VehicleCount;++car)
        for(int engine=0;engine<4;++engine)
            for(int grip=0;grip<4;++grip)
            {
                const PracticeConfiguration key{selected.Preset,{car,engine,grip},selected.Handling};
                const auto item=DescribePracticeExperiment(catalogue,selected,key);
                if(!item.Valid)
                {
                    ++out.Sufficient;
                    continue;
                }
                switch(item.Reason)
                {
                case PracticeExperimentReason::Untested:++out.Untested;break;
                case PracticeExperimentReason::NoFeeFreeFinish:++out.NoFeeFreeFinish;break;
                case PracticeExperimentReason::NeedsRepeats:++out.NeedsRepeats;break;
                default:return {};
                }
                int at=out.Count;
                while(at>0 && BetterPracticeExperiment(item,out.Candidates[static_cast<std::size_t>(at-1)]))
                {
                    out.Candidates[static_cast<std::size_t>(at)]=out.Candidates[static_cast<std::size_t>(at-1)];
                    --at;
                }
                out.Candidates[static_cast<std::size_t>(at)]=item;
                ++out.Count;
                if(item.Selected)
                {
                    out.Current=item;
                    continue;
                }
                if(BetterPracticeExperiment(item,out.NextAlternative))out.NextAlternative=item;
                if(DiffersOnlyOnPracticeAxis(selected,item.Key,PracticeSetupAxis::Vehicle) &&
                    BetterPracticeExperiment(item,out.NextVehicleOnly))out.NextVehicleOnly=item;
                if(DiffersOnlyOnPracticeAxis(selected,item.Key,PracticeSetupAxis::Engine) &&
                    BetterPracticeExperiment(item,out.NextEngineOnly))out.NextEngineOnly=item;
                if(DiffersOnlyOnPracticeAxis(selected,item.Key,PracticeSetupAxis::Grip) &&
                    BetterPracticeExperiment(item,out.NextGripOnly))out.NextGripOnly=item;
            }
    if(out.Count!=out.Remaining() ||
        out.Count+out.Sufficient!=PracticeSetupsPerGroup)return {};
    return out;
}

inline const char* PracticeExperimentReasonLabel(PracticeExperimentReason reason) noexcept
{
    switch(reason)
    {
    case PracticeExperimentReason::Untested:return "FIRST FINISH";
    case PracticeExperimentReason::NoFeeFreeFinish:return "FEE-FREE FINISH";
    case PracticeExperimentReason::NeedsRepeats:return "REPEAT FINISH";
    default:return "NO SUGGESTION";
    }
}

struct PracticeExperimentChoices final
{
    bool NextValid=false;
    GarageTuningKey Next{};
    PracticeExperimentReason Reason=PracticeExperimentReason::None;
    bool BestValid=false;
    GarageTuningKey Best{};
    double BestSeconds=0;

    bool operator==(const PracticeExperimentChoices&) const noexcept=default;
};

inline PracticeExperimentChoices GetPracticeExperimentChoices(
    const PracticeExperimentPlan& plan) noexcept
{
    if(!plan.Valid)return {};
    PracticeExperimentChoices out;
    if(plan.NextAlternative.Valid &&
        !(plan.NextAlternative.Key.Tuning==plan.Selected.Tuning))
    {
        out.NextValid=true;
        out.Next=plan.NextAlternative.Key.Tuning;
        out.Reason=plan.NextAlternative.Reason;
    }
    // Retest a known complete fee-free setup first. If none exists, the
    // penalty-inclusive best is still a measured finish and labeled so.
    const auto& best=plan.FastestFeeFree.Valid?plan.FastestFeeFree:plan.FastestObserved;
    if(best.Valid && !(best.Key.Tuning==plan.Selected.Tuning))
    {
        out.BestValid=true;
        out.Best=best.Key.Tuning;
        out.BestSeconds=plan.FastestFeeFree.Valid?best.BestFeeFreeFinish:best.BestFinish;
    }
    return out;
}
} // namespace latebrake
