#include "Racing/DrivingCoachInsights.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;

void Check(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"DrivingCoachInsights failure: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double x,double y,double tolerance=1.0e-7)
{
    return std::abs(x-y)<=tolerance*std::max({1.0,std::abs(x),std::abs(y)});
}

DrivingCoachSample Sample(double dt=0.05,double speed=1500.0,
    double lateral=0.0,bool braking=false)
{
    return {dt,speed,lateral,braking};
}

void SampleValidation()
{
    Check(ValidDrivingCoachSample(Sample()),"ordinary sample accepted");
    Check(ValidDrivingCoachSample(Sample(0.25,10000.0,-10000.0)),
        "inclusive upper sensor bounds");
    Check(!ValidDrivingCoachSample(Sample(0.0)),"zero time rejected");
    Check(!ValidDrivingCoachSample(Sample(-0.05)),"negative time rejected");
    Check(!ValidDrivingCoachSample(Sample(0.251)),"long frame rejected");
    Check(!ValidDrivingCoachSample(Sample(0.05,-0.01)),"reverse speed magnitude rejected");
    Check(!ValidDrivingCoachSample(Sample(0.05,10001.0)),"overspeed rejected");
    Check(!ValidDrivingCoachSample(Sample(0.05,1000.0,10001.0)),
        "lateral overspeed rejected");
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(double poison : {nan,inf,-inf})
    {
        Check(!ValidDrivingCoachSample(Sample(poison)),"nonfinite dt rejected");
        Check(!ValidDrivingCoachSample(Sample(0.05,poison)),
            "nonfinite planar speed rejected");
        Check(!ValidDrivingCoachSample(Sample(0.05,1000.0,poison)),
            "nonfinite lateral speed rejected");
        Check(!DrivingCoachHighSlip(Sample(0.05,poison,600.0)),
            "bad speed cannot claim slip event");
    }
    Check(!DrivingCoachHighSlip(Sample(0.05,850.0,800.0)),
        "below minimum speed cannot claim high slip");
    Check(!DrivingCoachHighSlip(Sample(0.05,1000.0,499.0)),
        "below lateral threshold cannot claim high slip");
    Check(DrivingCoachHighSlip(Sample(0.05,1000.0,500.0)),
        "both thresholds are inclusive");
    Check(!DrivingCoachHighSlip(Sample(0.05,2000.0,600.0)),
        "fraction threshold also required");
    Check(DrivingCoachHighSlip(Sample(0.05,2000.0,-700.0)),
        "fraction boundary and sign ignored for slip magnitude");
}

void FreshAndBasicObservation()
{
    DrivingCoachAccumulator coach;
    auto empty=coach.Review();
    Check(!empty.Valid && empty.Evidence==DrivingCoachEvidence::Unavailable,
        "fresh session has no invented driving evidence");
    Check(empty.NoGameplayAuthority && empty.NoPersistentData && empty.NoOfficialTiming,
        "fresh advisory-only declaration");
    for(int i=0;i<20;++i)
        Check(coach.Observe(Sample(0.05,1000.0,0.0,i<4)),
            "valid observed frame accepted");
    auto result=coach.Review();
    Check(result.Valid && result.Evidence==DrivingCoachEvidence::Limited,
        "one second cannot qualify useful review");
    Check(Near(result.ObservedSeconds,1.0) && Near(result.DistanceKm,0.01) &&
        Near(result.MeanSpeedKmh,36.0) && Near(result.PeakSpeedKmh,36.0),
        "distance and speed carry actual sampled units");
    Check(Near(result.BrakeSeconds,0.2) && Near(result.BrakeFraction,0.2),
        "brake share is time-weighted");
    Check(result.AcceptedSamples==20 && result.RefusedSamples==0,
        "samples counted exactly");
    Check(result.AcceptedBrakeEpisodes==1 && result.CompletedBrakeEpisodes==1,
        "observed brake edge retained as one episode");
    Check(result.RetainedBrakeEpisodes==1 && result.Brakes[0].Complete &&
        Near(result.Brakes[0].DurationSeconds,0.2),
        "brake episode duration and completion");
    Check(result.RetainedWindows==1 && !result.Windows[0].Complete &&
        Near(result.Windows[0].ObservedSeconds,1.0),
        "unfinished five-second window is labeled as partial");
    Check(result.Message==DrivingCoachMessage::CollectMoreSamples,
        "early session cannot give strong advice");
    Check(result.NoGameplayAuthority && result.NoPersistentData &&
        result.NoOfficialTiming,"valid session cannot gain race authority");
}

void BrakeEpisodeLifecycle()
{
    DrivingCoachAccumulator coach;
    Check(coach.Observe(Sample(0.1,2000.0,0,true)),"first braking frame");
    Check(coach.Observe(Sample(0.1,1800.0,0,true)),"second braking frame");
    auto review=coach.Review();
    Check(review.AcceptedBrakeEpisodes==1 && review.CompletedBrakeEpisodes==0,
        "active brake not yet finalized");
    Check(review.RetainedBrakeEpisodes==1 &&
        !review.Brakes[0].Complete && review.Brakes[0].Valid,
        "active episode preview is incomplete");
    Check(Near(review.Brakes[0].EntrySpeedKmh,72.0) &&
        Near(review.Brakes[0].ExitSpeedKmh,64.8) &&
        Near(review.Brakes[0].MinimumSpeedKmh,64.8) &&
        Near(review.Brakes[0].DurationSeconds,0.2),
        "episode evidence comes from sampled speed only");
    coach.Observe(Sample(0.1,1750.0,0,false));
    review=coach.Review();
    Check(review.CompletedBrakeEpisodes==1 && review.Brakes[0].Complete &&
        Near(review.Brakes[0].EndedSeconds,0.2),
        "release finalizes at last observed brake sample");
    coach.Observe(Sample(0.1,1900.0,0,true));
    review=coach.Review();
    Check(review.AcceptedBrakeEpisodes==2 && review.CompletedBrakeEpisodes==1 &&
        review.RetainedBrakeEpisodes==2,
        "re-application starts new episode without revising prior");
    coach.Suspend();
    review=coach.Review();
    Check(review.CompletedBrakeEpisodes==2 && review.Brakes[0].Complete,
        "suspension closes active brake without joining across pause");
    coach.Suspend();
    Check(coach.Review().CompletedBrakeEpisodes==2,
        "repeat pause does not duplicate one episode");
    coach.Observe(Sample(0.1,1900.0,0,true));
    Check(coach.Review().AcceptedBrakeEpisodes==3,
        "resumed held brake starts a fresh episode");
}

void SlipEpisodeLifecycle()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<30;++i)
        coach.Observe(Sample(0.1,2000.0,i<12?800.0:0.0));
    auto review=coach.Review();
    Check(Near(review.HighSlipSeconds,1.2) &&
        Near(review.HighSlipFraction,0.4),
        "high slip duration uses accepted frames only");
    Check(review.AcceptedSlipEpisodes==1 && review.CompletedSlipEpisodes==1 &&
        review.RetainedSlipEpisodes==1,"slip episode releases on real observation");
    Check(Near(review.Slips[0].PeakSlipFraction,0.4) &&
        Near(review.Slips[0].DistanceKm,0.024),
        "episode peak fraction and distance use real sample values");
    for(int i=0;i<30;++i)
        coach.Observe(Sample(0.1,2000.0,800.0));
    review=coach.Review();
    Check(review.Evidence==DrivingCoachEvidence::Useful &&
        review.Message==DrivingCoachMessage::HighLateralSlipObserved,
        "sufficient sample duration admits observed-slip advice");
    coach.Suspend();
    review=coach.Review();
    Check(review.CompletedSlipEpisodes==2 && review.Slips[0].Complete,
        "suspension closes slip episode instead of stitching teleport");
}

