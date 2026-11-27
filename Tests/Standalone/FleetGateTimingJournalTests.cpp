#include "AI/FleetGateTimingJournal.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Fixture final
{
    FleetGateTimingJournal Journal;
    FleetTimingIdentity Ids{101,102,103,104,105,106};
    std::array<FleetTimingEvidence, FleetTimingCarCount> Evidence{};
    int Gates;
    int Laps;
    double Clock = 0.0;

    Fixture(int gates = 4, int laps = 3) : Gates(gates), Laps(laps)
    {
        Require(Journal.Configure(1, Ids, Gates, Laps), "configure");
        for (auto& e : Evidence) e.Verified = true;
    }

    void Gate(int slot, double seconds)
    {
        const auto index = static_cast<std::size_t>(slot);
        auto& e = Evidence[index];
        Require(Journal.Record({1, Ids[index], slot, e.AcceptedGates + 1, seconds}),
                "record exact next gate");
        ++e.AcceptedGates;
        e.LastGateSeconds = seconds;
        if (seconds > Clock) Clock = seconds;
    }

    FleetTimingReadout Read() const
    {
        return Journal.Review(1, Ids, Gates, Laps, Clock, Evidence);
    }
};

void ConfigurationAndReceiptRejection()
{
    FleetGateTimingJournal journal;
    const FleetTimingIdentity ids{1,2,3,4,5,6};
    auto bad = ids;
    bad[4] = 0;
    Require(!journal.Configure(1,bad,4,3), "zero roster identity");
    bad = ids;
    bad[5] = bad[0];
    Require(!journal.Configure(1,bad,4,3), "duplicate human identity");
    Require(!journal.Configure(0,ids,4,3), "zero attempt");
    Require(!journal.Configure(1,ids,257,3), "invalid gate count");
    Require(!journal.Configure(1,ids,4,101), "invalid lap count");
    Require(!journal.Record({1,1,0,1,1}), "unconfigured receipt");
    Require(journal.Configure(1,ids,4,3), "valid config after bad configs");
    Require(!journal.Configure(2,ids,4,3), "no live rebind");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Require(!journal.Record({0,1,0,1,1}), "wrong attempt");
    Require(!journal.Record({1,2,0,1,1}), "wrong identity");
    Require(!journal.Record({1,1,-1,1,1}), "negative slot");
    Require(!journal.Record({1,1,6,1,1}), "out-of-range slot");
    Require(!journal.Record({1,1,0,2,1}), "skipped gate");
    Require(!journal.Record({1,1,0,1,0}), "zero clock");
    Require(!journal.Record({1,1,0,1,nan}), "NaN clock");
    Require(!journal.Record({1,1,0,1,inf}), "infinite clock");
    Require(!journal.Record({1,1,0,1,FleetTimingMaximumSeconds+1}), "clock cap");
    Require(journal.Record({1,1,0,1,1}), "first gate");
    Require(!journal.Record({1,1,0,1,2}), "duplicate");
    Require(!journal.Record({1,1,0,2,1}), "same clock in single stream");
    Require(!journal.Record({1,1,0,2,0.5}), "backward clock");
    Require(journal.Record({1,1,0,2,2}), "valid after rejected inputs");
    journal.Reset();
    Require(!journal.Record({1,1,0,3,3}), "old receipt after reset");
    Require(journal.Configure(2,ids,4,3), "explicit new attempt");
    Require(!journal.Record({1,1,0,1,1}), "old attempt cannot modify new");
    Require(journal.Record({2,1,0,1,1}), "new attempt starts at gate one");
}

