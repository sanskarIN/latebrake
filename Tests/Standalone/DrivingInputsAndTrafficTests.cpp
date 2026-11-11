#include "Racecraft/SteeringRackFeedbackEstimator.h"
#include "Racecraft/PhysicalDriverInputCalibration.h"
#include "Racecraft/RivalLaneOccupancyForecast.h"
#include "Racecraft/RivalOvertakeCommitment.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;
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
static void DriverControls()
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
int main()
{
    SteeringPhysics();
    DriverControls();
    TrafficSafety();
    OvertakePolicy();
    std::cout<<"DrivingInputsAndTraffic OK\n";
}
