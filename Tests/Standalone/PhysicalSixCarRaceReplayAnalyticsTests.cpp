#include "Racecraft/PhysicalSixCarRaceReplayAnalytics.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Expect(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"PhysicalReplayAnalytics FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static RivalLaneActor MakePhysicalCar(int slot,double x,double y,
                                     double speed)
{
    RivalLaneActor car;
    car.Identity.Session=771;
    car.Identity.SessionRevision=3;
    car.Identity.Actor=15100+static_cast<std::uint64_t>(slot);
    car.Identity.Frame=10;
    car.Identity.Slot=slot;
    car.Identity.EngineObserved=true;
    car.Identity.BodySimulating=true;
    car.Identity.OriginalTrack=true;
    car.Pose.Position={x,y};
    car.Pose.HeadingRadians=0.0;
    car.VelocityCmS={speed,0.0};
    car.TrackDistanceCm=10000.0+x;
    car.TrackLateralOffsetCm=y;
    car.PhysicsClockSeconds=3.0;
    car.SensorConfidence=1.0;
    car.RoadHalfWidthCm=800.0;
    car.OriginalChassisVerified=true;
    car.OriginalPavementVerified=true;
    car.LiveChaosVelocityQualified=true;
    return car;
}
static RivalLaneTrafficFrame RealSixCarFrame(double targetX=2500.0,
                                            double targetY=0.0)
{
    RivalLaneTrafficFrame frame;
    frame.Self=MakePhysicalCar(1,0.0,0.0,3500.0);
    int out=0;
    for(int slot=0;slot<PhysicalFleetCapacity;++slot)
    {
        if(slot==1)continue;
        frame.Others[static_cast<std::size_t>(out++)]=
            MakePhysicalCar(slot,-12000.0-1900.0*slot,0.0,3200.0);
    }
    frame.Others[1]=MakePhysicalCar(2,targetX,targetY,2200.0);
    frame.OtherCount=RivalTrafficCount;
    frame.DeltaSeconds=0.1;
    frame.CurrentSteering=0.0;
    frame.CurrentThrottle=0.65;
    frame.CurrentBrake=0.0;
    frame.OriginalCircuitConfirmed=true;
    frame.SixCarRosterAuthenticated=true;
    frame.RaceGreen=true;
    frame.OriginalTrackCollisionReady=true;
    return frame;
}
static void PhysicallyAdvance(RivalLaneTrafficFrame& f)
{
    for(auto* actor:{&f.Self,&f.Others[0],&f.Others[1],
                     &f.Others[2],&f.Others[3],&f.Others[4]})
    {
        actor->Pose.Position=actor->Pose.Position+
            actor->VelocityCmS*f.DeltaSeconds;
        ++actor->Identity.Frame;
        actor->PhysicsClockSeconds+=f.DeltaSeconds;
        actor->TrackDistanceCm+=
            Length(actor->VelocityCmS)*f.DeltaSeconds;
    }
}
static void SourceQualityGuards()
{
    PhysicalReplayAnalysisSettings settings;
    Expect(ValidPhysicalReplayAnalysisSettings(settings),
           "bounded, explicit analytics settings configure");
    auto bad=settings;
    bad.MaximumObservedSpeedCmS=
        std::numeric_limits<double>::quiet_NaN();
    Expect(!ValidPhysicalReplayAnalysisSettings(bad),
           "nonfinite physics speed limit rejected");
    bad=settings;
    bad.MinimumAnalysisFrames=1;
    Expect(!ValidPhysicalReplayAnalysisSettings(bad),
           "one measurement can never establish a temporal trend");
    bad=settings;
    bad.MaximumVelocityDisplacementErrorCm=-1.0;
    Expect(!ValidPhysicalReplayAnalysisSettings(bad),
           "negative source-uncertainty allowance rejected");
    auto f=RealSixCarFrame();
    const auto source=ReviewPhysicalTraceFrame(f);
    Expect(source.Valid && ValidPhysicalReplayAnalysisFrame(source,settings),
           "six actual frame-bound car identities authenticate source");
    auto forged=source;
    forged.Pairs[0].MeasuredOverlap=true;
    Expect(!ValidPhysicalReplayAnalysisFrame(forged,settings),
           "forged overlap flag cannot create an official incident");
    forged=source;
    forged.Pairs[0].CenterDistanceCm+=1000.0;
    Expect(!ValidPhysicalReplayAnalysisFrame(forged,settings),
           "recorded vehicle separation must match original positions");
    forged=source;
    forged.Actors[5].Identity.Actor=source.Actors[4].Identity.Actor;
    Expect(!ValidPhysicalReplayAnalysisFrame(forged,settings),
           "duplicated physical participant cannot enter telemetry summary");
    forged=source;
    forged.ActualEngineActuationOccurred=true;
    Expect(!ValidPhysicalReplayAnalysisFrame(forged,settings),
           "review never accepts material altered by an engine command");
    forged=source;
    forged.Actors[2].VelocityCmS={19000.0,0.0};
    Expect(!ValidPhysicalReplayAnalysisFrame(forged,settings),
           "physically implausible velocity sample is rejected");
}
static void TrafficAnalysisAndPackReview()
{
    auto f=RealSixCarFrame(1100.0,0.0);
    PhysicalSixCarRaceTrace trace;
    Expect(trace.Configure({}),"physical trace configured");
    Expect(trace.Append(f)==PhysicalTraceFault::None,
           "first physical frame accepted");
    auto first=AnalyzePhysicalReplayWindow(trace);
    Expect(!first.Valid &&
           first.Fault==PhysicalReplayAnalysisFault::InsufficientHistory,
           "one frame cannot invent race analytics or trajectory");
    for(int i=0;i<20;++i)
    {
        PhysicallyAdvance(f);
        Expect(trace.Append(f)==PhysicalTraceFault::None,
               "consecutive original chassis frames accepted for analytics");
    }
    const auto report=AnalyzePhysicalReplayWindow(trace);
    Expect(report.Valid &&
           report.Fault==PhysicalReplayAnalysisFault::None &&
           report.SourceFrames==21 &&
           report.ActorRoster[1]==15101 &&
           report.ActorRoster[2]==15102 &&
           report.ObservedDurationSeconds>1.9 &&
           report.Cars[1].ObservedFrames==21 &&
           report.Cars[2].ObservedFrames==21 &&
           report.Cars[1].MeanSpeedCmS>3400.0 &&
           report.Cars[2].MeanSpeedCmS>2100.0 &&
           report.Cars[1].OriginalVehiclePhysicsVerified &&
           !report.OfficialRaceClassificationIssued &&
           !report.RealUnrealVehicleActuationIssued,
           "source-qualified time-weighted actual car speeds are read-only");
    Expect(report.Cars[1].TotalMeasuredTravelCm>6000.0 &&
           report.Cars[1].TotalMeasuredTravelCm<8000.0 &&
           report.LastFramePack.Valid &&
           report.LastFramePack.SixPhysicalCarsVerified &&
           report.LastFramePack.ConnectedGroups>=1 &&
           report.LastFramePack.LargestConnectedGroup<=6,
           "trajectory distance and physical pack connectivity stay bounded");
    for(const auto& stats:report.Pairs)
        Expect(stats.Valid && stats.ObservedFrames==21 &&
               stats.PairIdentitiesStable,
               "all 15 distinct opponent pairs retain their chassis identities");
    const auto coach=report.DriverCoach;
    Expect(coach.Valid && coach.SourceSlot==1 &&
           coach.LimitingRivalSlot>=0 &&
           coach.NoSteeringOrBrakeCommandIssued &&
           coach.NoOfficialPenaltyOrLapCredited,
           "advisory race engineer cannot control brakes or award penalties");
    auto forged=trace;
    (void)forged;
    auto spare=ReviewPhysicalTraceFrame(f);
    spare.Actors[2].SensorConfidence=0.1;
    Expect(!ValidPhysicalReplayAnalysisFrame(spare,{}),
           "sudden sensor confidence loss removes physics qualification");
}
static void PhysicallyObservedPass()
{
    auto f=RealSixCarFrame(1300.0,360.0);
    PhysicalSixCarRaceTrace trace;
    Expect(trace.Configure({}) &&
           trace.Append(f)==PhysicalTraceFault::None,
           "separate passing lane and original target are physically observed");
    // The true 13 m/s relative closing speed takes the same car from
    // ahead to behind in a dozen consecutive 100ms chassis frames.
    for(int i=0;i<17;++i)
    {
        PhysicallyAdvance(f);
        Expect(trace.Append(f)==PhysicalTraceFault::None,
               "actual pass has continuous measured engine frames");
    }
    const auto report=AnalyzePhysicalReplayWindow(trace);
    Expect(report.Valid && report.SourceFrames==18 &&
           report.Pairs[5].Valid &&
           report.Pairs[5].FirstSlot==1 &&
           report.Pairs[5].SecondSlot==2 &&
           !report.OfficialRaceClassificationIssued,
           "source-only relative passing never becomes official classification");
    // Source slot 1 and slot 2 are the sixth pair in sorted i<j order.
    Expect(report.Pairs[5].PhysicallyCompletedPasses==1 &&
           report.TotalPhysicallyCompletedPasses==1 &&
           report.Cars[1].PassesObserved==1 &&
           report.Cars[2].PassedByRivals==1,
           "one actual same-identity lateral pass counts only after full clearance");
}
static void LiveAnalyticsSupervisor()
{
    auto f=RealSixCarFrame();
    PhysicalReplayAnalyticsSupervisor controller;
    Expect(controller.Configure({}),
           "bounded read-only replay analytics supervisor configured");
    Expect(controller.Observe(f)==
               PhysicalReplayAnalysisFault::InsufficientHistory &&
           controller.AcceptedFrames()==1 &&
           !controller.Review().Valid,
           "one source frame does not impersonate a completed race");
    PhysicallyAdvance(f);
    Expect(controller.Observe(f)==PhysicalReplayAnalysisFault::None &&
           controller.Review().Valid &&
           controller.AcceptedFrames()==2 &&
           controller.Review().SourceFrames==2 &&
           controller.Review().OriginalRaceStateUntouched,
           "second real physical frame enables read-only time analysis");
    Expect(controller.Observe(f)==
               PhysicalReplayAnalysisFault::InvalidPhysicalFrame &&
           controller.RejectedFrames()==1,
           "replayed chassis frame cannot inflate traffic summary");
    f.Others[0].Identity.Actor+=999;
    PhysicallyAdvance(f);
    Expect(controller.Observe(f)==
               PhysicalReplayAnalysisFault::InvalidPhysicalFrame &&
           controller.RejectedFrames()==2,
           "changed six-car roster invalidates continuous telemetry");
    Expect(controller.Review().Valid &&
           !controller.Review().RealUnrealVehicleActuationIssued,
           "last valid report is source-only and never issues engine controls");
    controller.Reset();
    Expect(controller.AcceptedFrames()==0 &&
           controller.RejectedFrames()==0 &&
           !controller.HasLiveSource(),
           "restarts clear stale source and race analysis state");
}
int main()
{
    SourceQualityGuards();
    TrafficAnalysisAndPackReview();
    PhysicallyObservedPass();
    LiveAnalyticsSupervisor();
    std::cout<<"PhysicalSixCarRaceReplayAnalytics OK\n";
}