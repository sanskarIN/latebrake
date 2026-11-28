#include "Racing/PlayerSectorMotionReview.h"

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
        std::cerr<<"PlayerSectorMotionReview: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double eps=1e-7){return std::abs(a-b)<=eps;}

struct Fixture final
{
    PlayerSectorMotionJournal Journal;
    SectorMotionFrame Frame;
    int Laps=3;
    double Boundary=0.0;
    double X=-2500.0;
    Fixture(int laps=3):Laps(laps)
    {
        Journal.Reset(CircuitGateCount,laps);
        Frame.Actor=11;
        Frame.PhysicalBodyObserved=true;
        Frame.ClockAdvanced=true;
        Frame.AdvancedSeconds=0.05;
        Frame.Position={X,SouthY};
        Frame.WorldZ=100.0;
        Frame.VelocityX=1000.0;
    }
    void Tick(bool observe=true)
    {
        ++Frame.EngineFrame;
        Frame.ElapsedSeconds+=Frame.AdvancedSeconds;
        Frame.Position.X+=Frame.VelocityX*Frame.AdvancedSeconds;
        Frame.Position.Y+=Frame.VelocityY*Frame.AdvancedSeconds;
        if(observe) Require(Journal.Observe(Frame),"qualified body sample");
    }
    void SetRoadPosition()
    {
        Frame.Position={-2500.0,SouthY};
    }
    bool Gate()
    {
        const auto split=Frame.ElapsedSeconds-Boundary;
        const int gate=Frame.Ordinal%CircuitGateCount;
        const bool result=Journal.AcceptGate({Frame.Ordinal+1,gate,
            Frame.ElapsedSeconds,Frame.PenaltySeconds,split});
        ++Frame.Ordinal;
        Boundary=Frame.ElapsedSeconds;
        SetRoadPosition();
        return result;
    }
    void Sector(int count)
    {
        for(int i=0;i<count;++i) Tick();
        Require(Gate(),"accepted real split");
    }
    SectorMotionReview Read() const
    {
        return Journal.Read(Frame.Ordinal,Frame.ElapsedSeconds,
            Frame.PenaltySeconds,Frame.Ordinal==Laps*CircuitGateCount);
    }
};

void EndpointValidation()
{
    Fixture f;
    f.Tick();
    const auto p=MeasureSectorMotionEndpoint(f.Frame);
    Require(p.Valid && p.Speed==1000 && Near(p.ForwardSpeed,1000) &&
        Near(p.LateralSpeed,0) && !p.FullyOffProjectedRoad &&
        !p.PartlyOffProjectedRoad && !p.HighSlip,"valid exact road motion");
    auto s=f.Frame;s.HeadingDegrees=90;
    auto e=MeasureSectorMotionEndpoint(s);
    Require(e.Valid && Near(e.ForwardSpeed,0) && Near(e.LateralSpeed,-1000) &&
        e.HighSlip,"actual local lateral motion");
    s=f.Frame;s.VelocityX=-1000;
    Require(MeasureSectorMotionEndpoint(s).ReverseMotion,"reverse");
    s=f.Frame;s.VelocityX=50;
    Require(MeasureSectorMotionEndpoint(s).LowSpeed,"low-speed sample");
    s=f.Frame;s.Position={0,0};
    e=MeasureSectorMotionEndpoint(s);
    Require(e.Valid && e.FullyOffProjectedRoad && e.PartlyOffProjectedRoad,
        "projected footprint, not contact claim");
    const auto reject=[](SectorMotionFrame bad)
    {Require(!MeasureSectorMotionEndpoint(bad).Valid,"malformed sample");};
    s=f.Frame;s.Actor=0;reject(s);
    s=f.Frame;s.EngineFrame=0;reject(s);
    s=f.Frame;s.Ordinal=-1;reject(s);
    s=f.Frame;s.Ordinal=400;reject(s);
    s=f.Frame;s.ClockAdvanced=false;reject(s);
    s=f.Frame;s.PhysicalBodyObserved=false;reject(s);
    s=f.Frame;s.PenaltySeconds=1;reject(s);
    s=f.Frame;s.AdvancedSeconds=0;reject(s);
    s=f.Frame;s.AdvancedSeconds=0.11;reject(s);
    s=f.Frame;s.VelocityZ=16000;reject(s);
    s=f.Frame;s.Position.X=100001;reject(s);
    s=f.Frame;s.HeadingDegrees=std::numeric_limits<double>::quiet_NaN();reject(s);
    s=f.Frame;s.WorldZ=std::numeric_limits<double>::infinity();reject(s);
}

