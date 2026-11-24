#include "Garage/GaragePurchasePlanner.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Check(bool ok,const char* why)
{
    if(!ok){std::cerr<<"GaragePurchasePlanner: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
void Replay(const GaragePurchasePlan& plan)
{
    auto baseline=plan.Baseline;
    baseline.Credits=100000;
    GarageState replay;
    replay.Load(baseline);
    int total=0;
    for(unsigned i=0;i<plan.ActionCount;++i)
    {
        const auto& a=plan.Actions[i];
        const int before=replay.Credits();
        switch(a.Kind)
        {
        case GaragePlanActionKind::UnlockNext:
            Check(replay.NextLockedVehicle()==a.Target&&replay.Unlock(a.Target),
                "real sequential unlock semantics");
            break;
        case GaragePlanActionKind::UpgradeEngine:
            Check(replay.Upgrade(UpgradeKind::Engine)&&replay.EngineLevel()==a.Target,
                "real engine tier semantics");
            break;
        case GaragePlanActionKind::UpgradeGrip:
            Check(replay.Upgrade(UpgradeKind::Grip)&&replay.GripLevel()==a.Target,
                "real grip tier semantics");
            break;
        case GaragePlanActionKind::SelectOwned:
            Check(replay.Select(a.Target),"real owned selection semantics");
            break;
        }
        total+=before-replay.Credits();
        Check(a.Cost==before-replay.Credits()&&a.CumulativeCost==total&&
            a.RequiresPurchaseConfirmation==(a.Cost>0),"step cost and consent");
    }
    const auto p=replay.Performance();
    Check(total==plan.TotalCost&&replay.SelectedVehicle()==plan.Target.VehicleId&&
        replay.EngineLevel()==plan.Target.EngineLevel&&replay.GripLevel()==plan.Target.GripLevel&&
        replay.Snapshot().OwnedVehicleMask==plan.ProjectedOwnedMask&&
        p.Acceleration==plan.Projected.Acceleration&&p.Grip==plan.Projected.Grip&&
        p.TopSpeed==plan.Projected.TopSpeed&&plan.ReadOnly,
        "hypothetical path matches authoritative local replay");
}
void ExhaustivePaths()
{
    for(int mask=0;mask<=KnownVehicleMask;++mask)
    for(int engine=0;engine<=3;++engine)
    for(int grip=0;grip<=3;++grip)
    for(int car=0;car<VehicleCount;++car)
    for(int targetEngine=engine;targetEngine<=3;++targetEngine)
    for(int targetGrip=grip;targetGrip<=3;++targetGrip)
    {
        const GarageSnapshot snapshot{2500,mask,car,engine,grip};
        const auto plan=PlanGaragePurchasePath(snapshot,{car,targetEngine,targetGrip,100});
        Check(plan.Status!=GaragePlanStatus::Invalid&&plan.ActionCount<=GaragePlanMaxActions,
            "all legitimate car/ownership/upgrade combinations");
        Check(plan.CreditsShort==std::max<std::int64_t>(0,plan.TotalCost+100-2500)&&
            (plan.Status==GaragePlanStatus::Affordable)==(plan.TotalCost<=2400),
            "reserve-aware affordability");
        Replay(plan);
    }
}
void BudgetAndFrontier()
{
    const auto nova=PlanGaragePurchasePath({}, {5,3,3,100});
    Check(nova.AdditionalCarsUnlocked==5&&nova.UnlockCost==9500&&
        nova.EngineCost==1050&&nova.GripCost==825&&nova.TotalCost==11375&&
        nova.RequiredCredits==11475&&nova.CreditsShort==11475,
        "complete sequential Nova route and all tier costs");
    const auto owned=PlanGaragePurchasePath({0,33,0,0,0},{5,0,0,0});
    Check(owned.Status==GaragePlanStatus::Affordable&&owned.TotalCost==0&&
        owned.ActionCount==1&&owned.AdditionalCarsUnlocked==0,
        "owned Nova requires no earlier locked purchases");
    const auto huge=PlanGaragePurchasePath({}, {5,3,3,std::numeric_limits<int>::max()});
    Check(huge.RequiredCredits>std::numeric_limits<int>::max(),
        "large reserve cannot overflow");
    Check(PlanGaragePurchasePath({}, {-1,0,0,0}).Status==GaragePlanStatus::Invalid&&
        PlanGaragePurchasePath({}, {0,4,0,0}).Status==GaragePlanStatus::Invalid&&
        PlanGaragePurchasePath({}, {0,0,0,-1}).Status==GaragePlanStatus::Invalid&&
        PlanGaragePurchasePath({0,1,0,2,0}, {0,1,0,0}).Status==GaragePlanStatus::Invalid,
        "invalid targets and downgrades");
    for(int credits:{0,150,600,2500,12000})
    for(bool locked:{false,true})
    {
        const GarageSnapshot snapshot{credits,1,0,0,0};
        GaragePlanPreferences pref{1.0,2.0,3.0,100,locked};
        const auto f=BuildGaragePlanFrontier(snapshot,pref);
        Check(f.Valid&&f.Count==(locked?96u:16u)&&f.FrontierCount>0u,
            "bounded exhaustive candidate enumeration");
        unsigned affordable=0,front=0;
        for(unsigned i=0;i<f.Count;++i)
        {
            const auto& row=f.Alternatives[i];
            bool dominated=false;
            for(unsigned j=0;j<f.Count;++j)
                if(i!=j&&GaragePlanDominates(f.Alternatives[j].Plan,row.Plan))
                    dominated=true;
            Check(row.Dominated==dominated,"Pareto frontier matches pairwise oracle");
            if(!dominated)++front;
            if(row.Plan.Status==GaragePlanStatus::Affordable)++affordable;
            if(!dominated&&f.BestOverallIndex>=0)
                Check(!GarageAlternativePreferred(row,
                    f.Alternatives[static_cast<unsigned>(f.BestOverallIndex)]),
                    "deterministic best overall frontier option");
            if(!dominated&&row.Plan.Status==GaragePlanStatus::Affordable&&
                f.BestAffordableIndex>=0)
                Check(!GarageAlternativePreferred(row,
                    f.Alternatives[static_cast<unsigned>(f.BestAffordableIndex)]),
                    "deterministic best affordable frontier option");
        }
        Check(affordable==f.AffordableCount&&front==f.FrontierCount&&
            (f.BestAffordableIndex>=0)==(affordable>0u),
            "frontier counts and affordability");
    }
    GaragePlanPreferences invalid{};
    invalid.GripWeight=std::numeric_limits<double>::quiet_NaN();
    Check(!BuildGaragePlanFrontier({},invalid).Valid,"nonfinite preference");
    invalid={0.0,0.0,0.0,0,true};
    Check(!BuildGaragePlanFrontier({},invalid).Valid,"zero weight vector");
}
}
int main()
{
    ExhaustivePaths();BudgetAndFrontier();
    std::cout<<"Garage purchase path, credit reserve and Pareto frontier PASS\n";
    return EXIT_SUCCESS;
}
