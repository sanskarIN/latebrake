#include "Garage/GarageBudgetAdvisor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"GarageBudgetAdvisor: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b)
{
    return std::abs(a-b)<1e-9;
}

void VerifyChoice(const GarageSnapshot& baseline,const GarageBudgetChoice& choice,
    GarageBudgetFocus focus)
{
    Require(choice.Valid,"choice available");
    const auto plan=PlanGaragePurchasePath(baseline,choice.Target);
    Require(plan.Status!=GaragePlanStatus::Invalid && plan.ReadOnly,"canonical plan");
    Require(choice.Cost==plan.TotalCost && choice.CreditsShort==plan.CreditsShort &&
        choice.AdditionalCars==plan.AdditionalCarsUnlocked && choice.ActionCount==plan.ActionCount,
        "exact planned route");
    Require(Near(choice.Score,GaragePlanScore(plan.Projected,GarageBudgetPreferences(focus,
        choice.Target.ReserveCredits))),"score is configured multiplier only");
    Require(Near(choice.Projected.Acceleration,plan.Projected.Acceleration) &&
        Near(choice.Projected.Grip,plan.Projected.Grip) &&
        Near(choice.Projected.TopSpeed,plan.Projected.TopSpeed),"same projected performance");
    Require(choice.HasFirstAction==(plan.ActionCount>0),"first action present only if needed");
    if(choice.HasFirstAction)
        Require(choice.FirstAction.Kind==plan.Actions[0].Kind &&
            choice.FirstAction.Target==plan.Actions[0].Target &&
            choice.FirstAction.Cost==plan.Actions[0].Cost,"first action from real route");
}

void InitialGarage()
{
    GarageSnapshot initial{};
    const auto r=BuildGarageBudgetReview(initial,5);
    Require(r.Valid && r.EnumeratedPlans==GaragePlanMaxAlternatives &&
        r.ViewedVehicle==5 && SameGarageBudgetSnapshot(initial,r.Baseline),"six-car baseline");
    for(const auto& focus:r.Focuses)
    {
        VerifyChoice(initial,focus.BestAffordable,focus.Focus);
        VerifyChoice(initial,focus.BestOverall,focus.Focus);
        Require(focus.BestAffordable.Target.VehicleId==0 && focus.BestAffordable.Cost==0 &&
            focus.BestAffordable.ActionCount==0 && focus.AffordablePlans==1 &&
            !focus.AffordableImprovesEquipped() && !focus.BestViewedAffordable.Valid &&
            focus.BestViewedOverall.Target.VehicleId==5 && focus.NextImprovement.Valid,
            "no credits cannot equip or upgrade locked cars");
        VerifyChoice(initial,focus.NextImprovement,focus.Focus);
        Require(focus.NextImprovement.CreditsShort>0 &&
            focus.NextImprovement.Score>focus.BestAffordable.Score,"real savings target");
    }
    const auto& accel=r.Focuses[static_cast<std::size_t>(GarageBudgetFocus::Acceleration)];
    Require(accel.NextImprovement.Cost==200 &&
        accel.NextImprovement.FirstAction.Kind==GaragePlanActionKind::UpgradeEngine,
        "cheapest acceleration gain is first engine level");
    const auto& grip=r.Focuses[static_cast<std::size_t>(GarageBudgetFocus::Grip)];
    Require(grip.NextImprovement.Cost==150 &&
        grip.NextImprovement.FirstAction.Kind==GaragePlanActionKind::UpgradeGrip,
        "cheapest grip gain is first grip level");
    const auto& speed=r.Focuses[static_cast<std::size_t>(GarageBudgetFocus::Speed)];
    Require(speed.NextImprovement.Cost==200,"first engine level improves modeled speed");
}

