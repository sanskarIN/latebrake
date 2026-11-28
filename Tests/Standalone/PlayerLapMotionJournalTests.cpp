#include "Racing/PlayerLapMotionJournal.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"PlayerLapMotionJournal: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
SectorMotionRow Row(int ordinal,double active,double fee,double observed,
    SectorMotionQuality quality=SectorMotionQuality::Qualified)
{
    SectorMotionRow row;
    row.ValidTiming=true;
    row.Ordinal=ordinal;
    row.Gate=(ordinal-1)%4;
    row.Lap=(ordinal-1)/4+1;
    row.StandingStart=ordinal==1;
    row.ActiveSeconds=active;
    row.FeeSeconds=fee;
    row.OfficialSeconds=active+fee;
    row.Coverage=observed/active;
    row.Quality=quality;
    row.Motion.ObservedSeconds=observed;
    if(observed>0)
    {
        row.Motion.Samples=100;
        row.Motion.Intervals=99;
        row.Motion.SpeedIntegral=1000*observed;
        row.Motion.PeakSpeed=1200;
        row.Motion.PlanarDistanceCm=1000*observed;
        row.Motion.LowSpeedSeconds=observed/8;
        row.Motion.HighSlipSeconds=observed/4;
    }
    if(quality==SectorMotionQuality::Interrupted)row.Motion.Breaks=1;
    return row;
}
void TenLapRollup()
{
    PlayerLapMotionJournal journal;
    journal.Reset(4,10);
    double elapsed=0,penalty=0;
    for(int i=1;i<=40;++i)
    {
        const int lap=(i-1)/4+1;
        const double active=lap==4?8:10;
        const double fee=lap==6 && i%4==1?2:0;
        const double observed=lap==8?active/2:active;
        const auto quality=fee?SectorMotionQuality::Penalized:lap==7?
            SectorMotionQuality::Interrupted:lap==8?
            SectorMotionQuality::InsufficientCoverage:SectorMotionQuality::Qualified;
        elapsed+=active+fee;penalty+=fee;
        Require(journal.Accept({i,(i-1)%4,elapsed,penalty,active+fee},
            Row(i,active,fee,observed,quality)),"accepted gate");
        const auto read=journal.Read(i,elapsed,penalty,i==40);
        Require(read.Valid && read.CompletedLaps==i/4 && read.Current.Gates==i%4,
            "partial lap stays separate");
    }
    const auto result=journal.Read(40,elapsed,penalty,true);
    Require(result.Valid && result.CompletedLaps==10 && result.AcceptedGates==40,
        "all ten laps retained beyond old sector ring");
    Require(result.QualifiedLaps==7 && result.ComparableLaps==6 &&
        result.PenalizedLaps==1 && result.InterruptedLaps==1 &&
        result.LowCoverageLaps==1 && result.MissingTimingLaps==0,"quality counts");
    Require(result.At(1).StandingStart && !result.At(1).Comparable() &&
        result.BestRolling.Lap==4 && result.SlowestRolling.Lap==2,"rolling lap selection");
    Require(result.Completed.ActiveSeconds==392 && result.Completed.FeeSeconds==2 &&
        result.Completed.OfficialSeconds==394 && result.Completed.ObservedSeconds==372,
        "whole attempt clocks and actual coverage");
    Require(LapMotionClose(result.Completed.MeanKmh(),36) &&
        LapMotionClose(result.Completed.DistanceMeters(),3720),"weighted observed movement");
    Require(result.At(10).Present && !result.At(11).Present && !result.At(0).Present,
        "bounded lookup");
}
void MissingAndMalformedMotion()
{
    PlayerLapMotionJournal journal;
    journal.Reset(4,2);
    Require(!journal.Accept({2,1,10,0,10},{}),"out of order");
    Require(journal.Accept({1,0,10,0,10},{}),"timing without motion");
    Require(!journal.Accept({1,0,10,0,10},{}),"duplicate gate");
    auto wrong=Row(2,10,0,10);
    wrong.Lap=99;
    Require(journal.Accept({2,1,20,0,10},wrong),"wrong motion does not erase timing");
    auto corrupt=Row(3,10,0,10);
    corrupt.Motion.SpeedIntegral=1e9;
    Require(journal.Accept({3,2,30,0,10},corrupt),"corrupt motion rejected only");
    Require(journal.Accept({4,3,40,0,10},Row(4,10,0,10)),"valid last gate");
    auto review=journal.Read(4,40,0,false);
    Require(review.Valid && review.At(1).Quality==LapMotionQuality::InsufficientCoverage &&
        review.At(1).Totals.TimedGates==4 && review.At(1).Totals.SampledGates==1 &&
        review.At(1).Totals.ObservedSeconds==10,"no invented missing motion");
    Require(journal.Accept({5,0,40,0,0},{}),"untimed legitimate gate");
    for(int i=6;i<=8;++i)
        Require(journal.Accept({i,(i-1)%4,40.0+10.0*(i-5),0,10},Row(i,10,0,10)),
            "remaining gate");
    review=journal.Read(8,70,0,true);
    Require(review.Valid && review.MissingTimingLaps==1 &&
        review.At(2).Quality==LapMotionQuality::MissingTiming,"untimed sector disclosed");
    Require(!journal.Read(8,71,0,true).Valid && !journal.Read(7,70,0,true).Valid &&
        !journal.Read(8,70,0,false).Valid,"finished clock and ordinal must reconcile");
}
void MotionBounds()
{
    auto row=Row(1,10,0,9);
    Require(ValidLapMotionMetrics(row.Motion,10),"valid observed sample");
    for(int fault=0;fault<10;++fault)
    {
        auto bad=row.Motion;
        switch(fault)
        {
        case 0:bad.ObservedSeconds=11;break;
        case 1:bad.Intervals=101;break;
        case 2:bad.SpeedIntegral=-1;break;
        case 3:bad.PlanarDistanceCm=-1;break;
        case 4:bad.PeakSpeed=16000;break;
        case 5:bad.HighSlipSeconds=11;break;
        case 6:bad.FullOffRoadSeconds=2;break;
        case 7:bad.ReverseSeconds=std::numeric_limits<double>::quiet_NaN();break;
        case 8:bad.Intervals=0;break;
        case 9:bad.ObservedSeconds=0;break;
        }
        Require(!ValidLapMotionMetrics(bad,10),"malformed metrics refused");
    }
}
void RandomizedWholeRun()
{
    std::mt19937 rng(3711);
    PlayerLapMotionJournal journal;
    for(int trial=0;trial<50;++trial)
    {
        const int laps=1+static_cast<int>(rng()%100);
        journal.Reset(4,laps);
        double elapsed=0,observed=0;
        for(int ordinal=1;ordinal<=laps*4;++ordinal)
        {
            const double active=5+static_cast<double>(rng()%40)/4;
            const double coverage=static_cast<double>(rng()%5+4)/8;
            const double seen=active*coverage;
            elapsed+=active;observed+=seen;
            Require(journal.Accept({ordinal,(ordinal-1)%4,elapsed,0,active},
                Row(ordinal,active,0,seen,coverage>=0.85?SectorMotionQuality::Qualified:
                    SectorMotionQuality::InsufficientCoverage)),"ordered randomized receipt");
        }
        const auto result=journal.Read(laps*4,elapsed,0,true);
        Require(result.Valid && result.CompletedLaps==laps &&
            LapMotionClose(result.Completed.ObservedSeconds,observed) &&
            LapMotionClose(result.Completed.OfficialSeconds,elapsed),
            "full physical attempt rollup");
        Require(result.QualifiedLaps+result.LowCoverageLaps==laps &&
            result.Completed.Gates==laps*4,"all lap quality categories reconcile");
    }
    journal.Reset(3,10);
    Require(!journal.Read(0,0,0,false).Valid,"unsupported track");
    journal.Reset(4,101);
    Require(!journal.Read(0,0,0,false).Valid,"bounded lap count");
}
}
int main()
{
    TenLapRollup();MissingAndMalformedMotion();MotionBounds();RandomizedWholeRun();
    std::cout<<"Player lap motion journal tests passed\n";
}
