#include "Racing/PhysicalSixCarReplayTimeline.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes,const char* what)
{
    if(!yes)
    {
        std::cerr<<"PhysicalSixCarReplayTimeline FAIL: "<<what<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x,double y,double tolerance=1e-6)
{
    return std::abs(x-y)<=tolerance;
}
latebrake::PhysicalReplayFrame Frame(
    std::uint64_t id,double time)
{
    using namespace latebrake;
    PhysicalReplayFrame frame;
    frame.Session=42;
    frame.SessionRevision=2;
    frame.SourceFrame=id;
    frame.ActiveClockSeconds=time;
    frame.SourceDeltaSeconds=0.05;
    frame.AllSixEngineActorsObserved=true;
    for(std::size_t i=0;i<PhysicalReplayCarCount;++i)
    {
        auto& c=frame.Cars[i];
        c.Actor=100u+i;
        c.Session=frame.Session;
        c.SessionRevision=frame.SessionRevision;
        c.SourceFrame=frame.SourceFrame;
        c.WorldXcm=100.0+i*1500.0+100.0*time;
        c.WorldYcm=500.0+i*450.0;
        c.WorldZcm=80.0;
        c.VelocityXcmps=100.0;
        c.VelocityYcmps=0.0;
        c.VelocityZcmps=0.0;
        c.HeadingRadians=0.12;
        c.SteeringNormalized=0.20;
        c.ThrottleNormalized=0.40;
        c.BrakeNormalized=0.0;
        c.RoadContactConfidence=1.0;
        c.ActualChaosBodyObserved=true;
        c.OriginalRoadPhysicalContact=true;
        c.EngineClockRunning=true;
    }
    return frame;
}
}

