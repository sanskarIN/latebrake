#pragma once

#include "Garage/GarageState.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Garage planning is a read-only preview. Every hypothetical purchase is
// applied only to a local GarageState copy. Actual UI confirmation, session
// restrictions, save transactions and unlocks remain GameMode's authority.
enum class GaragePlanActionKind : std::uint8_t
{
    UnlockNext, UpgradeEngine, UpgradeGrip, SelectOwned
};
enum class GaragePlanStatus : std::uint8_t
{
    Invalid, Affordable, NeedCredits
};
inline constexpr unsigned GaragePlanMaxLevel=3u;
inline constexpr unsigned GaragePlanMaxActions=
    static_cast<unsigned>(VehicleCount-1)+2u*GaragePlanMaxLevel+1u;
inline constexpr unsigned GaragePlanMaxAlternatives=
    static_cast<unsigned>(VehicleCount)*(GaragePlanMaxLevel+1u)*
    (GaragePlanMaxLevel+1u);

struct GaragePlanAction final
{
    GaragePlanActionKind Kind=GaragePlanActionKind::SelectOwned;
    int Target=0; // car ID or resulting upgrade level
    int Cost=0;
    int CumulativeCost=0;
    bool RequiresPurchaseConfirmation=false;
};

struct GaragePlanTarget final
{
    int VehicleId=0;
    int EngineLevel=0;
    int GripLevel=0;
    int ReserveCredits=0;
};

struct GaragePurchasePlan final
{
    GaragePlanStatus Status=GaragePlanStatus::Invalid;
    GaragePlanTarget Target{};
    GarageSnapshot Baseline{};
    VehiclePerformance Projected{};
    std::array<GaragePlanAction,GaragePlanMaxActions> Actions{};
    unsigned ActionCount=0u;
    int UnlockCost=0;
    int EngineCost=0;
    int GripCost=0;
    int TotalCost=0;
    int AdditionalCarsUnlocked=0;
    int ProjectedOwnedMask=0;
    std::int64_t RequiredCredits=0;
    std::int64_t CreditsShort=0;
    int CreditsAfterPurchases=0; // valid only when affordable
    bool ReadOnly=true;
};

inline bool ValidGaragePlanTarget(
    const GarageSnapshot& baseline,GaragePlanTarget target) noexcept
{
    return GarageState::IsVehicle(target.VehicleId) &&
        target.EngineLevel>=baseline.EngineLevel &&
        target.EngineLevel<=static_cast<int>(GaragePlanMaxLevel) &&
        target.GripLevel>=baseline.GripLevel &&
        target.GripLevel<=static_cast<int>(GaragePlanMaxLevel) &&
        target.ReserveCredits>=0;
}

