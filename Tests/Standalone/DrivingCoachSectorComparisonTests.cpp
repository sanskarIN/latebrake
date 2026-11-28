#include "Racing/DrivingCoachSectorComparison.h"

#include <algorithm>
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
        std::cerr << "DrivingCoachSectorComparison: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct Fixture final
{
    DrivingCoachAccumulator Chassis;
    DrivingCoachSectorJournal Journal;
    double Clock=0.0;
    int Ordinal=1;
    explicit Fixture(int gates=4){Journal.Reset(gates);}
    void Drive(int frames,double speed=2000.0,double lateral=0.0,
        bool brake=false,double clockRatio=1.0)
    {
        for(int i=0;i<frames;++i)
        {
            Require(Chassis.Observe({0.1,speed,lateral,brake}),
                "valid chassis sample");
            Clock+=0.1*clockRatio;
        }
    }
    bool Gate()
    {
        return Journal.ObserveGate({true,true,false,99,Ordinal++,4,
            Clock,Chassis.Review()});
    }
};
void OneSampleLapCannotInventPair()
{
    Fixture t;
    t.Drive(50);t.Gate();
    t.Drive(50);t.Gate();
    const auto summary=BuildCoachSectorPairOverview(t.Journal);
    Require(!summary.Valid && !ValidCoachSectorPairOverview(summary),
        "single observed gate interval has no earlier lap to compare");
}
void TwoLapsPermitSameGateSourceComparison()
{
    Fixture t;
    t.Drive(50);t.Gate();
    for(int i=0;i<8;++i)
    {
        const bool candidate=i>=4;
        t.Drive(50,candidate?2400.0:2000.0,
            candidate && (i%4)==1 ? 1000.0:0.0,
            candidate && (i%4)==2);
        Require(t.Gate(),"real gate-bound source interval accepted");
    }
    const auto all=BuildCoachSectorPairOverview(t.Journal);
    Require(ValidCoachSectorPairOverview(all) && all.ComparableGates==4 &&
        all.GateCount==4 && all.ChangedSpeedGates==4 &&
        all.LargestAbsoluteSpeedChange>0.0,
        "all four observed sectors compare their exact gate index");
    for(int gate=0;gate<4;++gate)
    {
        const auto& pair=all.Gates[static_cast<std::size_t>(gate)];
        Require(ValidCoachSectorLapPair(pair) && pair.GateIndex==gate &&
            pair.EarlierSequence<pair.NewerSequence &&
            pair.EarlierLap<pair.NewerLap &&
            pair.MeanSpeedKmh.Direction==CoachSectorDifference::HigherInNewer &&
            !pair.OfficialLapComparison &&
            !pair.CertifiedImprovement && !pair.MayAwardCareerCredits &&
            !pair.GuaranteedSameTrackConditions,
            "comparison describes speed change without claiming better lap");
    }
    Require(all.Gates[2].HighSlipPercent.Direction==
        CoachSectorDifference::HigherInNewer &&
        all.Gates[3].BrakingPercent.Direction==
            CoachSectorDifference::HigherInNewer,
        "braking and slip metrics tied to same gate interval in later lap");
}
void PartialCoverageIsNotCompared()
{
    Fixture t;
    t.Drive(50);t.Gate();
    for(int i=0;i<8;++i)
    {
        // One half-covered gate interval in newer lap must not hide behind
        // a previous, fully covered sample from the same index.
        const double multiplier=i==5 ? 2.0:1.0;
        t.Drive(50,1700.0,0.0,false,multiplier);
        Require(t.Gate(),"source keeps partially sampled gate entry");
    }
    const auto report=BuildCoachSectorPairOverview(t.Journal);
    Require(ValidCoachSectorPairOverview(report) && report.ComparableGates==3 &&
        !report.Gates[2].Valid,
        "under-covered newer interval cannot be compared to older lap");
}
void SameLapOrReversedSourceCannotPassDirectValidation()
{
    Fixture t;
    t.Drive(50);t.Gate();
    t.Drive(50);t.Gate();
    DrivingCoachSectorSample earlier;
    Require(t.Journal.ReadNewest(0,earlier),"source record exists");
    auto newer=earlier;
    newer.Sequence++;
    Require(!CompareCoachSectorSamples(earlier,newer,4).Valid,
        "same lap cannot be presented as an improvement comparison");
    newer.LapIndex++;
    newer.EndGateOrdinal+=4;
    newer.FirstGateOrdinal+=4;
    newer.StartClockSeconds+=100;
    newer.EndClockSeconds+=100;
    Require(ValidCoachSectorLapPair(
        CompareCoachSectorSamples(earlier,newer,4)),
        "separate ordered lap intervals can form sample-only pair");
    newer.ChassisCoverageFraction=0.1;
    Require(!CompareCoachSectorSamples(earlier,newer,4).Valid,
        "incomplete source cannot be used for same-gate comparison");
    newer=earlier;
    newer.Sequence++;
    newer.LapIndex++;
    newer.FirstGateOrdinal+=4;
    newer.EndGateOrdinal+=4;
    newer.StartClockSeconds+=100;
    newer.EndClockSeconds+=100;
    newer.AuthenticEngineWitness=true;
    Require(!CompareCoachSectorSamples(earlier,newer,4).Valid,
        "forged native engine provenance rejected");
}
void TamperedPairCannotGrantAuthority()
{
    Fixture t;
    t.Drive(50);t.Gate();
    for(int i=0;i<8;++i)
    {
        t.Drive(50);t.Gate();
    }
    auto report=BuildCoachSectorPairOverview(t.Journal);
    Require(ValidCoachSectorPairOverview(report),"original pair summary valid");
    report.Gates[0].MayAwardCareerCredits=true;
    Require(!ValidCoachSectorPairOverview(report),
        "cosmetic telemetry cannot grant career credits");
    report=BuildCoachSectorPairOverview(t.Journal);
    report.Gates[0].MeanSpeedKmh.NewerValue=
        std::numeric_limits<double>::infinity();
    Require(!ValidCoachSectorPairOverview(report),
        "nonfinite metrics fail closed");
    report=BuildCoachSectorPairOverview(t.Journal);
    ++report.ChangedBrakingGates;
    Require(!ValidCoachSectorPairOverview(report),
        "derived aggregate count cannot be invented");
    report=BuildCoachSectorPairOverview(t.Journal);
    report.NotAnOfficialRaceReview=false;
    Require(!ValidCoachSectorPairOverview(report),
        "falsely promoted race rankings are refused");
}
void RingExpiryUsesNewestComparableLapPairs()
{
    Fixture t;
    t.Drive(20);t.Gate();
    for(int i=0;i<200;++i)
    {
        t.Drive(20,1000.0+10*(i%20));
        Require(t.Gate(),"bounded contiguous source history");
    }
    const auto result=BuildCoachSectorPairOverview(t.Journal);
    Require(ValidCoachSectorPairOverview(result) &&
        result.ComparableGates==4,
        "older evicted runs cannot contaminate current lap comparison");
    for(int gate=0;gate<4;++gate)
    {
        const auto& pair=result.Gates[static_cast<std::size_t>(gate)];
        Require(pair.EarlierSequence>136 && pair.NewerSequence>136 &&
            pair.NewerSequence-pair.EarlierSequence==4,
            "newest two retained matching gate observations selected");
    }
}
void RandomizedMetricArithmeticRemainsDeterministic()
{
    std::mt19937 random{0x52CEB93};
    std::uniform_real_distribution<double> value(0.0,300.0);
    for(int i=0;i<60000;++i)
    {
        const double a=value(random),b=value(random);
        auto metric=MeasureCoachSectorChange(a,b,3.0);
        Require(ValidCoachSectorMetricChange(metric) &&
            std::abs(metric.Difference-(b-a))<1e-9 &&
            (metric.Direction==CoachSectorDifference::HigherInNewer)==
                (b-a>3.0) &&
            (metric.Direction==CoachSectorDifference::LowerInNewer)==
                (b-a< -3.0),
            "fuzzed metric comparisons preserve independent arithmetic oracle");
    }
    for(const auto bad:{-1.0,std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()})
    {
        Require(!MeasureCoachSectorChange(bad,3.0,3.0).Valid &&
            !MeasureCoachSectorChange(3.0,bad,3.0).Valid &&
            !MeasureCoachSectorChange(3.0,3.0,bad).Valid,
            "invalid comparative scalar cannot propagate to claims");
    }
}
}
int main()
{
    OneSampleLapCannotInventPair();
    TwoLapsPermitSameGateSourceComparison();
    PartialCoverageIsNotCompared();
    SameLapOrReversedSourceCannotPassDirectValidation();
    TamperedPairCannotGrantAuthority();
    RingExpiryUsesNewestComparableLapPairs();
    RandomizedMetricArithmeticRemainsDeterministic();
    std::cout << "Driving coach same-gate lap comparison tests passed.\n";
    return EXIT_SUCCESS;
}
