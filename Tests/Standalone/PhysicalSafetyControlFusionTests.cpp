#include "Racecraft/PhysicalSafetyControlFusion.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Check(bool accepted,const char* why)
{
    if (!accepted)
    {
        std::cerr<<"PhysicalSafetyControlFusion FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static RaceIdentity ValidIdentity(int slot)
{
    RaceIdentity id;
    id.Session=123;
    id.SessionRevision=1;
    id.Actor=4000+static_cast<std::uint64_t>(slot);
    id.Frame=50;
    id.Slot=slot;
    id.EngineObserved=true;
    id.BodySimulating=true;
    id.OriginalTrack=true;
    return id;
}
static SafetyControlFrame PhysicalFleetFrame()
{
    SafetyControlFrame x;
    x.Corner.Identity=ValidIdentity(1);
    x.Corner.VehicleVelocityCmS={3000.0,0.0};
    x.Corner.HeadingRadians=0.0;
    x.Corner.MassKg=1250.0;
    x.Corner.DeltaSeconds=0.016;
    x.Corner.ProposedThrottle=0.7;
    x.Corner.ProposedBrake=0.0;
    for(auto& wheel:x.Corner.Wheels)
    {
        wheel.PhysicalContact=true;
        wheel.ContactVerified=true;
        wheel.NormalLoadN=3200.0;
        wheel.Surface=ContactSurface::Asphalt;
        wheel.SurfaceGrip=1.2;
    }
    AeroObservation air;
    air.VehicleVelocityCmS={3000.0,0.0};
    air.DeltaSeconds=0.016;
    air.OriginalVehicleObserved=true;
    air.AeroBodyPhysicallyPresent=true;
    x.Corner.MeasuredAero=
        CalculateAerodynamicForces(air,{},0.0);
    x.Corner.KnotCount=4;
    for(int i=0;i<4;++i)
    {
        auto& knot=x.Corner.Route[
            static_cast<std::size_t>(i)];
        knot.AlongRouteCm=i*500.0;
        knot.ActualGeometryQualified=true;
        knot.OriginalAsphaltSample=true;
        knot.RealFrictionQualified=true;
        knot.DistanceToBarrierCm=850.0;
    }
    x.Corner.Route[2].CurvaturePerCm=0.00035;
    x.Corner.EngineRaceGreen=true;
    x.Corner.OriginalCircuitConfirmed=true;
    x.Corner.ExistingAeroCalculatedFromPhysics=true;
    x.Following.Self.Identity=x.Corner.Identity;
    x.Following.Self.VelocityCmS={3000.0,0.0};
    x.Following.Self.Pose.HeadingRadians=0.0;
    x.Following.OpponentCount=5;
    int i=0;
    for(int slot=0;slot<6;++slot)
    {
        if(slot==1)continue;
        FollowingActor other;
        other.Identity=ValidIdentity(slot);
        other.Pose.Position={-6000.0-i*500.0,0.0};
        other.VelocityCmS={3000.0,0.0};
        x.Following.Opponents[
            static_cast<std::size_t>(i++)]=other;
    }
    x.Following.DeltaSeconds=0.016;
    x.Following.ProposedThrottle=0.7;
    x.Following.ProposedBrake=0.0;
    x.Following.EnginePhysicsQualified=true;
    x.Following.RaceRunning=true;
    x.DriverThrottle=0.7;
    x.DriverBrake=0.0;
    x.DriverSteering=0.35;
    x.Confidence=1.0;
    x.EngineAuthorityQualified=true;
    x.ActualRivalActorConfirmed=true;
    x.EngineRaceRunning=true;
    x.OriginalMapConfirmed=true;
    return x;
}
static void CheckComposition()
{
    auto sample=PhysicalFleetFrame();
    Check(ValidSafetyControlFrame(sample,{}),
          "full real six-actor source and four-wheel profile accepted");
    const auto output=ResolveSafetyControl(sample);
    Check(output.Valid && output.SourceOnly &&
          !output.ActualForceApplied &&
          output.CornerQualified &&
          output.TrafficQualified &&
          output.PhysicalRivalsChecked==5,
          "all physical sources verified before optional engine controls");
    Check(output.GrantedThrottle<=sample.DriverThrottle &&
          output.GrantedBrake>=sample.DriverBrake &&
          std::abs(output.GrantedSteering)<=
              std::abs(sample.DriverSteering),
          "fusion can only restrict motor, brake and steering");
    Check(output.CornerBrakeFloor>0.0 &&
          output.GrantedThrottle==0.0,
          "measured sharp bend wins before the car reaches its apex");
    auto following=sample;
    following.Following.Opponents[0].
        Pose.Position={1000.0,0.0};
    following.Following.Opponents[0].
        VelocityCmS={300.0,0.0};
    const auto conflict=ResolveSafetyControl(following);
    Check(conflict.Valid &&
          conflict.ObservedClosingSpeedCmS>1000.0 &&
          conflict.TrafficBrakeFloor>=
              output.TrafficBrakeFloor &&
          conflict.GrantedBrake>=sample.DriverBrake,
          "dangerous slower car cannot remove corner braking");
    auto stable=sample;
    stable.StabilitySensorAvailable=true;
    stable.Stability.Valid=true;
    stable.Stability.ThrottleCeiling=0.35;
    stable.Stability.CombinedBrakeFloor=0.40;
    stable.Stability.SteeringCeilingFraction=0.30;
    stable.DriverSteering=0.9;
    const auto combined=ResolveSafetyControl(stable);
    Check(combined.Valid &&
          combined.StabilityQualified &&
          combined.StabilityThrottleCeiling==0.35 &&
          combined.StabilityBrakeFloor==0.4 &&
          std::abs(combined.GrantedSteering)<=0.3 &&
          combined.GrantedBrake>=0.4,
          "measured stability can add brake and limit steering");
    stable.Stability.SentActualChaosForce=true;
    Check(!ResolveSafetyControl(stable).Valid,
          "unverified Chaos force application is never accepted");
    auto originalBrake=sample;
    originalBrake.DriverBrake=0.65;
    originalBrake.Corner.ProposedBrake=0.65;
    originalBrake.Following.ProposedBrake=0.65;
    Check(ResolveSafetyControl(originalBrake).GrantedBrake>=0.65,
          "fusion cannot override existing user brake demand");
}
static void RejectUnqualified()
{
    auto sample=PhysicalFleetFrame();
    ++sample.Following.Self.Identity.Frame;
    Check(!ResolveSafetyControl(sample).Valid,
          "cross-frame sensor mix rejected");
    sample=PhysicalFleetFrame();
    ++sample.Following.Self.Identity.SessionRevision;
    Check(!ResolveSafetyControl(sample).Valid,
          "stale recovery revision rejected");
    sample=PhysicalFleetFrame();
    sample.Following.Self.VelocityCmS={400.0,0.0};
    Check(!ResolveSafetyControl(sample).Valid,
          "different wheel and traffic chassis velocities rejected");
    sample=PhysicalFleetFrame();
    ++sample.Following.Opponents[1].Identity.SessionRevision;
    Check(!ResolveSafetyControl(sample).Valid,
          "opponent from previous recovery revision cannot qualify");
    sample=PhysicalFleetFrame();
    --sample.Following.Opponents[2].Identity.Frame;
    Check(!ResolveSafetyControl(sample).Valid,
          "one stale rival frame invalidates all traffic grants");
    sample=PhysicalFleetFrame();
    sample.Following.OpponentCount=4;
    Check(!ResolveSafetyControl(sample).Valid,
          "missing fifth rival never becomes clear road");
    sample=PhysicalFleetFrame();
    sample.Following.Opponents[4].Identity.Actor=
        sample.Following.Opponents[3].Identity.Actor;
    Check(!ResolveSafetyControl(sample).Valid,
          "duplicate physical opponent fails closed");
    sample=PhysicalFleetFrame();
    sample.Following.Self.GripG=2.3;
    Check(!ResolveSafetyControl(sample).Valid,
          "optimistic traffic grip conflicts with real four-wheel evidence");
    sample=PhysicalFleetFrame();
    for(auto& wheel:sample.Corner.Wheels)
        wheel.SurfaceGrip=0.35;
    Check(!ResolveSafetyControl(sample).Valid,
          "wet four-wheel contact cannot be overridden by dry traffic grip");
    sample=PhysicalFleetFrame();
    sample.Following.Self.GripG=1.38;
    Check(ResolveSafetyControl(sample).Valid,
          "small physical tire and chassis grip differences are tolerated");
    sample=PhysicalFleetFrame();
    sample.EngineAuthorityQualified=false;
    Check(!ResolveSafetyControl(sample).Valid,
          "source-only controller needs actual engine authority");
    sample=PhysicalFleetFrame();
    sample.DriverThrottle=
        std::numeric_limits<double>::infinity();
    Check(!ResolveSafetyControl(sample).Valid,
          "nonfinite controls fail closed");
    sample=PhysicalFleetFrame();
    sample.Confidence=0.0;
    Check(!ResolveSafetyControl(sample).Valid,
          "unqualified sensor confidence fails closed");
    SafetyControlSettings settings;
    settings.MaximumPositionDisagreementCm=-1.0;
    Check(!ValidSafetyControlSettings(settings),
          "negative spatial coherence tolerance rejected");
}
static void MonotonicSupervisor()
{
    auto sample=PhysicalFleetFrame();
    SafetyControlSupervisor controller;
    Check(controller.Configure({}),
          "physical source supervisor configured");
    const auto first=controller.Step(sample);
    Check(first.Valid && first.GrantedThrottle<=
          sample.DriverThrottle &&
          first.GrantedBrake>=sample.DriverBrake,
          "first accepted frame never adds power");
    Check(!controller.Step(sample).Valid &&
          controller.Rejections()==1,
          "same frame cannot be replayed into an engine command");
    // A smaller ordinal in the identical actor epoch is also a replay,
    // not a valid recovery requiring only a short safety hold.
    auto rolledBack=sample;
    --rolledBack.Corner.Identity.Frame;
    --rolledBack.Following.Self.Identity.Frame;
    for(auto& opponent:rolledBack.Following.Opponents)
        --opponent.Identity.Frame;
    Check(!controller.Step(rolledBack).Valid &&
          controller.Rejections()==2 &&
          controller.Frames()==1,
          "older coherent input frame cannot pass as fresh evidence");
    ++sample.Corner.Identity.Frame;
    ++sample.Following.Self.Identity.Frame;
    for(auto& rival:sample.Following.Opponents)
        ++rival.Identity.Frame;
    const auto resume=controller.Step(sample);
    Check(resume.Valid && controller.Frames()==2,
          "fresh physics sample restores bounded decision");
    ++sample.Corner.Identity.Frame;
    ++sample.Following.Self.Identity.Frame;
    for(auto& rival:sample.Following.Opponents)
        ++rival.Identity.Frame;
    sample.Following.Opponents[0].Identity.Actor+=100;
    const auto swapped=controller.Step(sample);
    Check(swapped.Valid && swapped.RosterTransition &&
          swapped.GrantedThrottle==0.0 &&
          swapped.GrantedBrake==1.0 &&
          controller.RosterTransitions()==1,
          "changed physical actor holds a conservative brake");
}
int main()
{
    CheckComposition();
    RejectUnqualified();
    MonotonicSupervisor();
    std::cout<<"PhysicalSafetyControlFusion OK\n";
}
