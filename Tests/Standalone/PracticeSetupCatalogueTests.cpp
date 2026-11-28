#include "Racing/PracticeSetupCatalogue.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"PracticeSetupCatalogue: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
void Finish(PracticeSessionHistory& history,PracticeConfiguration key,double lap,double fee=0)
{
    const auto def=GetQuickPracticeDefinition(key.Preset);
    RaceProgress race(def.CheckpointCount,def.RequiredLaps);
    for(int i=0;i<def.RequiredLaps;++i)
        for(int g=0;g<def.CheckpointCount;++g)
        {
            race.Advance(lap/def.CheckpointCount);
            if(i==0 && g==0 && fee>0)Require(race.AddTimePenalty(fee),"fee");
            Require(race.PassCheckpoint(g),"ordered route");
        }
    Require(history.Begin(key,1),"begin configuration");
    const auto result=history.Finish(key,1,race);
    Require(result.Record.FinishedAttempts>0,"timed finish");
}
PracticeConfiguration Key(int car=0,int engine=0,int grip=0)
{
    return {QuickPracticePreset::SoloThree,{car,engine,grip},0};
}

void EmptyAndIsolation()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    const auto selected=Key();
    auto cat=BuildPracticeSetupCatalogue(*history,selected);
    Require(cat.Valid && cat.All.Tested==0 && !cat.Selected.Valid,"empty valid catalogue");
    Require(RankPracticeSetups(cat,PracticeSetupMetric::BestFinish).Count==0,"empty ranking");
    Require(!ReviewPracticeAxis(cat,PracticeSetupAxis::Engine,PracticeSetupMetric::BestFinish).Valid,
        "no baseline comparison");
    auto unrelated=selected;unrelated.Handling=1;
    Finish(*history,unrelated,10);
    unrelated=selected;unrelated.Preset=QuickPracticePreset::SoloFive;
    Finish(*history,unrelated,10);
    Require(BuildPracticeSetupCatalogue(*history,selected).All.Tested==0,"different mode excluded");
    Finish(*history,selected,20,3);
    auto alternative=Key(1);
    Finish(*history,alternative,21);
    Finish(*history,Key(0,1),18);
    cat=BuildPracticeSetupCatalogue(*history,selected);
    Require(cat.All.Tested==3 && cat.All.Finishes==3 && cat.All.FeeFreeTested==2 &&
        cat.All.FeeFreeFinishes==2,"group coverage");
    Require(cat.Selected.Valid && cat.Selected.BestFinish==63 &&
        cat.Selected.BestFeeFreeFinish==0,"selected measured evidence");
    Require(cat.Cars[0].Tested==2 && cat.Cars[1].Tested==1 &&
        cat.Engines[0].Tested==2 && cat.Engines[1].Tested==1 &&
        cat.Grips[0].Tested==3,"independent grouping");
    Require(cat.All.FastestFinish.Key==Key(0,1) &&
        cat.All.FastestFeeFreeFinish.Key==Key(0,1),"fastest observed configuration");
    Require(!cat.At(0,0,1).Valid && !cat.At(99,0,0).Valid,"no fabricated rows");
    Require(!BuildPracticeSetupCatalogue(*history,{}).Valid,"invalid group");
}
void MetricsAndTies()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    Finish(*history,Key(),20,6);
    Finish(*history,Key(),23);
    Finish(*history,Key(1),22);
    Finish(*history,Key(2),20);
    Finish(*history,Key(2),30);
    const auto cat=BuildPracticeSetupCatalogue(*history,Key(1));
    const auto best=RankPracticeSetups(cat,PracticeSetupMetric::BestFinish);
    Require(best.Count==3 && best.Rows[0].Key==Key(2) &&
        best.Rows[1].Key==Key() && best.Rows[2].Key==Key(1) &&
        best.SelectedRank==3,"deterministic equal time ordering");
    const auto clean=RankPracticeSetups(cat,PracticeSetupMetric::BestFeeFreeFinish);
    Require(clean.Count==3 && clean.Rows[0].Key==Key(2) &&
        clean.Rows[1].Key==Key(1) && clean.Rows[2].Key==Key(),"clean ranking distinct");
    const auto mean=RankPracticeSetups(cat,PracticeSetupMetric::MeanFinish);
    Require(mean.Rows[0].Key==Key(1) && mean.SelectedRank==1,"mean not best finish");
    const auto laps=RankPracticeSetups(cat,PracticeSetupMetric::BestLap);
    Require(laps.Rows[0].Key==Key() && laps.Rows[1].Key==Key(2),"best lap tie stable");
    const auto pair=ComparePracticeSetups(cat.Selected,cat.At(2,0,0),PracticeSetupMetric::BestFinish);
    Require(pair.Valid && pair.DeltaSeconds==-6 && pair.DeltaPercent()<0,"signed candidate difference");
    Require(!ComparePracticeSetups(cat.Selected,cat.Selected,PracticeSetupMetric::BestFinish).Valid,
        "self not alternative");
    auto mismatched=cat.At(2,0,0);mismatched.Key.Handling=1;
    Require(!ComparePracticeSetups(cat.Selected,mismatched,PracticeSetupMetric::BestFinish).Valid,
        "handling mismatch");
    Require(!RankPracticeSetups(cat,PracticeSetupMetric::BestFinish,VehicleCount).Valid,"bad filter");
    Require(RankPracticeSetups(cat,PracticeSetupMetric::BestFinish,0).Count==1,"car filter");
}
void SingleAxisComparisons()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    Finish(*history,Key(2,1,1),20);
    Finish(*history,Key(2,0,1),21);
    Finish(*history,Key(2,2,1),18);
    Finish(*history,Key(2,3,1),19,2);
    Finish(*history,Key(2,1,0),22);
    Finish(*history,Key(2,1,2),20);
    Finish(*history,Key(4,1,1),19);
    Finish(*history,Key(5,1,1),21);
    Finish(*history,Key(5,3,3),10); // deliberately confounded
    const auto cat=BuildPracticeSetupCatalogue(*history,Key(2,1,1));
    const auto engine=ReviewPracticeAxis(cat,PracticeSetupAxis::Engine,PracticeSetupMetric::BestFinish);
    Require(engine.Valid && engine.TestedAlternatives==3 && engine.ComparableAlternatives==3 &&
        engine.FastestAlternative.Candidate.Key==Key(2,2,1) &&
        engine.FastestAlternative.DeltaSeconds==-6 &&
        engine.SlowestAlternative.DeltaSeconds==3,"engine-only evidence");
    const auto clean=ReviewPracticeAxis(cat,PracticeSetupAxis::Engine,PracticeSetupMetric::BestFeeFreeFinish);
    Require(clean.ComparableAlternatives==2 && clean.TestedAlternatives==3,"no clean finish excludes row");
    const auto grip=ReviewPracticeAxis(cat,PracticeSetupAxis::Grip,PracticeSetupMetric::BestFinish);
    Require(grip.TestedAlternatives==2 && grip.FastestAlternative.DeltaSeconds==0 &&
        grip.SlowestAlternative.DeltaSeconds==6,"ties and slower alternatives");
    const auto car=ReviewPracticeAxis(cat,PracticeSetupAxis::Vehicle,PracticeSetupMetric::BestFinish);
    Require(car.TestedAlternatives==2 && car.FastestAlternative.Candidate.Key==Key(4,1,1),
        "multi-axis fastest cannot enter car-only comparison");
    Require(!ReviewPracticeAxis(cat,static_cast<PracticeSetupAxis>(255),PracticeSetupMetric::BestFinish).Valid,
        "invalid axis");
}
void FullCatalogueCoverage()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    std::mt19937 rng(52176);
    for(int car=0;car<VehicleCount;++car)
        for(int engine=0;engine<4;++engine)
            for(int grip=0;grip<4;++grip)
                Finish(*history,Key(car,engine,grip),20+static_cast<double>(rng()%50),car%2?1:0);
    const auto cat=BuildPracticeSetupCatalogue(*history,Key(3,2,1));
    Require(cat.All.Tested==96 && cat.All.Finishes==96 && cat.All.FeeFreeTested==48,"full setup coverage");
    for(const auto& car:cat.Cars)Require(car.Tested==16,"every car tested 16 setups");
    for(const auto& engine:cat.Engines)Require(engine.Tested==24,"every engine tested 24 setups");
    for(const auto& grip:cat.Grips)Require(grip.Tested==24,"every grip tested 24 setups");
    constexpr PracticeSetupMetric metrics[]={PracticeSetupMetric::BestFinish,
        PracticeSetupMetric::BestFeeFreeFinish,PracticeSetupMetric::BestLap,
        PracticeSetupMetric::BestFeeFreeLap,PracticeSetupMetric::MeanFinish};
    for(auto metric:metrics)
    {
        const auto rank=RankPracticeSetups(cat,metric);
        Require(rank.Valid && rank.Count==(metric==PracticeSetupMetric::BestFeeFreeFinish?48:96),
            "complete metric rows");
        for(int i=1;i<rank.Count;++i)
            Require(!BetterPracticeSetup(rank.Rows[static_cast<std::size_t>(i)],
                rank.Rows[static_cast<std::size_t>(i-1)],metric),"sorted exact measured times");
        for(int i=0;i<rank.Count;++i)
            for(int j=0;j<i;++j)
                Require(!(rank.Rows[static_cast<std::size_t>(i)].Key==
                    rank.Rows[static_cast<std::size_t>(j)].Key),"unique row");
    }
    history->ResetSession();
    Require(BuildPracticeSetupCatalogue(*history,Key()).All.Tested==0,"session reset clears catalogue");
}
}
int main()
{
    EmptyAndIsolation();MetricsAndTies();SingleAxisComparisons();FullCatalogueCoverage();
    std::cout<<"Practice setup catalogue tests passed\n";
}