int main()
{
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    PhysicalSixCarReplayTimeline replay;
    Require(!replay.HasReplay() &&
            replay.FrameCount()==0u &&
            replay.EarliestClockSeconds()==0.0,
            "new session begins with no invented observation");
    Require(!replay.Evaluate(0.0).Valid,
            "empty replay cannot render or fabricate official race state");
    auto initial=Frame(1,0.05);
    Require(ValidatePhysicalReplayFrame(initial)==
                PhysicalReplayReject::None,
            "valid six-engine-source observation accepted");
    auto first=replay.Record(initial);
    Require(first.Accepted &&
            first.Session==42 &&
            first.SessionRevision==2 &&
            first.AcceptedSourceFrame==1u &&
            replay.FrameCount()==1u &&
            !replay.HasReplay() &&
            !first.GameplayAuthorityChanged,
            "first replay record creates source-only motion baseline");
    auto second=Frame(2,0.10);
    auto good=replay.Record(second);
    Require(good.Accepted &&
            replay.FrameCount()==2u &&
            replay.HasReplay() &&
            Near(replay.LatestClockSeconds(),0.10),
            "second real frame opens bounded replay");
    auto between=replay.Evaluate(0.075);
    Require(between.Valid &&
            between.LowerSourceFrame==1 &&
            between.UpperSourceFrame==2 &&
            Near(between.InterpolationFraction,0.5) &&
            between.NoCollisionAuthority &&
            between.NoOfficialTimingAuthority &&
            between.SourceOnly &&
            between.Session==42 &&
            between.SessionRevision==2,
            "matching-source subframe interpolation has no scoring authority");
    for(std::size_t i=0u;i<PhysicalReplayCarCount;++i)
    {
        const auto& car=between.Cars[i];
        Require(car.Actor==100u+i &&
                car.ProvenanceMatched &&
                Near(car.Xcm,100.0+i*1500.0+7.5) &&
                Near(car.Ycm,500.0+i*450.0) &&
                Near(car.ObservedSpeedCmps,100.0) &&
                Near(car.SteeringNormalized,0.20),
                "all six authentic actor identities interpolate independently");
    }
    Require(replay.Evaluate(0.05).Valid &&
            replay.Evaluate(0.10).Valid &&
            !replay.Evaluate(0.04).Valid &&
            !replay.Evaluate(0.11).Valid &&
            !replay.Evaluate(nan).Valid,
            "no temporal extrapolation or nonfinite query");

    PhysicalReplayFrame fetched;
    Require(replay.TryReadFrame(0,fetched) &&
            fetched.SourceFrame==1u &&
            replay.TryReadFrame(1,fetched) &&
            fetched.SourceFrame==2u &&
            !replay.TryReadFrame(2,fetched) &&
            fetched.SourceFrame==0u,
            "chronological read bounds protect ring memory");

    auto invalid=Frame(3,0.15);
    invalid.Cars[3].Actor=invalid.Cars[2].Actor;
    Require(ValidatePhysicalReplayFrame(invalid)==
                PhysicalReplayReject::WrongRoster &&
            !replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::WrongRoster &&
            replay.FrameCount()==2u,
            "same-actor duplicate cannot create six independent cars");
    invalid=Frame(3,0.15);
    invalid.Cars[2].OriginalRoadPhysicalContact=false;
    Require(ValidatePhysicalReplayFrame(invalid)==
                PhysicalReplayReject::MissingRoadEvidence &&
            !replay.Record(invalid).Accepted,
            "unverified physical road contact blocks replay evidence");
    invalid=Frame(3,0.15);
    invalid.Cars[4].WorldYcm=nan;
    Require(ValidatePhysicalReplayFrame(invalid)==
                PhysicalReplayReject::InvalidChassis &&
            !replay.Record(invalid).Accepted,
            "nonfinite measured Chaos snapshot rejected");
    invalid=Frame(3,0.15);
    invalid.Cars[1].Actor=7777u;
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::WrongRoster,
            "replaced actor cannot inherit prior replay authorization");
    invalid=Frame(3,0.15);
    invalid.Cars[1].WorldXcm+=2000.0;
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::ImpossibleMotion &&
            replay.FrameCount()==2u,
            "in-lane teleport between physical samples is rejected");
    invalid=Frame(3,0.15);
    invalid.SessionRevision=3u;
    for(auto& car:invalid.Cars) car.SessionRevision=3u;
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::BadRaceEpoch,
            "restart epoch cannot keep an earlier physical motion witness");
    invalid=Frame(3,0.09);
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::TimeRollback,
            "active race clock rollback rejected");
    invalid=Frame(20,1.00);
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::FrameDiscontinuity,
            "large unverified frame gap rejected");
    invalid=Frame(3,0.15);
    invalid.AllSixEngineActorsObserved=false;
    Require(!replay.Record(invalid).Accepted &&
            replay.LastRejection()==PhysicalReplayReject::BadSource,
            "no invented five-rival roster on partial engine evidence");
    Require(replay.Record(Frame(3,0.15)).Accepted &&
            replay.FrameCount()==3u &&
            replay.LastRejection()==PhysicalReplayReject::None,
            "valid observation resumes without unauthorized state mutation");

    // A real ring can replay bounded recent history, not allocate one
    // entry forever or keep prior sessions after a full capacity wrap.
    for(std::uint64_t frame=4u;frame<=600u;++frame)
    {
        const double time=static_cast<double>(frame)*0.05;
        const auto receipt=replay.Record(Frame(frame,time));
        Require(receipt.Accepted,
                "long verified six-car cadence stays bounded");
        Require(replay.FrameCount()<=PhysicalReplayFrameCapacity,
                "no heap growth or unbounded frame history");
    }
    Require(replay.FrameCount()==PhysicalReplayFrameCapacity,
            "full ring retains exactly configured historical capacity");
    PhysicalReplayFrame oldest;
    PhysicalReplayFrame newest;
    Require(replay.TryReadFrame(0,oldest) &&
            replay.TryReadFrame(replay.FrameCount()-1u,newest),
            "full ring is still chronologically readable");
    Require(oldest.SourceFrame==600u-
                PhysicalReplayFrameCapacity+1u &&
            newest.SourceFrame==600u,
            "oldest frame correctly evicted after wrap");
    Require(Near(replay.EarliestClockSeconds(),
                 oldest.ActiveClockSeconds) &&
            Near(replay.LatestClockSeconds(),
                 newest.ActiveClockSeconds),
            "replay clock span strictly matches surviving physical frames");
    Require(!replay.Evaluate(oldest.ActiveClockSeconds-0.01).Valid &&
            replay.Evaluate(oldest.ActiveClockSeconds).Valid &&
            replay.Evaluate(29.975).Valid &&
            replay.Evaluate(30.0).Valid &&
            !replay.Evaluate(30.01).Valid,
            "binary lookup honors ring wrap and refuses extrapolation");
    replay.Reset();
    Require(replay.FrameCount()==0u &&
            !replay.HasReplay() &&
            !replay.Evaluate(30.0).Valid,
            "session reset erases all previous actor telemetry");
    auto restarted=Frame(1u,0.05);
    restarted.Session=90u;
    restarted.SessionRevision=8u;
    for(auto& car:restarted.Cars)
    {
        car.Session=90u;
        car.SessionRevision=8u;
    }
    Require(replay.Record(restarted).Accepted &&
            replay.FrameCount()==1u,
            "explicitly new session obtains independent replay roster");
    std::cout<<"PhysicalSixCarReplayTimeline: source-only tests passed\n";
    return EXIT_SUCCESS;
}
