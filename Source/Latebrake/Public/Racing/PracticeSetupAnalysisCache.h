#pragma once

#include "Racing/PracticeExperimentPlan.h"

#include <cstdint>
#include <limits>

namespace latebrake
{
// Per-HUD-owner read-through cache. Scanning 96 actual session records,
// and sorting the candidates, is necessary only when the evidence or
// selected setup changes. The cache never owns or modifies a race.
struct PracticeAnalysisCacheStats final
{
    std::uint64_t GroupBuilds=0;
    std::uint64_t SelectionChanges=0;
    std::uint64_t PlanBuilds=0;
};

class PracticeSetupAnalysisCache final
{
public:
    void Reset() noexcept
    {
        Source=nullptr;
        Revision=0;
        Selected={};
        Catalogue={};
        Plan={};
        PlanReady=false;
        Stats={};
    }

    const PracticeSetupCatalogue& ReadCatalogue(const PracticeSessionHistory& history,
        PracticeConfiguration selected) noexcept
    {
        Refresh(history,selected);
        return Catalogue;
    }

    const PracticeExperimentPlan& ReadPlan(const PracticeSessionHistory& history,
        PracticeConfiguration selected) noexcept
    {
        Refresh(history,selected);
        if(!Catalogue.Valid)return Plan;
        if(!PlanReady)
        {
            Plan=BuildPracticeExperimentPlan(Catalogue,Selected);
            PlanReady=true;
            Increment(Stats.PlanBuilds);
        }
        return Plan;
    }

    PracticeAnalysisCacheStats ReadStats() const noexcept {return Stats;}

private:
    static void Increment(std::uint64_t& count) noexcept
    {
        if(count!=std::numeric_limits<std::uint64_t>::max())++count;
    }

    void ClearValues() noexcept
    {
        Catalogue={};
        Plan={};
        Selected={};
        PlanReady=false;
        Source=nullptr;
        Revision=0;
    }

    void Refresh(const PracticeSessionHistory& history,PracticeConfiguration selected) noexcept
    {
        if(PracticeConfigurationIndex(selected)<0)
        {
            // An invalid selection must never return the previous valid
            // group. Counters remain available for regression diagnostics.
            ClearValues();
            return;
        }
        const auto revision=history.AnalysisRevision();
        const bool sameGroup=Source==&history && Revision==revision &&
            Catalogue.Valid && Catalogue.Preset==selected.Preset &&
            Catalogue.Handling==selected.Handling;
        if(!sameGroup)
        {
            Catalogue=BuildPracticeSetupCatalogue(history,selected);
            Source=&history;
            Revision=revision;
            Selected=selected;
            Plan={};
            PlanReady=false;
            Increment(Stats.GroupBuilds);
            return;
        }
        if(!(Selected==selected))
        {
            Selected=selected;
            // The 96 entries and group summaries are unchanged. Resolve
            // the newly selected row directly, not another record scan.
            Catalogue.Selected=Catalogue.At(selected.Tuning.Selected,
                selected.Tuning.Engine,selected.Tuning.Grip);
            Plan={};
            PlanReady=false;
            Increment(Stats.SelectionChanges);
        }
    }

    const PracticeSessionHistory* Source=nullptr;
    std::uint64_t Revision=0;
    PracticeConfiguration Selected{};
    PracticeSetupCatalogue Catalogue{};
    PracticeExperimentPlan Plan{};
    bool PlanReady=false;
    PracticeAnalysisCacheStats Stats{};
};
} // namespace latebrake