void BoundariesAndCoverage()
{
    Fixture f;
    f.Sector(20);
    const auto r=f.Read();
    const auto& row=r.Last;
    Require(r.Valid && r.AcceptedGates==1 && r.TimedSectors==1 &&
        r.QualifiedSectors==1 && row.Comparable(),"first real sector");
    Require(row.StandingStart && row.Lap==1 && row.Gate==0 &&
        Near(row.ActiveSeconds,1.0) && Near(row.Coverage,0.95),"standing start distinct");
    Require(row.Motion.Samples==20 && row.Motion.Intervals==19 &&
        Near(row.Motion.ObservedSeconds,0.95) && Near(row.Motion.AverageKmh(),36) &&
        Near(row.Motion.DistanceMeters(),9.5),"no first sample extrapolation");
    Require(!r.Gates[0].Best.Comparable() && !r.LastComparison.Valid,"no standing reference");
    f.Sector(20);
    const auto second=f.Read();
    Require(second.Last.Gate==1 && second.Gates[1].Best.Comparable() &&
        second.Gates[1].Best.Ordinal==2,"rolling reference");
    Require(second.Last.Motion.Samples==20 && second.Last.Motion.Intervals==19,
        "never join motion across gate boundary");
    SectorMotionRow previous;
    Require(f.Journal.TryReadRecent(1,previous) && previous.Ordinal==1,"chronological ring");
    Require(!f.Journal.TryReadRecent(2,previous),"out of range");
}

void SameGateComparisons()
{
    Fixture f;
    f.Sector(20);f.Sector(20);f.Sector(20);f.Sector(20);
    f.Frame.VelocityX=800;
    f.Sector(24);
    auto r=f.Read();
    Require(!r.LastComparison.Valid && r.Gates[0].Best.Lap==2,
        "first rolling start gate becomes reference");
    f.Sector(24);
    r=f.Read();
    Require(r.LastComparison.Valid && r.LastComparison.Gate==1 &&
        r.LastComparison.ReferenceLap==1 && r.LastComparison.ActualLap==2 &&
        Near(r.LastComparison.TimeDeltaSeconds,0.2) &&
        Near(r.LastComparison.MeanSpeedDeltaKmh,-7.2) &&
        r.LastComparison.MeaningfulTimeDelta,"same gate sampled comparison");
    f.Sector(24);f.Sector(24);
    f.Frame.VelocityX=1200;
    f.Sector(18);f.Sector(18);
    r=f.Read();
    Require(r.LastComparison.Valid && r.LastComparison.ReferenceLap==1 &&
        Near(r.LastComparison.TimeDeltaSeconds,-0.1) &&
        r.Gates[1].Best.Lap==3,"compare before best replacement");
    Require(!CompareSectorMotionRows(r.Last,r.Last).Valid,"never self compare");
    auto wrong=r.Gates[0].Best;
    Require(!CompareSectorMotionRows(r.Last,wrong).Valid,"never cross gate compare");
}

void MissingFramesAndTeleport()
{
    Fixture f;
    for(int i=0;i<10;++i) f.Tick();
    f.Tick(false);f.Tick();
    const double before=f.Read().Current.ObservedSeconds;
    ++f.Frame.TeleportRevision;
    f.Frame.Position.X+=5000;
    f.Tick();
    Require(Near(f.Read().Current.ObservedSeconds,before),"no recovery distance");
    f.Tick();
    ++f.Frame.SessionRevision;
    f.Tick();
    f.Tick();
    f.Frame.Position.X+=5000;
    f.Tick();
    for(int i=0;i<10;++i)f.Tick();
    Require(f.Gate(),"timing accepted despite incomplete evidence");
    const auto r=f.Read();
    Require(r.Last.Quality==SectorMotionQuality::Interrupted &&
        r.Last.Motion.Breaks==4 && r.Last.Motion.RecoveryBreaks==1 &&
        r.Last.Motion.PositionBreaks==1 && !r.Gates[0].Best.Comparable(),
        "all continuity interruptions explicit");
    Require(r.Last.Motion.DistanceMeters()<20.0,"teleport path omitted");
    f.Sector(20);
    Require(f.Read().Last.Comparable(),"next sector can recover");
}

void FeesAndInsufficientCoverage()
{
    Fixture f;
    for(int i=0;i<20;++i)f.Tick();
    f.Frame.ElapsedSeconds+=3;
    f.Frame.PenaltySeconds+=3;
    f.Tick();
    Require(f.Gate(),"official fee split");
    auto r=f.Read();
    Require(r.Last.Quality==SectorMotionQuality::Penalized &&
        Near(r.Last.FeeSeconds,3) && Near(r.Last.ActiveSeconds,1.05) &&
        Near(r.Last.OfficialSeconds,4.05),"fee excluded from active motion");
    for(int i=0;i<20;++i)f.Tick(i>=15);
    Require(f.Gate(),"timed with sparse motion");
    r=f.Read();
    Require(r.Last.Quality==SectorMotionQuality::InsufficientCoverage &&
        Near(r.Last.Coverage,0.2) && !r.Gates[1].Best.Comparable(),
        "cannot label partial coverage qualified");
    f.Sector(20);
    Require(f.Read().Last.Comparable(),"clean next sector unaffected");
}