void SpeedBandBoundaries()
{
    DrivingCoachAccumulator coach;
    constexpr std::array<double,8> speeds{{0.0,29.999,30.0,60.0,
        90.0,120.0,180.0,360.0}};
    for(double speed:speeds)
        Check(coach.Observe(Sample(0.25,speed/0.036)),
            "speed edge sample accepted");
    auto review=coach.Review();
    const auto& b=review.SpeedBands;
    Check(b[0].Samples==2 && b[1].Samples==1 && b[2].Samples==1 &&
        b[3].Samples==1 && b[4].Samples==1 && b[5].Samples==1 &&
        b[6].Samples==1,"speed bands use half-open boundaries");
    double sumSeconds=0.0,sumDistance=0.0;
    std::uint64_t count=0;
    for(const auto& row:b)
    {
        Check(row.Seconds>=0.0 && row.DistanceKm>=0.0 &&
            row.BrakingSeconds<=row.Seconds && row.HighSlipSeconds<=row.Seconds,
            "all band aggregates remain bounded");
        sumSeconds+=row.Seconds;
        sumDistance+=row.DistanceKm;
        count+=row.Samples;
    }
    Check(Near(sumSeconds,review.ObservedSeconds) &&
        Near(sumDistance,review.DistanceKm) && count==review.AcceptedSamples,
        "every accepted observation occupies exactly one speed band");
}

