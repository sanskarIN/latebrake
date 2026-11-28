#include "Career/CareerRoadmap.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"CareerRoadmap: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}

void RookieAndProloguePath()
{
    CareerSnapshot career;
    GarageSnapshot garage;
    auto plan=BuildCareerRoadmap(career,garage,5);
    Require(plan.Valid && plan.Target.Readiness==CareerRoadmapReadiness::PrerequisiteMissing &&
        plan.Target.MissingPrerequisite==4 && plan.StepCount==6 &&
        plan.FirstIncompleteEvent==0 && plan.FirstMedalsNeeded==6 &&
        plan.RequiredRaceLaps==14,"full prerequisite chain");
    int cumulative=0;
    for(int i=0;i<6;++i)
    {
        const auto& step=plan.Steps[static_cast<std::size_t>(i)];
        Require(step.Valid && step.EventId==i && step.FirstCompletion &&
            step.IsTarget==(i==5) && step.CreditsBefore==cumulative,
            "ordered first medals");
        Require(step.ConditionalFirstCredits==CareerEvents[static_cast<std::size_t>(i)].FirstCredits &&
            step.ConditionalChampionshipBonus==(i==2?350:i==5?550:0),
            "conditional championship bonus earned once");
        cumulative+=step.ConditionalCredited;
        Require(step.ConditionalEndingCredits==cumulative,"conditional balance");
    }
    Require(plan.TotalPurchaseCost==0 && plan.MinimumAdditionalFunding==0 &&
        plan.NominalConditionalRewards==2225 && plan.ConditionalEndingCredits==2225,
        "all six first medal rewards and two bonuses");
    career.Medals[0]=1;career.Medals[1]=2;career.Medals[2]=3;
    garage.Credits=70;
    plan=BuildCareerRoadmap(career,garage,5);
    Require(plan.Valid && plan.StepCount==3 && plan.FirstIncompleteEvent==3 &&
        plan.NominalConditionalRewards==1350 &&
        plan.ConditionalEndingCredits==1420,"completed cup not repeated or re-awarded");
    plan=BuildCareerRoadmap(career,garage,2);
    Require(plan.Valid && plan.StepCount==1 && !plan.Steps[0].FirstCompletion &&
        plan.Target.NextMedal==CareerMedal::None && plan.NominalConditionalRewards==0,
        "mastered target replay has no reward");
}

void RequiredCarAndFunding()
{
    CareerSnapshot career;
    GarageSnapshot garage;
    auto plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.Valid && plan.StepCount==4 && plan.FirstIncompleteEvent==0 &&
        plan.Target.RequiredVehicle==1 && plan.Target.RequiredUnlockCost==600 &&
        plan.Target.CurrentUnlockShortfall==600 &&
        plan.Target.Readiness==CareerRoadmapReadiness::PrerequisiteMissing,
        "locked mastery requires actual sequential route");
    const auto& target=plan.Steps[3];
    Require(target.CreditsBefore==825 && target.PurchaseCost==600 &&
        target.AdditionalCars==1 && target.HasFirstPurchase &&
        target.FirstPurchase.Kind==GaragePlanActionKind::UnlockNext &&
        target.FirstPurchase.Target==1 && target.ConditionalEndingCredits==425 &&
        plan.MinimumAdditionalFunding==0,"earlier conditional rewards can fund car");
    career.Medals[0]=1;career.Medals[1]=1;career.Medals[2]=1;
    plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.Valid && plan.StepCount==1 && plan.Target.Readiness==CareerRoadmapReadiness::RequiredCarMissing &&
        plan.FirstFundingEvent==10 && plan.MinimumAdditionalFunding==600 &&
        plan.Steps[0].CreditsAfterPurchase==0 &&
        plan.ConditionalEndingCredits==200,"target reward cannot fund its own admission");
    garage.Credits=599;
    plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.MinimumAdditionalFunding==1 && plan.Target.CurrentUnlockShortfall==1,
        "exact credit shortfall");
    garage.Credits=600;
    plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.MinimumAdditionalFunding==0 && plan.Target.Readiness==CareerRoadmapReadiness::RequiredCarMissing,
        "affordable purchase is still not ownership");
    garage.OwnedVehicleMask|=2;
    plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.Target.Readiness==CareerRoadmapReadiness::Ready &&
        plan.TotalPurchaseCost==0 && !plan.Steps[0].HasFirstPurchase &&
        plan.ConditionalEndingCredits==800,"owned required car ready");
    career.Medals[10]=3;garage.OwnedVehicleMask=1;garage.Credits=0;
    plan=BuildCareerRoadmap(career,garage,10);
    Require(plan.Valid && plan.Target.Readiness==CareerRoadmapReadiness::RequiredCarMissing &&
        plan.FirstMedalsNeeded==0 && plan.NominalConditionalRewards==0 &&
        plan.MinimumAdditionalFunding==600,"corrupt missing ownership never enables replay or reward");
}

