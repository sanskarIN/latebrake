#include "Racecraft/AerodynamicWakeTraffic.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Require(bool condition,const char* reason)
{
    if (!condition)
    {
        std::cerr<<"AerodynamicWakeTraffic FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}

static WakeActor Observed(int slot,double x,double y=0.0)
{
    WakeActor car;
    car.Identity.Session=71;
    car.Identity.SessionRevision=3;
    car.Identity.Actor=900+static_cast<std::uint64_t>(slot);
    car.Identity.Frame=90;
    car.Identity.Slot=slot;
    car.Identity.EngineObserved=true;
    car.Identity.BodySimulating=true;
    car.Identity.OriginalTrack=true;
    car.Pose.Position={x,y};
    car.Pose.HeadingRadians=0.0;
    car.VelocityCmS={3000.0,0.0};
    car.WindVelocityCmS={0.0,0.0};
    car.SourceCertainty=1.0;
    car.ClockSeconds=6.0;
    car.OriginalAsphaltQualified=true;
    car.ContactPhysicsQualified=true;
    return car;
}
static WakeFleetObservation SixPhysicalCars()
{
    WakeFleetObservation observation;
    observation.FollowingCar=Observed(5,0.0);
    for (int slot=0;slot<5;++slot)
    {
        observation.OtherCars[static_cast<std::size_t>(slot)]=
            Observed(slot,-4000.0-500.0*slot,220.0);
    }
    observation.OtherCars[1]=Observed(1,1400.0);
    observation.OtherCount=5;
    observation.DeltaSeconds=0.016;
    observation.EngineRaceRunning=true;
    observation.OriginalMapConfirmed=true;
    observation.CompletePhysicalRoster=true;
    return observation;
}
static void IdentityContract()
{
    auto observation=SixPhysicalCars();
    Require(ValidateWakeFleet(observation,{}),
            "six genuine, same-session engine-observed actors are valid");
    const auto original=observation.OtherCars[2];
    observation.OtherCars[2].Identity.Actor=
        observation.OtherCars[1].Identity.Actor;
    Require(!ValidateWakeFleet(observation,{}),
            "duplicate source actor must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].Identity.Slot=1;
    Require(!ValidateWakeFleet(observation,{}),
            "duplicate slot must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].Identity.Session=72;
    Require(!ValidateWakeFleet(observation,{}),
            "cross-session roster must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].Identity.Frame=89;
    Require(!ValidateWakeFleet(observation,{}),
            "stale measured frame must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].Identity.SessionRevision=4;
    Require(!ValidateWakeFleet(observation,{}),
            "mismatched recovery revision must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].ClockSeconds=5.0;
    Require(!ValidateWakeFleet(observation,{}),
            "stale observed race clock must fail");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].OriginalAsphaltQualified=false;
    Require(!ValidateWakeFleet(observation,{}),
            "off-road opponent cannot grant aerodynamic credit");
    observation.OtherCars[2]=original;
    observation.OtherCars[2].ContactPhysicsQualified=false;
    Require(!ValidateWakeFleet(observation,{}),
            "missing Chaos contact cannot grant aerodynamic credit");
    observation.OtherCars[2]=original;
    observation.OtherCount=4;
    Require(!ValidateWakeFleet(observation,{}),
            "missing sixth physical car cannot become clear air");
    observation=SixPhysicalCars();
    observation.FollowingCar.VelocityCmS.X=
        std::numeric_limits<double>::quiet_NaN();
    Require(!ValidateWakeFleet(observation,{}),
            "NaN physics sample must fail");
    observation=SixPhysicalCars();
    observation.OtherCars[3].SourceCertainty=0.2;
    Require(!ValidateWakeFleet(observation,{}),
            "uncertain sensor evidence rejected");
}
static void WakeGeometry()
{
    auto observation=SixPhysicalCars();
    auto snapshot=EvaluateAerodynamicWake(observation);
    Require(snapshot.Valid && snapshot.ProvenOriginalRoster &&
            snapshot.QualifiedLeaders==5,
            "all five physical leader slots audited");
    Require(snapshot.WakeLeaders==1 &&
            snapshot.StrongestLeaderSlot==1 &&
            snapshot.HasUsableWake,
            "only real leading vehicle creates wake");
    Require(snapshot.DragReliefFraction>0.0 &&
            snapshot.FrontDownforceLoss>0.0 &&
            snapshot.RearDownforceLoss>0.0,
            "drafting reduces drag and dirty-air corner downforce");
    Require(snapshot.DragReliefFraction<0.29 &&
            snapshot.FrontDownforceLoss<0.21,
            "wake effect remains bounded");
    const double baseline=snapshot.DragReliefFraction;
    std::swap(observation.OtherCars[1],observation.OtherCars[4]);
    snapshot=EvaluateAerodynamicWake(observation);
    Require(snapshot.Valid && snapshot.StrongestLeaderSlot==1 &&
            std::abs(snapshot.DragReliefFraction-baseline)<1e-9,
            "fleet order cannot affect physical drafting decision");
    observation=SixPhysicalCars();
    observation.OtherCars[1].Pose.Position.X=-1400.0;
    Require(EvaluateAerodynamicWake(observation).Valid &&
            !EvaluateAerodynamicWake(observation).HasUsableWake,
            "rear car cannot create a leading wake");
    observation=SixPhysicalCars();
    observation.OtherCars[1].Pose.Position.Y=4000.0;
    Require(!EvaluateAerodynamicWake(observation).HasUsableWake,
            "distant separate lane produces negligible wake");
    observation=SixPhysicalCars();
    observation.OtherCars[1].VelocityCmS={0.0,0.0};
    Require(!EvaluateAerodynamicWake(observation).HasUsableWake,
            "stationary leader has no high-speed drafting effect");
    observation=SixPhysicalCars();
    observation.OtherCars[1].Pose.HeadingRadians=Pi;
    Require(!EvaluateAerodynamicWake(observation).HasUsableWake,
            "oncoming vehicle cannot grant slipstream");
    observation=SixPhysicalCars();
    observation.OtherCars[1].Pose.Position.X=250.0;
    Require(!EvaluateAerodynamicWake(observation).HasUsableWake,
            "body overlap cannot be rewarded with clean drafting");
    observation=SixPhysicalCars();
    observation.OtherCars[1].SourceCertainty=0.9;
    Require(EvaluateAerodynamicWake(observation).CombinedWakeFraction>
            0.0 &&
            EvaluateAerodynamicWake(observation).CombinedWakeFraction<
            baseline/0.29,
            "source-certainty scales measured aerodynamic influence");
}
static void CoupledAerodynamics()
{
    AeroObservation observation;
    observation.VehicleVelocityCmS={3000.0,0.0};
    observation.OriginalVehicleObserved=true;
    observation.AeroBodyPhysicallyPresent=true;
    observation.DeltaSeconds=0.016;
    auto freeAir=CalculateAerodynamicForces(
        observation,{},0.0);
    Require(freeAir.Valid && freeAir.TotalDownforceN>0.0,
            "existing physical free-air model yields finite forces");
    auto wake=EvaluateAerodynamicWake(SixPhysicalCars());
    const auto adjusted=ApplyWakeToModeledAero(freeAir,wake);
    Require(adjusted.Valid && !adjusted.AppliedToChaos &&
            adjusted.Adjusted.Valid &&
            !adjusted.Adjusted.AuthoritativeChaosForce,
            "wake only changes modeled force, never real Chaos");
    Require(adjusted.DragRemovedN>0.0 &&
            adjusted.DownforceRemovedN>0.0 &&
            adjusted.CoolingRateRemoved>0.0,
            "drag, downforce and cooling have bounded dirty-air impact");
    Require(adjusted.Adjusted.TotalDownforceN>=0.0 &&
            adjusted.Adjusted.FrontDownforceN>=0.0 &&
            adjusted.Adjusted.RearDownforceN>=0.0 &&
            adjusted.Adjusted.EngineCoolingCoefficient>=0.0,
            "no negative physical modeled forces");
    wake=EvaluateAerodynamicWake(SixPhysicalCars());
    wake.Valid=false;
    Require(!ApplyWakeToModeledAero(freeAir,wake).Valid,
            "unqualified wake cannot change force predictions");
}
static void TemporalAndRecovery()
{
    auto observation=SixPhysicalCars();
    AerodynamicWakeSupervisor supervisor;
    Require(supervisor.Configure({}),
            "supervisor accepts plausible physical settings");
    const auto first=supervisor.Step(observation);
    Require(first.Valid && first.HasUsableWake &&
            first.CombinedWakeFraction<
                EvaluateAerodynamicWake(observation).CombinedWakeFraction,
            "draft builds gradually instead of teleporting drag");
    for(int n=0;n<100;++n)
    {
        ++observation.FollowingCar.Identity.Frame;
        for(auto& car:observation.OtherCars)
            ++car.Identity.Frame;
        observation.FollowingCar.ClockSeconds+=0.016;
        for(auto& car:observation.OtherCars)
            car.ClockSeconds+=0.016;
        Require(supervisor.Step(observation).Valid,
                "validated sequence remains stable");
    }
    Require(supervisor.Frames()==101 &&
            supervisor.TimeInWakeSeconds()>1.0 &&
            supervisor.PeakDraftFraction()>first.CombinedWakeFraction,
            "accepted temporal samples accumulate real drafting time");
    observation.OtherCars[1].Identity.Actor+=100;
    ++observation.FollowingCar.Identity.Frame;
    observation.FollowingCar.ClockSeconds+=0.016;
    for(auto& car:observation.OtherCars)
    {
        ++car.Identity.Frame;
        car.ClockSeconds+=0.016;
    }
    const auto reset=supervisor.Step(observation);
    Require(reset.Valid && supervisor.RosterResets()==1 &&
            reset.CombinedWakeFraction<
                supervisor.PeakDraftFraction(),
            "changed actor identity restarts the wake transition");
    // A changed recovery revision must clear the previous aero filter.
    ++observation.FollowingCar.Identity.SessionRevision;
    ++observation.FollowingCar.Identity.Frame;
    observation.FollowingCar.ClockSeconds+=0.016;
    for(auto& car:observation.OtherCars)
    {
        ++car.Identity.SessionRevision;
        ++car.Identity.Frame;
        car.ClockSeconds+=0.016;
    }
    const auto newRevision=supervisor.Step(observation);
    Require(newRevision.Valid &&
            supervisor.RosterResets()==2 &&
            newRevision.CombinedWakeFraction<
                supervisor.PeakDraftFraction(),
            "recovery revision cannot inherit old slipstream");
    Require(!supervisor.Step(observation).Valid &&
            supervisor.Rejects()==1,
            "replayed engine frame cannot count as fresh drafting");
    ++observation.FollowingCar.Identity.Frame;
    observation.FollowingCar.ClockSeconds+=0.016;
    for(auto& car:observation.OtherCars)
    {
        ++car.Identity.Frame;
        car.ClockSeconds+=0.016;
    }
    Require(supervisor.Step(observation).Valid,
            "fresh frame can recover from a rejected replay");
    // Even with a new ordinal, an unchanged race clock is stale.
    ++observation.FollowingCar.Identity.Frame;
    for(auto& car:observation.OtherCars)
        ++car.Identity.Frame;
    Require(!supervisor.Step(observation).Valid &&
            supervisor.Rejects()==2,
            "new frame with repeated clock must not renew wake");
    observation.FollowingCar.ClockSeconds+=0.016;
    for(auto& car:observation.OtherCars)
        car.ClockSeconds+=0.016;
    Require(supervisor.Step(observation).Valid,
            "fresh consistent clock permits new aerodynamic evidence");
    observation.EngineRaceRunning=false;
    Require(!supervisor.Step(observation).Valid &&
            supervisor.Rejects()==3,
            "paused or unqualified race immediately loses wake");
    WakeParameters invalid;
    invalid.MaximumDragReliefFraction=1.0;
    Require(!ValidWakeParameters(invalid) &&
            !supervisor.Configure(invalid),
            "unphysical full drag cancellation is invalid");
}
int main()
{
    IdentityContract();
    WakeGeometry();
    CoupledAerodynamics();
    TemporalAndRecovery();
    std::cout<<"AerodynamicWakeTraffic OK\n";
}
