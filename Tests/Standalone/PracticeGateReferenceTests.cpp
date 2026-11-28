#include "Racing/PracticeGateReference.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* message)
{
    if(!ok)
    {
        std::cerr<<"PracticeGateReference: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

PracticeConfiguration Key(QuickPracticePreset preset=QuickPracticePreset::SoloThree)
{
    return {preset,{0,0,0},0};
}
PracticeComparison Result(PracticeConfiguration key,
    PracticeComparisonStatus status=PracticeComparisonStatus::Improved)
{
    PracticeComparison out;
    out.Configuration=key;
    out.Status=status;
    return out;
}

void Record(PracticeGateReferenceHistory& journal,RaceProgress& race,
    PracticeConfiguration key,int actor,int gate,double seconds,double fee=0)
{
    race.Advance(seconds);
    if(fee>0)Require(race.AddTimePenalty(fee),"real fee");
    Require(race.PassCheckpoint(gate),"physical gate");
    Require(journal.ObserveGate(key,static_cast<std::uint64_t>(actor),race,gate),
        "real gate receipt");
}

void FirstAndRepeatedReference()
{
    PracticeGateReferenceHistory history;
    const auto key=Key();
    Require(history.Begin(key,11),"begin valid");
    RaceProgress first(4,3);
    for(int i=0;i<12;++i)Record(history,first,key,11,i%4,10);
    auto review=history.Finish(Result(key,PracticeComparisonStatus::First),first);
    Require(review.Finished && review.NewReference && !review.Valid &&
        !review.ReferenceAvailable && history.ReferenceFinish(key)==120,
        "first complete trace becomes reference without self comparison");
    Require(history.Begin(key,12) && history.Read().ReferenceAvailable &&
        history.Read().ReferenceFinishSeconds==120,"next matching run loads trace");
    RaceProgress second(4,3);
    for(int i=0;i<12;++i)
    {
        const double seconds=i==0?9:i==1?12:10;
        const double fee=i==0?3:0;
        Record(history,second,key,12,i%4,seconds,fee);
        const auto row=history.Read().Latest;
        Require(row.Valid && row.Ordinal==i+1 && row.Gate==i%4 &&
            row.Lap==i/4+1,"exact ordinal, lap and gate");
        if(i==0)
            Require(row.TotalDeltaSeconds==2 && row.ActiveDeltaSeconds==-1 &&
                row.FeeDeltaSeconds==3 && row.SectorDeltaSeconds==2,
                "penalty attributed separately");
        if(i==1)
            Require(row.TotalDeltaSeconds==4 && row.SectorDeltaSeconds==2 &&
                row.SectorFeeDeltaSeconds==0,"sector delta not cumulative delta");
    }
    review=history.Finish(Result(key,PracticeComparisonStatus::Slower),second);
    Require(review.Valid && review.Finished && !review.NewReference &&
        review.ObservedGates==12 && review.SlowerSectors==2 &&
        review.MatchedSectors==10 && review.FasterSectors==0 &&
        review.LargestLoss.Ordinal==1 && review.Latest.TotalDeltaSeconds==4,
        "slower run retains earlier reference");
    Require(history.ReferenceFinish(key)==120,"old best preserved");
    history.Begin(key,13);
    RaceProgress third(4,3);
    for(int i=0;i<12;++i)Record(history,third,key,13,i%4,9);
    review=history.Finish(Result(key),third);
    Require(review.Valid && review.NewReference && review.ReferenceFinishSeconds==120 &&
        review.Latest.TotalDeltaSeconds==-12 && review.FasterSectors==12 &&
        review.LargestGain.Ordinal==1 && history.ReferenceFinish(key)==108,
        "new reference and previous comparison remain distinct");
    Require(!history.Finish(Result(key),third).Valid,"duplicate finish inert");
    history.CancelAttempt();
    Require(history.ReferenceFinish(key)==108 && !history.Read().Valid,
        "cancel clears only current state");
}

void InvalidGateAndConfiguration()
{
    PracticeGateReferenceHistory history;
    auto key=Key();
    history.Begin(key,1);
    RaceProgress race(4,3);
    Record(history,race,key,1,0,10);
    Require(!history.ObserveGate(key,1,race,0) && history.Read().EvidenceRejected,
        "duplicate ordinal refused");
    race.Advance(10);race.PassCheckpoint(1);
    Require(!history.ObserveGate(key,1,race,1),"rejected trace cannot recover mid attempt");
    history.Begin(key,1);
    RaceProgress gap(4,3);
    gap.Advance(10);gap.PassCheckpoint(0);gap.Advance(10);gap.PassCheckpoint(1);
    Require(!history.ObserveGate(key,1,gap,1),"skipped receipt rejected");
    history.Begin(key,1);
    RaceProgress feeOnly(4,3);
    Require(feeOnly.AddTimePenalty(2) && feeOnly.PassCheckpoint(0),"penalty-only gate");
    Require(!history.ObserveGate(key,1,feeOnly,0),"fee alone is not active sector time");
    history.Begin(key,1);
    RaceProgress wrongShape(4,5);
    wrongShape.Advance(10);wrongShape.PassCheckpoint(0);
    Require(!history.ObserveGate(key,1,wrongShape,0),"wrong practice shape");
    history.Begin(key,1);
    auto other=key;other.Handling=1;
    history.ObserveConfiguration(other,1);
    history.ObserveConfiguration(key,1);
    Require(history.Read().EvidenceRejected,"switch-back cannot repair mixed configuration");
    history.Begin(key,1);
    history.ObserveConfiguration(key,2);
    Require(history.Read().EvidenceRejected,"replaced actor invalidates");
    history.Begin(key,1);
    RaceProgress complete(4,3);
    for(int i=0;i<12;++i)Record(history,complete,key,1,i%4,10);
    auto result=Result(other);
    Require(history.Finish(result,complete).EvidenceRejected && history.ReferenceFinish(key)==0,
        "mismatched finish authority refused");
    history.Begin(key,1);
    RaceProgress incomplete(4,3);
    Record(history,incomplete,key,1,0,10);
    Require(history.Finish(Result(key),incomplete).EvidenceRejected,
        "partial attempt cannot be a reference");
    Require(!history.Begin({},1) && !history.Begin(key,0),"invalid begin");
}

void AllPresetsAndConfigurationIsolation()
{
    PracticeGateReferenceHistory history;
    constexpr std::array<QuickPracticePreset,4> presets{
        QuickPracticePreset::SoloThree,QuickPracticePreset::SoloFive,
        QuickPracticePreset::RivalFive,QuickPracticePreset::RivalTen};
    for(auto preset:presets)
    for(int car=0;car<VehicleCount;++car)
    for(int engine=0;engine<4;++engine)
    for(int grip=0;grip<4;++grip)
    for(int handling=0;handling<2;++handling)
    {
        PracticeConfiguration key{preset,{car,engine,grip},handling};
        const auto def=GetQuickPracticeDefinition(preset);
        const int count=def.CheckpointCount*def.RequiredLaps;
        Require(history.ReferenceFinish(key)==0 && history.Begin(key,1),"independent slot");
        RaceProgress race(def.CheckpointCount,def.RequiredLaps);
        const double time=10+PracticeConfigurationIndex(key);
        for(int i=0;i<count;++i)Record(history,race,key,1,i%def.CheckpointCount,time);
        const auto review=history.Finish(Result(key,PracticeComparisonStatus::First),race);
        Require(review.NewReference && review.ObservedGates==count &&
            history.ReferenceFinish(key)==time*count,"bounded exact trace");
    }
    for(auto preset:presets)
    for(int car=0;car<VehicleCount;++car)
    for(int engine=0;engine<4;++engine)
    for(int grip=0;grip<4;++grip)
    for(int handling=0;handling<2;++handling)
    {
        PracticeConfiguration key{preset,{car,engine,grip},handling};
        const auto def=GetQuickPracticeDefinition(preset);
        Require(history.ReferenceFinish(key)==(10+PracticeConfigurationIndex(key))*
            def.CheckpointCount*def.RequiredLaps,"all references retained independently");
    }
    history.ResetSession();
    Require(history.ReferenceFinish(Key())==0 && !history.Read().Valid,"session reset");
}

void RandomizedSectorAccounting()
{
    PracticeGateReferenceHistory history;
    const auto key=Key(QuickPracticePreset::RivalTen);
    std::mt19937 rng(3491);
    for(int sample=0;sample<100;++sample)
    {
        history.ResetSession();
        history.Begin(key,1);
        RaceProgress ref(4,10);
        std::array<double,40> times{},fees{};
        double refTotal=0,refFee=0;
        for(int i=0;i<40;++i)
        {
            times[static_cast<std::size_t>(i)]=5+static_cast<double>(rng()%32)/4;
            fees[static_cast<std::size_t>(i)]=rng()%5==0?2:0;
            Record(history,ref,key,1,i%4,times[static_cast<std::size_t>(i)],
                fees[static_cast<std::size_t>(i)]);
            refTotal+=times[static_cast<std::size_t>(i)]+fees[static_cast<std::size_t>(i)];
            refFee+=fees[static_cast<std::size_t>(i)];
        }
        Require(history.Finish(Result(key,PracticeComparisonStatus::First),ref).NewReference,
            "reference ready");
        history.Begin(key,2);
        RaceProgress run(4,10);
        int faster=0,slower=0,matched=0;
        double total=0,feeTotal=0,refPrefix=0,refFeePrefix=0;
        for(int i=0;i<40;++i)
        {
            const double time=5+static_cast<double>(rng()%32)/4;
            const double fee=rng()%5==0?2:0;
            const auto index=static_cast<std::size_t>(i);
            Record(history,run,key,2,i%4,time,fee);
            total+=time+fee;feeTotal+=fee;
            refPrefix+=times[index]+fees[index];refFeePrefix+=fees[index];
            const double sector=time+fee-times[index]-fees[index];
            if(sector<0)++faster;else if(sector>0)++slower;else ++matched;
            const auto row=history.Read().Latest;
            Require(row.TotalDeltaSeconds==total-refPrefix &&
                row.FeeDeltaSeconds==feeTotal-refFeePrefix &&
                row.ActiveDeltaSeconds==total-feeTotal-(refPrefix-refFeePrefix) &&
                row.SectorDeltaSeconds==sector &&
                row.SectorFeeDeltaSeconds==fee-fees[index],"independent gate clock oracle");
        }
        auto review=history.Finish(Result(key),run);
        Require(review.Valid && review.ObservedGates==40 && review.FasterSectors==faster &&
            review.SlowerSectors==slower && review.MatchedSectors==matched &&
            review.Latest.TotalDeltaSeconds==total-refTotal &&
            review.Latest.FeeDeltaSeconds==feeTotal-refFee &&
            review.NewReference==(total<refTotal) &&
            history.ReferenceFinish(key)==std::min(total,refTotal),
            "complete ten-lap comparison and best reference selection");
    }
}
}
int main()
{
    FirstAndRepeatedReference();
    InvalidGateAndConfiguration();
    AllPresetsAndConfigurationIsolation();
    RandomizedSectorAccounting();
    std::cout<<"Practice exact gate reference tests passed\n";
}
