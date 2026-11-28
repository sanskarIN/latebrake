#pragma once

#include "Career/CareerProgress.h"
#include "Garage/GaragePurchasePlanner.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Career roadmap is a hypothetical, read-only plan, not a medal grant.
// Rewards below are conditional on a real first medal and successful
// save. A future budget cannot authorize any present purchase or launch.
enum class CareerRoadmapReadiness : std::uint8_t
{
    Invalid,
    Ready,
    PrerequisiteMissing,
    RequiredCarMissing
};

inline const char* CareerRoadmapObjectiveName(CareerObjective objective) noexcept
{
    switch(objective)
    {
        case CareerObjective::Time: return "TOTAL TIME";
        case CareerObjective::Clean: return "RECOVERIES";
        case CareerObjective::Precision: return "ORDERED GATES / TOTAL TIME";
        case CareerObjective::Consistency: return "LAP SPREAD";
        case CareerObjective::Vehicle: return "REQUIRED CAR / TOTAL TIME";
        case CareerObjective::Endurance: return "ENDURANCE / TOTAL TIME";
        default: return "UNAVAILABLE";
    }
}

struct CareerRoadmapEvent final
{
    bool Valid=false;
    int EventId=-1;
    CareerStatus Status=CareerStatus::Locked;
    CareerMedal Medal=CareerMedal::None;
    CareerRoadmapReadiness Readiness=CareerRoadmapReadiness::Invalid;
    int MissingPrerequisite=-1;
    int RequiredVehicle=-1;
    int RequiredUnlockCost=0;
    std::int64_t CurrentUnlockShortfall=0;
    int FirstCredits=0;
    int DirectSuccessors=0;
    int AvailableSuccessors=0;
    double NextMedalTarget=0.0;
    CareerMedal NextMedal=CareerMedal::None;
};

inline CareerMedal NextCareerMedal(CareerMedal medal) noexcept
{
    switch(medal)
    {
        case CareerMedal::None: return CareerMedal::Bronze;
        case CareerMedal::Bronze: return CareerMedal::Silver;
        case CareerMedal::Silver: return CareerMedal::Gold;
        default: return CareerMedal::None;
    }
}

inline double CareerTargetForMedal(const CareerEvent& event,CareerMedal medal) noexcept
{
    switch(medal)
    {
        case CareerMedal::Bronze: return event.BronzeTarget;
        case CareerMedal::Silver: return event.SilverTarget;
        case CareerMedal::Gold: return event.GoldTarget;
        default: return 0.0;
    }
}

inline CareerRoadmapEvent DescribeCareerRoadmapEvent(const CareerProgress& progress,
    const GarageState& garage,int id) noexcept
{
    CareerRoadmapEvent out;
    const auto* event=GetCareerEvent(id);
    if(!event) return out;
    out.EventId=id;
    out.Medal=progress.Medal(id);
    out.Status=progress.Status(id);
    out.RequiredVehicle=event->RequiredVehicle;
    out.FirstCredits=out.Medal==CareerMedal::None?event->FirstCredits:0;
    out.NextMedal=NextCareerMedal(out.Medal);
    out.NextMedalTarget=CareerTargetForMedal(*event,out.NextMedal);
    if(event->RequiredVehicle>=0)
    {
        if(!GarageState::IsVehicle(event->RequiredVehicle)) return {};
        const auto path=PlanGaragePurchasePath(garage.Snapshot(),
            {event->RequiredVehicle,garage.EngineLevel(),garage.GripLevel(),0});
        if(path.Status==GaragePlanStatus::Invalid) return {};
        out.RequiredUnlockCost=path.UnlockCost;
        out.CurrentUnlockShortfall=path.CreditsShort;
    }
    if(out.Status==CareerStatus::Locked)
    {
        out.Readiness=CareerRoadmapReadiness::PrerequisiteMissing;
        out.MissingPrerequisite=event->Prerequisite;
    }
    else if(event->RequiredVehicle>=0 && !garage.Owns(event->RequiredVehicle))
        out.Readiness=CareerRoadmapReadiness::RequiredCarMissing;
    else
        out.Readiness=CareerRoadmapReadiness::Ready;
    for(const auto& other:CareerEvents)
    {
        if(other.Prerequisite!=id) continue;
        ++out.DirectSuccessors;
        if(progress.Status(other.Id)!=CareerStatus::Locked) ++out.AvailableSuccessors;
    }
    out.Valid=true;
    return out;
}

