#include "Racing/DrivingCoachSectorOverview.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool value,const char* reason)
{
    if(!value)
    {
        std::cerr << "DrivingCoachSectorOverview: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double eps=1e-6)
{
    return std::abs(a-b)<=eps;
}
struct Track final
{
    DrivingCoachAccumulator Samples;
    DrivingCoachSectorJournal Journal;
    int NextOrdinal=1;
    double ActiveSeconds=0.0;
    double TimeMultiplier=1.0;
    std::uint64_t Actor=18;
    Track(){Journal.Reset(4);}
    void Drive(int frames,double speed,double slip=0.0,bool braking=false)
    {
        for(int i=0;i<frames;++i)
        {
            Require(Samples.Observe({0.10,speed,slip,braking}),
                "fixture always uses accepted speed observations");
            ActiveSeconds+=0.10*TimeMultiplier;
        }
    }
    bool Gate()
    {
        return Journal.ObserveGate({true,true,false,Actor,NextOrdinal++,4,
            ActiveSeconds,Samples.Review()});
    }
};
void NoGateNoSampledOverview()
{
    DrivingCoachSectorJournal empty;
    const auto a=BuildDrivingCoachSectorOverview(empty);
    Require(!a.Valid && !ValidCoachSectorOverview(a),
        "uninitialized source cannot invent interval samples");
    Track f;
    f.Drive(40,1400.0);f.Gate();
    Require(!BuildDrivingCoachSectorOverview(f.Journal).Valid,
        "single accepted gate not enough for an observed interval");
}
void OneIntervalIsSparseNotACompetitiveRank()
{
    Track t;
    t.Drive(30,1400.0);t.Gate();
    t.Drive(30,1400.0,900.0,true);
    Require(t.Gate(),"first measured interval created");
    const auto report=BuildDrivingCoachSectorOverview(t.Journal);
    Require(ValidCoachSectorOverview(report) && report.ObservedGates==1 &&
        report.RepresentativeGates==0 && report.SuggestedReviewGate==-1 &&
        report.Gates[1].Quality==CoachSectorObservationQuality::Sparse &&
        report.Gates[1].Focus==CoachSectorAdvisoryFocus::ObserveMore &&
        !report.Gates[1].EquatesToOfficialSectorTime &&
        report.NoRaceAuthority && report.NoCompetitiveRanking,
        "single high-slip sample never promoted to coaching consensus");
    Require(Near(report.OverallSampledSeconds,3.0) &&
        Near(report.Gates[1].WeightedBrakingFraction,1.0) &&
        Near(report.Gates[1].WeightedHighSlipFraction,1.0) &&
        Near(report.Gates[1].WeightedMeanSpeedKmh,50.4),
        "weighted sample metrics computed from actual chassis speed");
}
void RepeatedGateObservationsProduceQualifiedFocus()
{
    Track t;
    t.Drive(40,1900.0);t.Gate();
    for(int i=0;i<9;++i)
    {
        const int gate=i%4;
        const bool harsh=gate==1;
        t.Drive(50,harsh?1800.0:2000.0,
            harsh?900.0:60.0,gate==2);
        Require(t.Gate(),"each adjacent gate accepted");
    }
    const auto report=BuildDrivingCoachSectorOverview(t.Journal);
    Require(ValidCoachSectorOverview(report) && report.ObservedGates==4 &&
        report.RepresentativeGates>=4,
        "repeated independent lap segments have adequate source coverage");
    Require(report.Gates[2].Focus==CoachSectorAdvisoryFocus::ReviewLateralControl &&
        report.Gates[3].Focus==CoachSectorAdvisoryFocus::ReviewBrakingDistribution &&
        report.SuggestedReviewGate==2,
        "only measured repeated slip/brake exposure yields advisory focus");
    for(int i=0;i<4;++i)
    {
        const auto& gate=report.Gates[static_cast<std::size_t>(i)];
        Require(ValidCoachSectorAggregate(gate,i) &&
            gate.Quality==CoachSectorObservationQuality::Representative,
            "each repeated witnessed interval is independently reviewable");
    }
}
void PartialCoverageCannotPromoteInconsistentSector()
{
    Track f;
    f.Drive(40,2200);f.Gate();
    f.TimeMultiplier=2.0;
    for(int i=0;i<9;++i)
    {
        f.Drive(40,2200);
        Require(f.Gate(),"partial coverage still records only sampled fraction");
    }
    auto report=BuildDrivingCoachSectorOverview(f.Journal);
    Require(ValidCoachSectorOverview(report) &&
        report.RepresentativeGates==0 &&
        report.OverallCoverageFraction<0.6,
        "high sample count cannot disguise half-time observation coverage");
    for(const auto& row:report.Gates)
        if(row.Valid)
            Require(row.Quality==CoachSectorObservationQuality::PartiallySampled &&
                row.Focus==CoachSectorAdvisoryFocus::ObserveMore,
                "limited coverage withholds brake/slip coaching recommendation");
}
void WindowWrapOnlyRetainsCurrentSourceIntervals()
{
    Track t;
    t.Drive(10,2000);t.Gate();
    for(int i=0;i<190;++i)
    {
        t.Drive(20,2000+(i%7)*40);
        Require(t.Gate(),"long race sequence contains consistent gates");
    }
    const auto report=BuildDrivingCoachSectorOverview(t.Journal);
    Require(ValidCoachSectorOverview(report) &&
        report.TotalObservedIntervals==DrivingCoachSectorHistoryCapacity &&
        report.OverallSampledSeconds>0.0 &&
        report.ObservedGates==4 && report.RepresentativeGates==4,
        "ring expiry never counts inaccessible ancient sectors");
    std::uint64_t count=0;
    for(int i=0;i<4;++i)
        count+=report.Gates[static_cast<std::size_t>(i)].ObservedIntervals;
    Require(count==DrivingCoachSectorHistoryCapacity,
        "per-sector summary partitions exactly retained ring entries");
}
void FalsifiedReadoutCannotAuthorizeClaims()
{
    Track t;
    t.Drive(40,1000);t.Gate();
    t.Drive(50,1000);t.Gate();
    t.Drive(50,1000);t.Gate();
    auto report=BuildDrivingCoachSectorOverview(t.Journal);
    Require(ValidCoachSectorOverview(report),"valid baseline overview");
    report.NoCompetitiveRanking=false;
    Require(!ValidCoachSectorOverview(report),
        "claim of official competitive rank must be rejected");
    report=BuildDrivingCoachSectorOverview(t.Journal);
    report.Gates[1].WeightedMeanSpeedKmh=
        std::numeric_limits<double>::infinity();
    Require(!ValidCoachSectorOverview(report),
        "infinite speed cannot be used for advice");
    report=BuildDrivingCoachSectorOverview(t.Journal);
    report.Gates[1].EquatesToOfficialSectorTime=true;
    Require(!ValidCoachSectorOverview(report),
        "advisory sector readout cannot become an official split");
    report=BuildDrivingCoachSectorOverview(t.Journal);
    ++report.TotalObservedIntervals;
    Require(!ValidCoachSectorOverview(report),
        "counter mismatch cannot invent recorded intervals");
    report=BuildDrivingCoachSectorOverview(t.Journal);
    report.OverallSlipFraction= -0.1;
    Require(!ValidCoachSectorOverview(report),
        "negative fabricated lateral slip is refused");
}
void RandomizedWeightedSummaryConservation()
{
    std::mt19937 random{0x0041AD18};
    std::uniform_real_distribution<double> speed(300.0,3200.0);
    std::uniform_real_distribution<double> lateral(0.0,900.0);
    Track t;
    t.Drive(40,1400.0);t.Gate();
    for(int g=0;g<480;++g)
    {
        const int n=10+static_cast<int>(random()%35u);
        for(int f=0;f<n;++f)
        {
            Require(t.Samples.Observe({0.1,speed(random),lateral(random),
                random()%3u==0u}),"seeded randomized chassis readout");
            t.ActiveSeconds+=0.1;
        }
        Require(t.Gate(),"randomized adjacent gate accepted");
        const auto o=BuildDrivingCoachSectorOverview(t.Journal);
        Require(ValidCoachSectorOverview(o) &&
            o.TotalObservedIntervals==std::min<std::uint64_t>(
                g+1,DrivingCoachSectorHistoryCapacity) &&
            o.OverallSampledDistanceKm<=o.OverallSampledSeconds*0.1+1e-5,
            "time, distance and ring counts remain conserved at every gate");
        double time=0.0,distance=0.0;
        std::uint64_t count=0;
        for(int sector=0;sector<4;++sector)
        {
            const auto& row=o.Gates[static_cast<std::size_t>(sector)];
            if(!row.Valid)continue;
            time+=row.SampledSeconds;
            distance+=row.EstimatedSampledDistanceKm;
            count+=row.ObservedIntervals;
        }
        Require(Near(time,o.OverallSampledSeconds,1e-5) &&
            Near(distance,o.OverallSampledDistanceKm,1e-5) &&
            count==o.TotalObservedIntervals,
            "weighted sums preserve all observed samples exactly once");
    }
}
}
int main()
{
    NoGateNoSampledOverview();
    OneIntervalIsSparseNotACompetitiveRank();
    RepeatedGateObservationsProduceQualifiedFocus();
    PartialCoverageCannotPromoteInconsistentSector();
    WindowWrapOnlyRetainsCurrentSourceIntervals();
    FalsifiedReadoutCannotAuthorizeClaims();
    RandomizedWeightedSummaryConservation();
    std::cout << "Driving coach gate interval overview tests passed.\n";
    return EXIT_SUCCESS;
}