void WindowsAndSuspensions()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<120;++i)
        coach.Observe(Sample(0.05,1000.0,0.0,i%2==0));
    auto review=coach.Review();
    Check(review.RetainedWindows==2,"five-second window and active tail");
    Check(review.Windows[1].Complete==true &&
        Near(review.Windows[1].ObservedSeconds,5.0),
        "full window has exact sampled five-second duration");
    Check(!review.Windows[0].Complete &&
        Near(review.Windows[0].ObservedSeconds,1.0),
        "second bucket remains a partial observational window");
    Check(Near(review.Windows[1].BrakingFraction,0.5),
        "window brake share counts samples by duration");
    coach.Suspend();
    review=coach.Review();
    Check(review.RetainedWindows==2 && review.Windows[0].Complete==false,
        "partial bucket archived without inventing completeness");
    for(int i=0;i<100;++i)
        coach.Observe(Sample(0.05,1200.0,0.0));
    review=coach.Review();
    Check(review.RetainedWindows==3 && review.Windows[0].Complete &&
        review.Windows[0].Sequence==3,
        "new observed window does not bridge pause");
    Check(review.Windows[1].Sequence==2 && review.Windows[2].Sequence==1,
        "window ordering is newest first");
}

void RingRetentionAndMonotonicity()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<85;++i)
    {
        coach.Observe(Sample(0.05,2000.0,800.0,true));
        coach.Observe(Sample(0.05,2000.0,0.0,false));
    }
    auto review=coach.Review();
    Check(review.AcceptedBrakeEpisodes==85 &&
        review.CompletedBrakeEpisodes==85 &&
        review.AcceptedSlipEpisodes==85 &&
        review.CompletedSlipEpisodes==85,
        "total counts never truncated by archive retention");
    Check(review.RetainedBrakeEpisodes==DrivingCoachEpisodeCapacity &&
        review.RetainedSlipEpisodes==DrivingCoachEpisodeCapacity,
        "fixed episode rings have bounded memory");
    Check(review.Brakes[0].StartedSeconds>review.Brakes[31].StartedSeconds &&
        review.Slips[0].StartedSeconds>review.Slips[31].StartedSeconds,
        "recent episode ordering survives ring wrap");
    for(int i=0;i<12000;++i)
        coach.Observe(Sample(0.05,1200.0,0.0,false));
    review=coach.Review();
    Check(review.RetainedWindows==DrivingCoachWindowCapacity,
        "window ring eventually wraps at capacity");
    for(std::size_t i=1;i<review.RetainedWindows;++i)
        Check(review.Windows[i-1].Sequence>review.Windows[i].Sequence,
            "newest-first ring order preserved after many wraps");
}

