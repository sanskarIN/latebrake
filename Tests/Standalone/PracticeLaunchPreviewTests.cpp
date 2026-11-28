#include "Racing/PracticeLaunchPreview.h"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* why)
{
    if(!condition){std::cerr<<"PracticeLaunchPreview: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
PracticeConfiguration Key(QuickPracticePreset preset=QuickPracticePreset::SoloThree,
    int vehicle=0,int engine=0,int grip=0,int handling=0)
{
    return {preset,{vehicle,engine,grip},handling};
}
PracticeTrialReadout Setup(PracticeConfiguration key,bool enabled=false,bool owned=true)
{
    return {enabled,key.Tuning,owned,PracticeTrialPerformance(key.Tuning)};
}
void Finish(PracticeSessionHistory& history,PracticeGateReferenceHistory& gates,
    PracticeConfiguration key,double lap,double fee=0,bool completeGates=true)
{
    const auto def=GetQuickPracticeDefinition(key.Preset);
    RaceProgress race(def.CheckpointCount,def.RequiredLaps);
    Require(history.Begin(key,1) && gates.Begin(key,1),"accepted attempt begin");
    int ordinal=0;
    for(int i=0;i<def.RequiredLaps;++i)
        for(int g=0;g<def.CheckpointCount;++g)
        {
            race.Advance(lap/def.CheckpointCount);
            if(i==0 && g==0 && fee>0)Require(race.AddTimePenalty(fee),"fee");
            Require(race.PassCheckpoint(g),"ordered gate");
            if(completeGates || ordinal!=0)gates.ObserveGate(key,1,race,g);
            ++ordinal;
        }
    const auto result=history.Finish(key,1,race);
    Require(result.Record.FinishedAttempts>0,"measured finish");
    gates.Finish(result,race);
}
void EmptyAndDefinitions()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    auto gates=std::make_unique<PracticeGateReferenceHistory>();
    constexpr QuickPracticePreset presets[]={QuickPracticePreset::SoloThree,
        QuickPracticePreset::SoloFive,QuickPracticePreset::RivalFive,QuickPracticePreset::RivalTen};
    const int laps[]={3,5,5,10};
    for(int p=0;p<4;++p)
        for(int car=0;car<VehicleCount;++car)
            for(int handling=0;handling<2;++handling)
            {
                auto key=Key(presets[p],car,3,2,handling);
                const auto view=BuildPracticeLaunchPreview(key,Setup(key,true,car==0),*history,*gates);
                Require(view.Valid && view.Key==key && view.Laps==laps[p] &&
                    view.Gates==4*laps[p] && view.Rivals==(p>=2?1:0),"exact event definition");
                Require(view.TemporaryTrial && view.Owned==(car==0),"trial ownership disclosure");
                Require(!view.HasMeasuredFinish && !view.HasFeeFreeFinish &&
                    !view.HasExactGateReference,"no fabricated session evidence");
                Require(view.Performance.Acceleration==PracticeTrialPerformance(key.Tuning).Acceleration,
                    "actual configured multiplier");
            }
    const auto key=Key();
    Require(!BuildPracticeLaunchPreview({},Setup(key),*history,*gates).Valid,"invalid key");
    Require(!BuildPracticeLaunchPreview(key,Setup(Key(QuickPracticePreset::SoloThree,1)),
        *history,*gates).Valid,"mismatched staged setup");
    Require(!BuildPracticeLaunchPreview(key,Setup(key,false,false),*history,*gates).Valid,
        "unowned nontrial setup rejected");
}
void MeasuredEvidenceAndReferenceGap()
{
    auto history=std::make_unique<PracticeSessionHistory>();
    auto gates=std::make_unique<PracticeGateReferenceHistory>();
    const auto key=Key(QuickPracticePreset::RivalTen,5,3,2,1);
    auto view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(view.Valid && !view.HasMeasuredFinish,"before first finish");
    Finish(*history,*gates,key,20,5);
    view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(view.HasMeasuredFinish && !view.HasFeeFreeFinish && view.HasExactGateReference &&
        view.Record.BestFinishSeconds==205 && view.GateReferenceSeconds==205 &&
        view.GateReferenceGapSeconds==0,"penalty-inclusive actual reference");
    Finish(*history,*gates,key,19,0,false);
    view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(view.HasFeeFreeFinish && view.Record.BestFinishSeconds==190 &&
        view.Series.FeeFreeFinish.Minimum==190 && view.GateReferenceSeconds==205 &&
        view.GateReferenceGapSeconds==15,"faster finish without complete gate evidence");
    Finish(*history,*gates,key,18);
    view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(view.Record.FinishedAttempts==3 && view.Record.BestFinishSeconds==180 &&
        view.GateReferenceSeconds==180 && view.GateReferenceGapSeconds==0,"new exact reference");
    auto other=key;other.Handling=0;
    Require(!BuildPracticeLaunchPreview(other,Setup(other,true,false),*history,*gates).HasMeasuredFinish,
        "different handling isolated");
    other=key;other.Tuning.Grip=1;
    Require(!BuildPracticeLaunchPreview(other,Setup(other,true,false),*history,*gates).HasExactGateReference,
        "different tuning isolated");
    gates->ResetSession();
    view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(view.HasMeasuredFinish && !view.HasExactGateReference,"missing gate data separate");
    Finish(*history,*gates,key,21);
    history->ResetSession();
    view=BuildPracticeLaunchPreview(key,Setup(key,true,false),*history,*gates);
    Require(!view.HasMeasuredFinish && !view.HasExactGateReference,"orphaned reference not displayed");
}
}
int main()
{
    EmptyAndDefinitions();MeasuredEvidenceAndReferenceGap();
    std::cout<<"Practice launch preview tests passed\n";
}