struct CareerRoadmapStep final
{
    bool Valid=false;
    int EventId=-1;
    bool FirstCompletion=false;
    bool IsTarget=false;
    int RequiredVehicle=-1;
    int PurchaseCost=0;
    int AdditionalCars=0;
    std::int64_t AdditionalFunding=0;
    int CreditsBefore=0;
    int CreditsAfterPurchase=0;
    int ConditionalFirstCredits=0;
    int ConditionalChampionshipBonus=0;
    int ConditionalCredited=0;
    int ConditionalEndingCredits=0;
    int ResultOwnedMask=0;
    bool HasFirstPurchase=false;
    GaragePlanAction FirstPurchase{};
};

struct CareerRoadmap final
{
    bool Valid=false;
    CareerRoadmapEvent Target{};
    int InitialCredits=0;
    int StepCount=0;
    int FirstIncompleteEvent=-1;
    int FirstFundingEvent=-1;
    int TotalPurchaseCost=0;
    std::int64_t MinimumAdditionalFunding=0;
    int NominalConditionalRewards=0;
    int ConditionalCredited=0;
    int ConditionalEndingCredits=0;
    int RequiredRaceLaps=0;
    int FirstMedalsNeeded=0;
    std::array<CareerRoadmapStep,CareerEventCount> Steps{};
};

inline bool RoadmapChampionshipComplete(const std::array<bool,CareerEventCount>& medals,
    int series) noexcept
{
    const auto* championship=GetChampionship(series);
    if(!championship) return false;
    for(int id:championship->Rounds)
        if(!IsCareerId(id) || !medals[static_cast<std::size_t>(id)]) return false;
    return true;
}

// Reconstruct the exact missing ancestor chain. Already-completed
// prerequisites need not be raced again. A completed target itself is
// retained as a replay step, but earns no first-medal credits.
inline CareerRoadmap BuildCareerRoadmap(const CareerSnapshot& saved,
    GarageSnapshot garageSaved,int target) noexcept
{
    CareerRoadmap out;
    if(!IsCareerId(target)) return out;
    CareerProgress progress;
    progress.Load(saved);
    GarageState garage;
    garage.Load(garageSaved);
    out.InitialCredits=garage.Credits();
    out.Target=DescribeCareerRoadmapEvent(progress,garage,target);
    if(!out.Target.Valid) return {};

    std::array<int,CareerEventCount> reversed{};
    std::array<bool,CareerEventCount> seen{};
    int count=0;
    int id=target;
    while(id>=0)
    {
        const auto* event=GetCareerEvent(id);
        if(!event || count>=CareerEventCount || seen[static_cast<std::size_t>(id)])
            return {};
        if(id!=target && progress.Medal(id)!=CareerMedal::None) break;
        seen[static_cast<std::size_t>(id)]=true;
        reversed[static_cast<std::size_t>(count++)]=id;
        id=event->Prerequisite;
    }
    if(count<1) return {};
    std::array<bool,CareerEventCount> medals{};
    for(int e=0;e<CareerEventCount;++e)
        medals[static_cast<std::size_t>(e)]=progress.Medal(e)!=CareerMedal::None;

    for(int i=count-1;i>=0;--i)
    {
        const int eventId=reversed[static_cast<std::size_t>(i)];
        const auto* event=GetCareerEvent(eventId);
        if(!event || event->Laps<1 || event->FirstCredits<0 ||
            (event->Prerequisite>=0 && (!IsCareerId(event->Prerequisite) ||
                !medals[static_cast<std::size_t>(event->Prerequisite)])))
            return {};
        CareerRoadmapStep step;
        step.Valid=true;
        step.EventId=eventId;
        step.FirstCompletion=!medals[static_cast<std::size_t>(eventId)];
        step.IsTarget=eventId==target;
        step.RequiredVehicle=event->RequiredVehicle;
        step.CreditsBefore=garage.Credits();
        if(step.FirstCompletion)
        {
            ++out.FirstMedalsNeeded;
            if(out.FirstIncompleteEvent<0) out.FirstIncompleteEvent=eventId;
        }
        if(event->RequiredVehicle>=0)
        {
            const auto plan=PlanGaragePurchasePath(garage.Snapshot(),
                {event->RequiredVehicle,garage.EngineLevel(),garage.GripLevel(),0});
            if(plan.Status==GaragePlanStatus::Invalid || plan.EngineCost!=0 || plan.GripCost!=0)
                return {};
            step.PurchaseCost=plan.UnlockCost;
            step.AdditionalCars=plan.AdditionalCarsUnlocked;
            step.AdditionalFunding=plan.CreditsShort;
            for(unsigned a=0;a<plan.ActionCount;++a)
            {
                if(plan.Actions[a].Kind==GaragePlanActionKind::UnlockNext)
                {
                    step.HasFirstPurchase=true;
                    step.FirstPurchase=plan.Actions[a];
                    break;
                }
            }
            // Hypothetical outside funding is introduced only as needed
            // BEFORE this event. Later conditional credits cannot pay
            // for the car required to enter it.
            auto hypothetical=garage.Snapshot();
            const auto funded=static_cast<std::int64_t>(hypothetical.Credits)+step.AdditionalFunding;
            if(funded<step.PurchaseCost || funded>std::numeric_limits<int>::max()) return {};
            hypothetical.Credits=static_cast<int>(funded)-step.PurchaseCost;
            hypothetical.OwnedVehicleMask=plan.ProjectedOwnedMask;
            hypothetical.SelectedVehicle=event->RequiredVehicle;
            garage.Load(hypothetical);
            if(!garage.Owns(event->RequiredVehicle)) return {};
            if(step.AdditionalFunding>0 && out.FirstFundingEvent<0)
                out.FirstFundingEvent=eventId;
            out.TotalPurchaseCost+=step.PurchaseCost;
            out.MinimumAdditionalFunding+=step.AdditionalFunding;
        }
        step.CreditsAfterPurchase=garage.Credits();
        out.RequiredRaceLaps+=event->Laps;
        if(step.FirstCompletion)
        {
            const bool previous=RoadmapChampionshipComplete(medals,event->Series);
            medals[static_cast<std::size_t>(eventId)]=true;
            step.ConditionalFirstCredits=event->FirstCredits;
            if(event->Series>=0 && !previous &&
                RoadmapChampionshipComplete(medals,event->Series))
            {
                const auto* championship=GetChampionship(event->Series);
                if(!championship || championship->FirstCompletionBonus<0) return {};
                step.ConditionalChampionshipBonus=championship->FirstCompletionBonus;
            }
            const int reward=step.ConditionalFirstCredits+step.ConditionalChampionshipBonus;
            out.NominalConditionalRewards+=reward;
            garage.EarnCredits(reward);
            step.ConditionalCredited=garage.Credits()-step.CreditsAfterPurchase;
            out.ConditionalCredited+=step.ConditionalCredited;
        }
        step.ConditionalEndingCredits=garage.Credits();
        step.ResultOwnedMask=garage.Snapshot().OwnedVehicleMask;
        out.Steps[static_cast<std::size_t>(out.StepCount++)]=step;
    }
    out.ConditionalEndingCredits=garage.Credits();
    const auto balance=static_cast<std::int64_t>(out.InitialCredits)+
        out.MinimumAdditionalFunding-out.TotalPurchaseCost+out.ConditionalCredited;
    if(balance!=out.ConditionalEndingCredits || out.StepCount!=count) return {};
    out.Valid=true;
    return out;
}

