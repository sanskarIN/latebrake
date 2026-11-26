#include "Racing/RaceProgress.h"
#include "Racing/FinishTimingEligibility.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <initializer_list>

namespace
{
using namespace latebrake;
void Check(bool value,const char* why)
{
    if(!value){std::cerr<<"MeasuredLapSplitReference: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b){return std::abs(a-b)<1e-8;}
void Gate(RaceProgress& race,double duration,double fee=0.0)
{
    race.Advance(duration);
    if(fee>0.0)Check(race.AddTimePenalty(fee),"charge real fee");
    Check(race.PassCheckpoint(race.GetNextCheckpoint()),"accept ordered gate");
}
void ReferenceAndDeltas()
{
    RaceProgress race(4,5);
    for(double t:{4.0,6.0,5.0,5.0})Gate(race,t);
    const auto& history=race.GetMeasuredLapSplits();
    Check(history.BestReference().Complete&&history.BestReference().LapNumber==1&&
        history.BestReference().LapSeconds==20.0&&!history.LastComparison().Valid,
        "first actual fee-free lap establishes reference without self-comparison");
    Gate(race,3.0);
    auto gate=history.LatestGate();
    Check(gate.Valid&&gate.ReferenceLapNumber==1&&gate.LapNumber==2&&
        gate.GateIndex==0&&Near(gate.CumulativeDeltaSeconds,-1.0),
        "same gate cumulative delta from actual first lap");
    race.Advance(2.0);
    Check(history.LatestGate().CumulativeDeltaSeconds==-1.0,
        "unfinished segment never changes measured checkpoint delta");
    Gate(race,2.0,1.0);
    gate=history.LatestGate();
    Check(gate.Valid&&Near(gate.ActualSectorSeconds,5.0)&&
        Near(gate.CumulativeDeltaSeconds,-2.0)&&Near(gate.IncludedFeeSeconds,1.0),
        "charged fee included in measured delta");
    Gate(race,4.0);Gate(race,4.0);
    const auto& paid=history.LastComparison();
    Check(paid.Valid&&paid.LapNumber==2&&paid.ReferenceLapNumber==1&&
        Near(paid.DeltaSeconds,-4.0)&&paid.FasterGates==4&&
        Near(paid.IncludedFeeSeconds,1.0)&&history.BestReference().LapNumber==1,
        "even faster penalized lap cannot replace fee-free reference");
    for(double t:{4.0,4.0,5.0,5.0})Gate(race,t);
    Check(history.LastComparison().Valid&&history.LastComparison().ReferenceLapNumber==1&&
        Near(history.LastComparison().DeltaSeconds,-2.0)&&
        history.LastComparison().LargestGainGate==1&&history.LastComparison().MatchedGates==3&&
        history.BestReference().LapNumber==3,
        "reference replacement occurs after comparison with previous lap");
    for(double t:{5.0,3.0,5.0,7.0})Gate(race,t);
    Check(history.LastComparison().Valid&&history.LastComparison().ReferenceLapNumber==3&&
        Near(history.LastComparison().DeltaSeconds,2.0)&&
        history.LastComparison().LargestLossGate==3&&
        history.LastComparison().LargestGainGate==1&&
        history.LastComparison().SlowerGates==2&&
        history.LastComparison().FasterGates==1,
        "gain/loss attribution sums to complete lap delta");
    for(double t:{4.0,4.0,5.0,5.0})Gate(race,t);
    Check(race.IsFinished()&&CanAwardFinishedRace(race)&&
        history.LastComparison().Valid&&history.LastComparison().MatchedGates==4&&
        history.BestReference().LapNumber==3&&Near(race.GetBestLapSeconds(),16.0),
        "tie retains first reference and official best semantics are unchanged");
    race.Reset();
    Check(!history.BestReference().Complete&&!history.LastComparison().Valid&&
        !history.LatestGate().Valid,"restart clears all advisory history");
}
void InvalidAndRecovery()
{
    RaceProgress race(2,6);
    Check(race.AddTimePenalty(3.0)&&race.PassCheckpoint(0),"fee only gate");
    Gate(race,5.0);
    Check(!race.GetMeasuredLapSplits().BestReference().Complete,
        "positive lap with penalty-only gate is not a reference");
    Gate(race,4.0);Gate(race,4.0);
    Check(race.GetMeasuredLapSplits().BestReference().LapNumber==2,
        "new physical lap recovers independent sector evidence");
    Gate(race,3.0);
    Check(race.GetMeasuredLapSplits().LatestGate().Valid,"valid current prefix");
    Check(race.PassCheckpoint(1),"zero-duration final gate");
    Check(!race.GetMeasuredLapSplits().LastComparison().Valid&&
        race.GetMeasuredLapSplits().BestReference().LapNumber==2,
        "zero-duration split invalidates whole comparison without replacing reference");
    Gate(race,4.0);Gate(race,4.0);
    Check(race.GetMeasuredLapSplits().LastComparison().Valid,"following lap valid");
    Check(!race.PassCheckpoint(1),"out of order gate rejected before observation");
    Check(!race.GetMeasuredLapSplits().LatestGate().Valid,"no false gate");
    Gate(race,5.0);Gate(race,4.0);
    Check(race.GetMeasuredLapSplits().LastComparison().LapNumber==5,"ordered lap preserved");
    Gate(race,4.0);Gate(race,4.0);
    Check(race.IsFinished()&&CanAwardFinishedRace(race),
        "advisory stricter evidence does not alter official eligibility");
}
void AllGateCounts()
{
    for(int gates=1;gates<=MeasuredLapMaxGates;++gates)
    {
        RaceProgress race(gates,3);
        double base=0.0,second=0.0,fees=0.0;
        for(int i=0;i<gates;++i)
        {
            const double t=2.0+static_cast<double>(i%7)*0.25;
            Gate(race,t);base+=t;
        }
        for(int i=0;i<gates;++i)
        {
            const double t=1.5+static_cast<double>(i%7)*0.25;
            const double fee=i%5==0?0.5:0.0;
            Gate(race,t,fee);second+=t+fee;fees+=fee;
            if(i+1<gates)
                Check(race.GetMeasuredLapSplits().LatestGate().Valid,
                    "all partial physical gate counts compare");
        }
        const auto& report=race.GetMeasuredLapSplits().LastComparison();
        Check(report.Valid&&report.GateCount==gates&&Near(report.ReferenceLapSeconds,base)&&
            Near(report.LapSeconds,second)&&Near(report.DeltaSeconds,second-base)&&
            Near(report.IncludedFeeSeconds,fees)&&
            report.FasterGates+report.SlowerGates+report.MatchedGates==gates,
            "all gate capacities reconcile exact fee and clock totals");
        double sum=0.0;
        for(int i=0;i<gates;++i)
        {
            const auto& row=report.Rows[static_cast<std::size_t>(i)];
            Check(row.Valid&&row.GateIndex==i,"every row corresponds to ordered gate");
            sum+=row.SectorDeltaSeconds;
            Check(Near(sum,row.CumulativeDeltaSeconds),"split sums match cumulative delta");
        }
        for(int i=0;i<gates;++i)Gate(race,3.0);
        Check(race.IsFinished()&&race.GetMeasuredLapSplits().LastComparison().Valid,
            "last race lap retains report");
    }
}
void PublicGateComparisonRequiresWholeEvidence()
{
    MeasuredLapSplitReference source;
    source.Reset(4);
    for (int i=0;i<4;++i)
        source.Observe(1,i,5.0*(i+1),0.0,true);
    source.Complete(1,20.0,0.0);
    const auto reference=source.BestReference();
    auto prefix=reference;
    prefix.LapNumber=2;
    prefix.Complete=false;
    prefix.GateCount=3;
    prefix.LapSeconds=0.0;
    Check(CompareMeasuredLapGate(prefix,reference,2).Valid,
        "valid actual prefix compares with fully valid reference");
    auto invalid=prefix;
    invalid.Gates[0].CumulativeSeconds=std::numeric_limits<double>::quiet_NaN();
    Check(!CompareMeasuredLapGate(invalid,reference,2).Valid,
        "earlier corrupt actual gate cannot be hidden by selected pair");
    invalid=reference;
    invalid.Gates[3].CumulativeSeconds=19.0;
    Check(!CompareMeasuredLapGate(prefix,invalid,0).Valid,
        "invalid later reference gate cannot be ignored");
    invalid=reference;
    invalid.LapSeconds=21.0;
    Check(!CompareMeasuredLapGate(prefix,invalid,0).Valid,
        "reference end clock must match its last gate");
    invalid=prefix;
    invalid.Complete=true;
    invalid.LapSeconds=15.0;
    Check(!CompareMeasuredLapGate(invalid,reference,0).Valid,
        "completed trace cannot have fewer gates than its reference");
    invalid=prefix;
    invalid.Gates[2].CumulativeFeeSeconds=16.0;
    Check(!CompareMeasuredLapGate(invalid,reference,0).Valid,
        "corrupt later actual prefix invalidates its earlier comparisons");
}
void PublicInputGuards()
{
    MeasuredLapSplitReference reference;
    reference.Reset(2);
    reference.Observe(1,1,3.0,0.0,true);
    reference.Complete(1,3.0,0.0);
    Check(!reference.BestReference().Complete,"wrong order cannot manufacture reference");
    reference.Observe(2,0,2.0,0.0,true);
    reference.Observe(2,1,4.0,0.0,true);
    reference.Complete(2,4.0,0.0);
    Check(reference.BestReference().Complete,"next lap independent");
    auto forged=reference.BestReference();
    forged.LapNumber=3;
    forged.Gates[0].CumulativeSeconds=std::numeric_limits<double>::quiet_NaN();
    Check(!CompareMeasuredLaps(forged,reference.BestReference()).Valid,"reject NaN trace");
    forged=reference.BestReference();forged.LapNumber=3;
    forged.GateCount=MeasuredLapMaxGates+1;
    Check(!CompareMeasuredLaps(forged,reference.BestReference()).Valid,"reject invalid capacity");
    forged=reference.BestReference();forged.LapNumber=3;
    forged.Gates[0].CumulativeFeeSeconds=9.0;
    Check(!CompareMeasuredLaps(forged,reference.BestReference()).Valid,"reject impossible fee");
    Check(!CompareMeasuredLaps(reference.BestReference(),reference.BestReference()).Valid,
        "never compare reference against itself");
}
}
int main()
{
    ReferenceAndDeltas();InvalidAndRecovery();AllGateCounts();PublicInputGuards();
    PublicGateComparisonRequiresWholeEvidence();
    std::cout<<"Actual fee-free lap reference and all 256 gate capacities passed\n";
    return EXIT_SUCCESS;
}
