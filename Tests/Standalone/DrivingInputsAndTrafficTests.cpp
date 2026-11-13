#include "Racecraft/SteeringRackFeedbackEstimator.h"
#include "Racecraft/PhysicalDriverInputCalibration.h"
#include "Racecraft/RivalLaneOccupancyForecast.h"
#include "Racecraft/RivalOvertakeCommitment.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;
using namespace latebrake::racecraft::commitment;
using namespace latebrake::racecraft::rackfeedback;
static void Require(bool ok,const char* message)
{
    if (!ok)
    {
        std::cerr<<"DrivingInputsAndTraffic FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static RaceIdentity Identity(int slot)
{
    RaceIdentity id;
    id.Session=530;
    id.SessionRevision=2;
    id.Actor=900+static_cast<std::uint64_t>(slot);
    id.Frame=6;
    id.Slot=slot;
    id.EngineObserved=true;
    id.BodySimulating=true;
    id.OriginalTrack=true;
    return id;
}
static SteeringRackObservation Steering()
{
    SteeringRackObservation s;
    s.Identity=Identity(1);
    s.ActualSteeringColumnAngleRadians=0.1;
    s.DriverSteeringInput=0.3;
    s.MeasuredVehicleSpeedCmS=3600.0;
    s.MeasuredRoadGrip=1.0;
    s.MeasuredColumnTorqueNm=0.3;
    s.PhysicsClockSeconds=1.0;
    s.DeltaSeconds=0.016;
    s.ActualChaosSteeringReady=true;
    s.OriginalMapVerified=true;
    s.DriverInputAuthoritative=true;
    for(auto& w:s.FrontWheels)
    {
        w.Wheel.PhysicalContact=true;
        w.Wheel.ContactVerified=true;
        w.Wheel.NormalLoadN=3200.0;
        w.Wheel.SurfaceGrip=1.0;
        w.MeasuredLateralForceN=800.0;
        w.MeasuredAligningMomentNm=3.0;
        w.MeasuredSteerAngleRadians=0.08;
        w.ActualWheelSteeringComponent=true;
        w.AlignmentSensorReady=true;
        w.OriginalContactPatchQualified=true;
    }
    return s;
}
static DriverInputObservation Input()
{
    DriverInputObservation s;
    s.Identity=Identity(1);
    s.Device=DriverInputDevice::Gamepad;
    s.DeviceToken=40;
    s.PhysicsClockSeconds=1.0;
    s.ControllerClockSeconds=1.0;
    s.DeltaSeconds=0.016;
    s.ActualControllerReady=true;
    s.OriginalPlayerPawn=true;
    s.WindowHasFocus=true;
    s.DeviceConnected=true;
    s.RaceInputAllowed=true;
    s.HumanDriverAuthenticated=true;
    for(auto& axis:s.Axes)
    {
        axis.RawValue=0.50;
        axis.SourceTimeSeconds=1.0;
        axis.Present=true;
        axis.AnalogSource=true;
        axis.DeviceAuthoritative=true;
        axis.ValidHardwareEvent=true;
    }
    s.Axes[static_cast<std::size_t>(
        DriverAxis::Brake)].RawValue=0.0;
    return s;
}
static RivalLaneActor Actor(int slot,double x,double speed)
{
    RivalLaneActor a;
    a.Identity=Identity(slot);
    a.Pose.Position={x,0.0};
    a.VelocityCmS={speed,0.0};
    a.SensorConfidence=1.0;
    a.PhysicsClockSeconds=10.0;
    a.RoadHalfWidthCm=800.0;
    a.OriginalChassisVerified=true;
    a.OriginalPavementVerified=true;
    a.LiveChaosVelocityQualified=true;
    return a;
}
static RivalLaneTrafficFrame Traffic(bool close)
{
    RivalLaneTrafficFrame f;
    f.Self=Actor(1,0.0,3200.0);
    int index=0;
    for(int slot=0;slot<6;++slot)
    {
        if(slot==1)continue;
        f.Others[static_cast<std::size_t>(index++)]=
            Actor(slot,-7000.0-slot*100.0,3200.0);
    }
    f.Others[1]=Actor(2,close?1100.0:2000.0,
                         close?200.0:2800.0);
    f.OtherCount=5;
    f.DeltaSeconds=0.016;
    f.CurrentThrottle=0.70;
    f.CurrentBrake=0.0;
    f.OriginalCircuitConfirmed=true;
    f.SixCarRosterAuthenticated=true;
    f.RaceGreen=true;
    f.OriginalTrackCollisionReady=true;
    return f;
}
static void SteeringPhysics()
{
    auto sample=Steering();
    const auto s=EstimateRackFeedback(sample);
    Require(s.Valid && s.ContactWheels==2 &&
            std::isfinite(s.RecommendedForceFeedbackTorqueNm) &&
            !s.ActualHidCommandIssued &&
            !s.ActualChaosSteeringModified,
            "source-only steering estimate is physically sensor grounded");
    PhysicalSteeringFeedbackSupervisor controller;
    Require(controller.Configure({}) &&
            controller.Step(sample).Valid,
            "steering controller accepts actual chassis frame");
    Require(!controller.Step(sample).Valid &&
            controller.Rejections()==1,
            "steering cannot reuse an old source frame");
    sample.FrontWheels[0].Wheel.PhysicalContact=false;
    Require(!EstimateRackFeedback(sample).Valid,
            "lost front contact invalidates rack feedback");
}
static void TestDriverControls()
{
    DriverInputSettings settings;
    settings.Axes[static_cast<std::size_t>(
        DriverAxis::Steering)].Bidirectional=true;
    const auto original=Input();
    const auto output=CalibratePhysicalDriverInput(
        original,settings);
    Require(output.Valid && output.QualifiedAxes==5 &&
            output.Throttle>0.0 &&
            !output.ActualUnrealControlsApplied,
            "verified human input produces a bounded source-only command");
    PhysicalDriverInputSupervisor supervisor;
    Require(supervisor.Configure(settings) &&
            supervisor.Step(original).Valid,
            "real gamepad source accepts one frame");
    Require(!supervisor.Step(original).Valid &&
            supervisor.Rejections()==1,
            "duplicated control frame rejected");
    auto bad=original;
    bad.Axes[0].SourceTimeSeconds=0.5;
    Require(!CalibratePhysicalDriverInput(
                bad,settings).Valid,
            "stale gamepad event cannot apply throttle");
    bad=original;
    ++bad.Identity.Frame;
    bad.WindowHasFocus=false;
    Require(!supervisor.Step(bad).Valid &&
            supervisor.FocusLossEvents()==1,
            "lost window focus clears controller authority");
}
static void TrafficSafety()
{
    const auto close=Traffic(true);
    const auto risk=ForecastPhysicalRivalLanes(close);
    Require(risk.Valid && risk.VerifiedOtherCars==5 &&
            risk.NoUnrealActuation &&
            risk.TotalPotentialContacts>0 &&
            risk.ProposedThrottle<=close.CurrentThrottle,
            "six measured cars produce conservative lane safety");
    auto damaged=close;
    damaged.Others[2].Identity.Actor=
        damaged.Others[1].Identity.Actor;
    Require(!ForecastPhysicalRivalLanes(damaged).Valid,
            "duplicated actual actor invalidates all lanes");
    damaged=close;
    --damaged.Others[3].Identity.Frame;
    Require(!ForecastPhysicalRivalLanes(damaged).Valid,
            "stale rival frame cannot clear an overtaking lane");
    RivalLaneOccupancySupervisor controller;
    Require(controller.Configure({}) &&
            controller.Step(close).Valid,
            "six-car traffic forecast accepted");
    Require(!controller.Step(close).Valid &&
            controller.Rejections()==1,
            "repeated six-car forecast cannot grant propulsion");
}
static void OvertakePolicy()
{
    auto route=Traffic(false);
    const auto occupancy=ForecastPhysicalRivalLanes(route);
    Require(occupancy.Valid &&
            occupancy.TotalPotentialContacts==0,
            "distant slower leader allows measured passing review");
    OvertakeObservation input;
    input.Traffic=route;
    input.Occupancy=occupancy;
    input.SelfGripG=1.2;
    input.PhysicalVisibilityDistanceCm=5000.0;
    input.DistanceUntilCornerCm=5000.0;
    input.ActualRoadHalfWidthCm=800.0;
    input.OfficialGreenFlag=true;
    input.ActualEnginePhysicsQualified=true;
    input.WheelGripMeasured=true;
    input.OriginalRaceTrackVerified=true;
    const auto pass=ProposeOvertake(input);
    Require(pass.Valid &&
            pass.TargetActor==route.Others[1].Identity.Actor &&
            pass.ProposedThrottle<=route.CurrentThrottle &&
            !pass.UnrealSteeringApplied &&
            !pass.OfficialRaceStateChanged,
            "valid source-only passing review references actual leader");
    input.LocalYellowActive=true;
    const auto caution=ProposeOvertake(input);
    Require(caution.Valid &&
            caution.Veto==OvertakeVeto::YellowFlag &&
            caution.State==OvertakeState::Following,
            "official yellow overrides passing request");
    input.LocalYellowActive=false;
    RivalOvertakeSupervisor supervisor;
    Require(supervisor.Configure({}) &&
            supervisor.Step(input).Valid,
            "stateful passing starts with sensor-qualified review");
    Require(!supervisor.Step(input).Valid &&
            supervisor.Rejections()==1,
            "replayed overtake frame cannot issue steering");
}
// Integrate ONLY physically qualified and exact-slot six-car observations.
// The assistant's old passing loop was falsely aborted when the leader
// crossed the acquisition-only 500-cm gap and could never reach clearing.
static OvertakeObservation PassingFixture()
{
    OvertakeObservation s;
    s.Traffic=Traffic(false);
    // 8 m/s measured closing rate leaves time for an actual lane move.
    s.Traffic.Others[1].Pose.Position.X=2600.0;
    s.Traffic.Others[1].VelocityCmS={2400.0,0.0};
    s.Traffic.DeltaSeconds=0.10;
    s.Occupancy=ForecastPhysicalRivalLanes(s.Traffic);
    s.SelfGripG=1.2;
    s.PhysicalVisibilityDistanceCm=6000.0;
    s.DistanceUntilCornerCm=6000.0;
    s.ActualRoadHalfWidthCm=800.0;
    s.OfficialGreenFlag=true;
    s.ActualEnginePhysicsQualified=true;
    s.WheelGripMeasured=true;
    s.OriginalRaceTrackVerified=true;
    Require(s.Occupancy.Valid &&
            s.Occupancy.TotalPotentialContacts==0 &&
            ProposeOvertake(s).Valid &&
            ProposeOvertake(s).TargetActor==
                s.Traffic.Others[1].Identity.Actor,
            "fully qualified six-car opening pass acquisition");
    return s;
}
static void AdvancePassingFrame(
    OvertakeObservation& s,
    double measuredTrackOffsetCm)
{
    auto& f=s.Traffic;
    // The driver advances at 32 m/s; the same original target advances
    // at 24 m/s. Each tenth-second therefore reduces the real gap by 80 cm.
    f.Self.Identity.Frame++;
    f.Self.Pose.Position.X+=320.0;
    f.Self.Pose.Position.Y=measuredTrackOffsetCm;
    f.Self.TrackLateralOffsetCm=measuredTrackOffsetCm;
    f.Self.VelocityCmS={3200.0,0.0};
    f.Self.PhysicsClockSeconds+=0.10;
    for(std::size_t i=0;i<f.Others.size();++i)
    {
        auto& rival=f.Others[i];
        ++rival.Identity.Frame;
        rival.Pose.Position.X+=(i==1?240.0:320.0);
        rival.PhysicsClockSeconds+=0.10;
    }
    f.DeltaSeconds=0.10;
    s.Occupancy=ForecastPhysicalRivalLanes(f);
    Require(s.Occupancy.Valid &&
            s.Occupancy.VerifiedOtherCars==RivalTrafficCount,
            "exact-frame physical six-car observation remains qualified");
}
static void PrepareObservedOvertake(
    RivalOvertakeSupervisor& controller,
    OvertakeObservation& sample)
{
    Require(controller.Configure({}),
            "real pass state machine has valid calibration");
    auto first=controller.Step(sample);
    Require(first.Valid &&
            first.State==OvertakeState::Evaluating &&
            first.TargetActor==sample.Traffic.Others[1].Identity.Actor &&
            controller.Commitments()==0,
            "fresh overtaking request acquires one authenticated rival");
    for(int step=1;step<=3;++step)
    {
        AdvancePassingFrame(sample,0.0);
        const auto decision=controller.Step(sample);
        Require(decision.Valid && decision.Veto==OvertakeVeto::None &&
                decision.ProposedThrottle<=
                    sample.Occupancy.ProposedThrottle &&
                !decision.UnrealSteeringApplied,
                "pre-commit approach must respect six-car brake authority");
    }
    Require(controller.CurrentState()==OvertakeState::Committed &&
            controller.Commitments()==1 &&
            controller.TrackedTargetActor()==
                sample.Traffic.Others[1].Identity.Actor &&
            controller.TrackedTargetSlot()==2 &&
            std::abs(controller.CommittedOffsetCm())==400.0,
            "stable physical target and absolute passing lane commit");
}
static void CompleteObservedOvertake()
{
    auto sample=PassingFixture();
    RivalOvertakeSupervisor controller;
    PrepareObservedOvertake(controller,sample);
    const auto exactActor=controller.TrackedTargetActor();
    const auto committedOffset=controller.CommittedOffsetCm();
    bool sawAcquisitionExpire=false;
    bool wasAlongside=false;
    bool sawRealClearance=false;
    for(int frame=4;frame<=44;++frame)
    {
        const double lateral=committedOffset*
            std::min(1.0,(frame-3)/4.0);
        AdvancePassingFrame(sample,lateral);
        const auto acquisition=ProposeOvertake(sample);
        const auto tracked=InspectTrackedOvertake(
            sample,exactActor,2,committedOffset);
        const auto decision=controller.Step(sample);
        Require(decision.Valid &&
                !decision.UnrealSteeringApplied &&
                !decision.OfficialRaceStateChanged &&
                decision.ProposedThrottle<=
                    sample.Traffic.CurrentThrottle &&
                decision.ProposedThrottle<=
                    sample.Occupancy.ProposedThrottle &&
                decision.ProposedBrake>=
                    sample.Occupancy.ProposedBrake,
                "tracked pass cannot override real traffic or race authority");
        if(acquisition.Veto==OvertakeVeto::NoPassingAdvantage)
            sawAcquisitionExpire=true;
        if(decision.State==OvertakeState::Committed ||
           decision.State==OvertakeState::Alongside)
        {
            Require(decision.TargetActor==exactActor &&
                    decision.TargetActorSlot==2 &&
                    decision.Lane==RivalLaneClass::Left &&
                    decision.Veto==OvertakeVeto::None &&
                    !decision.EarlyAbortRequested,
                    "approach gap closing cannot switch target or drop pass");
        }
        if(decision.State==OvertakeState::Alongside)
        {
            wasAlongside=true;
            Require(tracked.InCommittedLane &&
                    (tracked.SideBySide ||
                     (tracked.ForwardDistanceCm<0.0 &&
                      !tracked.PassedWithFullClearance)),
                    "alongside clearance remains pending until the rival is fully behind");
        }
        if(controller.ClearedPasses()>0)
        {
            Require(decision.State==OvertakeState::Clearing &&
                    tracked.PassedWithFullClearance &&
                    tracked.InCommittedLane,
                    "pass counts only after original rival is fully behind");
            sawRealClearance=true;
            break;
        }
    }
    Require(sawAcquisitionExpire &&
            wasAlongside &&
            sawRealClearance &&
            controller.ClearedPasses()==1 &&
            controller.AbortedPasses()==0,
            "complete physically tracked pass survives acquisition expiry");
    for(int i=0;i<23;++i)
    {
        AdvancePassingFrame(sample,committedOffset);
        const auto recovery=controller.Step(sample);
        Require(recovery.Valid &&
                controller.ClearedPasses()==1 &&
                controller.Commitments()==1 &&
                !recovery.OfficialRaceStateChanged,
                "clearing cannot double-count completed physical pass");
    }
    Require(controller.CurrentState()==OvertakeState::Following &&
            controller.TrackedTargetActor()==0,
            "post-pass recovery releases target without phantom lap credit");
}
static void RejectUnsafeOvertakeChanges()
{
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        AdvancePassingFrame(sample,0.0);
        sample.LocalYellowActive=true;
        const auto result=controller.Step(sample);
        Require(result.Valid &&
                result.Veto==OvertakeVeto::YellowFlag &&
                result.State==OvertakeState::Aborted &&
                result.EarlyAbortRequested &&
                result.ProposedThrottle==0.0 &&
                result.ProposedBrake>=0.20 &&
                controller.AbortedPasses()==1 &&
                controller.ClearedPasses()==0,
                "real local yellow vetoes already committed overtaking");
        sample.LocalYellowActive=false;
        for(int frame=0;frame<24;++frame)
        {
            AdvancePassingFrame(sample,0.0);
            const auto recovering=controller.Step(sample);
            Require(recovering.Valid &&
                    !recovering.UnrealSteeringApplied &&
                    controller.AbortedPasses()==1 &&
                    controller.ClearedPasses()==0,
                    "yellow abort recovers without phantom overtake credit");
        }
        Require(controller.CurrentState()==OvertakeState::Following &&
                controller.TrackedTargetActor()==0,
                "aborted passing recovers without a new target");
    }
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        AdvancePassingFrame(sample,0.0);
        sample.Traffic.Others[1].Identity.Actor+=10000;
        sample.Occupancy=ForecastPhysicalRivalLanes(sample.Traffic);
        const auto result=controller.Step(sample);
        Require(result.Valid &&
                result.Veto==OvertakeVeto::RosterChanged &&
                result.State==OvertakeState::Aborted &&
                controller.ClearedPasses()==0,
                "replaced real target must not be mistaken for old opponent");
    }
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        AdvancePassingFrame(sample,0.0);
        sample.Traffic.Others[2].Pose.Position.X=
            sample.Traffic.Self.Pose.Position.X+200.0;
        sample.Traffic.Others[2].Pose.Position.Y=
            controller.CommittedOffsetCm();
        sample.Traffic.Others[2].TrackLateralOffsetCm=
            controller.CommittedOffsetCm();
        sample.Occupancy=ForecastPhysicalRivalLanes(sample.Traffic);
        const auto evidence=InspectTrackedOvertake(
            sample,controller.TrackedTargetActor(),
            controller.TrackedTargetSlot(),
            controller.CommittedOffsetCm());
        const auto result=controller.Step(sample);
        Require(evidence.Valid &&
                evidence.ConflictingObservedActors>=1 &&
                result.Valid &&
                result.State==OvertakeState::Aborted &&
                result.Veto==OvertakeVeto::MultipleThreats &&
                result.ProposedThrottle==0.0,
                "real rival occupying committed strip triggers safe abort");
    }
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        AdvancePassingFrame(sample,0.0);
        sample.Traffic.Self.RoadHalfWidthCm=520.0;
        sample.Occupancy=ForecastPhysicalRivalLanes(sample.Traffic);
        const auto result=controller.Step(sample);
        Require(result.Valid &&
                result.State==OvertakeState::Aborted &&
                result.Veto==OvertakeVeto::UnsafeRoad,
                "narrowing original road rejects committed passing strip");
    }
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        const auto replay=controller.Step(sample);
        Require(!replay.Valid &&
                controller.Rejections()==1 &&
                controller.ClearedPasses()==0 &&
                controller.CurrentState()==OvertakeState::Aborted,
                "replayed engine frame invalidates rather than advances pass");
    }
    {
        auto sample=PassingFixture();
        RivalOvertakeSupervisor controller;
        PrepareObservedOvertake(controller,sample);
        AdvancePassingFrame(sample,0.0);
        auto stale=sample;
        std::swap(stale.Occupancy.Contacts[0],
                  stale.Occupancy.Contacts[1]);
        Require(!ValidOvertakeObservation(stale,{}) &&
                !controller.Step(stale).Valid &&
                controller.ClearedPasses()==0,
                "misordered per-slot contact evidence cannot certify pass");
    }
}
int main()
{
    SteeringPhysics();
    TestDriverControls();
    TrafficSafety();
    OvertakePolicy();
    CompleteObservedOvertake();
    RejectUnsafeOvertakeChanges();
    std::cout<<"DrivingInputsAndTraffic OK\n";
}