struct CareerRoadmapOverview final
{
    int Completed=0;
    int Mastered=0;
    int Ready=0;
    int ReadyFirstMedals=0;
    int MissingCar=0;
    int Locked=0;
    int RemainingFirstCredits=0;
    int RemainingChampionshipBonuses=0;
    std::array<CareerRoadmapEvent,CareerEventCount> Events{};
};

inline CareerRoadmapOverview BuildCareerRoadmapOverview(const CareerSnapshot& saved,
    GarageSnapshot garageSaved) noexcept
{
    CareerProgress progress;
    progress.Load(saved);
    GarageState garage;
    garage.Load(garageSaved);
    CareerRoadmapOverview out;
    for(int id=0;id<CareerEventCount;++id)
    {
        auto& row=out.Events[static_cast<std::size_t>(id)];
        row=DescribeCareerRoadmapEvent(progress,garage,id);
        if(!row.Valid) return {};
        if(row.Medal!=CareerMedal::None) ++out.Completed;
        if(row.Medal==CareerMedal::Gold) ++out.Mastered;
        out.RemainingFirstCredits+=row.FirstCredits;
        switch(row.Readiness)
        {
            case CareerRoadmapReadiness::Ready:
                ++out.Ready;
                if(row.Medal==CareerMedal::None) ++out.ReadyFirstMedals;
                break;
            case CareerRoadmapReadiness::RequiredCarMissing: ++out.MissingCar; break;
            case CareerRoadmapReadiness::PrerequisiteMissing: ++out.Locked; break;
            default: return {};
        }
    }
    for(const auto& series:Championships)
        if(!progress.ChampionshipComplete(series.Id))
            out.RemainingChampionshipBonuses+=series.FirstCompletionBonus;
    return out;
}
} // namespace latebrake
