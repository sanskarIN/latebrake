#include "Racing/PracticeSetupAnalysisCache.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* why)
{
    if(!condition)
    {
        std::cerr<<"PracticeSetupAnalysisCache: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
PracticeConfiguration Key(int car=0,int engine=0,int grip=0)
{
    return {QuickPracticePreset::SoloThree,{car,engine,grip},0};
}
RaceProgress MakeRace(QuickPracticePreset preset,double lap,double fee=0)
{
    const auto def=GetQuickPracticeDefinition(preset);
    RaceProgress race(def.CheckpointCount,def.RequiredLaps);
    for(int l=0;l<def.RequiredLaps;++l)
        for(int g=0;g<def.CheckpointCount;++g)
        {
            race.Advance(lap/def.CheckpointCount);
            if(l==0 && g==0 && fee>0)Require(race.AddTimePenalty(fee),"fee");
            Require(race.PassCheckpoint(g),"actual checkpoint");
        }
    return race;
}
void Finish(PracticeSessionHistory& history,PracticeConfiguration key,double lap,double fee=0)
{
    Require(history.Begin(key,1),"begin");
    Require(history.Finish(key,1,MakeRace(key.Preset,lap,fee)).Record.FinishedAttempts>0,
        "accepted finish");
}
void SameCandidate(PracticeExperimentCandidate a,PracticeExperimentCandidate b)
{
    Require(a.Valid==b.Valid,"candidate availability");
    if(!a.Valid)return;
    Require(a.Key==b.Key && a.Reason==b.Reason && a.Finishes==b.Finishes &&
        a.FeeFreeFinishes==b.FeeFreeFinishes && a.ChangedAxes==b.ChangedAxes &&
        a.TuningDistance==b.TuningDistance && a.Selected==b.Selected,"exact candidate");
}
void SameEntry(const PracticeSetupEntry& a,const PracticeSetupEntry& b)
{
    Require(a.Valid==b.Valid,"entry availability");
    if(!a.Valid)return;
    Require(a.Key==b.Key && a.Finishes==b.Finishes && a.FeeFreeFinishes==b.FeeFreeFinishes &&
        a.BestFinish==b.BestFinish && a.BestFeeFreeFinish==b.BestFeeFreeFinish &&
        a.BestLap==b.BestLap && a.BestFeeFreeLap==b.BestFeeFreeLap &&
        a.MeanFinish==b.MeanFinish && a.FinishDeviation==b.FinishDeviation,"same measured entry");
}
void SameCoverage(const PracticeSetupCoverage& a,const PracticeSetupCoverage& b)
{
    Require(a.Tested==b.Tested && a.FeeFreeTested==b.FeeFreeTested &&
        a.Finishes==b.Finishes && a.FeeFreeFinishes==b.FeeFreeFinishes,"same coverage");
    SameEntry(a.FastestFinish,b.FastestFinish);
    SameEntry(a.FastestFeeFreeFinish,b.FastestFeeFreeFinish);
    SameEntry(a.FastestLap,b.FastestLap);
    SameEntry(a.FastestFeeFreeLap,b.FastestFeeFreeLap);
    SameEntry(a.LowestMean,b.LowestMean);
}
void SameCatalogue(const PracticeSetupCatalogue& a,const PracticeSetupCatalogue& b)
{
    Require(a.Valid==b.Valid,"catalogue availability");
    if(!a.Valid)return;
    Require(a.Preset==b.Preset && a.Handling==b.Handling,"same group");
    SameEntry(a.Selected,b.Selected);
    SameCoverage(a.All,b.All);
    for(std::size_t i=0;i<a.Cars.size();++i)SameCoverage(a.Cars[i],b.Cars[i]);
    for(std::size_t i=0;i<a.Engines.size();++i)SameCoverage(a.Engines[i],b.Engines[i]);
    for(std::size_t i=0;i<a.Grips.size();++i)SameCoverage(a.Grips[i],b.Grips[i]);
    for(std::size_t i=0;i<a.Entries.size();++i)SameEntry(a.Entries[i],b.Entries[i]);
}
void SamePlan(const PracticeExperimentPlan& a,const PracticeExperimentPlan& b)
{
    Require(a.Valid==b.Valid,"plan availability");
    if(!a.Valid)return;
    Require(a.Selected==b.Selected && a.Untested==b.Untested &&
        a.NoFeeFreeFinish==b.NoFeeFreeFinish && a.NeedsRepeats==b.NeedsRepeats &&
        a.Sufficient==b.Sufficient && a.ObservedFinishes==b.ObservedFinishes &&
        a.Count==b.Count,"same plan summary");
    SameCandidate(a.Current,b.Current);
    SameCandidate(a.NextAlternative,b.NextAlternative);
    SameCandidate(a.NextVehicleOnly,b.NextVehicleOnly);
    SameCandidate(a.NextEngineOnly,b.NextEngineOnly);
    SameCandidate(a.NextGripOnly,b.NextGripOnly);
    SameEntry(a.FastestObserved,b.FastestObserved);
    SameEntry(a.FastestFeeFree,b.FastestFeeFree);
    for(int i=0;i<a.Count;++i)
        SameCandidate(a.Candidates[static_cast<std::size_t>(i)],b.Candidates[static_cast<std::size_t>(i)]);
}
void CacheLifecycle()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    PracticeSetupAnalysisCache cache;
    const auto key=Key();
    Require(history->AnalysisRevision()!=0,"valid initial revision");
    const auto initial=history->AnalysisRevision();
    Require(cache.ReadCatalogue(*history,key).All.Tested==0,"empty real group");
    Require(cache.ReadStats().GroupBuilds==1 && cache.ReadStats().PlanBuilds==0,"lazy plan");
    for(int i=0;i<1000;++i)
    {
        Require(cache.ReadPlan(*history,key).Untested==96,"empty plan stable");
        Require(cache.ReadCatalogue(*history,key).All.Tested==0,"empty catalogue stable");
    }
    Require(cache.ReadStats().GroupBuilds==1 && cache.ReadStats().PlanBuilds==1,
        "idle HUD frames do not scan or sort");
    history->Begin(key,1);
    history->CancelAttempt();
    Require(history->AnalysisRevision()==initial,"cancel leaves evidence revision");
    history->Begin(key,1);
    history->ObserveConfiguration(Key(1),1);
    history->Finish(key,1,MakeRace(key.Preset,20));
    Require(history->AnalysisRevision()==initial,"rejected finish leaves revision");
    Require(cache.ReadPlan(*history,key).Untested==96 &&
        cache.ReadStats().PlanBuilds==1,"rejected finish no rebuild");
    Finish(*history,key,20);
    Require(history->AnalysisRevision()!=initial,"new evidence changes revision");
    Require(cache.ReadPlan(*history,key).Untested==95 &&
        cache.ReadStats().GroupBuilds==2 && cache.ReadStats().PlanBuilds==2,"valid finish refreshes");
    const auto alternative=Key(2,3,1);
    Require(cache.ReadPlan(*history,alternative).Selected==alternative &&
        cache.ReadStats().GroupBuilds==2 && cache.ReadStats().SelectionChanges==1 &&
        cache.ReadStats().PlanBuilds==3,"same-group selection avoids scan");
    auto other=alternative;other.Handling=1;
    Require(cache.ReadPlan(*history,other).Untested==96 &&
        cache.ReadStats().GroupBuilds==3,"handling changes evidence group");
    other.Preset=QuickPracticePreset::RivalTen;
    Require(cache.ReadPlan(*history,other).Untested==96 &&
        cache.ReadStats().GroupBuilds==4,"preset changes evidence group");
    Require(!cache.ReadCatalogue(*history,{}).Valid &&
        !cache.ReadPlan(*history,{}).Valid,"invalid selection clears data");
    Require(cache.ReadCatalogue(*history,key).All.Tested==1 &&
        cache.ReadStats().GroupBuilds==5,"valid selection recovers");
    const auto revision=history->AnalysisRevision();
    history->ResetSession();
    Require(history->AnalysisRevision()!=revision,"reset invalidates");
    Require(cache.ReadPlan(*history,key).Untested==96 &&
        cache.ReadStats().GroupBuilds==6,"reset refreshes");
    cache.Reset();
    Require(cache.ReadStats().GroupBuilds==0 && cache.ReadStats().PlanBuilds==0,
        "explicit cache reset");
    Require(cache.ReadPlan(*history,key).Valid && cache.ReadStats().GroupBuilds==1,
        "rebuild after reset");
}
void DistinctHistorySources()
{
    auto a=std::make_unique<PracticeSessionHistory>();
    auto b=std::make_unique<PracticeSessionHistory>();
    const auto key=Key();
    Finish(*a,key,20);
    Finish(*b,key,30);
    Require(a->AnalysisRevision()==b->AnalysisRevision(),"equal revisions in distinct stores");
    PracticeSetupAnalysisCache cache;
    Require(cache.ReadCatalogue(*a,key).Selected.BestFinish==60,"first source");
    Require(cache.ReadCatalogue(*b,key).Selected.BestFinish==90 &&
        cache.ReadStats().GroupBuilds==2,"source identity prevents cross-session evidence");
    Require(cache.ReadCatalogue(*a,key).Selected.BestFinish==60,"return to original");
}
void RandomizedReferenceEquivalence()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    PracticeSetupAnalysisCache cache;
    std::mt19937 rng(56825);
    const QuickPracticePreset presets[]={QuickPracticePreset::SoloThree,
        QuickPracticePreset::SoloFive,QuickPracticePreset::RivalFive,QuickPracticePreset::RivalTen};
    auto key=Key();
    for(int i=0;i<600;++i)
    {
        switch(rng()%8)
        {
        case 0:Finish(*history,key,20+static_cast<double>(rng()%20),static_cast<double>(rng()%3));break;
        case 1:history->Begin(key,1);history->CancelAttempt();break;
        case 2:key.Tuning.Selected=static_cast<int>(rng()%VehicleCount);break;
        case 3:key.Tuning.Engine=static_cast<int>(rng()%4);break;
        case 4:key.Tuning.Grip=static_cast<int>(rng()%4);break;
        case 5:key.Handling=1-key.Handling;break;
        case 6:key.Preset=presets[rng()%4];break;
        default:if(i%41==0)history->ResetSession();break;
        }
        const auto direct=BuildPracticeSetupCatalogue(*history,key);
        SameCatalogue(cache.ReadCatalogue(*history,key),direct);
        SamePlan(cache.ReadPlan(*history,key),BuildPracticeExperimentPlan(direct,key));
        const auto stats=cache.ReadStats();
        SamePlan(cache.ReadPlan(*history,key),BuildPracticeExperimentPlan(direct,key));
        Require(cache.ReadStats().GroupBuilds==stats.GroupBuilds &&
            cache.ReadStats().PlanBuilds==stats.PlanBuilds,"same frame reuse");
    }
}
}
int main()
{
    CacheLifecycle();DistinctHistorySources();RandomizedReferenceEquivalence();
    std::cout<<"Practice setup analysis cache tests passed\n";
}