void OverviewAndMedalTargets()
{
    CareerSnapshot career;
    GarageSnapshot garage;
    auto overview=BuildCareerRoadmapOverview(career,garage);
    Require(overview.Completed==0 && overview.Mastered==0 && overview.Ready==2 &&
        overview.ReadyFirstMedals==2 && overview.Locked==10 &&
        overview.MissingCar==0 && overview.RemainingChampionshipBonuses==900,
        "initial catalogue availability");
    Require(overview.Events[0].NextMedal==CareerMedal::Bronze &&
        overview.Events[0].NextMedalTarget==170 &&
        overview.Events[0].DirectSuccessors==2,"rookie target and dependent events");
    career.Medals[0]=1;career.Medals[1]=2;career.Medals[2]=3;
    overview=BuildCareerRoadmapOverview(career,garage);
    Require(overview.Completed==3 && overview.Mastered==1 &&
        overview.Events[0].NextMedal==CareerMedal::Silver &&
        overview.Events[0].NextMedalTarget==140 &&
        overview.Events[1].NextMedal==CareerMedal::Gold &&
        overview.Events[1].NextMedalTarget==220 &&
        overview.Events[2].NextMedal==CareerMedal::None &&
        overview.MissingCar==1 && overview.RemainingChampionshipBonuses==550,
        "medal-specific next targets and current unlock gate");
    Require(overview.Ready+overview.Locked+overview.MissingCar==CareerEventCount,
        "every event classified once");
    Require(CareerTargetForMedal(CareerEvents[3],CareerMedal::Gold)==0.0 &&
        NextCareerMedal(CareerMedal::Gold)==CareerMedal::None,
        "zero recovery target distinct from no next medal");
}

void InvalidProfilesAndSaturation()
{
    CareerSnapshot career;
    GarageSnapshot garage{std::numeric_limits<int>::max(),KnownVehicleMask,0,3,3};
    const auto plan=BuildCareerRoadmap(career,garage,5);
    Require(plan.Valid && plan.NominalConditionalRewards==2225 &&
        plan.ConditionalCredited==0 &&
        plan.ConditionalEndingCredits==std::numeric_limits<int>::max(),
        "existing saturation rules preserved");
    career.Medals[5]=3;career.Medals[10]=3;
    auto r=BuildCareerRoadmap(career,garage,5);
    Require(r.Valid && r.StepCount==6 && r.FirstMedalsNeeded==6,
        "impossible saved medals sanitized");
    career.Schema=CareerSaveSchema+1;
    career.Medals.fill(3);
    r=BuildCareerRoadmap(career,garage,5);
    Require(r.Valid && r.FirstMedalsNeeded==6,"unknown save schema never grants progress");
    Require(!BuildCareerRoadmap(career,garage,-1).Valid &&
        !BuildCareerRoadmap(career,garage,CareerEventCount).Valid,"invalid target refused");
}