void StaleAndForeignObservations()
{
    Fixture f;
    f.Tick();f.Tick();
    const auto baseline=f.Read().Current;
    Require(!f.Journal.Observe(f.Frame),"duplicate rejected");
    auto s=f.Frame;s.Actor=44;++s.EngineFrame;s.ElapsedSeconds+=0.05;
    Require(!f.Journal.Observe(s),"foreign actor rejected");
    s=f.Frame;--s.EngineFrame;
    Require(!f.Journal.Observe(s),"old frame rejected");
    f.Tick();
    const auto now=f.Read().Current;
    Require(now.Samples==baseline.Samples+1 && now.Breaks==0 &&
        Near(now.ObservedSeconds,baseline.ObservedSeconds+0.05),"foreign callback does not poison");
    s=f.Frame;++s.EngineFrame;s.ElapsedSeconds+=0.05;
    s.VelocityX=std::numeric_limits<double>::quiet_NaN();
    Require(!f.Journal.Observe(s),"invalid body observation");
    f.Tick(false);f.Tick();
    Require(f.Read().Current.Breaks==1 && f.Read().Current.Rejected==1,
        "malformed actual sample cuts duration");
}

void ReceiptAndFinishAuthority()
{
    Fixture f(1);
    for(int i=0;i<20;++i)f.Tick();
    Require(!f.Journal.AcceptGate({2,1,1,0,1}),"out of order refused");
    Require(!f.Journal.AcceptGate({1,1,1,0,1}),"wrong physical gate refused");
    Require(!f.Journal.AcceptGate({1,0,-1,0,1}),"invalid clock refused");
    Require(!f.Journal.AcceptGate({1,0,f.Frame.ElapsedSeconds,0,99}),
        "mismatched official split unavailable");
    ++f.Frame.Ordinal;f.Boundary=f.Frame.ElapsedSeconds;f.SetRoadPosition();
    auto r=f.Read();
    Require(r.Valid && r.AcceptedGates==1 && r.MissingTiming==1 &&
        !r.Last.ValidTiming,"unavailable timing not invented");
    Require(!f.Journal.Read(0,f.Frame.ElapsedSeconds,0,false).Valid,"wrong authoritative ordinal");
    f.Sector(20);f.Sector(20);f.Sector(20);
    r=f.Read();
    Require(r.Valid && r.Finished && r.TimedSectors==3 && r.QualifiedSectors==3,
        "finished exact real sectors");
    Require(!f.Journal.Observe(f.Frame),"no samples after completion");
    Require(!f.Journal.AcceptGate({5,0,10,0,1}),"no extra gate");
    Require(!f.Journal.Read(4,f.Frame.ElapsedSeconds,0,false).Valid,"finish state mismatch");
    f.Journal.Reset(4,1);
    Require(f.Journal.Read(0,0,0,false).Valid &&
        f.Journal.Read(0,0,0,false).RetainedRows==0,"restart clears references");
    f.Journal.Reset(5,1);
    Require(!f.Journal.Read(0,0,0,false).Valid,"unsupported track not misrepresented");
}

void FinishedReviewRequiresExactClocks()
{
    Fixture f(1);
    for(int i=0;i<CircuitGateCount;++i)f.Sector(20);
    const double elapsed=f.Frame.ElapsedSeconds;
    const double fee=f.Frame.PenaltySeconds;
    const int ordinal=f.Frame.Ordinal;
    Require(f.Journal.ReadFinishedHistory(ordinal,elapsed,fee).Valid,
        "exact finished history accepted");
    Require(!f.Journal.Read(ordinal,elapsed+1,fee,true).Valid,
        "post-finish active time cannot change frozen review");
    Require(!f.Journal.Read(ordinal,elapsed+1,fee+1,true).Valid,
        "post-finish fee cannot change frozen review");
    Require(!f.Journal.ReadFinishedHistory(ordinal,elapsed+1,fee+1).Valid,
        "finished export also requires exact clock");
    Require(f.Journal.Read(ordinal,elapsed+SectorMotionClockTolerance/2,fee,true).Valid,
        "floating point boundary tolerance retained");
}

