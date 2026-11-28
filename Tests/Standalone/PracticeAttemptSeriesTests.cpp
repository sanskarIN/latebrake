#include "Racing/PracticeSessionHistory.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <vector>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"PracticeAttemptSeries: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<=tolerance*std::max(1.0,std::max(std::abs(a),std::abs(b)));
}
RaceProgress Race(double lap,int laps,int feeLap=-1,double fee=0)
{
    RaceProgress race(4,laps);
    for(int i=0;i<laps;++i)
        for(int gate=0;gate<4;++gate)
        {
            race.Advance(lap/4);
            if(i==feeLap && gate==0 && fee>0)Require(race.AddTimePenalty(fee),"fee applied");
            Require(race.PassCheckpoint(gate),"physical order");
        }
    return race;
}
void SampleEvidence()
{
    auto clean=MakePracticeAttemptSample(Race(20,3));
    Require(clean.Valid && clean.FeeFreeFinish() && clean.FeeFreeLaps==3 &&
        clean.BestFeeFreeLapSeconds==20 && clean.FinishSeconds==60,"real clean race");
    auto fee=MakePracticeAttemptSample(Race(21,3,1,4));
    Require(fee.Valid && !fee.FeeFreeFinish() && fee.FeeFreeLaps==2 &&
        fee.FeeSeconds==4 && fee.ActiveSeconds()==63 &&
        fee.BestFeeFreeLapSeconds==21,"real penalized race");
    RaceProgress unfinished(4,3);
    unfinished.Advance(12);
    Require(!MakePracticeAttemptSample(unfinished).Valid,"unfinished not sampled");
    RaceProgress untimed(4,3);
    for(int i=0;i<3;++i)for(int g=0;g<4;++g)untimed.PassCheckpoint(g);
    Require(!MakePracticeAttemptSample(untimed).Valid,"zero clock not sampled");
    Require(!MakePracticeAttemptSample(Race(20,11)).Valid,"unsupported practice length");
}
void KnownMomentsAndRecentRing()
{
    PracticeAttemptSeries series;
    Require(!series.Read().Valid,"no manufactured statistics");
    Require(series.Observe(MakePracticeAttemptSample(Race(20,3))),"first");
    Require(series.Observe(MakePracticeAttemptSample(Race(21,3,1,4))),"second");
    Require(series.Observe(MakePracticeAttemptSample(Race(19,3))),"third");
    const auto view=series.Read();
    Require(view.Valid && view.Finishes==3 && view.FeeFreeFinishes==2,"counts");
    Require(view.CompletedLaps==9 && view.FeeFreeLaps==8,"physical lap counts");
    Require(Near(view.Finish.Mean,184.0/3) && Near(view.Active.Mean,60) &&
        Near(view.Fees.Mean,4.0/3),"fee attribution and means");
    Require(Near(view.Finish.PopulationDeviation,std::sqrt((std::pow(60-184.0/3,2)+
        std::pow(67-184.0/3,2)+std::pow(57-184.0/3,2))/3)),"population deviation");
    Require(view.BestFinishAttempt==3 && view.BestFeeFreeFinishAttempt==3 &&
        view.BestFeeFreeLapAttempt==3 && view.BestFeeFreeLap==19,"actual source attempts");
    Require(Near(view.FeeFreeFinishPercent(),200.0/3) &&
        Near(view.FeeFreeLapPercent(),800.0/9),"clean ratios");
    for(int i=0;i<15;++i)
        Require(series.Observe(MakePracticeAttemptSample(Race(25+i,3))),"ring append");
    const auto recent=series.Read();
    Require(recent.RecentCount==8 && recent.Finishes==18,"bounded recent history");
    for(int i=0;i<recent.RecentCount;++i)
        Require(recent.Recent[static_cast<std::size_t>(i)].Number==18u-static_cast<unsigned>(i),
            "newest first exact attempt number");
    Require(recent.BestFinishAttempt==3 && recent.Finish.Minimum==57,"eviction preserves all-session best");
    series.Reset();
    Require(!series.Read().Valid && series.Read().RecentCount==0,"reset all");
}
void MalformedSamplesAreAtomic()
{
    auto sample=MakePracticeAttemptSample(Race(20,3));
    PracticeAttemptSeries series;
    Require(series.Observe(sample),"seed");
    for(int fault=0;fault<12;++fault)
    {
        auto bad=sample;
        switch(fault)
        {
        case 0:bad.Valid=false;break;
        case 1:bad.FinishSeconds=0;break;
        case 2:bad.BestLapSeconds=70;break;
        case 3:bad.FeeSeconds=-1;break;
        case 4:bad.FeeSeconds=60;break;
        case 5:bad.CompletedLaps=0;break;
        case 6:bad.FeeFreeLaps=4;break;
        case 7:bad.FeeFreeLaps=2;break;
        case 8:bad.BestFeeFreeLapSeconds=1;break;
        case 9:bad.BestFeeFreeLapSeconds=70;break;
        case 10:bad.FinishSeconds=std::numeric_limits<double>::infinity();break;
        case 11:bad.BestLapSeconds=std::numeric_limits<double>::quiet_NaN();break;
        }
        Require(!series.Observe(bad),"reject bad sample");
        Require(series.Read().Finishes==1 && series.Read().Finish.Mean==60,"no partial mutation");
    }
    Require(!series.Observe(MakePracticeAttemptSample(Race(20,5))),"different lap count rejected");
    Require(series.Read().Finishes==1,"length mismatch atomic");
}
void RandomizedMomentCrossCheck()
{
    PracticeAttemptSeries series;
    std::mt19937 rng(5968);
    std::vector<double> totals,actives,fees;
    unsigned clean=0;
    for(int i=0;i<1000;++i)
    {
        const double lap=12+static_cast<double>(rng()%800)/16;
        const double fee=static_cast<double>(rng()%5);
        const auto sample=MakePracticeAttemptSample(Race(lap,5,2,fee));
        Require(series.Observe(sample),"valid generated real race");
        totals.push_back(sample.FinishSeconds);
        actives.push_back(sample.ActiveSeconds());
        fees.push_back(sample.FeeSeconds);
        clean+=sample.FeeFreeFinish()?1u:0u;
    }
    auto compare=[](const std::vector<double>& values,const PracticeAttemptMoments& moments)
    {
        long double sum=0;
        for(double v:values)sum+=v;
        const long double mean=sum/values.size();
        long double m2=0;
        for(double v:values)m2+=(v-mean)*(v-mean);
        Require(moments.Count==values.size() && Near(moments.Mean,static_cast<double>(mean)) &&
            Near(moments.PopulationDeviation,static_cast<double>(std::sqrt(m2/values.size()))) &&
            Near(moments.Minimum,*std::min_element(values.begin(),values.end())) &&
            Near(moments.Maximum,*std::max_element(values.begin(),values.end())),"two-pass reference moments");
    };
    const auto view=series.Read();
    compare(totals,view.Finish);compare(actives,view.Active);compare(fees,view.Fees);
    Require(view.FeeFreeFinishes==clean && view.CompletedLaps==5000 &&
        view.FeeFreeLaps==4000+clean,"all-run counts survive ring wrap");
}
void SessionAuthority()
{
    // Heap allocation here avoids platform-specific stack limits for
    // the full per-configuration session store.
    auto history=std::make_unique<PracticeSessionHistory>();
    const PracticeConfiguration a{QuickPracticePreset::SoloThree,{0,0,0},0};
    auto b=a;b.Tuning.Engine=1;
    history->Begin(a,1);
    Require(history->Finish(a,1,Race(20,3)).Record.FinishedAttempts==1,"first valid");
    Require(history->ReadSeries(a).Finishes==1 && !history->ReadSeries(b).Valid,"distinct slots");
    history->Begin(a,1);history->ObserveConfiguration(b,1);
    Require(history->Finish(a,1,Race(18,3)).Status==PracticeComparisonStatus::ConfigurationChanged,
        "changed setup rejected");
    Require(history->ReadSeries(a).Finishes==1,"invalid finish cannot enter series");
    history->Begin(a,2);history->Finish(a,2,Race(21,3,1,2));
    Require(history->ReadSeries(a).Finishes==history->Read(a).FinishedAttempts,"transaction aligned");
    history->CancelAttempt();
    Require(history->ReadSeries(a).Finishes==2,"cancel retains past finishes");
    history->ResetSession();
    Require(!history->ReadSeries(a).Valid && !history->ReadSeries(b).Valid,"clear session");
}
}
int main()
{
    SampleEvidence();KnownMomentsAndRecentRing();MalformedSamplesAreAtomic();
    RandomizedMomentCrossCheck();SessionAuthority();
    std::cout<<"Practice attempt series tests passed\n";
}
