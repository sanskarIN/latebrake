#include "Racing/DriveCommandTrace.h"

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
        std::cerr<<"DriveCommandTrace FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double epsilon=1e-8)
{
    return std::abs(a-b)<=epsilon;
}
DriveCommandSample Sample(std::uint64_t frame,double delta=0.05)
{
    return {frame,7,4,3,delta,0.8,0.5,-0.6,-0.4,false,false,true,true};
}
bool Push(DriveCommandTrace& trace,DriveCommandSample sample)
{
    trace.BeginFrame(sample.Frame);
    return trace.Observe(sample);
}
DriveCommandReview Read(const DriveCommandTrace& trace,std::uint64_t frame)
{
    return trace.Read(frame,7,4,3,true);
}
void CheckConservation(const DriveCommandReview& r)
{
    Require(r.Valid,"expected accepted observation");
    double binTime=0.0;
    double brake=0.0,hand=0.0,ground=0.0;
    for(const auto& b:r.Bins)
    {
        Require(b.ObservedSeconds>=0 && b.ObservedSeconds<=DriveTraceBinSeconds+1e-8,
            "bounded bin coverage");
        Require(std::abs(b.ModeratedThrottle)<=1.0+1e-8 &&
            std::abs(b.ModeratedSteering)<=1.0+1e-8,"bounded weighted axes");
        binTime+=b.ObservedSeconds;
        brake+=b.ObservedSeconds*b.ServiceBrakeFraction;
        hand+=b.ObservedSeconds*b.HandbrakeFraction;
        ground+=b.ObservedSeconds*b.GroundFraction;
    }
    Require(Near(binTime,r.ObservedSeconds),"bin time conserved");
    Require(Near(brake,r.ServiceBrakeSeconds) && Near(hand,r.HandbrakeSeconds) &&
        Near(ground,r.GroundSeconds),"weighted binary coverage conserved");
    Require(Near(r.MotorSeconds+r.OpposingThrottleSeconds+r.CoastSeconds+
        r.BrakeCommandSeconds+r.UnsupportedSeconds,r.ObservedSeconds),
        "exclusive command path times conserved");
    Require(Near(r.GroundSeconds+r.UnsupportedSeconds,r.ObservedSeconds),
        "support coverage conserved");
}
void FreshnessAndDiscontinuity()
{
    DriveCommandTrace trace;
    Require(!Read(trace,1).Valid,"no invented initial history");
    Require(Push(trace,Sample(1)),"first");
    auto r=Read(trace,1);
    CheckConservation(r);
    Require(Near(r.ObservedSeconds,0.05) &&
        Near(r.Bins.back().ObservedSeconds,0.05),"partial newest bucket");
    Require(Near(r.MeanThrottleModeration,0.3) &&
        Near(r.MeanSteeringModeration,0.2),"actual filtered-command difference");
    for(std::size_t i=0;i<DriveTraceBinCount-1;++i)
        Require(r.Bins[i].ObservedSeconds==0,"no prehistory");
    Require(Read(trace,2).Valid && !Read(trace,3).Valid,"one-frame draw tolerance");
    Require(!trace.Read(1,8,4,3,true).Valid,"session identity");
    Require(!trace.Read(1,7,5,3,true).Valid,"teleport identity");
    Require(!trace.Read(1,7,4,8,true).Valid,"controller identity");
    Require(!trace.Read(1,7,4,3,false).Valid,"paused view");
    trace.BeginFrame(2);
    Require(!Read(trace,2).Valid,"uncompleted physics frame");
    Require(Push(trace,Sample(3)),"resume after skipped frame");
    Require(Near(Read(trace,3).ObservedSeconds,0.05),"no bridge through failed tick");
    Require(Push(trace,Sample(4)),"adjacent frame");
    Require(Near(Read(trace,4).ObservedSeconds,0.1),"continuous duration");
    Require(Push(trace,Sample(7)),"gap starts fresh");
    Require(Near(Read(trace,7).ObservedSeconds,0.05),"gap cuts history");
    auto changed=Sample(8);changed.Teleport=5;
    Require(Push(trace,changed),"recovery observation");
    Require(Near(trace.Read(8,7,5,3,true).ObservedSeconds,0.05),"recovery cuts history");
    changed.Frame=9;changed.Session=8;
    Require(Push(trace,changed),"new race");
    Require(Near(trace.Read(9,8,5,3,true).ObservedSeconds,0.05),"race cuts history");
    changed.Frame=10;changed.Controller=4;
    Require(Push(trace,changed),"new owner");
    Require(Near(trace.Read(10,8,5,4,true).ObservedSeconds,0.05),"owner cuts history");
    trace.Suspend();
    Require(!trace.Read(10,8,5,4,true).Valid,"explicit input interruption");
}
void CommandPaths()
{
    DriveCommandTrace trace;
    auto s=Sample(1);
    Require(Push(trace,s),"motor frame");
    s.Frame=2;s.MotorBranch=false;s.ModeratedThrottle=-0.4;
    Require(Push(trace,s),"opposing throttle frame");
    s.Frame=3;s.ModeratedThrottle=0;s.MotorBranch=true;
    Require(Push(trace,s),"coast frame");
    s.Frame=4;s.ServiceBrake=true;s.MotorBranch=false;
    Require(Push(trace,s),"service frame");
    s.Frame=5;s.Handbrake=true;s.ServiceBrake=false;
    Require(Push(trace,s),"handbrake frame");
    s.Frame=6;s.GroundSupported=false;
    Require(Push(trace,s),"airborne frame");
    auto r=Read(trace,6);
    CheckConservation(r);
    Require(r.Path==DriveCommandPath::NoGroundSupport,"no ground path priority");
    Require(Near(r.MotorSeconds,0.05) && Near(r.OpposingThrottleSeconds,0.05) &&
        Near(r.CoastSeconds,0.05) && Near(r.BrakeCommandSeconds,0.1) &&
        Near(r.UnsupportedSeconds,0.05),"independent path durations");
    Require(Near(r.ServiceBrakeSeconds,0.05) && Near(r.HandbrakeSeconds,0.1),
        "brake requests are independent of support");
}
void ExactWindowAndWeightedBuckets()
{
    DriveCommandTrace trace;
    for(std::uint64_t frame=1;frame<=100;++frame)
    {
        auto s=Sample(frame);
        if(frame>60)
        {
            s.FilteredThrottle=-1;
            s.ModeratedThrottle=-0.5;
            s.FilteredSteering=0.2;
            s.ModeratedSteering=0;
        }
        Require(Push(trace,s),"continuous window");
    }
    auto r=Read(trace,100);
    CheckConservation(r);
    Require(Near(r.ObservedSeconds,4.0),"exact four-second clip");
    Require(Near(r.Bins.front().ModeratedThrottle,0.5) &&
        Near(r.Bins.back().ModeratedThrottle,-0.5),"time-ordered signed axes");
    Require(Near(r.MeanThrottleModeration,0.4),"weighted moderation after clipping");
    trace.Suspend();
    auto a=Sample(101,0.07);a.ModeratedThrottle=1.0;
    auto b=Sample(102,0.08);b.ModeratedThrottle=-1.0;
    Require(Push(trace,a) && Push(trace,b),"uneven frames");
    r=Read(trace,102);
    CheckConservation(r);
    Require(Near(r.Bins.back().ModeratedThrottle,(0.045-0.08)/0.125),
        "split frame at exact bin boundary");
    Require(Near(r.Bins[DriveTraceBinCount-2].ObservedSeconds,0.025),
        "oldest partial bin");
}
void TransientCommandsAndWindowExtrema()
{
    DriveCommandTrace trace;
    auto positive=Sample(1,0.06);
    positive.FilteredThrottle=0.5;
    positive.ModeratedThrottle=1;
    positive.FilteredSteering=0;
    positive.ModeratedSteering=-1;
    auto negative=Sample(2,0.06);
    negative.FilteredThrottle=0;
    negative.ModeratedThrottle=-1;
    negative.FilteredSteering=0.5;
    negative.ModeratedSteering=1;
    Require(Push(trace,positive) && Push(trace,negative),"brief signed commands");
    auto review=Read(trace,2);
    CheckConservation(review);
    const auto& bin=review.Bins.back();
    Require(Near(bin.ModeratedThrottle,0) && Near(bin.ModeratedSteering,0),
        "opposite commands cancel in the mean");
    Require(bin.ModeratedThrottleRange.Minimum==-1 &&
        bin.ModeratedThrottleRange.Maximum==1 &&
        bin.ModeratedSteeringRange.Minimum==-1 &&
        bin.ModeratedSteeringRange.Maximum==1,
        "actual brief extrema retained despite cancellation");
    Require(Near(review.PeakThrottleModeration,1) &&
        Near(review.PeakSteeringModeration,1),"peak command differences");
    Require(Near(review.MeanThrottleModeration,0.75) &&
        Near(review.MeanSteeringModeration,0.75),"mean differs from peak");
    for(std::uint64_t frame=3;frame<=82;++frame)
    {
        auto current=Sample(frame);
        current.ModeratedThrottle=current.FilteredThrottle=0.25;
        current.ModeratedSteering=current.FilteredSteering=-0.25;
        Require(Push(trace,current),"advance past transient observation");
    }
    review=Read(trace,82);
    Require(Near(review.ObservedSeconds,4) && Near(review.PeakThrottleModeration,0) &&
        Near(review.PeakSteeringModeration,0),"old peaks expire with observed window");
    for(const auto& current:review.Bins)
    {
        Require(current.ModeratedThrottleRange.Minimum==0.25 &&
            current.ModeratedThrottleRange.Maximum==0.25 &&
            current.ModeratedSteeringRange.Minimum==-0.25 &&
            current.ModeratedSteeringRange.Maximum==-0.25,
            "constant observed extrema");
    }
    trace.Suspend();
    Require(Push(trace,Sample(83)),"new trace after reset");
    review=Read(trace,83);
    Require(review.Bins.back().ModeratedThrottleRange.Minimum==0.5 &&
        review.Bins.back().ModeratedThrottleRange.Maximum==0.5,
        "no transient range leaks into a new trace");
}
void RejectMalformedSamples()
{
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(int test=0;test<12;++test)
    {
        DriveCommandTrace trace;
        Require(Push(trace,Sample(1)),"seed before malformed sample");
        auto s=Sample(2);
        switch(test)
        {
        case 0:s.DeltaSeconds=nan;break;
        case 1:s.DeltaSeconds=0;break;
        case 2:s.DeltaSeconds=0.101;break;
        case 3:s.FilteredThrottle=1.001;break;
        case 4:s.ModeratedThrottle=nan;break;
        case 5:s.FilteredSteering=-1.001;break;
        case 6:s.ModeratedSteering=nan;break;
        case 7:s.Controller=0;break;
        case 8:s.ServiceBrake=true;break;
        case 9:s.GroundSupported=false;break;
        case 10:s.Handbrake=true;s.MotorBranch=false;break;
        case 11:s.Frame=0;break;
        default:break;
        }
        trace.BeginFrame(2);
        Require(!trace.Observe(s),"malformed observation rejected");
        Require(!Read(trace,2).Valid,"malformed sample invalidates old readout");
        Require(Push(trace,Sample(3)),"recover after invalid sample");
        Require(Near(Read(trace,3).ObservedSeconds,0.05),"malformed frame cuts history");
    }
    DriveCommandTrace trace;
    Require(Push(trace,Sample(1)),"one receipt");
    Require(!trace.Observe(Sample(1)),"duplicate receipt not counted");
    Require(!Read(trace,1).Valid,"duplicate fails closed");
}
void CapacityAndVariableRate()
{
    DriveCommandTrace trace;
    for(std::uint64_t frame=1;frame<=1200;++frame)
        Require(Push(trace,Sample(frame,0.001)),"high-frequency samples");
    auto r=Read(trace,1200);
    CheckConservation(r);
    Require(Near(r.ObservedSeconds,1.024),"bounded capacity reports actual coverage");
    trace.Suspend();
    std::mt19937 random(319u);
    std::uniform_real_distribution<double> dt(0.001,0.099);
    std::uniform_real_distribution<double> axis(-1.0,1.0);
    for(std::uint64_t frame=1;frame<=4000;++frame)
    {
        auto s=Sample(frame,dt(random));
        s.FilteredThrottle=axis(random);
        s.ModeratedThrottle=axis(random);
        s.FilteredSteering=axis(random);
        s.ModeratedSteering=axis(random);
        s.ServiceBrake=random()%5==0;
        s.Handbrake=random()%7==0;
        s.GroundSupported=random()%8!=0;
        s.MotorBranch=s.GroundSupported && !s.ServiceBrake && !s.Handbrake;
        if(s.Handbrake)s.ModeratedThrottle=0;
        Require(Push(trace,s),"random valid sample");
        if(frame%23==0)
        {
            r=Read(trace,frame);
            CheckConservation(r);
            Require(r.ObservedSeconds<=4.0+1e-8,"random coverage bounded");
        }
    }
}
}
int main()
{
    FreshnessAndDiscontinuity();
    CommandPaths();
    ExactWindowAndWeightedBuckets();
    TransientCommandsAndWindowExtrema();
    RejectMalformedSamples();
    CapacityAndVariableRate();
    std::cout<<"DriveCommandTrace passed\n";
}