void RefusalDoesNotInventDriving()
{
    DrivingCoachAccumulator coach;
    coach.Observe(Sample(0.1,2500.0,0,true));
    const auto before=coach.Review();
    Check(!coach.Observe(Sample(0.4,1000.0,0,true)),
        "invalid frame is rejected");
    const auto after=coach.Review();
    Check(Near(after.ObservedSeconds,before.ObservedSeconds) &&
        Near(after.DistanceKm,before.DistanceKm) &&
        after.AcceptedSamples==before.AcceptedSamples &&
        after.RefusedSamples==before.RefusedSamples+1,
        "rejected observation cannot inflate time, distance or accepted samples");
    Check(after.CompletedBrakeEpisodes==1 && after.Suspensions==1,
        "invalid sample forces discontinuity boundary");
    coach.Observe(Sample(0.1,2500.0,0,true));
    Check(coach.Review().AcceptedBrakeEpisodes==2,
        "new valid sample after refusal begins a new observed episode");
}

void EvidenceAndMessagePolicy()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,1500.0,0.0,false));
    auto review=coach.Review();
    Check(review.Evidence==DrivingCoachEvidence::Useful &&
        review.Message==DrivingCoachMessage::StableObservedMotion,
        "ten seconds of clean samples yields useful neutral review");
    coach.Reset();
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,1500.0,0.0,i%2==0));
    review=coach.Review();
    Check(review.AcceptedBrakeEpisodes==100 &&
        review.Message==DrivingCoachMessage::FrequentBrakeReapplication,
        "short repeated presses produce descriptive brake observation");
    coach.Reset();
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,1500.0,0.0,i<100));
    review=coach.Review();
    Check(review.AcceptedBrakeEpisodes==1 &&
        review.Message==DrivingCoachMessage::HeavyBrakingShare,
        "long measured brake share differs from repeated tapping");
    coach.Reset();
    for(int i=0;i<1300;++i)
        coach.Observe(Sample(0.05,2500.0,0.0,false));
    review=coach.Review();
    Check(review.Evidence==DrivingCoachEvidence::Broad &&
        review.ObservedSeconds>60.0,
        "long observed session qualifies broad evidence");
    for(auto message : {DrivingCoachMessage::CollectMoreSamples,
        DrivingCoachMessage::HighLateralSlipObserved,
        DrivingCoachMessage::FrequentBrakeReapplication,
        DrivingCoachMessage::HeavyBrakingShare,
        DrivingCoachMessage::StableObservedMotion,
        DrivingCoachMessage::LowMotionCoverage})
        Check(DrivingCoachMessageLabel(message)[0]!='\0',
            "all advice codes have descriptive labels");
}

void DeterministicReferenceOracle()
{
    DrivingCoachAccumulator coach;
    std::mt19937_64 random(0x5EED0123449ULL);
    std::uniform_real_distribution<double> speed(0.0,9999.9999);
    std::uniform_real_distribution<double> lateral(-9999.0,9999.0);
    std::uniform_real_distribution<double> dt(0.005,0.15);
    std::uniform_int_distribution<int> select(0,6);
    double expectedTime=0.0;
    double expectedDistance=0.0;
    double expectedBrake=0.0;
    double expectedSlip=0.0;
    std::uint64_t expectedAccepted=0;
    std::array<std::uint64_t,DrivingCoachSpeedBandCount> expectedBand{};
    for(int i=0;i<18000;++i)
    {
        const auto decision=select(random);
        if(decision==0)
        {
            coach.Suspend();
            continue;
        }
        const auto observation=Sample(dt(random),speed(random),
            lateral(random),decision==1);
        Check(coach.Observe(observation),"bounded random input accepted");
        expectedTime+=observation.DeltaSeconds;
        expectedDistance+=observation.DeltaSeconds*observation.PlanarSpeedCmps/100000.0;
        if(observation.Braking)expectedBrake+=observation.DeltaSeconds;
        if(DrivingCoachHighSlip(observation))expectedSlip+=observation.DeltaSeconds;
        ++expectedAccepted;
        const double kmh=observation.PlanarSpeedCmps*0.036;
        const std::size_t band=kmh<30?0:kmh<60?1:kmh<90?2:
            kmh<120?3:kmh<180?4:kmh<240?5:6;
        ++expectedBand[band];
        if((i%37)==0)
        {
            auto review=coach.Review();
            Check(Near(review.ObservedSeconds,expectedTime) &&
                Near(review.DistanceKm,expectedDistance) &&
                Near(review.BrakeSeconds,expectedBrake) &&
                Near(review.HighSlipSeconds,expectedSlip) &&
                review.AcceptedSamples==expectedAccepted,
                "online observations equal independent accumulation oracle");
            for(std::size_t k=0;k<expectedBand.size();++k)
                Check(review.SpeedBands[k].Samples==expectedBand[k],
                    "each random speed belongs to exactly its oracle band");
        }
    }
    const auto final=coach.Review();
    Check(final.AcceptedSamples==expectedAccepted &&
        Near(final.ObservedSeconds,expectedTime) &&
        final.RetainedWindows<=DrivingCoachWindowCapacity &&
        final.RetainedBrakeEpisodes<=DrivingCoachEpisodeCapacity &&
        final.RetainedSlipEpisodes<=DrivingCoachEpisodeCapacity,
        "long seeded fixture bounded and deterministic");
}