void CutDoesNotEraseLatestSampleClock()
{
    Fixture f(1);
    for(int i=0;i<20;++i)f.Tick();
    f.Journal.Suspend();
    // Retained motion spans almost a second; the latest sample is later
    // than this receipt even though the accumulated duration fits it.
    const double early=f.Frame.ElapsedSeconds-0.025;
    Require(!f.Journal.AcceptGate({1,0,early,0,early}),
        "suspended future sample cannot qualify an earlier receipt");
    const auto review=f.Journal.Read(1,early,0,false);
    Require(review.Valid && review.MissingTiming==1 && !review.Last.ValidTiming,
        "ordered receipt consumed without fabricated timing evidence");

    Fixture g;
    g.Tick();g.Tick();
    g.Journal.Suspend();
    auto regressed=g.Frame;
    ++regressed.EngineFrame;
    regressed.ElapsedSeconds-=0.025;
    Require(!g.Journal.Observe(regressed),"cut cannot permit elapsed regression");
    Require(g.Read().Current.Rejected==1,"clock regression recorded");
    g.Tick(false);
    g.Tick();
    Require(g.Read().Current.Samples==3 && g.Read().Current.Breaks==1,
        "later valid frame resumes without bridging the cut");
    g.Tick();
    Require(Near(g.Read().Current.ObservedSeconds,0.10),
        "only real intervals on either side of cut counted");
}

void ConditionalIntervalsAndLongRuns()
{
    Fixture f;
    f.Frame.VelocityX=50;
    for(int i=0;i<6;++i)f.Tick();
    f.Frame.VelocityX=1000;
    f.Frame.HeadingDegrees=90;
    for(int i=0;i<8;++i)f.Tick();
    f.Journal.Suspend();
    for(int i=0;i<6;++i)f.Tick();
    Require(f.Gate(),"timing");
    const auto m=f.Read().Last.Motion;
    Require(Near(m.LowSpeedSeconds,0.25) && Near(m.LongestLowSpeedSeconds,0.25) &&
        Near(m.HighSlipSeconds,0.6) && Near(m.LongestSlipSeconds,0.35),
        "episodes cut at suspension and endpoints");
    Require(m.HighSlipSeconds<=m.ObservedSeconds && m.LowSpeedSeconds<=m.ObservedSeconds &&
        m.MinimumSpeed==50 && m.PeakSpeed==1000,"bounded sampled motion");
}

void LongHistory()
{
    Fixture f(30);
    for(int i=0;i<120;++i)f.Sector(20);
    const auto r=f.Read();
    Require(r.Valid && r.Finished && r.AcceptedGates==120 &&
        r.RetainedRows==SectorMotionHistoryCapacity && r.QualifiedSectors==120,
        "bounded history with whole-run summaries");
    SectorMotionRow row;
    Require(f.Journal.TryReadRecent(31,row) && row.Ordinal==89 &&
        !f.Journal.TryReadRecent(32,row),"oldest retained only");
    for(const auto& g:r.Gates)
        Require(g.Timed==30 && g.Qualified==30 && g.Best.Comparable() &&
            g.Best.Gate==g.Gate && Near(g.QualifiedMeanKmh(),36),"stable per-gate totals");
}

void RandomizedCoverage()
{
    Fixture f(20);
    std::mt19937 rng(42781);
    for(int sector=0;sector<80;++sector)
    {
        const int frames=20+static_cast<int>(rng()%20);
        for(int i=0;i<frames;++i)
        {
            f.Frame.VelocityX=500+static_cast<double>(rng()%1000);
            if(rng()%23==0) ++f.Frame.TeleportRevision;
            if(rng()%29==0) ++f.Frame.SessionRevision;
            f.Tick(rng()%17!=0);
        }
        Require(f.Gate(),"random accepted gate");
        const auto r=f.Read();
        const auto& row=r.Last;
        const auto& m=row.Motion;
        Require(row.Coverage>=0 && row.Coverage<=1 && m.ObservedSeconds<=row.ActiveSeconds+1e-6 &&
            m.Intervals<=m.Samples && m.LowSpeedSeconds<=m.ObservedSeconds+1e-6 &&
            m.FullOffRoadSeconds<=m.PartialRoadSeconds+1e-6 &&
            m.PartialRoadSeconds<=m.ObservedSeconds+1e-6 &&
            m.AverageKmh()<=m.PeakKmh()+1e-6,"interval conservation");
        if(row.Comparable())
            Require(row.Coverage>=SectorMotionMinimumCoverage && !m.Breaks &&
                row.FeeSeconds==0,"strict qualification");
    }
    Require(f.Read().Finished,"all authoritative gates");
}
}

int main()
{
    EndpointValidation();
    BoundariesAndCoverage();
    SameGateComparisons();
    MissingFramesAndTeleport();
    FeesAndInsufficientCoverage();
    StaleAndForeignObservations();
    ReceiptAndFinishAuthority();
    FinishedReviewRequiresExactClocks();
    CutDoesNotEraseLatestSampleClock();
    ConditionalIntervalsAndLongRuns();
    LongHistory();
    RandomizedCoverage();
    std::cout<<"Player sector motion, continuity and exact checkpoint review tests passed\n";
}
