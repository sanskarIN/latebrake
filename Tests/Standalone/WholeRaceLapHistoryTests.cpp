#include "Racing/WholeRaceLapHistory.h"
#include "Racing/FinishTimingEligibility.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

namespace
{
using namespace latebrake;
void Require(bool good,const char* why)
{
    if(!good)
    {
        std::cerr<<"WholeRaceLapHistory: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<=tolerance;
}

void CompleteLap(RaceProgress& race,double active,double penalty=0.0)
{
    for(int gate=0;gate<race.GetCheckpointCount();++gate)
    {
        race.Advance(active/race.GetCheckpointCount());
        if(gate==0 && penalty>0) Require(race.AddTimePenalty(penalty),"charge");
        Require(race.PassCheckpoint(gate),"ordered physical gate");
    }
}

void WholePracticeBeyondRing()
{
    RaceProgress race(4,10);
    Require(!race.GetWholeRaceLapSummary().Valid,"no fabricated samples");
    for(int lap=1;lap<=10;++lap)
        CompleteLap(race,static_cast<double>(21-lap),lap==2 || lap==7?5.0:0.0);
    Require(CanAwardFinishedRace(race),"physical timing valid");
    Require(race.GetRecentTimedLapCount()==8 && race.GetRecentTimedLap(7).CompletedLap==3,
        "first two laps absent from detail ring");
    const auto s=race.GetWholeRaceLapSummary();
    Require(s.Valid && s.All.Count==10 && s.FeeFree.Count==8 && s.Penalized.Count==2,
        "all ten observed and classified");
    Require(Near(s.All.TotalSeconds,165.0) && Near(s.TotalPenaltySeconds,10.0) &&
        Near(s.All.MeanSeconds,16.5),"full totals include evicted laps");
    Require(s.All.FastestLap==10 && s.All.SlowestLap==2 &&
        Near(s.All.FastestSeconds,11) && Near(s.All.SlowestSeconds,24),
        "physical lap identifiers survive ring eviction");
    Require(s.FeeFree.FastestLap==10 && s.FeeFree.SlowestLap==1 &&
        Near(s.FeeFree.TotalSeconds,122) && Near(s.Penalized.TotalSeconds,43),
        "fee-free group has no manufactured times");
    Require(s.LongestFeeFreeStreak==4 && s.CurrentFeeFreeStreak==3 &&
        s.LongestImprovementStreak==4 && s.CurrentImprovementStreak==3,
        "adjacent streaks across full race");
    Require(s.HasOpeningClosingComparison && Near(s.ClosingVsOpeningSeconds,-9),
        "real opening and closing laps");
    const auto before=s.All.TotalSeconds;
    race.Advance(100);
    Require(!race.PassCheckpoint(0) && !race.AddTimePenalty(1) &&
        race.GetWholeRaceLapSummary().All.TotalSeconds==before,"finished race frozen");
    race.Reset();
    Require(!race.GetWholeRaceLapSummary().Valid,"reset removes prior run");
    CompleteLap(race,15);
    race.Advance(4);
    Require(race.AddTimePenalty(2),"unfinished lap penalty");
    const auto live=race.GetWholeRaceLapSummary();
    Require(live.Valid && live.All.Count==1 && Near(live.All.TotalSeconds,15) &&
        live.TotalPenaltySeconds==0 && !live.HasOpeningClosingComparison,
        "unfinished lap is excluded");
}

void FaultIsolation()
{
    for(int mode=0;mode<8;++mode)
    {
        WholeRaceLapHistory journal;
        Require(journal.Observe(1,50,true,0),"valid first");
        const double nan=std::numeric_limits<double>::quiet_NaN();
        const double inf=std::numeric_limits<double>::infinity();
        bool accepted=false;
        switch(mode)
        {
            case 0: accepted=journal.Observe(1,50,true,0);break;
            case 1: accepted=journal.Observe(3,50,true,0);break;
            case 2: accepted=journal.Observe(2,0,true,0);break;
            case 3: accepted=journal.Observe(2,50,false,0);break;
            case 4: accepted=journal.Observe(2,nan,true,0);break;
            case 5: accepted=journal.Observe(2,50,true,inf);break;
            case 6: accepted=journal.Observe(2,50,true,51);break;
            case 7: accepted=journal.Observe(2,50,true,-1);break;
        }
        Require(!accepted && !journal.Review(1,50,0).Valid &&
            !journal.Observe(2,50,true,0),"bad sample cannot become partial valid summary");
        journal.Reset();
        Require(journal.Observe(1,50,true,0) && journal.Review(1,50,0).Valid,
            "explicit reset recovers");
        Require(!journal.Review(2,50,0).Valid && !journal.Review(1,49,0).Valid &&
            !journal.Review(1,50,1).Valid,"authoritative totals must match");
    }
    RaceProgress untimed(1,2);
    Require(untimed.AddTimePenalty(10) && untimed.PassCheckpoint(0),"penalty-only lap");
    CompleteLap(untimed,20);
    Require(!CanAwardFinishedRace(untimed) && !untimed.GetWholeRaceLapSummary().Valid,
        "invalid official timing never presented as full run");
    WholeRaceLapHistory extreme;
    Require(extreme.Observe(1,604799,true,0) &&
        !extreme.Observe(2,2,true,0) && !extreme.Review(1,604799,0).Valid,
        "whole-clock bound");
}

void VerifyGroup(const WholeRaceLapGroup& group,const std::vector<double>& seconds,
    const std::vector<int>& ids)
{
    Require(group.Count==static_cast<int>(seconds.size()),"exact group count");
    if(seconds.empty())
    {
        Require(group.FastestLap==0 && group.SlowestLap==0 && group.TotalSeconds==0,
            "empty group no implied lap");
        return;
    }
    long double sum=0;
    std::size_t fastest=0,slowest=0;
    for(std::size_t i=0;i<seconds.size();++i)
    {
        sum+=seconds[i];
        if(seconds[i]<seconds[fastest])fastest=i;
        if(seconds[i]>seconds[slowest])slowest=i;
    }
    const long double mean=sum/seconds.size();
    long double m2=0;
    for(double v:seconds)
    {
        const auto delta=v-mean;
        m2+=delta*delta;
    }
    Require(Near(group.TotalSeconds,static_cast<double>(sum),1e-6) &&
        Near(group.MeanSeconds,static_cast<double>(mean)) &&
        Near(group.PopulationDeviationSeconds,static_cast<double>(std::sqrt(m2/seconds.size()))),
        "online moments match independent two-pass oracle");
    Require(group.FastestLap==ids[fastest] && group.SlowestLap==ids[slowest] &&
        group.FastestSeconds==seconds[fastest] && group.SlowestSeconds==seconds[slowest],
        "first tied physical lap deterministic");
}

void RandomizedFullRunOracle()
{
    std::mt19937 rng(618u);
    for(int sample=0;sample<200;++sample)
    {
        WholeRaceLapHistory journal;
        std::vector<double> all,free,paid;
        std::vector<int> allIds,freeIds,paidIds;
        double total=0,fees=0;
        const int count=1+static_cast<int>(rng()%250);
        int freeStreak=0,maxFree=0,improve=0,maxImprove=0;
        double previous=0;
        for(int lap=1;lap<=count;++lap)
        {
            const double fee=rng()%4==0?5.0:0.0;
            const double seconds=30+static_cast<double>(rng()%160)/8+fee;
            Require(journal.Observe(lap,seconds,true,fee),"observed timed lap");
            all.push_back(seconds);allIds.push_back(lap);
            if(fee==0)
            {
                free.push_back(seconds);freeIds.push_back(lap);++freeStreak;
            }
            else
            {
                paid.push_back(seconds);paidIds.push_back(lap);freeStreak=0;
            }
            if(lap>1 && seconds<previous)++improve;else improve=0;
            maxFree=std::max(maxFree,freeStreak);maxImprove=std::max(maxImprove,improve);
            previous=seconds;
            total+=seconds;fees+=fee;
        }
        const auto s=journal.Review(count,total,fees);
        Require(s.Valid && s.TotalPenaltySeconds==fees &&
            s.LongestFeeFreeStreak==maxFree && s.CurrentFeeFreeStreak==freeStreak &&
            s.LongestImprovementStreak==maxImprove && s.CurrentImprovementStreak==improve,
            "full history totals and streaks");
        VerifyGroup(s.All,all,allIds);
        VerifyGroup(s.FeeFree,free,freeIds);
        VerifyGroup(s.Penalized,paid,paidIds);
    }
}

void NumericalAndLongHistory()
{
    WholeRaceLapHistory history;
    double total=0;
    for(int lap=1;lap<=10000;++lap)
    {
        const double seconds=1.0+static_cast<double>(lap%2)*1e-9;
        Require(history.Observe(lap,seconds,true,0),"long constant memory history");
        total+=seconds;
    }
    const auto s=history.Review(10000,total,0);
    Require(s.Valid && s.All.Count==10000 && s.FeeFree.Count==10000 &&
        s.LongestFeeFreeStreak==10000 && s.All.FastestLap==2 && s.All.SlowestLap==1,
        "long history and stable tie handling");
    Require(Near(s.All.PopulationDeviationSeconds,5e-10,1e-14),"tiny variance retained");
}
}

int main()
{
    WholePracticeBeyondRing();
    FaultIsolation();
    RandomizedFullRunOracle();
    NumericalAndLongHistory();
    std::cout<<"Whole race lap timing aggregates passed\n";
}
