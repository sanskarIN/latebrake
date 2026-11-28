#pragma once

#include "Garage/GaragePurchasePlanner.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake
{
// Read-only shopping advice for the fictional six-car garage. Existing
// purchase paths remain authoritative for sequential unlocks, shared
// tuning, prices and equip eligibility. No actual spending is performed.
enum class GarageBudgetFocus : std::uint8_t
{
    Balanced,
    Acceleration,
    Grip,
    Speed,
    Count
};

inline constexpr std::size_t GarageBudgetFocusCount=
    static_cast<std::size_t>(GarageBudgetFocus::Count);

inline GaragePlanPreferences GarageBudgetPreferences(GarageBudgetFocus focus,
    int reserve=0) noexcept
{
    switch(focus)
    {
        case GarageBudgetFocus::Balanced: return {1.0,1.0,1.0,reserve,true};
        case GarageBudgetFocus::Acceleration: return {1.0,0.0,0.0,reserve,true};
        case GarageBudgetFocus::Grip: return {0.0,1.0,0.0,reserve,true};
        case GarageBudgetFocus::Speed: return {0.0,0.0,1.0,reserve,true};
        default: return {0.0,0.0,0.0,-1,false};
    }
}

inline const char* GarageBudgetFocusName(GarageBudgetFocus focus) noexcept
{
    switch(focus)
    {
        case GarageBudgetFocus::Balanced: return "BALANCED";
        case GarageBudgetFocus::Acceleration: return "ACCEL";
        case GarageBudgetFocus::Grip: return "GRIP";
        case GarageBudgetFocus::Speed: return "SPEED";
        default: return "UNAVAILABLE";
    }
}

inline bool SameGarageBudgetSnapshot(const GarageSnapshot& a,
    const GarageSnapshot& b) noexcept
{
    return a.Credits==b.Credits && a.OwnedVehicleMask==b.OwnedVehicleMask &&
        a.SelectedVehicle==b.SelectedVehicle && a.EngineLevel==b.EngineLevel &&
        a.GripLevel==b.GripLevel;
}

inline constexpr double GarageBudgetScoreEpsilon=1e-9;

struct GarageBudgetChoice final
{
    bool Valid=false;
    GaragePlanTarget Target{};
    VehiclePerformance Projected{};
    double Score=0.0;
    int Cost=0;
    std::int64_t CreditsShort=0;
    int AdditionalCars=0;
    unsigned ActionCount=0;
    bool HasFirstAction=false;
    GaragePlanAction FirstAction{};
};

inline GarageBudgetChoice SummarizeGarageBudgetChoice(const GaragePurchasePlan& plan,
    double score) noexcept
{
    if(plan.Status==GaragePlanStatus::Invalid || !plan.ReadOnly ||
        !std::isfinite(score) || score<=0.0 || plan.TotalCost<0 ||
        plan.CreditsShort<0 || plan.ActionCount>plan.Actions.size())
        return {};
    GarageBudgetChoice out;
    out.Valid=true;
    out.Target=plan.Target;
    out.Projected=plan.Projected;
    out.Score=score;
    out.Cost=plan.TotalCost;
    out.CreditsShort=plan.CreditsShort;
    out.AdditionalCars=plan.AdditionalCarsUnlocked;
    out.ActionCount=plan.ActionCount;
    if(plan.ActionCount>0)
    {
        out.HasFirstAction=true;
        out.FirstAction=plan.Actions[0];
    }
    return out;
}

// Use an exact, deterministic total ordering. The epsilon below is used
// only to decide whether a score is materially higher than a baseline.
inline bool BetterGarageBudgetChoice(const GarageBudgetChoice& a,
    const GarageBudgetChoice& b) noexcept
{
    if(!a.Valid) return false;
    if(!b.Valid) return true;
    if(a.Score!=b.Score) return a.Score>b.Score;
    if(a.Cost!=b.Cost) return a.Cost<b.Cost;
    if(a.ActionCount!=b.ActionCount) return a.ActionCount<b.ActionCount;
    if(a.Target.VehicleId!=b.Target.VehicleId)
        return a.Target.VehicleId<b.Target.VehicleId;
    if(a.Target.EngineLevel!=b.Target.EngineLevel)
        return a.Target.EngineLevel<b.Target.EngineLevel;
    return a.Target.GripLevel<b.Target.GripLevel;
}

struct GarageBudgetFocusReview final
{
    GarageBudgetFocus Focus=GarageBudgetFocus::Count;
    double EquippedScore=0.0;
    GarageBudgetChoice BestAffordable{};
    GarageBudgetChoice NextImprovement{};
    GarageBudgetChoice BestOverall{};
    GarageBudgetChoice BestViewedAffordable{};
    GarageBudgetChoice BestViewedOverall{};
    unsigned AffordablePlans=0;
    unsigned ViewedAffordablePlans=0;

    bool AffordableImprovesEquipped() const noexcept
    {
        return BestAffordable.Valid &&
            BestAffordable.Score>EquippedScore+GarageBudgetScoreEpsilon;
    }

    double AffordableImprovementPercent() const noexcept
    {
        return BestAffordable.Valid && EquippedScore>0.0
            ? (BestAffordable.Score/EquippedScore-1.0)*100.0 : 0.0;
    }
};

struct GarageBudgetReview final
{
    bool Valid=false;
    GarageSnapshot Baseline{};
    int ViewedVehicle=-1;
    int ReserveCredits=0;
    unsigned EnumeratedPlans=0;
    std::array<GarageBudgetFocusReview,GarageBudgetFocusCount> Focuses{};
};

inline bool PreferGarageSavingsTarget(const GarageBudgetChoice& candidate,
    const GarageBudgetChoice& current) noexcept
{
    if(!candidate.Valid || candidate.CreditsShort<=0) return false;
    if(!current.Valid) return true;
    if(candidate.CreditsShort!=current.CreditsShort)
        return candidate.CreditsShort<current.CreditsShort;
    return BetterGarageBudgetChoice(candidate,current);
}

inline GarageBudgetReview BuildGarageBudgetReview(GarageSnapshot persisted,
    int viewedVehicle,int reserveCredits=0) noexcept
{
    if(!GarageState::IsVehicle(viewedVehicle) || reserveCredits<0) return {};
    GarageState initial;
    initial.Load(persisted);
    GarageBudgetReview out;
    out.Baseline=initial.Snapshot();
    out.ViewedVehicle=viewedVehicle;
    out.ReserveCredits=reserveCredits;
    const auto equipped=initial.Performance();
    for(std::size_t i=0;i<out.Focuses.size();++i)
    {
        auto& focus=out.Focuses[i];
        focus.Focus=static_cast<GarageBudgetFocus>(i);
        focus.EquippedScore=GaragePlanScore(equipped,GarageBudgetPreferences(focus.Focus,reserveCredits));
        if(focus.EquippedScore<=0.0) return {};
    }

    // Enumerate the canonical purchase paths once. They are small and
    // bounded (6 cars x 4 engine levels x 4 grip levels). Keep the compact
    // choices rather than copying the entire action arrays into the HUD.
    std::array<GaragePurchasePlan,GaragePlanMaxAlternatives> plans{};
    for(int id=0;id<VehicleCount;++id)
    for(int engine=out.Baseline.EngineLevel;engine<=static_cast<int>(GaragePlanMaxLevel);++engine)
    for(int grip=out.Baseline.GripLevel;grip<=static_cast<int>(GaragePlanMaxLevel);++grip)
    {
        if(out.EnumeratedPlans>=plans.size()) return {};
        auto& plan=plans[out.EnumeratedPlans++];
        plan=PlanGaragePurchasePath(out.Baseline,{id,engine,grip,reserveCredits});
        if(plan.Status==GaragePlanStatus::Invalid) return {};
        for(auto& focus:out.Focuses)
        {
            const auto choice=SummarizeGarageBudgetChoice(plan,
                GaragePlanScore(plan.Projected,GarageBudgetPreferences(focus.Focus,reserveCredits)));
            if(!choice.Valid) return {};
            if(BetterGarageBudgetChoice(choice,focus.BestOverall)) focus.BestOverall=choice;
            if(id==viewedVehicle && BetterGarageBudgetChoice(choice,focus.BestViewedOverall))
                focus.BestViewedOverall=choice;
            if(plan.Status!=GaragePlanStatus::Affordable) continue;
            ++focus.AffordablePlans;
            if(BetterGarageBudgetChoice(choice,focus.BestAffordable)) focus.BestAffordable=choice;
            if(id==viewedVehicle)
            {
                ++focus.ViewedAffordablePlans;
                if(BetterGarageBudgetChoice(choice,focus.BestViewedAffordable))
                    focus.BestViewedAffordable=choice;
            }
        }
    }
    // The first attainable improvement must beat today's best affordable
    // configuration, not just the currently equipped car. This avoids
    // telling players to save for something they can already outperform.
    for(unsigned i=0;i<out.EnumeratedPlans;++i)
    {
        const auto& plan=plans[i];
        if(plan.Status!=GaragePlanStatus::NeedCredits) continue;
        for(auto& focus:out.Focuses)
        {
            const double threshold=focus.BestAffordable.Valid
                ? std::max(focus.EquippedScore,focus.BestAffordable.Score)
                : focus.EquippedScore;
            const auto choice=SummarizeGarageBudgetChoice(plan,
                GaragePlanScore(plan.Projected,GarageBudgetPreferences(focus.Focus,reserveCredits)));
            if(!choice.Valid) return {};
            if(choice.Score>threshold+GarageBudgetScoreEpsilon &&
                PreferGarageSavingsTarget(choice,focus.NextImprovement))
                focus.NextImprovement=choice;
        }
    }
    out.Valid=out.EnumeratedPlans>0;
    return out;
}

// Per-HUD cache. The snapshot comparison includes credits, selection,
// ownership and both shared upgrade levels; changing the viewed locked
// car also refreshes its advice. A cached recommendation is never an
// authorization token for a purchase or equip request.
class GarageBudgetAdvisor final
{
public:
    const GarageBudgetReview& Read(GarageSnapshot persisted,int viewedVehicle,
        int reserveCredits=0) noexcept
    {
        GarageState initial;
        initial.Load(persisted);
        const auto sanitized=initial.Snapshot();
        if(!Cached.Valid || !SameGarageBudgetSnapshot(Cached.Baseline,sanitized) ||
            Cached.ViewedVehicle!=viewedVehicle || Cached.ReserveCredits!=reserveCredits)
        {
            Cached=BuildGarageBudgetReview(sanitized,viewedVehicle,reserveCredits);
            if(Rebuilds!=std::numeric_limits<std::uint64_t>::max()) ++Rebuilds;
        }
        return Cached;
    }

    void Reset() noexcept {Cached={};Rebuilds=0;}
    std::uint64_t RebuildCount() const noexcept {return Rebuilds;}

private:
    GarageBudgetReview Cached{};
    std::uint64_t Rebuilds=0;
};
} // namespace latebrake