void BudgetAndOwnership()
{
    GarageSnapshot state{600,1,0,0,0};
    auto r=BuildGarageBudgetReview(state,1);
    Require(r.Valid,"affordable options");
    for(const auto& f:r.Focuses)
    {
        Require(f.BestAffordable.Valid && f.BestAffordable.CreditsShort==0 &&
            f.BestAffordable.Cost<=600 && f.BestViewedAffordable.Valid &&
            f.BestViewedAffordable.Target.VehicleId==1 &&
            f.BestViewedAffordable.Cost==600 &&
            f.BestViewedAffordable.FirstAction.Kind==GaragePlanActionKind::UnlockNext,
            "locked viewed option includes exact price and first action");
        VerifyChoice(state,f.BestViewedAffordable,f.Focus);
    }
    r=BuildGarageBudgetReview(state,1,1);
    for(const auto& f:r.Focuses)
        Require(!f.BestViewedAffordable.Valid && f.BestAffordable.Cost<=599,
            "reserve not spent");
    r=BuildGarageBudgetReview(state,5,601);
    for(const auto& f:r.Focuses)
        Require(!f.BestAffordable.Valid && !f.BestViewedAffordable.Valid &&
            f.BestOverall.Valid,"unfunded reserve does not claim affordability");
    state={0,(1<<0)|(1<<5),0,3,3};
    r=BuildGarageBudgetReview(state,5);
    for(const auto& f:r.Focuses)
    {
        Require(f.BestViewedAffordable.Valid && f.BestViewedAffordable.Cost==0 &&
            f.BestViewedAffordable.FirstAction.Kind==GaragePlanActionKind::SelectOwned,
            "nonsequential saved ownership needs no prior unlocks");
        VerifyChoice(state,f.BestViewedAffordable,f.Focus);
    }
}

void MaxedAndInvalid()
{
    const GarageSnapshot all{std::numeric_limits<int>::max(),KnownVehicleMask,5,3,3};
    auto r=BuildGarageBudgetReview(all,5);
    Require(r.Valid && r.EnumeratedPlans==6,"maxed levels produce exactly six plans");
    for(const auto& f:r.Focuses)
    {
        Require(f.BestAffordable.Valid && f.BestOverall.Valid &&
            !f.NextImprovement.Valid && f.AffordablePlans==6 &&
            f.BestAffordable.Cost==0 && f.BestViewedAffordable.ActionCount==0,
            "all attainable options funded");
        VerifyChoice(all,f.BestAffordable,f.Focus);
    }
    Require(!BuildGarageBudgetReview(all,-1).Valid &&
        !BuildGarageBudgetReview(all,VehicleCount).Valid &&
        !BuildGarageBudgetReview(all,0,-1).Valid,"invalid request denied");
    const GarageSnapshot malformed{-10,-1,100,-5,12};
    r=BuildGarageBudgetReview(malformed,0);
    Require(r.Valid && r.Baseline.Credits==0 && r.Baseline.OwnedVehicleMask==1 &&
        r.Baseline.SelectedVehicle==0 && r.Baseline.EngineLevel==0 &&
        r.Baseline.GripLevel==3,"shared save sanitation");
    Require(!ValidGaragePlanPreferences(GarageBudgetPreferences(GarageBudgetFocus::Count)),
        "invalid focus cannot score");
    Require(!SummarizeGarageBudgetChoice({},1).Valid &&
        !SummarizeGarageBudgetChoice(PlanGaragePurchasePath(all,{0,3,3,0}),
            std::numeric_limits<double>::quiet_NaN()).Valid,"invalid choice suppressed");
}

void CacheInvalidation()
{
    GarageBudgetAdvisor advisor;
    GarageSnapshot s{};
    Require(advisor.Read(s,0).Valid && advisor.RebuildCount()==1,"first build");
    for(int i=0;i<50;++i) Require(advisor.Read(s,0).Valid,"stable read");
    Require(advisor.RebuildCount()==1,"no per-frame planning");
    ++s.Credits;
    advisor.Read(s,0);
    s.OwnedVehicleMask|=2;advisor.Read(s,0);
    s.SelectedVehicle=1;advisor.Read(s,0);
    s.EngineLevel=1;advisor.Read(s,0);
    s.GripLevel=1;advisor.Read(s,0);
    advisor.Read(s,1);
    advisor.Read(s,1,1);
    Require(advisor.RebuildCount()==8,"every meaningful change invalidates cache");
    Require(!advisor.Read(s,99).Valid,"invalid view cannot return old results");
    advisor.Reset();
    Require(advisor.RebuildCount()==0 && advisor.Read(s,1).Valid &&
        advisor.RebuildCount()==1,"reset discards cache");
}