void ExhaustiveSanitizedProgress()
{
    for(unsigned mask=0;mask<(1u<<CareerEventCount);++mask)
    {
        CareerSnapshot saved;
        for(int id=0;id<CareerEventCount;++id)
            if(mask&(1u<<id)) saved.Medals[static_cast<std::size_t>(id)]=1+id%3;
        CareerProgress actual;
        actual.Load(saved);
        const auto baseline=actual.Snapshot();
        const GarageSnapshot garage{static_cast<int>(mask%900),
            static_cast<int>((mask%64)|1),0,0,0};
        auto overview=BuildCareerRoadmapOverview(saved,garage);
        Require(overview.Completed==actual.CompletedCount() &&
            overview.Ready+overview.Locked+overview.MissingCar==CareerEventCount,
            "overview exact for sanitized medal graph");
        for(int target=0;target<CareerEventCount;++target)
        {
            const auto plan=BuildCareerRoadmap(saved,garage,target);
            Require(plan.Valid && plan.StepCount>=1 && plan.StepCount<=CareerEventCount &&
                plan.Steps[static_cast<std::size_t>(plan.StepCount-1)].EventId==target,
                "every real event has a bounded ordered route");
            std::array<bool,CareerEventCount> seen{};
            for(int id=0;id<CareerEventCount;++id)
                seen[static_cast<std::size_t>(id)]=actual.Medal(id)!=CareerMedal::None;
            int credits=plan.InitialCredits;
            int cost=0,rewards=0,first=0,laps=0;
            std::int64_t funding=0;
            for(int i=0;i<plan.StepCount;++i)
            {
                const auto& step=plan.Steps[static_cast<std::size_t>(i)];
                const auto& event=CareerEvents[static_cast<std::size_t>(step.EventId)];
                Require(step.CreditsBefore==credits && step.FirstCompletion==
                    !seen[static_cast<std::size_t>(step.EventId)] &&
                    (event.Prerequisite<0 || seen[static_cast<std::size_t>(event.Prerequisite)]),
                    "topological prerequisites and chronological budget");
                if(step.FirstCompletion) ++first;
                const bool prior=RoadmapChampionshipComplete(seen,event.Series);
                seen[static_cast<std::size_t>(step.EventId)]=true;
                const auto* series=GetChampionship(event.Series);
                const int bonus=series && step.FirstCompletion && !prior &&
                    RoadmapChampionshipComplete(seen,event.Series)?series->FirstCompletionBonus:0;
                Require(step.ConditionalChampionshipBonus==bonus &&
                    step.ConditionalFirstCredits==(step.FirstCompletion?event.FirstCredits:0),
                    "no duplicate medals or championship rewards");
                if(event.RequiredVehicle>=0)
                    Require((step.ResultOwnedMask&(1<<event.RequiredVehicle))!=0,
                        "required car is present before hypothetical reward");
                Require(step.AdditionalFunding==std::max<std::int64_t>(0,
                    static_cast<std::int64_t>(step.PurchaseCost)-credits),
                    "only necessary outside funding");
                credits=static_cast<int>(credits+step.AdditionalFunding-step.PurchaseCost);
                Require(credits==step.CreditsAfterPurchase,"exact debit");
                credits+=step.ConditionalCredited;
                Require(credits==step.ConditionalEndingCredits,"exact conditional credit");
                funding+=step.AdditionalFunding;
                cost+=step.PurchaseCost;
                rewards+=step.ConditionalCredited;
                laps+=event.Laps;
            }
            Require(plan.MinimumAdditionalFunding==funding && plan.TotalPurchaseCost==cost &&
                plan.ConditionalCredited==rewards && plan.ConditionalEndingCredits==credits &&
                plan.FirstMedalsNeeded==first && plan.RequiredRaceLaps==laps,
                "totals reconcile");
        }
        Require(actual.Snapshot().Medals==baseline.Medals,"no actual career mutation");
    }
}
}

int main()
{
    RookieAndProloguePath();
    RequiredCarAndFunding();
    OverviewAndMedalTargets();
    InvalidProfilesAndSaturation();
    ExhaustiveSanitizedProgress();
    std::cout<<"Career roadmap, medal targets and conditional progression tests passed\n";
}
