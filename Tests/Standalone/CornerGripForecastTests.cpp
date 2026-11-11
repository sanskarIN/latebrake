#include "Racecraft/CornerGripForecast.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Check(bool ok,const char* reason)
{
    if (!ok)
    {
        std::cerr<<"CornerGripForecast FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static CornerForecastObservation CarInCorner()
{
    CornerForecastObservation s;
    s.Identity.Session=44;
    s.Identity.Actor=811;
    s.Identity.Frame=99;
    s.Identity.SessionRevision=1;
    s.Identity.Slot=1;
    s.Identity.EngineObserved=true;
    s.Identity.BodySimulating=true;
    s.Identity.OriginalTrack=true;
    s.VehicleVelocityCmS={3500.0,0.0};
    s.HeadingRadians=0.0;
    s.MassKg=1350.0;
    s.DeltaSeconds=0.016;
    s.ProposedThrottle=0.9;
    s.ProposedBrake=0.0;
    for (auto& wheel:s.Wheels)
    {
        wheel.NormalLoadN=3300.0;
        wheel.PhysicalContact=true;
        wheel.ContactVerified=true;
        wheel.Surface=ContactSurface::Asphalt;
        wheel.SurfaceGrip=1.2;
        wheel.TemperatureC=78.0;
        wheel.WearFraction=0.05;
        wheel.SlipRatio=0.03;
        wheel.SlipAngleRadians=0.04;
    }
    AeroObservation air;
    air.VehicleVelocityCmS={3500.0,0.0};
    air.OriginalVehicleObserved=true;
    air.AeroBodyPhysicallyPresent=true;
    air.DeltaSeconds=0.016;
    s.MeasuredAero=CalculateAerodynamicForces(air,{},0.0);
    s.KnotCount=4;
    for(int i=0;i<4;++i)
    {
        auto& knot=s.Route[static_cast<std::size_t>(i)];
        knot.AlongRouteCm=600.0*i;
        knot.ActualGeometryQualified=true;
        knot.OriginalAsphaltSample=true;
        knot.RealFrictionQualified=true;
        knot.MeasuredFrictionScale=1.0;
        knot.DistanceToBarrierCm=800.0;
        knot.RoadHalfWidthCm=800.0;
    }
    s.Route[1].CurvaturePerCm=0.00020;
    s.Route[2].CurvaturePerCm=0.00040;
    s.EngineRaceGreen=true;
    s.OriginalCircuitConfirmed=true;
    s.ExistingAeroCalculatedFromPhysics=true;
    return s;
}
static WakeActor RealWakeActor(int slot,double x)
{
    WakeActor actor;
    actor.Identity.Session=44;
    actor.Identity.Actor=810+
        static_cast<std::uint64_t>(slot);
    actor.Identity.Frame=99;
    actor.Identity.SessionRevision=1;
    actor.Identity.Slot=slot;
    actor.Identity.EngineObserved=true;
    actor.Identity.BodySimulating=true;
    actor.Identity.OriginalTrack=true;
    actor.Pose.Position={x,0.0};
    actor.VelocityCmS={3500.0,0.0};
    actor.ClockSeconds=4.0;
    actor.OriginalAsphaltQualified=true;
    actor.ContactPhysicsQualified=true;
    return actor;
}
static WakeSnapshot RealDirtyAir()
{
    WakeFleetObservation s;
    s.FollowingCar=RealWakeActor(1,0.0);
    s.OtherCars[0]=RealWakeActor(0,-3000.0);
    s.OtherCars[1]=RealWakeActor(2,1400.0);
    s.OtherCars[2]=RealWakeActor(3,-4000.0);
    s.OtherCars[3]=RealWakeActor(4,-5000.0);
    s.OtherCars[4]=RealWakeActor(5,-6000.0);
    s.OtherCount=5;
    s.EngineRaceRunning=true;
    s.OriginalMapConfirmed=true;
    s.CompletePhysicalRoster=true;
    s.DeltaSeconds=0.016;
    return EvaluateAerodynamicWake(s);
}
static void PhysicsAndBraking()
{
    auto observation=CarInCorner();
    Check(ValidCornerForecastObservation(observation,{}),
          "actual four-wheel source and route samples validated");
    const auto plan=ForecastPhysicalCornerGrip(observation);
    Check(plan.Valid && plan.KnotCount==4 &&
          plan.NoUnrealForceIssued && plan.EdgesVerified,
          "real measured wheel and track policy is source-only");
    Check(plan.MinimumUpcomingCornerSpeedCmS<
          plan.MeasuredSpeedCmS,
          "tight measured corner lowers entry pace");
    Check(plan.PlannedEntrySpeedCmS<
          plan.MeasuredSpeedCmS &&
          plan.CommandThrottle<=observation.ProposedThrottle &&
          plan.CommandBrake>0.0,
          "backward braking from apex lowers throttle and requests brake");
    Check(plan.AvailableBrakingCmS2>0.0 &&
          plan.EffectiveTireGrip>0.0 &&
          plan.Knots[1].SafeCornerSpeedCmS<
          plan.Knots[0].SafeCornerSpeedCmS,
          "measured curvature predicts sensible lateral acceleration");
    auto slow=observation;
    slow.VehicleVelocityCmS={600.0,0.0};
    const auto cruise=ForecastPhysicalCornerGrip(slow);
    Check(cruise.Valid && cruise.CommandBrake==0.0 &&
          cruise.CommandThrottle<=slow.ProposedThrottle,
          "low-speed approach does not invent brake demand");
    auto gentle=observation;
    gentle.Route[1].CurvaturePerCm=0.0;
    gentle.Route[2].CurvaturePerCm=0.0;
    const auto straight=ForecastPhysicalCornerGrip(gentle);
    Check(straight.Valid &&
          straight.PlannedEntrySpeedCmS>
          plan.PlannedEntrySpeedCmS,
          "physically straight asphalt permits a higher entry speed");
    auto wet=observation;
    for(auto& wheel:wet.Wheels)
        wheel.SurfaceGrip=0.7;
    for(int i=0;i<wet.KnotCount;++i)
        wet.Route[static_cast<std::size_t>(i)].
            MeasuredFrictionScale=0.7;
    const auto rain=ForecastPhysicalCornerGrip(wet);
    Check(rain.Valid &&
          rain.PlannedEntrySpeedCmS<
          plan.PlannedEntrySpeedCmS &&
          rain.EffectiveTireGrip<plan.EffectiveTireGrip,
          "wet physical contacts reduce safe corner speed");
    auto worn=observation;
    for(auto& wheel:worn.Wheels)
    {
        wheel.WearFraction=0.90;
        wheel.SlipAngleRadians=0.40;
    }
    const auto poorTires=ForecastPhysicalCornerGrip(worn);
    Check(poorTires.Valid &&
          poorTires.PlannedEntrySpeedCmS<
          plan.PlannedEntrySpeedCmS,
          "worn sliding tires require more conservative pace");
}
static void DirtyAirAndRoad()
{
    auto observation=CarInCorner();
    const auto clean=ForecastPhysicalCornerGrip(observation);
    observation.Wake=RealDirtyAir();
    observation.WakeRosterMatchesIdentity=true;
    Check(observation.Wake.Valid &&
          observation.Wake.HasUsableWake,
          "real physical six-car dirty-air observation exists");
    const auto dirty=ForecastPhysicalCornerGrip(observation);
    Check(dirty.Valid && dirty.DirtyAirGripLoss &&
          dirty.AvailableDownforceN<
              clean.AvailableDownforceN &&
          dirty.PlannedEntrySpeedCmS<=
              clean.PlannedEntrySpeedCmS,
          "dirty air reduces modeled grip and corner entry budget");
    observation.Wake.Valid=false;
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "unqualified wake evidence rejected");
    observation=CarInCorner();
    observation.Wake=RealDirtyAir();
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "valid wake with unconfirmed roster binding rejected");
    observation=CarInCorner();
    observation.Route[2].DistanceToBarrierCm=50.0;
    const auto barrier=ForecastPhysicalCornerGrip(observation);
    Check(barrier.Valid &&
          barrier.Knots[2].SafeCornerSpeedCmS==0.0 &&
          barrier.PlannedEntrySpeedCmS<
              clean.PlannedEntrySpeedCmS,
          "real nearby physical barrier lowers speed");
    observation=CarInCorner();
    observation.Route[2].OriginalAsphaltSample=false;
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "invented or off-asphalt trajectory rejected");
    observation=CarInCorner();
    observation.Route[1].AlongRouteCm=0.0;
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "non-increasing preview distance rejected");
    observation=CarInCorner();
    observation.Wheels[1].PhysicalContact=false;
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "missing grounded wheel rejected");
    observation=CarInCorner();
    observation.VehicleVelocityCmS.X=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "invalid actual chassis velocity rejected");
    observation=CarInCorner();
    observation.MeasuredAero.Valid=false;
    Check(!ForecastPhysicalCornerGrip(observation).Valid,
          "unqualified aerodynamic model rejected");
}
static void StatefulPolicy()
{
    CornerGripSupervisor supervisor;
    Check(supervisor.Configure({}),
          "qualified corner controller configuration");
    auto source=CarInCorner();
    auto first=supervisor.Step(source);
    Check(first.Valid && first.CommandThrottle<=
          source.ProposedThrottle &&
          first.CommandBrake>=source.ProposedBrake,
          "stateful step never grants additional motor authority");
    auto duplicated=supervisor.Step(source);
    Check(!duplicated.Valid && supervisor.Rejections()==1,
          "duplicate frame cannot reuse stale engine observations");
    ++source.Identity.Frame;
    const auto resumed=supervisor.Step(source);
    Check(resumed.Valid && supervisor.Frames()==2,
          "new actual source frame permits resumed corner policy");
    CornerForecastSettings invalid;
    invalid.MaximumBrakingDecelerationCmS2=0.0;
    Check(!ValidCornerForecastSettings(invalid) &&
          !supervisor.Configure(invalid),
          "zero braking capability never qualifies a controller");
}
int main()
{
    PhysicsAndBraking();
    DirtyAirAndRoad();
    StatefulPolicy();
    std::cout<<"CornerGripForecast OK\n";
}
