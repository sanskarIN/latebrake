#include "Racing/DrivingCoachSectorJournal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* reason)
{
    if(!ok)
    {
        std::cerr << "DrivingCoachSectorJournal: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x,double y,double margin=1e-7)
{
    return std::abs(x-y)<=margin;
}
struct Fixture final
{
    DrivingCoachAccumulator Chassis;
    DrivingCoachSectorJournal Journal;
    int Ordinal=1;
    double Clock=0.0;
    explicit Fixture(int gates=4)
    {
        Journal.Reset(gates);
    }
    void Sample(unsigned count=50,double velocity=2000.0,
        double lateral=200.0,bool braking=false)
    {
        for(unsigned i=0;i<count;++i)
            Require(Chassis.Observe({0.1,velocity,lateral,braking}),
                "sample accepted");
        Clock+=0.1*count;
    }
    DrivingCoachGateObservation Receipt(int ordinal=-1) const
    {
        return {true,true,false,7,ordinal>0?ordinal:Ordinal,4,
            Clock,Chassis.Review()};
    }
    bool Gate()
    {
        const auto result=Journal.ObserveGate(Receipt());
        ++Ordinal;
        return result;
    }
};
void BasicCheckpointSampling()
{
    Fixture f;
    f.Sample(50,1000.0);
    Require(!f.Gate(),"first gate only establishes baseline");
    auto review=f.Journal.Review();
    Require(!review.Valid && review.AcceptedGateReceipts==1 &&
        review.Status==DrivingCoachSectorStatus::AwaitingSecondGate,
        "one gate cannot claim a measured sector");
    f.Sample(50,2000.0,1000.0,true);
    Require(f.Gate(),"second verified adjacent gate recorded");
    review=f.Journal.Review();
    Require(review.Valid && review.RetainedSectorSamples==1 &&
        review.TotalRecordedSectors==1 && review.GateCount==4 &&
        review.AcceptedGateReceipts==2 &&
        review.BestSampleSectorCount==1,
        "conservative portable sampled sector review");
    DrivingCoachSectorSample newest;
    Require(f.Journal.ReadNewest(0,newest) &&
        ValidDrivingCoachSectorSample(newest,4) &&
        newest.FirstGateOrdinal==1 && newest.EndGateOrdinal==2 &&
        newest.SectorIndex==1 && newest.LapIndex==0 &&
        Near(newest.ChassisSampleSeconds,5.0) &&
        Near(newest.ClockIntervalSeconds,5.0) &&
        Near(newest.SampledDistanceKm,0.1) &&
        Near(newest.MeanSpeedKmh,72.0) &&
        Near(newest.BrakingFraction,1.0) &&
        Near(newest.HighSlipFraction,1.0) &&
        !newest.AuthenticEngineWitness && !newest.OfficialSectorTime &&
        !newest.ChangesRaceOutcome,
        "sector uses actual sampled velocity and no official time claims");
    Require(!f.Journal.ReadNewest(1,newest),"absent older sector not invented");
}
void BoundariesDoNotInventStartToFirstGate()
{
    Fixture f;
    f.Sample(25,1800.0);
    Require(!f.Gate(),"pre-gate movement never called an accepted sector");
    f.Sample(40,1800.0);
    Require(f.Gate(),"same lap second gate qualifies");
    f.Sample(40,1800.0);
    Require(f.Gate(),"third gate qualifies");
    f.Sample(40,1800.0);
    Require(f.Gate(),"final gate of lap qualifies");
    f.Sample(40,1800.0);
    Require(f.Gate(),"first gate next lap qualifies only prior-to-current interval");
    const auto review=f.Journal.Review();
    Require(review.Valid && review.TotalRecordedSectors==4 &&
        review.BestSampleSectorCount==4 &&
        review.BestSamples[0].LapIndex==1 &&
        review.BestSamples[0].FirstGateOrdinal==4 &&
        review.BestSamples[0].EndGateOrdinal==5,
        "wrapped gate index still distinguishes the actual lap");
}
void OutOfOrderRefusalsNeverReorderSource()
{
    Fixture f;
    f.Sample();f.Gate();
    f.Sample();
    auto invalid=f.Receipt(1);
    Require(!f.Journal.ObserveGate(invalid),"duplicate gate rejected");
    Require(f.Journal.Review().RefusedInputs==1,
        "rejection is explicit and not silently counted as segment");
    f.Sample();
    Require(!f.Gate(),"next accepted gate only re-primes after gap");
    f.Sample();
    Require(f.Gate(),"next contiguous gate can recover sample recording");
    const auto review=f.Journal.Review();
    Require(review.Valid && review.TotalRecordedSectors==1 &&
        review.Discontinuities>=1,
        "duplicate receipt cannot be repaired by interpolation");
}
void ChangedActorAndRecoveredSamplesRefused()
{
    Fixture f;
    f.Sample();f.Gate();
    f.Sample();
    auto bad=f.Receipt();
    bad.ActorToken=8;
    Require(!f.Journal.ObserveGate(bad),"actor identity change rejected");
    Require(f.Journal.Review().RefusedInputs==1 &&
        f.Journal.Review().TotalRecordedSectors==0,
        "identity change never claims physical continuity");
    Require(!f.Gate(),"rebound actor first gate primes baseline");
    f.Sample();
    Require(f.Gate(),"subsequent bound actor can record again");
    f.Chassis.Suspend();
    f.Sample();
    Require(!f.Gate(),"new teleport/paused episode discontinues source");
    f.Sample();
    Require(f.Gate(),"new continuous sequence may resume");
    Require(f.Journal.Review().Discontinuities>=2,
        "recovery retains truthful discontinuity evidence");
}
void MissingSourceAndClockAnomaliesFailClosed()
{
    Fixture f;
    f.Sample();f.Gate();
    f.Sample();
    auto mismatch=f.Receipt();
    mismatch.ActiveClockSeconds-=4.0;
    Require(!f.Journal.ObserveGate(mismatch),
        "sampled time cannot exceed active source interval");
    Require(f.Journal.Review().TotalRecordedSectors==0,
        "rejected source clock cannot create a sampled sector");
    // The malformed second-gate receipt still used the live second ordinal.
    // It was refused and became a non-authoritative baseline for diagnostics;
    // advance the synthetic gate source exactly as the race would.
    ++f.Ordinal;
    f.Sample();
    Require(f.Gate(),"valid third gate eventually records against real second baseline");
    auto malicious=f.Receipt(f.Ordinal);
    malicious.Chassis.ObservedSeconds=std::numeric_limits<double>::quiet_NaN();
    Require(!f.Journal.ObserveGate(malicious),"nonfinite sample rejected");
    malicious=f.Receipt(f.Ordinal);
    malicious.MenuOrFocusSuspended=true;
    Require(!f.Journal.ObserveGate(malicious),"paused receipt rejected");
    malicious=f.Receipt(f.Ordinal);
    malicious.AcceptedByRaceProgress=false;
    Require(!f.Journal.ObserveGate(malicious),"unscored gate has no authority");
    malicious=f.Receipt(f.Ordinal);
    malicious.DriverPhysicsReady=false;
    Require(!f.Journal.ObserveGate(malicious),"missing real chassis refused");
    Require(f.Journal.Review().RefusedInputs>=4,
        "different source rejection reasons tracked");
}
void ClockRoundingAndSparseSampleCoverage()
{
    Fixture f;
    f.Sample(50);f.Gate();
    f.Sample(50);
    auto next=f.Receipt();
    next.ActiveClockSeconds-=0.0001;
    Require(f.Journal.ObserveGate(next),
        "small source-sampling clock tolerance accepted");
    DrivingCoachSectorSample row;
    Require(f.Journal.ReadNewest(0,row) &&
        row.ChassisCoverageFraction<=1.0 &&
        row.ChassisCoverageFraction>0.99,
        "display-only coverage never exceeds 100 percent");
    // New samples can be sparse without claiming a full covered interval.
    f.Ordinal++;
    f.Sample(50);
    f.Clock+=5.0;
    Require(f.Gate(),"sparse physical samples still describe limited interval");
    Require(f.Journal.ReadNewest(0,row) &&
        row.ChassisCoverageFraction<0.6,
        "partial coverage is explicit and never artificially filled");
}
void ResetFreezeAndNeverClaimOfficialTime()
{
    Fixture f;
    f.Sample();f.Gate();f.Sample();f.Gate();
    f.Journal.Freeze();
    Require(!f.Journal.ObserveGate(f.Receipt()),
        "frozen run refuses new gates");
    auto old=f.Journal.Review();
    Require(old.Valid && old.NoRaceClockAuthority && old.NoRewardAuthority,
        "freeze preserves advisory history and cannot control clocks");
    f.Journal.Reset(4);
    auto fresh=f.Journal.Review();
    Require(!fresh.Valid && fresh.TotalRecordedSectors==0 &&
        fresh.ActorToken==0 && fresh.RetainedSectorSamples==0,
        "race restart clears old identity and all old samples");
    f.Journal.Reset(0);
    Require(!f.Journal.ObserveGate(f.Receipt()),"invalid circuit has no gates");
}
void BoundedRingAndPerSectorReview()
{
    Fixture f;
    f.Sample(2);
    Require(!f.Gate(),"first gate primes");
    for(int i=0;i<120;++i)
    {
        f.Sample(2,1400.0+(i%6)*10);
        Require(f.Gate(),"contiguous accepted gates create continuous samples");
    }
    const auto review=f.Journal.Review();
    Require(review.Valid && review.TotalRecordedSectors==120 &&
        review.RetainedSectorSamples==DrivingCoachSectorHistoryCapacity &&
        review.LastGateOrdinal==121,
        "bounded ring retains only recent sectors under long session");
    DrivingCoachSectorSample newest;
    DrivingCoachSectorSample oldest;
    Require(f.Journal.ReadNewest(0,newest) &&
        f.Journal.ReadNewest(DrivingCoachSectorHistoryCapacity-1u,oldest) &&
        newest.Sequence==120 && oldest.Sequence==57 &&
        !f.Journal.ReadNewest(DrivingCoachSectorHistoryCapacity,newest),
        "ring overwrites strictly oldest sample and never wraps sequence");
    Require(review.BestSampleSectorCount==4 &&
        review.TotalSampledSectorSeconds>0.0 &&
        review.TotalSampledSectorDistanceKm>0.0,
        "best local chassis intervals visible in each sector");
}
void RandomizedSamplesPreserveReadoutInvariants()
{
    std::mt19937 generator{0xB5A4436};
    std::uniform_real_distribution<double> speed(0.0,3500.0);
    std::uniform_real_distribution<double> slip(-1200.0,1200.0);
    Fixture f;
    f.Sample(20);f.Gate();
    for(int g=0;g<160;++g)
    {
        for(int i=0;i<8;++i)
        {
            Require(f.Chassis.Observe({0.1,speed(generator),slip(generator),
                ((i+g)%9)<4}),"fuzz sample accepted");
            f.Clock+=0.1;
        }
        Require(f.Gate(),"fuzz gate records paired snapshots");
        auto review=f.Journal.Review();
        Require(review.Valid && review.TotalRecordedSectors==
            static_cast<std::uint64_t>(g+1),
            "fuzz count tracks exact number of adjacent real gates");
        for(std::size_t offset=0;offset<review.RetainedSectorSamples;++offset)
        {
            DrivingCoachSectorSample row;
            Require(f.Journal.ReadNewest(offset,row) &&
                ValidDrivingCoachSectorSample(row,4) &&
                row.SampledDistanceKm>=0 && row.ChassisCoverageFraction<=1.0,
                "random source samples preserve finite, bounded sector output");
        }
    }
}
}

int main()
{
    BasicCheckpointSampling();
    BoundariesDoNotInventStartToFirstGate();
    OutOfOrderRefusalsNeverReorderSource();
    ChangedActorAndRecoveredSamplesRefused();
    MissingSourceAndClockAnomaliesFailClosed();
    ClockRoundingAndSparseSampleCoverage();
    ResetFreezeAndNeverClaimOfficialTime();
    BoundedRingAndPerSectorReview();
    RandomizedSamplesPreserveReadoutInvariants();
    std::cout << "Driving coach checkpoint sample journal tests passed.\n";
    return EXIT_SUCCESS;
}
