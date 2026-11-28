#include "Racing/PracticeSessionHistory.h"

#include <array>
#include <cstdlib>
#include <iostream>

namespace
{
using namespace latebrake;
void Require(bool value,const char* why)
{
    if(!value)
    {
        std::cerr<<"PracticeSessionHistory: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
RaceProgress RaceFor(QuickPracticePreset preset,double lapSeconds,double fee=0.0)
{
    const auto def=GetQuickPracticeDefinition(preset);
    RaceProgress race(def.CheckpointCount,def.RequiredLaps);
    for(int lap=0;lap<def.RequiredLaps;++lap)
    for(int gate=0;gate<def.CheckpointCount;++gate)
    {
        race.Advance(lapSeconds/def.CheckpointCount);
        if(lap==0 && gate==0 && fee>0) Require(race.AddTimePenalty(fee),"real fee");
        Require(race.PassCheckpoint(gate),"ordered gate");
    }
    return race;
}

PracticeConfiguration Key(QuickPracticePreset preset=QuickPracticePreset::SoloThree)
{
    return {preset,{0,0,0},0};
}

void RepeatedRunsAndDeltas()
{
    PracticeSessionHistory history;
    auto key=Key();
    Require(history.Begin(key,101),"valid attempt");
    auto result=history.Finish(key,101,RaceFor(key.Preset,20));
    Require(result.Status==PracticeComparisonStatus::First &&
        result.Record.FinishedAttempts==1 && result.Record.BestFinishSeconds==60 &&
        result.Record.BestLapSeconds==20 && result.PreviousFinishSeconds==0,"first timed finish");
    Require(history.Finish(key,101,RaceFor(key.Preset,10)).Status==
        PracticeComparisonStatus::Unavailable && history.Read(key).FinishedAttempts==1,
        "duplicate finish not recorded");
    Require(history.Begin(key,102),"new actor allowed only for new attempt");
    result=history.Finish(key,102,RaceFor(key.Preset,19,1));
    Require(result.Status==PracticeComparisonStatus::Improved &&
        result.Record.BestFinishSeconds==58 && result.Record.LastPenaltySeconds==1 &&
        result.DeltaToPreviousBestSeconds==-2 && result.DeltaToPreviousFinishSeconds==-2,
        "exact penalty-inclusive improvement");
    history.Begin(key,102);
    result=history.Finish(key,102,RaceFor(key.Preset,21));
    Require(result.Status==PracticeComparisonStatus::Slower &&
        result.Record.BestFinishSeconds==58 && result.Record.LastFinishSeconds==63 &&
        result.DeltaToPreviousBestSeconds==5 && result.DeltaToPreviousFinishSeconds==5,
        "slower attempt preserves earlier best");
    history.Begin(key,102);
    result=history.Finish(key,102,RaceFor(key.Preset,19,1));
    Require(result.Status==PracticeComparisonStatus::Matched &&
        result.DeltaToPreviousBestSeconds==0 && result.DeltaToPreviousFinishSeconds==-5 &&
        result.Record.FinishedAttempts==4,"comparison with previous and best are distinct");
    Require(history.LastComparison().Status==PracticeComparisonStatus::Matched,"results frozen");
    history.CancelAttempt();
    Require(history.LastComparison().Status==PracticeComparisonStatus::Unavailable &&
        history.Read(key).FinishedAttempts==4,"abandoned run preserves past comparable records");
    history.ResetSession();
    Require(history.Read(key).FinishedAttempts==0,"session reset clears records");
}

void MixedConfigurationAndMissingEvidence()
{
    PracticeSessionHistory history;
    auto key=Key(QuickPracticePreset::RivalFive);
    auto changed=key;changed.Tuning.Engine=1;
    auto race=RaceFor(key.Preset,20);
    history.Begin(key,101);
    history.ObserveConfiguration(changed,101);
    history.ObserveConfiguration(key,101);
    Require(history.Finish(key,101,race).Status==PracticeComparisonStatus::ConfigurationChanged &&
        history.Read(key).FinishedAttempts==0,"tuning rollback cannot requalify");
    history.Begin(key,101);
    history.ObserveConfiguration(key,102);
    Require(history.Finish(key,101,race).Status==PracticeComparisonStatus::ConfigurationChanged,
        "driver substitution permanently invalidates run");
    history.Begin(key,101);
    history.InvalidateConfiguration();
    Require(history.Finish(key,101,race).Status==PracticeComparisonStatus::ConfigurationChanged,
        "handling change notification");
    history.Begin(key,101);
    Require(history.Finish(key,101,RaceFor(QuickPracticePreset::SoloThree,10)).Status==
        PracticeComparisonStatus::Unavailable,"wrong actual lap count");
    history.Begin(key,101);
    RaceProgress incomplete(4,5);
    incomplete.Advance(5);
    Require(history.Finish(key,101,incomplete).Status==PracticeComparisonStatus::Unavailable,
        "incomplete route not recorded");
    history.Begin(key,101);
    RaceProgress untimed(4,5);
    for(int i=0;i<5;++i)
    for(int gate=0;gate<4;++gate)Require(untimed.PassCheckpoint(gate),"untimed route");
    Require(history.Finish(key,101,untimed).Status==PracticeComparisonStatus::Unavailable,
        "untimed ordered finish not comparable");
    Require(history.Read(key).FinishedAttempts==0,"refused attempts never affect reference");
    Require(!history.Begin(key,0) && !history.Begin({},101),"no invalid attempt");
    Require(history.Begin(key,101) &&
        history.Finish(key,101,race).Status==PracticeComparisonStatus::First,
        "next real attempt recovers cleanly");
}

void AllConfigurationsAreIndependent()
{
    PracticeSessionHistory history;
    std::array<bool,PracticeConfigurationCount> seen{};
    constexpr std::array<QuickPracticePreset,4> presets{
        QuickPracticePreset::SoloThree,QuickPracticePreset::SoloFive,
        QuickPracticePreset::RivalFive,QuickPracticePreset::RivalTen};
    for(auto preset:presets)
    for(int car=0;car<VehicleCount;++car)
    for(int engine=0;engine<4;++engine)
    for(int grip=0;grip<4;++grip)
    for(int handling=0;handling<2;++handling)
    {
        const PracticeConfiguration key{preset,{car,engine,grip},handling};
        const int index=PracticeConfigurationIndex(key);
        Require(index>=0 && index<static_cast<int>(seen.size()) &&
            !seen[static_cast<std::size_t>(index)],"bijective slot index");
        seen[static_cast<std::size_t>(index)]=true;
        Require(history.Read(key).FinishedAttempts==0,"distinct configuration empty");
        Require(history.Begin(key,101),"valid configuration");
        const auto result=history.Finish(key,101,RaceFor(preset,20+index));
        Require(result.Status==PracticeComparisonStatus::First &&
            result.Record.FinishedAttempts==1,"each configuration gets own record");
    }
    for(bool exists:seen)Require(exists,"complete fixed index coverage");
    for(auto preset:presets)
    for(int car=0;car<VehicleCount;++car)
    for(int engine=0;engine<4;++engine)
    for(int grip=0;grip<4;++grip)
    for(int handling=0;handling<2;++handling)
    {
        const PracticeConfiguration key{preset,{car,engine,grip},handling};
        const int index=PracticeConfigurationIndex(key);
        const auto record=history.Read(key);
        Require(record.FinishedAttempts==1 &&
            record.BestFinishSeconds==(20+index)*GetQuickPracticeDefinition(preset).RequiredLaps,
            "previous independent record preserved");
    }
    for(int invalid:{-1,4,99})
    {
        auto key=Key();key.Tuning.Engine=invalid;
        Require(PracticeConfigurationIndex(key)<0 && history.Read(key).FinishedAttempts==0,
            "invalid engine level rejected");
        key=Key();key.Tuning.Grip=invalid;
        Require(PracticeConfigurationIndex(key)<0,"invalid grip level rejected");
        key=Key();key.Handling=invalid;
        Require(PracticeConfigurationIndex(key)<0,"invalid handling rejected");
    }
    auto key=Key();key.Tuning.Selected=VehicleCount;
    Require(PracticeConfigurationIndex(key)<0,"invalid car rejected");
    key=Key();key.Preset=static_cast<QuickPracticePreset>(255);
    Require(PracticeConfigurationIndex(key)<0,"invalid preset rejected");
}
}
int main()
{
    RepeatedRunsAndDeltas();
    MixedConfigurationAndMissingEvidence();
    AllConfigurationsAreIndependent();
    std::cout<<"Practice session comparison history passed\n";
}