inline GaragePurchasePlan PlanGaragePurchasePath(
    GarageSnapshot persisted,GaragePlanTarget target) noexcept
{
    GarageState initial;
    initial.Load(persisted);
    const auto baseline=initial.Snapshot();
    if(!ValidGaragePlanTarget(baseline,target))return {};

    GaragePurchasePlan result{};
    result.Baseline=baseline;
    result.Target=target;
    // Determine the complete route even if not currently affordable.
    // This credit balance exists in a temporary hypothetical state only.
    auto funded=baseline;
    funded.Credits=std::numeric_limits<int>::max();
    GarageState simulation;
    simulation.Load(funded);

    const auto append=[&](GaragePlanActionKind kind,int id,int cost)
    {
        if(cost<0||result.ActionCount>=result.Actions.size()||
           cost>std::numeric_limits<int>::max()-result.TotalCost)
            return false;
        result.TotalCost+=cost;
        result.Actions[result.ActionCount++]={
            kind,id,cost,result.TotalCost,cost>0};
        return true;
    };

    // Current GameMode exposes Q: unlock next locked vehicle, not an
    // arbitrary locked ID. A plan for Nova must include any earlier
    // still-locked cars. Already-owned targets need no preceding unlocks.
    while(!simulation.Owns(target.VehicleId))
    {
        const int next=simulation.NextLockedVehicle();
        if(!GarageState::IsVehicle(next)||next>target.VehicleId)return {};
        const int cost=GarageState::Definition(next).UnlockCost;
        if(cost<=0||!simulation.Unlock(next)||
           !append(GaragePlanActionKind::UnlockNext,next,cost))return {};
        result.UnlockCost+=cost;
        ++result.AdditionalCarsUnlocked;
    }
    while(simulation.EngineLevel()<target.EngineLevel)
    {
        const int cost=simulation.NextUpgradeCost(UpgradeKind::Engine);
        if(cost<=0||!simulation.Upgrade(UpgradeKind::Engine)||
           !append(GaragePlanActionKind::UpgradeEngine,
                   simulation.EngineLevel(),cost))return {};
        result.EngineCost+=cost;
    }
    while(simulation.GripLevel()<target.GripLevel)
    {
        const int cost=simulation.NextUpgradeCost(UpgradeKind::Grip);
        if(cost<=0||!simulation.Upgrade(UpgradeKind::Grip)||
           !append(GaragePlanActionKind::UpgradeGrip,
                   simulation.GripLevel(),cost))return {};
        result.GripCost+=cost;
    }
    if(simulation.SelectedVehicle()!=target.VehicleId)
    {
        if(!simulation.Select(target.VehicleId)||
           !append(GaragePlanActionKind::SelectOwned,target.VehicleId,0))
            return {};
    }
    if(result.TotalCost!=funded.Credits-simulation.Credits()||
       result.TotalCost!=result.UnlockCost+result.EngineCost+result.GripCost)
        return {};

    result.Projected=simulation.Performance();
    result.ProjectedOwnedMask=simulation.Snapshot().OwnedVehicleMask;
    result.RequiredCredits=static_cast<std::int64_t>(result.TotalCost)+
        target.ReserveCredits;
    result.CreditsShort=std::max<std::int64_t>(
        0,result.RequiredCredits-baseline.Credits);
    result.Status=result.CreditsShort==0?
        GaragePlanStatus::Affordable:GaragePlanStatus::NeedCredits;
    if(result.Status==GaragePlanStatus::Affordable)
        result.CreditsAfterPurchases=baseline.Credits-result.TotalCost;
    return result;
}

struct GaragePlanPreferences final
{
    double AccelerationWeight=1.0;
    double GripWeight=1.0;
    double TopSpeedWeight=1.0;
    int ReserveCredits=0;
    bool IncludeLockedVehicles=true;
};

inline bool ValidGaragePlanPreferences(GaragePlanPreferences p) noexcept
{
    const auto weight=[](double value)
    {
        return std::isfinite(value)&&value>=0.0&&value<=100.0;
    };
    return weight(p.AccelerationWeight)&&weight(p.GripWeight)&&
        weight(p.TopSpeedWeight)&&
        p.AccelerationWeight+p.GripWeight+p.TopSpeedWeight>0.0&&
        p.ReserveCredits>=0;
}

inline double GaragePlanScore(
    VehiclePerformance performance,GaragePlanPreferences p) noexcept
{
    if(!ValidGaragePlanPreferences(p)||
       !std::isfinite(performance.Acceleration)||performance.Acceleration<=0.0||
       !std::isfinite(performance.Grip)||performance.Grip<=0.0||
       !std::isfinite(performance.TopSpeed)||performance.TopSpeed<=0.0)
        return -1.0;
    const double sum=p.AccelerationWeight+p.GripWeight+p.TopSpeedWeight;
    const double result=performance.Acceleration*(p.AccelerationWeight/sum)+
        performance.Grip*(p.GripWeight/sum)+
        performance.TopSpeed*(p.TopSpeedWeight/sum);
    return std::isfinite(result)?result:-1.0;
}

struct GaragePlanAlternative final
{
    GaragePurchasePlan Plan{};
    double PreferenceScore=0.0;
    bool Dominated=false;
};

struct GaragePlanFrontier final
{
    bool Valid=false;
    std::array<GaragePlanAlternative,GaragePlanMaxAlternatives> Alternatives{};
    unsigned Count=0u;
    unsigned AffordableCount=0u;
    unsigned FrontierCount=0u;
    int BestAffordableIndex=-1;
    int BestOverallIndex=-1;
};