void SameGateNotSameLatestClock()
{
    Fixture f;
    auto read = f.Read();
    Require(read.ValidPlayer && read.WaitingRivals == 5, "no phantom matched gates");
    f.Gate(0,10); f.Gate(0,20); f.Gate(0,30);
    f.Gate(1,9); f.Gate(1,19); f.Gate(1,29); f.Gate(1,40);
    f.Gate(2,10.25); f.Gate(2,20.25); f.Gate(2,30.25);
    f.Gate(3,11); f.Gate(3,22);
    f.Gate(4,12);
    f.Gate(5,8); f.Gate(5,18); f.Gate(5,28);
    read = f.Read();
    Require(read.MatchedRivals == 5, "five pairwise observations");
    const auto& ahead = read.Rivals[0];
    Require(ahead.CommonOrdinal == 3 && ahead.RivalAtGateSeconds == 29 &&
        ahead.RivalMinusPlayerSeconds == -1 &&
        ahead.Relation == FleetTimingRelation::RivalEarlierObserved, "compare same gate");
    Require(ahead.PreviousSector.Valid && ahead.PreviousSector.FromOrdinal == 2 &&
        ahead.PreviousSector.RivalMinusPlayerSeconds == 0, "sector difference");
    Require(!ahead.LatestCommonLap.Valid, "no common completed lap");
    Require(read.Rivals[1].Relation == FleetTimingRelation::SameClockWindow,
        "inclusive near-clock tolerance");
    Require(read.Rivals[2].CommonOrdinal == 2 &&
        read.Rivals[2].RivalMinusPlayerSeconds == 2, "behind rival exact gate");
    Require(read.Rivals[3].CommonOrdinal == 1 &&
        read.Rivals[3].PreviousSector.FromOrdinal == 0, "first sector origin");
    Require(read.CommonCohortValid && read.CohortOrdinal == 1 &&
        read.CohortEarlier == 2 && read.CohortUncertain == 1 && read.CohortLater == 2,
        "cohort compares same gate, not pairwise mixture");
    f.Gate(0,40);
    read = f.Read();
    Require(read.Rivals[0].LapNumber == 1 && read.Rivals[0].GateNumber == 4 &&
        read.Rivals[0].LatestCommonLap.Valid &&
        read.Rivals[0].LatestCommonLap.RivalMinusPlayerSeconds == 0,
        "complete first matched lap");
    Require(read.Rivals[0].Relation == FleetTimingRelation::SameClockWindow,
        "identical callback clock uncertain");
}

void EvidenceAndClockBoundaries()
{
    Fixture f;
    f.Gate(0,5);
    for (int slot=1;slot<FleetTimingCarCount;++slot) f.Gate(slot,4+slot);
    auto r=f.Read();
    Require(r.CommonCohortValid, "initial complete cohort");
    f.Evidence[2].LastGateSeconds+=0.01;
    r=f.Read();
    Require(r.MatchedRivals==4 && r.InvalidRivals==1 &&
        !r.CommonCohortValid && !r.Rivals[1].PreviousSector.Valid,
        "independent rival clock mismatch isolated");
    f.Evidence[2].LastGateSeconds-=0.01;
    f.Evidence[3].Verified=false;
    Require(f.Read().InvalidRivals==1, "route evidence required");
    f.Evidence[3].Verified=true;
    ++f.Evidence[0].AcceptedGates;
    Require(!f.Read().ValidPlayer, "player count mismatch hides all timing");
    --f.Evidence[0].AcceptedGates;
    const double saved=f.Clock;
    f.Clock=4.9;
    Require(!f.Read().ValidPlayer, "player receipt in future");
    f.Clock=8;
    Require(f.Read().InvalidRivals==1, "rival receipt in future");
    f.Clock=saved;
    auto ids=f.Ids;ids[1]=42;
    Require(!f.Journal.Review(1,ids,4,3,f.Clock,f.Evidence).ValidPlayer,
        "changed physical roster");
    Require(!f.Journal.Review(2,f.Ids,4,3,f.Clock,f.Evidence).ValidPlayer,
        "changed attempt");
    Require(!f.Journal.Review(1,f.Ids,3,4,f.Clock,f.Evidence).ValidPlayer,
        "changed gate shape with same total");
    Require(!f.Journal.Review(1,f.Ids,4,3,std::numeric_limits<double>::quiet_NaN(),f.Evidence).ValidPlayer,
        "bad current clock");
}

