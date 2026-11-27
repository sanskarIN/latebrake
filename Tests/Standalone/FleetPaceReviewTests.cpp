#include "AI/FleetGateTimingJournal.h"
#include "AI/FleetRaceReview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

namespace
{
using namespace latebrake;

void Require(bool value,const char* message)
{
    if (!value)
    {
        std::cerr<<"FleetPaceReview: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<=tolerance;
}

struct StreamFixture final
{
    FleetPaceStream Stream;
    int Gates;
    int Laps;
    int Ordinal=0;
    double Clock=0.0;
    StreamFixture(int gates=4,int laps=5) : Gates(gates),Laps(laps)
    {
        Require(Stream.Configure(gates,laps),"configure stream");
    }
    void Gate(double duration)
    {
        Clock+=duration;
        Require(Stream.Record(++Ordinal,Clock),"accepted gate");
    }
    void Lap(const std::vector<double>& sectors)
    {
        Require(static_cast<int>(sectors.size())==Gates && Ordinal%Gates==0,"fixture lap");
        for (const double time:sectors) Gate(time);
    }
};

struct JournalFixture final
{
    FleetGateTimingJournal Journal;
    FleetTimingIdentity Ids{101,102,103,104,105,106};
    std::array<FleetTimingEvidence,FleetTimingCarCount> Evidence{};
    int Gates;
    int Laps;
    double Active=0.0;
    explicit JournalFixture(int gates=4,int laps=4) : Gates(gates),Laps(laps)
    {
        Require(Journal.Configure(1,Ids,Gates,Laps),"configure journal");
        for (auto& row:Evidence) row.Verified=true;
    }
    void Gate(int slot,double duration)
    {
        auto& e=Evidence[static_cast<std::size_t>(slot)];
        const double now=e.LastGateSeconds+duration;
        Require(Journal.Record({1,Ids[static_cast<std::size_t>(slot)],slot,e.AcceptedGates+1,now}),
            "admitted physical receipt");
        ++e.AcceptedGates;
        e.LastGateSeconds=now;
        Active=std::max(Active,now);
    }
    void Lap(int slot,double sectorSeconds)
    {
        for (int i=0;i<Gates;++i) Gate(slot,sectorSeconds);
    }
    FleetPaceReadout Read() const
    {
        return Journal.ReviewPace(1,Ids,Gates,Laps,Active,Evidence);
    }
};

void MomentStatistics()
{
    FleetPaceMoments moments;
    Require(!moments.HasVariationEvidence() && moments.PopulationDeviation()==0,"empty moments");
    Require(!moments.Add(std::numeric_limits<double>::quiet_NaN()),"NaN denied");
    Require(!moments.Add(std::numeric_limits<double>::infinity()),"infinity denied");
    for (double value:{2.0,4.0,4.0,4.0,5.0,5.0,7.0,9.0})
        Require(moments.Add(value),"finite moment");
    Require(moments.Count==8 && moments.Minimum==2 && moments.Maximum==9 &&
        Near(moments.Sum,40) && Near(moments.Mean,5) &&
        Near(moments.PopulationDeviation(),2),"population deviation reference");
    FleetPaceMoments stable;
    for (int i=0;i<100;++i) Require(stable.Add(600000.0+(i%2)*0.001),"stable high magnitude");
    Require(Near(stable.PopulationDeviation(),0.0005,1e-9),"stable variance recurrence");
    Require(!stable.Add(1) && stable.Count==100,"bounded sample count");
    FleetPaceMoments signedValues;
    Require(signedValues.Add(-2) && signedValues.Add(2),"signed paired differences");
    Require(signedValues.Mean==0 && signedValues.PopulationDeviation()==2,"signed spread");
}

void StreamReceiptAndReset()
{
    FleetPaceStream stream;
    Require(!stream.Record(1,2),"unconfigured");
    Require(!stream.Configure(0,1) && !stream.Configure(257,1) &&
        !stream.Configure(4,101) && !stream.Configure(4,0),"invalid course shapes");
    Require(stream.Configure(4,2),"valid shape");
    Require(!stream.Configure(4,2),"no silent rebind");
    Require(!stream.Record(2,1) && !stream.Record(1,0) &&
        !stream.Record(1,std::numeric_limits<double>::quiet_NaN()),"bad initial receipts");
    Require(stream.Record(1,2),"first gate");
    Require(!stream.Record(1,3) && !stream.Record(2,2) &&
        !stream.Record(2,1),"duplicate and clock rollback");
    Require(stream.Record(2,4) && stream.Record(3,6),"accepted contiguous partial");
    FleetPaceLap lap;
    FleetPaceSector sector;
    Require(!stream.TryLap(1,lap) && !stream.TrySector(1,sector),"partial lap does not publish");
    auto summary=stream.Summary(0);
    Require(summary.Valid && summary.PendingSectors==3 &&
        summary.PendingElapsedSeconds==6 && !summary.AllLaps.Valid,"partial time only");
    Require(stream.Record(4,8),"completed lap");
    Require(stream.TryLap(1,lap) && lap.StartSeconds==0 && lap.EndSeconds==8 &&
        lap.DurationSeconds==8 && lap.SlowestSector==1,"standing-start lap record");
    Require(stream.TrySector(1,sector) && sector.Duration.Count==1 && sector.LatestSeconds==2,
        "completed sectors admitted");
    for (int ordinal=5;ordinal<=8;++ordinal) Require(stream.Record(ordinal,2.0*ordinal),"second lap");
    Require(!stream.Record(9,18),"no gate after configured finish");
    Require(!stream.TryLap(0,lap) && !stream.TryLap(3,lap) &&
        !stream.TrySector(5,sector),"read bounds");
    stream.Reset();
    Require(!stream.Summary(0).Valid && !stream.TryLap(1,lap),"restart clears all history");
    Require(stream.Configure(1,1) && stream.Record(1,1),"new shape after reset");
    Require(stream.Summary(1).AllLaps.BestSeconds==1,"no inherited laps");
}

void WholeLapAndSectorStatistics()
{
    StreamFixture f;
    f.Lap({4,2,3,1});
    f.Lap({2,2,4,2});
    f.Lap({3,3,3,3});
    auto s=f.Stream.Summary(0);
    Require(s.AllLaps.Valid && s.CompleteLaps==3 &&
        s.AllLaps.Count==3 && s.AllLaps.BestLap==1 &&
        s.AllLaps.WorstLap==3 && s.AllLaps.MedianSeconds==10 &&
        Near(s.AllLaps.MeanSeconds,32.0/3.0),"three observed lap distribution");
    Require(s.AllLaps.TrendValid && s.AllLaps.LastMinusFirstSeconds==2 &&
        s.AllLaps.VariationValid,"trend and variance require two samples");
    Require(s.AfterLaunch.Valid && s.AfterLaunch.FirstLap==2 &&
        s.AfterLaunch.Count==2 && s.AfterLaunch.MedianSeconds==11,
        "standing-start exclusion explicit");
    Require(s.SectorEvidenceValid && Near(s.IdealObservedSectorSum,8) &&
        s.SlowestMeanSector==3 && s.SectorVariationValid &&
        s.MostVariableSector==1,"whole-lap sector profiles");
    FleetPaceSector sector;
    Require(f.Stream.TrySector(1,sector) && sector.BestLap==2 && sector.WorstLap==1 &&
        sector.Duration.Count==3 && sector.Duration.Mean==3,"sector extrema");
    Require(f.Stream.TrySector(2,sector) && sector.BestLap==1,"earliest equal best");
    f.Gate(0.1);
    f.Gate(0.1);
    auto partial=f.Stream.Summary(0);
    Require(partial.CompleteLaps==3 && partial.PendingSectors==2 &&
        Near(partial.PendingElapsedSeconds,0.2) &&
        partial.IdealObservedSectorSum==s.IdealObservedSectorSum,"unfinished lap excluded");
    Require(!f.Stream.Summarize(0,1).Valid && !f.Stream.Summarize(3,2).Valid &&
        !f.Stream.Summarize(1,4).Valid && !f.Stream.Summary(6).Valid,"summary bounds");
    const auto one=f.Stream.Summarize(2,2);
    Require(one.Valid && one.Count==1 && !one.VariationValid && !one.TrendValid &&
        one.MedianSeconds==10,"single observation no false consistency");
}

void PairedLapsAndClockWindow()
{
    StreamFixture player,rival;
    player.Lap({3,3,3,3});
    rival.Lap({4,3,3,3});
    player.Lap({2,2,2,2});
    rival.Lap({2,2,2,2.25});
    player.Lap({4,4,4,4});
    rival.Lap({3,3,3,3});
    const auto p=CompareFleetPaceLaps(player.Stream,rival.Stream,1,3);
    Require(p.Valid && p.Count==3 && p.PlayerShorter==1 && p.RivalShorter==1 &&
        p.Close==1 && p.Latest.Number==3,"paired same-lap outcomes");
    Require(Near(p.TotalRivalMinusPlayerSeconds,-2.75) &&
        Near(p.Difference.Mean,-2.75/3.0) && Near(p.MeanAbsoluteDifference,5.25/3.0),
        "signed and absolute matched deltas");
    Require(p.BiggestPlayerGain.Number==1 && p.BiggestPlayerGain.RivalMinusPlayerSeconds==1 &&
        p.BiggestPlayerLoss.Number==3 && p.BiggestPlayerLoss.RivalMinusPlayerSeconds==-4,
        "measured biggest gains and losses");
    Require(FleetPaceCompareDuration(-0.25)==FleetPaceRelation::WithinWindow &&
        FleetPaceCompareDuration(0.25)==FleetPaceRelation::WithinWindow &&
        FleetPaceCompareDuration(0.250001)==FleetPaceRelation::PlayerShorter &&
        FleetPaceCompareDuration(-0.250001)==FleetPaceRelation::RivalShorter,
        "inclusive sampled clock window");
    Require(FleetPaceCompareDuration(std::numeric_limits<double>::infinity())==
        FleetPaceRelation::Unavailable,"invalid delta");
    player.Lap({1,1,1,1});
    const auto row=BuildFleetPaceRival(1,player.Stream,rival.Stream);
    Require(row.EvidenceValid && row.CommonLaps.Count==3 &&
        row.CommonLaps.Player.LastLap==3 &&
        row.CommonLaps.Player.BestSeconds==8 &&
        row.CommonAfterLaunch.Count==2 &&
        !row.Sectors.Valid,"faster unmatched fourth lap excluded from pair");
    Require(!CompareFleetPaceLaps(player.Stream,rival.Stream,1,4).Valid,
        "cannot compare absent rival lap");
    Require(!BuildFleetPaceRival(0,player.Stream,rival.Stream).EvidenceValid,
        "human not opponent");
    StreamFixture different(2,5);
    different.Lap({1,1});
    Require(!CompareFleetPaceLaps(player.Stream,different.Stream,1,1).Valid,
        "course shape mismatch");
}

void EqualCoverageSectorComparison()
{
    StreamFixture player,rival;
    player.Lap({2,4,6,8});
    rival.Lap({3,3,6,10});
    player.Lap({2,4,6,8});
    rival.Lap({3,2,6,8});
    const auto s=CompareFleetPaceSectors(player.Stream,rival.Stream);
    Require(s.Valid && s.MatchedLaps==2 && s.PlayerShorter==2 &&
        s.RivalShorter==1 && s.Close==1,"same coverage sector counts");
    Require(s.LargestPlayerGain.Gate==1 && s.LargestPlayerGain.RivalMinusPlayerSeconds==1,
        "earliest equal gain");
    Require(s.LargestPlayerLoss.Gate==2 && s.LargestPlayerLoss.RivalMinusPlayerSeconds==-1.5,
        "largest mean loss");
    Require(Near(s.TotalMeanDifferenceSeconds,0.5),"mean delta sum");
    rival.Gate(0.1);
    Require(CompareFleetPaceSectors(player.Stream,rival.Stream).Valid,
        "unmatched incomplete sector does not change completed-lap population");
    rival.Gate(1);
    rival.Gate(1);
    rival.Gate(1);
    Require(!CompareFleetPaceSectors(player.Stream,rival.Stream).Valid,
        "unequal completed lap populations withheld");
}

void CohortUsesCommonPrefix()
{
    StreamFixture player;
    std::array<StreamFixture,5> rivals{};
    player.Lap({5,5,5,5});
    player.Lap({1,1,1,1});
    for (std::size_t i=0;i<rivals.size();++i)
    {
        const double t=i==0?4.0:(i==1?5.0:6.0);
        rivals[i].Lap({t,t,t,t});
        if (i!=4) rivals[i].Lap({1,1,1,1});
    }
    std::array<const FleetPaceStream*,5> refs{};
    for (std::size_t i=0;i<refs.size();++i) refs[i]=&rivals[i].Stream;
    auto c=BuildFleetPaceCohort(player.Stream,refs);
    Require(c.Valid && c.CommonLaps==1 && c.ComparedRivals==5 &&
        c.PlayerMeanSeconds==20 && c.RivalMeanMinimumSeconds==16 &&
        c.RivalMeanMaximumSeconds==24 && c.PlayerShorterMeans==3 &&
        c.RivalShorterMeans==1 && c.CloseMeans==1,"cohort same prefix for every driver");
    refs[3]=nullptr;
    Require(!BuildFleetPaceCohort(player.Stream,refs).Valid,"missing rival not aggregate");
    StreamFixture empty;
    refs[3]=&empty.Stream;
    Require(!BuildFleetPaceCohort(player.Stream,refs).Valid,"zero common complete laps");
}

void JournalEvidenceAndRejectedReceipts()
{
    JournalFixture f;
    auto r=f.Read();
    Require(r.ValidPlayer && !r.Player.AllLaps.Valid && r.VerifiedRivals==5 &&
        r.ComparableRivals==0 && !r.Cohort.Valid,"no premature pace comparisons");
    f.Lap(0,4);
    for (int slot=1;slot<6;++slot) f.Lap(slot,3+slot*0.25);
    r=f.Read();
    Require(r.ValidPlayer && r.ComparableRivals==5 && r.Cohort.Valid &&
        r.Player.AllLaps.BestSeconds==16,"journal accepted lap receipt");
    const auto before=r.Rivals[0].CommonLaps.Latest.RivalSeconds;
    Require(!f.Journal.Record({2,f.Ids[1],1,5,20}),"foreign attempt");
    Require(!f.Journal.Record({1,999,1,5,20}),"foreign actor");
    Require(!f.Journal.Record({1,f.Ids[1],1,4,20}),"duplicate gate");
    Require(!f.Journal.Record({1,f.Ids[1],1,6,20}),"skipped gate");
    Require(f.Read().Rivals[0].CommonLaps.Latest.RivalSeconds==before,
        "rejections cannot modify summaries");
    f.Evidence[2].Verified=false;
    r=f.Read();
    Require(r.ValidPlayer && r.UnavailableRivals==1 && r.ComparableRivals==4 &&
        !r.Rivals[1].EvidenceValid && !r.Cohort.Valid,"per-rival receipt evidence isolated");
    f.Evidence[2].Verified=true;
    f.Evidence[3].LastGateSeconds+=0.1;
    Require(f.Read().UnavailableRivals==1,"independent latest clock mismatch");
    f.Evidence[3].LastGateSeconds-=0.1;
    ++f.Evidence[0].AcceptedGates;
    Require(!f.Read().ValidPlayer,"player authority mismatch");
    --f.Evidence[0].AcceptedGates;
    Require(!f.Journal.ReviewPace(1,f.Ids,4,4,1,f.Evidence).ValidPlayer,"future player receipt");
    auto ids=f.Ids;ids[1]=999;
    Require(!f.Journal.ReviewPace(1,ids,4,4,f.Active,f.Evidence).ValidPlayer,"changed roster");
    Require(!f.Journal.ReviewPace(2,f.Ids,4,4,f.Active,f.Evidence).ValidPlayer,"changed attempt");
    Require(!f.Journal.ReviewPace(1,f.Ids,2,8,f.Active,f.Evidence).ValidPlayer,"changed gate shape");
    f.Journal.Reset();
    Require(!f.Read().ValidPlayer,"restart clears pace");
}

void WholeRunBeyondRingRetention()
{
    JournalFixture f(256,100);
    for (int lap=1;lap<=100;++lap)
        f.Lap(0,2.0);
    for (int lap=1;lap<=2;++lap) f.Lap(1,3.0);
    const auto recent=f.Journal.Review(1,f.Ids,256,100,f.Active,f.Evidence);
    Require(recent.Rivals[0].Availability==FleetTimingAvailability::OutsideRetainedHistory,
        "short ring actually wrapped");
    const auto r=f.Read();
    Require(r.Player.CompleteLaps==100 && r.Player.AllLaps.Count==100 &&
        Near(r.Player.AllLaps.BestSeconds,512) && r.Player.AfterLaunch.Count==99,
        "all 100 laps retained independently");
    Require(r.Rivals[0].CommonLaps.Valid && r.Rivals[0].CommonLaps.Count==2 &&
        Near(r.Rivals[0].CommonLaps.Difference.Mean,256) &&
        !r.Rivals[0].Sectors.Valid,"same-prefix comparison survives ring wrap");
    Require(!f.Journal.Record({1,f.Ids[0],0,25601,f.Active+2}),"no post-finish lap");
}

void FinishedReviewAdapter()
{
    JournalFixture f(4,2);
    FleetRaceReview review;
    review.ValidRoster=true;
    review.PlayerRouteValid=true;
    review.GateCount=4;
    review.RequiredLaps=2;
    f.Lap(0,3);
    f.Lap(0,2);
    for (int slot=1;slot<6;++slot) f.Lap(slot,3);
    review.PlayerAcceptedGates=8;
    review.PlayerActiveSeconds=f.Active;
    for (int slot=1;slot<6;++slot)
    {
        auto& row=review.Rivals[static_cast<std::size_t>(slot-1)];
        row.Slot=slot;
        row.BodyObserved=true;
        row.RouteEvidenceValid=true;
        row.AcceptedGates=f.Evidence[static_cast<std::size_t>(slot)].AcceptedGates;
        row.LastGateActiveSeconds=f.Evidence[static_cast<std::size_t>(slot)].LastGateSeconds;
    }
    Require(!ReviewFleetWholeRunPace(review,f.Journal,1,f.Ids).ValidPlayer,
        "no live heavy results computation");
    review.PlayerFinished=true;
    auto r=ReviewFleetWholeRunPace(review,f.Journal,1,f.Ids);
    Require(r.ValidPlayer && r.ComparableRivals==5 && r.Player.CompleteLaps==2,
        "frozen finish review");
    review.Rivals[2].BodyObserved=false;
    r=ReviewFleetWholeRunPace(review,f.Journal,1,f.Ids);
    Require(r.UnavailableRivals==1 && !r.Cohort.Valid,"missing body hides comparison");
    review.ValidRoster=false;
    Require(!ReviewFleetWholeRunPace(review,f.Journal,1,f.Ids).ValidPlayer,"invalid roster");
}

void RandomizedReferenceDistributions()
{
    std::mt19937 rng(987654);
    std::uniform_int_distribution<int> ticks(1,300);
    for (int trial=0;trial<160;++trial)
    {
        const int gates=1+static_cast<int>(rng()%12);
        const int laps=2+static_cast<int>(rng()%24);
        StreamFixture a(gates,laps),b(gates,laps);
        std::vector<double> x,y;
        std::vector<std::vector<double>> sx(static_cast<std::size_t>(gates)),
            sy(static_cast<std::size_t>(gates));
        for (int lap=0;lap<laps;++lap)
        {
            std::vector<double> pa,pb;
            for (int gate=0;gate<gates;++gate)
            {
                const double p=ticks(rng)*0.125;
                const double r=ticks(rng)*0.125;
                pa.push_back(p);pb.push_back(r);
                sx[static_cast<std::size_t>(gate)].push_back(p);
                sy[static_cast<std::size_t>(gate)].push_back(r);
            }
            a.Lap(pa);b.Lap(pb);
            x.push_back(std::accumulate(pa.begin(),pa.end(),0.0));
            y.push_back(std::accumulate(pb.begin(),pb.end(),0.0));
        }
        const int first=1+static_cast<int>(rng()%laps);
        const int last=first+static_cast<int>(rng()%(laps-first+1));
        const auto pair=CompareFleetPaceLaps(a.Stream,b.Stream,first,last);
        Require(pair.Valid && pair.Count==last-first+1,"random pair coverage");
        std::vector<double> expected(x.begin()+first-1,x.begin()+last);
        std::sort(expected.begin(),expected.end());
        const auto n=expected.size();
        const double median=n%2?expected[n/2]:(expected[n/2-1]+expected[n/2])*0.5;
        const double mean=std::accumulate(expected.begin(),expected.end(),0.0)/n;
        double variance=0;
        for (double v:expected) variance+=(v-mean)*(v-mean);
        Require(Near(pair.Player.MedianSeconds,median) && Near(pair.Player.MeanSeconds,mean) &&
            Near(pair.Player.DeviationSeconds,std::sqrt(variance/n)) &&
            pair.Player.BestSeconds==expected.front() &&
            pair.Player.WorstSeconds==expected.back(),"distribution matches independent sorted oracle");
        double signedSum=0,absoluteSum=0;
        int earlier=0,later=0,close=0;
        for (int lap=first;lap<=last;++lap)
        {
            const double d=y[static_cast<std::size_t>(lap-1)]-x[static_cast<std::size_t>(lap-1)];
            signedSum+=d;absoluteSum+=std::abs(d);
            if (d>0.25) ++earlier;
            else if (d< -0.25) ++later;
            else ++close;
        }
        Require(Near(pair.TotalRivalMinusPlayerSeconds,signedSum) &&
            Near(pair.MeanAbsoluteDifference,absoluteSum/n) &&
            pair.PlayerShorter==earlier && pair.RivalShorter==later && pair.Close==close,
            "pair signs match reference");
        const auto sectors=CompareFleetPaceSectors(a.Stream,b.Stream);
        Require(sectors.Valid && sectors.PlayerShorter+sectors.RivalShorter+sectors.Close==gates,
            "all sector profiles accounted for");
        double sum=0;
        for (int gate=0;gate<gates;++gate)
        {
            FleetPaceSector p,r;
            Require(a.Stream.TrySector(gate+1,p) && b.Stream.TrySector(gate+1,r),"profiles retained");
            const auto idx=static_cast<std::size_t>(gate);
            const double pm=std::accumulate(sx[idx].begin(),sx[idx].end(),0.0)/laps;
            const double rm=std::accumulate(sy[idx].begin(),sy[idx].end(),0.0)/laps;
            Require(Near(p.Duration.Mean,pm) && Near(r.Duration.Mean,rm),"sector reference means");
            sum+=rm-pm;
        }
        Require(Near(sectors.TotalMeanDifferenceSeconds,sum),"sector deltas match oracle");
    }
}
}

int main()
{
    MomentStatistics();
    StreamReceiptAndReset();
    WholeLapAndSectorStatistics();
    PairedLapsAndClockWindow();
    EqualCoverageSectorComparison();
    CohortUsesCommonPrefix();
    JournalEvidenceAndRejectedReceipts();
    WholeRunBeyondRingRetention();
    FinishedReviewAdapter();
    RandomizedReferenceDistributions();
    std::cout<<"Fleet whole-run lap, matched-prefix and sector statistics passed\n";
}