// A dominated option is no better in any modeled performance multiplier
// and no cheaper, with at least one strict disadvantage. Ownership is a
// separate preference: this comparison does not assign collection value.
inline bool GaragePlanDominates(
    const GaragePurchasePlan& a,const GaragePurchasePlan& b) noexcept
{
    if(a.Status==GaragePlanStatus::Invalid||
       b.Status==GaragePlanStatus::Invalid)return false;
    const auto x=a.Projected;
    const auto y=b.Projected;
    return a.TotalCost<=b.TotalCost &&
        x.Acceleration>=y.Acceleration &&
        x.Grip>=y.Grip && x.TopSpeed>=y.TopSpeed &&
        (a.TotalCost<b.TotalCost || x.Acceleration>y.Acceleration ||
         x.Grip>y.Grip || x.TopSpeed>y.TopSpeed);
}

inline bool GarageAlternativePreferred(
    const GaragePlanAlternative& a,const GaragePlanAlternative& b) noexcept
{
    if(a.PreferenceScore!=b.PreferenceScore)
        return a.PreferenceScore>b.PreferenceScore;
    if(a.Plan.TotalCost!=b.Plan.TotalCost)
        return a.Plan.TotalCost<b.Plan.TotalCost;
    if(a.Plan.ActionCount!=b.Plan.ActionCount)
        return a.Plan.ActionCount<b.Plan.ActionCount;
    if(a.Plan.Target.VehicleId!=b.Plan.Target.VehicleId)
        return a.Plan.Target.VehicleId<b.Plan.Target.VehicleId;
    if(a.Plan.Target.EngineLevel!=b.Plan.Target.EngineLevel)
        return a.Plan.Target.EngineLevel<b.Plan.Target.EngineLevel;
    return a.Plan.Target.GripLevel<b.Plan.Target.GripLevel;
}

inline GaragePlanFrontier BuildGaragePlanFrontier(
    GarageSnapshot persisted,GaragePlanPreferences preferences) noexcept
{
    if(!ValidGaragePlanPreferences(preferences))return {};
    GarageState initial;
    initial.Load(persisted);
    const auto baseline=initial.Snapshot();
    GaragePlanFrontier result{};
    for(int car=0;car<VehicleCount;++car)
    {
        if(!preferences.IncludeLockedVehicles&&!initial.Owns(car))continue;
        for(int engine=baseline.EngineLevel;
            engine<=static_cast<int>(GaragePlanMaxLevel);++engine)
        for(int grip=baseline.GripLevel;
            grip<=static_cast<int>(GaragePlanMaxLevel);++grip)
        {
            auto plan=PlanGaragePurchasePath(baseline,
                {car,engine,grip,preferences.ReserveCredits});
            if(plan.Status==GaragePlanStatus::Invalid||
               result.Count>=result.Alternatives.size())return {};
            const double score=GaragePlanScore(plan.Projected,preferences);
            if(score<0.0)return {};
            result.Alternatives[result.Count++]={plan,score,false};
        }
    }
    // Bounded O(96^2), computed on menu request only, not vehicle Tick.
    for(unsigned i=0;i<result.Count;++i)
    {
        auto& entry=result.Alternatives[i];
        for(unsigned j=0;j<result.Count;++j)
        {
            if(i!=j&&GaragePlanDominates(
                result.Alternatives[j].Plan,entry.Plan))
            {
                entry.Dominated=true;
                break;
            }
        }
        if(!entry.Dominated)++result.FrontierCount;
        if(entry.Plan.Status==GaragePlanStatus::Affordable)
            ++result.AffordableCount;
        // A dominated option is not returned as a recommended upgrade.
        if(entry.Dominated)continue;
        const auto better=[&](int prior)
        {
            return prior<0||GarageAlternativePreferred(
                entry,result.Alternatives[static_cast<unsigned>(prior)]);
        };
        if(better(result.BestOverallIndex))
            result.BestOverallIndex=static_cast<int>(i);
        if(entry.Plan.Status==GaragePlanStatus::Affordable&&
           better(result.BestAffordableIndex))
            result.BestAffordableIndex=static_cast<int>(i);
    }
    result.Valid=result.Count>0u&&result.FrontierCount>0u&&
        result.BestOverallIndex>=0;
    return result;
}
} // namespace latebrake
