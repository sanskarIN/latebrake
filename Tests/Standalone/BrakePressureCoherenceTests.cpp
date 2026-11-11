#include "Racecraft/BrakePedalPressureCoherence.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;
static void Check(bool ok,const char* message)
{
    if (!ok)
    {
        std::cerr<<"BrakePressureCoherence FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static BrakePressureObservation ObservedBrake()
{
    BrakePressureObservation o;
    o.Identity.Session=900;
    o.Identity.SessionRevision=1;
    o.Identity.Actor=500;
    o.Identity.Frame=10;
    o.Identity.Slot=1;
    o.Identity.EngineObserved=true;
    o.Identity.BodySimulating=true;
    o.Identity.OriginalTrack=true;
    o.DriverPedalFraction=0.5;
    o.MasterCylinderPressureBar=60.0;
    o.PedalTravelCm=4.2;
    o.MeasuredPedalForceN=480.0;
    o.MeasuredBoosterGain=1.0;
    o.LongitudinalSpeedCmS=3200.0;
    o.RoadGripG=1.1;
    o.DeltaSeconds=0.016;
    o.PhysicsClockSeconds=2.0;
    o.RaceRunning=true;
    o.EngineBrakePhysicsQualified=true;
    o.OriginalMapVerified=true;
    o.AbsActualSensorReady=true;
    for(std::size_t i=0;i<4;++i)
    {
        auto& c=o.Circuits[i];
        c.Wheel.ContactVerified=true;
        c.Wheel.PhysicalContact=true;
        c.Wheel.NormalLoadN=3200.0;
        c.MeasuredLinePressureBar=i<2?22.0:13.0;
        c.MeasuredDiscTemperatureC=150.0;
        c.MeasuredWheelBrakeTorqueNm=100.0;
        c.SourceClockSeconds=2.0;
        c.OriginalCaliperPresent=true;
        c.PressureSensorQualified=true;
        c.MeasuredTorqueQualified=true;
    }
    return o;
}
static void PhysicalBraking()
{
    const auto s=ObservedBrake();
    const auto d=EvaluateBrakePressureCoherence(s);
    Check(d.Valid && d.QualifiedCircuits==4 &&
          !d.ActualChaosCommandIssued,
          "four observed brake calipers produce source-only diagnostics");
    Check(d.ExpectedTotalTorqueNm>0.0 &&
          d.MeasuredTotalTorqueNm>0.0 &&
          d.ConservativeStopDistanceCm>0.0,
          "measured pedal and line pressure produce finite braking estimate");
    Check(d.LeakingCircuits==0 &&
          d.OverheatedCircuits==0 &&
          d.Fault==BrakePressureFault::None,
          "normal thermal hydraulic sample does not invent failure");
    const auto front=d.Circuits[0];
    const auto rear=d.Circuits[2];
    Check(front.ExpectedWheelTorqueNm>
          rear.ExpectedWheelTorqueNm,
          "front-biased circuit distributes more braking torque");
}
static void FailureDetections()
{
    auto sample=ObservedBrake();
    sample.Circuits[1].MeasuredLinePressureBar=0.0;
    const auto leak=EvaluateBrakePressureCoherence(sample);
    Check(leak.Valid && leak.LeakingCircuits==1 &&
          leak.Fault==BrakePressureFault::HydraulicLeak &&
          leak.MayRequirePitInspection,
          "measured pressure residual exposes failed brake circuit");
    sample=ObservedBrake();
    sample.Circuits[0].Wheel.SlipRatio=-0.50;
    const auto locked=EvaluateBrakePressureCoherence(sample);
    Check(locked.Valid && locked.AbsActiveCircuits==1 &&
          locked.Circuits[0].RequiredAbsReleaseFraction>0.0,
          "actual locked wheel is flagged for ABS review");
    sample=ObservedBrake();
    sample.Circuits[0].MeasuredDiscTemperatureC=850.0;
    sample.Circuits[1].MeasuredDiscTemperatureC=850.0;
    const auto hot=EvaluateBrakePressureCoherence(sample);
    Check(hot.Valid && hot.OverheatedCircuits==2 &&
          hot.MayRequirePitInspection &&
          hot.MaximumObservedFade>0.5,
          "measured high brake temperature reduces pad effectiveness");
    sample=ObservedBrake();
    sample.Circuits[1].OriginalCaliperPresent=false;
    Check(!EvaluateBrakePressureCoherence(sample).Valid,
          "missing physical brake caliper is not a valid sample");
    sample=ObservedBrake();
    sample.Circuits[1].SourceClockSeconds=1.0;
    Check(!EvaluateBrakePressureCoherence(sample).Valid,
          "stale hydraulic sensor cannot qualify current physics");
    sample=ObservedBrake();
    sample.RoadGripG=
        std::numeric_limits<double>::quiet_NaN();
    Check(!EvaluateBrakePressureCoherence(sample).Valid,
          "unknown road grip cannot produce braking benefit");
}
static void Controller()
{
    BrakePressureConsistencySupervisor supervisor;
    Check(supervisor.Configure({}),
          "finite brake parameters configure");
    auto s=ObservedBrake();
    Check(supervisor.Step(s).Valid,
          "first actual hydraulic frame accepted");
    Check(!supervisor.Step(s).Valid &&
          supervisor.Rejections()==1,
          "replayed physical brake sample rejected");
    s.Identity.Frame=11;
    s.PhysicsClockSeconds+=0.016;
    for(auto& c:s.Circuits)
        c.SourceClockSeconds=s.PhysicsClockSeconds;
    Check(supervisor.Step(s).Valid &&
          supervisor.Frames()==2 &&
          supervisor.MeasuredBrakeWorkJoules()>0.0,
          "new physical frame records brake work in joules");
}
int main()
{
    PhysicalBraking();
    FailureDetections();
    Controller();
    std::cout<<"BrakePressureCoherence OK\n";
}
