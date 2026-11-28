#include "Racing/PracticeExperimentPlan.h"
#include "Racing/PracticeTrialSetup.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"PracticeExperimentPlan: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
PracticeConfiguration Key(int car=0,int engine=0,int grip=0)
{
    return {QuickPracticePreset::SoloThree,{car,engine,grip},0};
}
void Finish(PracticeSessionHistory& history,PracticeConfiguration key,double lap,double fee=0)
{
    auto def=GetQuickPracticeDefinition(key.Preset);
    RaceProgress race(def.CheckpointCount,def.RequiredLaps);
    for(int i=0;i<def.RequiredLaps;++i)
        for(int g=0;g<def.CheckpointCount;++g)
        {
            race.Advance(lap/def.CheckpointCount);
            if(i==0 && g==0 && fee>0)Require(race.AddTimePenalty(fee),"fee charged");
            Require(race.PassCheckpoint(g),"ordered physical gate");
        }
    Require(history.Begin(key,1),"valid begin");
    Require(history.Finish(key,1,race).Record.FinishedAttempts>0,"valid finish");
}
PracticeExperimentPlan Plan(const PracticeSessionHistory& history,PracticeConfiguration key)
{
    return BuildPracticeExperimentPlan(BuildPracticeSetupCatalogue(history,key),key);
}
void EmptyCoverage()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    const auto key=Key();
    const auto plan=Plan(*history,key);
    Require(plan.Valid && plan.Untested==96 && plan.Remaining()==96 &&
        plan.Sufficient==0 && plan.Count==96,"all configurations untested");
    Require(plan.Current.Valid && plan.Current.Selected &&
        plan.Current.Reason==PracticeExperimentReason::Untested,"selected baseline needs first finish");
    Require(plan.Candidates[0].Key==key && plan.Candidates[0].ChangedAxes==0,"baseline first");
    Require(plan.NextAlternative.Valid && plan.NextAlternative.ChangedAxes==1 &&
        plan.NextAlternative.Key==Key(0,0,1),"deterministic nearby alternative");
    Require(plan.NextVehicleOnly.Key==Key(1,0,0) &&
        plan.NextEngineOnly.Key==Key(0,1,0) &&
        plan.NextGripOnly.Key==Key(0,0,1),"single-axis choices");
    Require(!plan.FastestObserved.Valid && !plan.FastestFeeFree.Valid,"no fabricated measurements");
    const auto choices=GetPracticeExperimentChoices(plan);
    Require(choices.NextValid && choices.Next==Key(0,0,1).Tuning &&
        !choices.BestValid,"untested suggestion but no measured best");
    Require(!Plan(*history,{}).Valid,"invalid configuration");
}
void EvidencePriorities()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    const auto key=Key();
    Finish(*history,key,20,5);
    auto plan=Plan(*history,key);
    Require(plan.Untested==95 && plan.NoFeeFreeFinish==1 &&
        plan.NeedsRepeats==0 && plan.Current.Reason==PracticeExperimentReason::NoFeeFreeFinish,
        "penalized finish is a real test but not fee-free");
    Finish(*history,key,22);
    plan=Plan(*history,key);
    Require(plan.Current.Reason==PracticeExperimentReason::NeedsRepeats &&
        plan.Current.Finishes==2 && plan.Current.FeeFreeFinishes==1,"repeat target");
    Finish(*history,key,21);
    plan=Plan(*history,key);
    Require(plan.Sufficient==1 && !plan.Current.Valid &&
        plan.ObservedFinishes==3 && plan.Untested==95,"complete baseline coverage");
    auto other=Key(0,0,1);
    Finish(*history,other,17,2);
    auto choices=GetPracticeExperimentChoices(Plan(*history,other));
    Require(choices.BestValid && choices.Best==key.Tuning &&
        choices.BestSeconds==63,"prefer observed fee-free reference");
    choices=GetPracticeExperimentChoices(Plan(*history,key));
    Require(!choices.BestValid,"already staged fee-free best");
    auto unrelated=key;unrelated.Handling=1;
    Finish(*history,unrelated,10);
    unrelated=key;unrelated.Preset=QuickPracticePreset::RivalFive;
    Finish(*history,unrelated,10);
    Require(Plan(*history,key).ObservedFinishes==4,"other modes and handling excluded");
}
void CompleteAndReopen()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    for(int car=0;car<VehicleCount;++car)
        for(int engine=0;engine<4;++engine)
            for(int grip=0;grip<4;++grip)
            {
                auto key=Key(car,engine,grip);
                Finish(*history,key,25);
                Finish(*history,key,24);
                Finish(*history,key,23);
            }
    auto plan=Plan(*history,Key(3,2,1));
    Require(plan.Valid && plan.CoverageComplete() && plan.Sufficient==96 &&
        plan.ObservedFinishes==288 && plan.Remaining()==0 &&
        !plan.NextAlternative.Valid,"all 96 setups with three measured finishes");
    const auto choices=GetPracticeExperimentChoices(plan);
    Require(!choices.NextValid && choices.BestValid &&
        choices.Best==Key().Tuning && choices.BestSeconds==69,"deterministic equal measured best");
    history->ResetSession();
    Require(Plan(*history,Key()).Untested==96,"session reset");
}
void StagingNeverMutatesActiveOrGarage()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    const GarageSnapshot saved{999,1,0,1,1};
    PracticeTrialSetup setup;
    PracticeTrialAccess access{true,MenuPage::QuickRace,true,false,QuickPracticePreset::SoloThree};
    const auto initial=setup.Pending(saved);
    Require(!setup.Stage(access,{5,3,3}),"trial disabled");
    Require(setup.Edit(access,PracticeTrialAction::Toggle,saved),"enable trial");
    Require(setup.BeginPractice(access),"active attempt");
    const auto active=setup.Active(access.Preset,saved);
    const PracticeConfiguration key{access.Preset,initial.Key,0};
    const auto choices=GetPracticeExperimentChoices(Plan(*history,key));
    Require(choices.NextValid && setup.Stage(access,choices.Next),"explicit next setup");
    Require(setup.Pending(saved).Key==choices.Next &&
        setup.Active(access.Preset,saved).Key==active.Key,"pending-only staging");
    Require(!setup.Stage(access,choices.Next),"same setup no-op");
    Require(!setup.Stage(access,{99,3,3}),"invalid target");
    auto blocked=access;blocked.Suspended=true;
    Require(!setup.Stage(blocked,{5,3,3}),"suspended stage");
    blocked=access;blocked.Page=MenuPage::Garage;
    Require(!setup.Stage(blocked,{5,3,3}),"wrong page");
    blocked=access;blocked.Preset=QuickPracticePreset::None;
    Require(!setup.Stage(blocked,{5,3,3}),"normal race");
    Require(setup.BeginPractice(access),"separate launch");
    Require(setup.Active(access.Preset,saved).Key==choices.Next,"latched only on launch");
    setup.EndPractice();
    Require(setup.Active(access.Preset,saved).Key==MakeGarageTuningKey(saved),
        "saved configuration restored");
}
void RandomizedCoverageAndOrder()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    std::mt19937 rng(64320);
    for(int step=0;step<350;++step)
    {
        const auto key=Key(static_cast<int>(rng()%6),static_cast<int>(rng()%4),static_cast<int>(rng()%4));
        Finish(*history,key,20+static_cast<double>(rng()%10),static_cast<double>(rng()%4));
        const auto plan=Plan(*history,key);
        Require(plan.Valid && plan.ObservedFinishes==static_cast<std::uint64_t>(step+1),
            "exact total observed attempts");
        Require(plan.Untested+plan.NoFeeFreeFinish+plan.NeedsRepeats+plan.Sufficient==96,
            "exhaustive distinct coverage categories");
        Require(plan.Remaining()==plan.Count,"candidate count");
        for(int i=0;i<plan.Count;++i)
        {
            const auto& row=plan.Candidates[static_cast<std::size_t>(i)];
            Require(row.Valid && row.Key.Preset==key.Preset && row.Key.Handling==key.Handling,
                "same evidence group");
            Require(row.ChangedAxes==PracticeExperimentAxisDistance(key.Tuning,row.Key.Tuning),
                "exact setup change axes");
            if(i>0)Require(!BetterPracticeExperiment(row,plan.Candidates[static_cast<std::size_t>(i-1)]),
                "deterministic sorted priorities");
        }
        if(plan.NextAlternative.Valid)
            Require(!(plan.NextAlternative.Key==key),"never suggest same setup as alternative");
    }
}
}
int main()
{
    EmptyCoverage();EvidencePriorities();CompleteAndReopen();
    StagingNeverMutatesActiveOrGarage();RandomizedCoverageAndOrder();
    std::cout<<"Practice experiment plan tests passed\n";
}