void ExactSavingsAndExhaustiveAlternatives()
{
    std::mt19937 rng(96021);
    for(int sample=0;sample<300;++sample)
    {
        GarageSnapshot s{static_cast<int>(rng()%13000),static_cast<int>(rng()%64),
            static_cast<int>(rng()%6),static_cast<int>(rng()%4),static_cast<int>(rng()%4)};
        const int view=static_cast<int>(rng()%6);
        const int reserve=static_cast<int>(rng()%1000);
        const auto review=BuildGarageBudgetReview(s,view,reserve);
        Require(review.Valid,"valid bounded review");
        GarageState actual;
        actual.Load(s);
        const auto baseline=actual.Snapshot();
        Require(SameGarageBudgetSnapshot(baseline,review.Baseline),"canonical baseline");
        for(const auto& f:review.Focuses)
        {
            const auto pref=GarageBudgetPreferences(f.Focus,reserve);
            const auto frontier=BuildGaragePlanFrontier(baseline,pref);
            Require(frontier.Valid && frontier.Count==review.EnumeratedPlans &&
                frontier.AffordableCount==f.AffordablePlans,"canonical enumeration");
            VerifyChoice(baseline,f.BestOverall,f.Focus);
            Require(Near(f.BestOverall.Score,
                frontier.Alternatives[static_cast<std::size_t>(frontier.BestOverallIndex)].PreferenceScore),
                "best score agrees with canonical frontier");
            if(frontier.BestAffordableIndex>=0)
            {
                VerifyChoice(baseline,f.BestAffordable,f.Focus);
                Require(Near(f.BestAffordable.Score,
                    frontier.Alternatives[static_cast<std::size_t>(frontier.BestAffordableIndex)].PreferenceScore),
                    "best funded score agrees with canonical frontier");
                Require(f.BestAffordable.Cost+static_cast<std::int64_t>(reserve)<=baseline.Credits,
                    "reserve survives all proposed purchases");
            }
            else Require(!f.BestAffordable.Valid,"unfunded budget");
            unsigned viewed=0;
            GarageBudgetChoice expectedNext{};
            const double threshold=f.BestAffordable.Valid?
                std::max(f.EquippedScore,f.BestAffordable.Score):f.EquippedScore;
            for(unsigned i=0;i<frontier.Count;++i)
            {
                const auto& a=frontier.Alternatives[i];
                const auto c=SummarizeGarageBudgetChoice(a.Plan,a.PreferenceScore);
                Require(!BetterGarageBudgetChoice(c,f.BestOverall),"no higher overall candidate");
                if(a.Plan.Status==GaragePlanStatus::Affordable)
                {
                    Require(!BetterGarageBudgetChoice(c,f.BestAffordable),"no better funded candidate");
                    if(c.Target.VehicleId==view)
                    {
                        ++viewed;
                        Require(!BetterGarageBudgetChoice(c,f.BestViewedAffordable),"best viewed candidate");
                    }
                }
                else if(c.Score>threshold+GarageBudgetScoreEpsilon &&
                    PreferGarageSavingsTarget(c,expectedNext)) expectedNext=c;
            }
            Require(viewed==f.ViewedAffordablePlans &&
                f.BestViewedAffordable.Valid==(viewed>0),"exact viewed count");
            Require(f.NextImprovement.Valid==expectedNext.Valid,"savings target completeness");
            if(expectedNext.Valid)
            {
                VerifyChoice(baseline,f.NextImprovement,f.Focus);
                Require(f.NextImprovement.CreditsShort==expectedNext.CreditsShort &&
                    f.NextImprovement.Score==expectedNext.Score,"minimum incremental savings target");
            }
        }
        Require(SameGarageBudgetSnapshot(actual.Snapshot(),baseline),"no mutation");
    }
}
}

int main()
{
    InitialGarage();
    BudgetAndOwnership();
    MaxedAndInvalid();
    CacheInvalidation();
    ExactSavingsAndExhaustiveAlternatives();
    std::cout<<"Garage budget advisor and exact purchase-path tests passed\n";
}