void RetentionAndLapBoundaries()
{
    Fixture f(256,3);
    for (int gate=1;gate<=512;++gate)
    {
        f.Gate(0,gate*2.0);
        f.Gate(1,gate*2.0+1.0);
    }
    auto r=f.Read();
    const auto& lap=r.Rivals[0].LatestCommonLap;
    Require(lap.Valid && lap.FromOrdinal==256 && lap.ToOrdinal==512 &&
        lap.PlayerSeconds==512 && lap.RivalSeconds==512 &&
        lap.RivalMinusPlayerSeconds==0, "largest lap fits bounded history");
    f.Gate(0,1026);
    r=f.Read();
    Require(r.Rivals[0].Availability==FleetTimingAvailability::Matched &&
        !r.Rivals[0].LatestCommonLap.Valid &&
        r.Rivals[0].PreviousSector.Valid, "old lap start omitted, not aliased");
    for (int gate=514;gate<=768;++gate) f.Gate(0,gate*2.0);
    r=f.Read();
    Require(r.Rivals[0].Availability==FleetTimingAvailability::Matched,
        "oldest retained common gate remains usable");
    f.Gate(1,1027);
    Require(f.Read().Rivals[0].CommonOrdinal==513, "catch-up uses newer exact receipt");
    Fixture lost(256,3);
    for (int gate=1;gate<=300;++gate) lost.Gate(0,gate*2.0);
    lost.Gate(1,2);
    r=lost.Read();
    Require(r.Rivals[0].Availability==FleetTimingAvailability::OutsideRetainedHistory &&
        r.HistoryUnavailableRivals==1 &&
        r.Rivals[0].Relation==FleetTimingRelation::Unavailable,
        "overwritten history never produces fabricated split");
}

void DeterministicAsynchronousSweep()
{
    Fixture f(32,100);
    std::mt19937 rng(73428);
    std::array<double,FleetTimingCarCount> clocks{};
    for (int step=0;step<12000;++step)
    {
        const int slot=static_cast<int>(rng()%FleetTimingCarCount);
        const auto idx=static_cast<std::size_t>(slot);
        if (f.Evidence[idx].AcceptedGates==3200) continue;
        clocks[idx]+=1.0+static_cast<double>(rng()%100)/100.0;
        f.Gate(slot,clocks[idx]);
        const auto r=f.Read();
        Require(r.ValidPlayer, "valid asynchronous player stream");
        Require(r.MatchedRivals+r.WaitingRivals+r.HistoryUnavailableRivals+r.InvalidRivals==5,
            "every rival accounted for once");
        for (const auto& row:r.Rivals)
        {
            if (row.Availability!=FleetTimingAvailability::Matched) continue;
            const int expected=f.Evidence[0].AcceptedGates<
                f.Evidence[static_cast<std::size_t>(row.Slot)].AcceptedGates
                ? f.Evidence[0].AcceptedGates
                : f.Evidence[static_cast<std::size_t>(row.Slot)].AcceptedGates;
            Require(row.CommonOrdinal==expected &&
                row.GateNumber==(expected-1)%32+1 && row.LapNumber==(expected-1)/32+1,
                "exact common ordinal and gate decomposition");
            Require(row.RivalMinusPlayerSeconds==row.RivalAtGateSeconds-row.PlayerAtGateSeconds,
                "delta derived from real same-gate observations");
        }
        if (r.CommonCohortValid)
            Require(r.CohortEarlier+r.CohortUncertain+r.CohortLater==5,
                "cohort counts exactly five physical rivals");
    }
}

void MaximumConfigurationAndFinalGate()
{
    Fixture f(256,100);
    for (int ordinal=1;ordinal<=25600;++ordinal)
    {
        f.Gate(0,ordinal*2.0);
        f.Gate(1,ordinal*2.0+1.0);
    }
    auto r=f.Read();
    Require(r.Rivals[0].CommonOrdinal==25600 && r.Rivals[0].LapNumber==100 &&
        r.Rivals[0].GateNumber==256 && r.Rivals[0].LatestCommonLap.Valid,
        "final maximum gate and lap");
    Require(!f.Journal.Record({1,f.Ids[0],0,25601,51202}), "no post-finish receipt");
}
}

int main()
{
    ConfigurationAndReceiptRejection();
    SameGateNotSameLatestClock();
    EvidenceAndClockBoundaries();
    RetentionAndLapBoundaries();
    DeterministicAsynchronousSweep();
    MaximumConfigurationAndFinalGate();
    std::cout<<"Fleet matched-gate timing journal passed\n";
}