void MotionCoverageAndVariation()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,0.0,0.0,false));
    auto r=coach.Review();
    Check(r.Message==DrivingCoachMessage::LowMotionCoverage &&
        Near(r.StationarySeconds,10.0) && Near(r.MovingSeconds,0.0) &&
        Near(r.MovingFraction,0.0),
        "stationary observation cannot claim stable moving skill");
    coach.Reset();
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,i%2==0?1000.0:1500.0));
    r=coach.Review();
    Check(Near(r.MovingSeconds,10.0) && Near(r.MovingFraction,1.0) &&
        r.ObservedStrongSpeedChanges==199 &&
        Near(r.PeakObservedSpeedChangeCmps2,10000.0) &&
        Near(r.MeanObservedAbsoluteSpeedChangeCmps2,10000.0),
        "paired consecutive speed samples detect descriptive changes");
    coach.Suspend();
    coach.Observe(Sample(0.05,5000.0));
    r=coach.Review();
    Check(r.ObservedStrongSpeedChanges==199,
        "speed jump over pause cannot become a sampled acceleration claim");
    Check(Near(r.MovingSeconds+r.StationarySeconds+
        r.IntermediateSpeedSeconds,r.ObservedSeconds),
        "all accepted motion bands partition observed time");
}

void ResetRestoresFreshAuthority()
{
    DrivingCoachAccumulator coach;
    for(int i=0;i<200;++i)
        coach.Observe(Sample(0.05,2500.0,900.0,i%3==0));
    coach.Suspend();
    Check(coach.Review().AcceptedSamples==200,"pre-reset evidence exists");
    coach.Reset();
    const auto review=coach.Review();
    Check(!review.Valid && review.AcceptedSamples==0 &&
        review.RefusedSamples==0 && review.AcceptedBrakeEpisodes==0 &&
        review.AcceptedSlipEpisodes==0 && review.RetainedWindows==0,
        "reset erases all session-local aggregates");
    Check(review.NoGameplayAuthority && review.NoPersistentData &&
        review.NoOfficialTiming,"reset does not alter authority contract");
}
}

int main()
{
    SampleValidation();
    FreshAndBasicObservation();
    BrakeEpisodeLifecycle();
    SlipEpisodeLifecycle();
    SpeedBandBoundaries();
    WindowsAndSuspensions();
    RingRetentionAndMonotonicity();
    RefusalDoesNotInventDriving();
    EvidenceAndMessagePolicy();
    DeterministicReferenceOracle();
    ResetRestoresFreshAuthority();
    MotionCoverageAndVariation();
    std::cout<<"DrivingCoachInsights: portable observation and regression tests passed.\n";
    return EXIT_SUCCESS;
}
