#include "Racecraft/RoadVerticalProfileSampler.h"
#include "Racecraft/ActiveDamperRideControl.h"
#include "Racecraft/MeasuredContactPatchEnvelope.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Require(bool ok,const char* message)
{
    if (!ok)
    {
        std::cerr<<"PhysicalRideAndContact FAIL: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static RaceIdentity RealIdentity()
{
    RaceIdentity r;
    r.Session=8001;
    r.SessionRevision=4;
    r.Actor=981;
    r.Slot=1;
    r.Frame=55;
    r.EngineObserved=true;
    r.BodySimulating=true;
    r.OriginalTrack=true;
    return r;
}
static RoadVerticalObservation Road()
{
    RoadVerticalObservation r;
    r.Identity=RealIdentity();
    r.Count=4;
    r.PhysicsClockSeconds=15.0;
    r.ChassisForwardSpeedCmS=3600.0;
    r.CurrentRideHeightCm=12.0;
    r.DeltaSeconds=0.016;
    r.ActualChaosBodyReady=true;
    r.OriginalCircuitKnown=true;
    r.PhysicsTracesFresh=true;
    for(int i=0;i<r.Count;++i)
    {
        auto& s=r.Samples[static_cast<std::size_t>(i)];
        s.ForwardDistanceCm=100.0*i;
        s.CenterHeightCm=i==2?9.0:0.0;
        s.LeftHeightCm=i==2?12.0:0.0;
        s.RightHeightCm=i==2?7.0:0.0;
        s.LeftRoughnessCm=0.5;
        s.RightRoughnessCm=0.4;
        s.SourceComponent=111;
        s.Surface=ContactSurface::Asphalt;
        s.TraceClockSeconds=15.0;
        s.OriginalCollisionHit=true;
        s.RoadContactVerified=true;
        s.LeftWheelTraceVerified=true;
        s.RightWheelTraceVerified=true;
    }
    return r;
}
static WheelObservation RealWheel()
{
    WheelObservation w;
    w.NormalLoadN=3200.0;
    w.PhysicalContact=true;
    w.ContactVerified=true;
    w.Surface=ContactSurface::Asphalt;
    w.SurfaceGrip=1.05;
    w.TemperatureC=85.0;
    w.WearFraction=0.04;
    return w;
}
static DamperRideObservation Ride()
{
    DamperRideObservation r;
    r.Identity=RealIdentity();
    r.Road=ForecastRoadVerticalProfile(Road());
    r.PhysicsClockSeconds=15.0;
    r.DeltaSeconds=0.016;
    r.CurrentSpeedCmS=3600.0;
    r.DriverDemandedStiffness=0.65;
    r.ActualChaosSuspensionReady=true;
    r.FourPhysicalContactsVerified=true;
    r.OriginalMapVerified=true;
    for(auto& w:r.Wheels)
    {
        w.Wheel=RealWheel();
        w.ShaftVelocityCmS=15.0;
        w.MeasuredSuspensionForceN=3000.0;
        w.MeasuredRideHeightCm=12.0;
        w.WheelTravelDemandCm=9.0;
        w.SensorQualified=true;
        w.OriginalSuspensionComponent=true;
    }
    return r;
}
static ContactPatchWheelSample Contact()
{
    ContactPatchWheelSample s;
    s.Identity=RealIdentity();
    s.Wheel=RealWheel();
    s.MeasuredPressureKpa=210.0;
    s.MeasuredTreadDepthMm=7.0;
    s.ActualForwardSpeedCmS=3200.0;
    s.ActualWaterDepthMm=0.0;
    s.MeasuredRoadFriction=1.0;
    s.DeltaSeconds=0.016;
    s.OriginalWheelObserved=true;
    s.PhysicalRoadFrictionReady=true;
    s.PhysicalWaterSensorReady=true;
    s.ActualChaosBodyReady=true;
    return s;
}
static void RoadProfile()
{
    auto s=Road();
    const auto a=ForecastRoadVerticalProfile(s);
    Require(a.Valid && a.VerifiedTraces==4 &&
            a.PeakRiseCm==9.0 &&
            a.PeakDropCm==9.0 &&
            a.NearestBumpCm==200.0,
            "measured rise and drop are preserved");
    Require(a.ActualChaosActuation==false &&
            a.RecommendedSpeedCeilingCmS>0.0 &&
            a.MinimumConfidence>=0.7,
            "source-only bounded speed forecast");
    RoadVerticalProfileSupervisor controller;
    Require(controller.Configure({}) &&
            controller.Step(s).Valid,
            "real chassis profile accepted");
    Require(!controller.Step(s).Valid &&
            controller.Rejections()==1,
            "replayed source frame rejected");
    ++s.Identity.Frame;
    Require(controller.Step(s).Valid,
            "new chassis frame accepted");
    s.Samples[1].SourceComponent=0;
    Require(!ForecastRoadVerticalProfile(s).Valid,
            "fabricated road collision component rejected");
    s=Road();
    s.Samples[1].ForwardDistanceCm=400.0;
    Require(!ForecastRoadVerticalProfile(s).Valid,
            "unsorted road sample cannot invent geometry");
    s=Road();
    s.Samples[2].LeftWheelTraceVerified=false;
    Require(!ForecastRoadVerticalProfile(s).Valid,
            "missing wheel trace cannot qualify wheel bump");
}
static void DamperRide()
{
    auto input=Ride();
    Require(input.Road.Valid,
            "damper consumes physically verified vertical road");
    const auto sport=ForecastDamperRide(
        input,DamperRideMode::Sport,0.8);
    Require(sport.Valid && sport.ContactWheels==4 &&
            sport.TotalDampingPowerW>0.0 &&
            sport.FrontMeanForceN>0.0 &&
            !sport.ActualChaosForceIssued,
            "force proposals use physical wheel observations only");
    const auto softer=ForecastDamperRide(
        input,DamperRideMode::Comfort,0.35);
    Require(softer.Valid &&
            softer.TotalDampingPowerW<
            sport.TotalDampingPowerW,
            "reduced valve stiffness reduces damping power");
    ActiveDamperRideSupervisor controller;
    Require(controller.Configure({},DamperRideMode::Road) &&
            controller.Step(input).Valid,
            "qualified suspension source accepted");
    Require(!controller.Step(input).Valid &&
            controller.Rejections()==1,
            "repeated damper frame rejected");
    ++input.Identity.Frame;
    ++input.Road.Identity.Frame;
    Require(controller.Step(input).Valid,
            "new physics frame permits updated suspension");
    input.Wheels[1].Wheel.ContactVerified=false;
    Require(!ForecastDamperRide(
                input,DamperRideMode::Road,0.5).Valid,
            "missing original tire contact blocks damping estimate");
    input=Ride();
    input.Road.Identity.SessionRevision++;
    Require(!ForecastDamperRide(
                input,DamperRideMode::Road,0.5).Valid,
            "road geometry from previous recovery epoch is rejected");
}
static void ContactPatches()
{
    auto wheel=Contact();
    const auto dry=EvaluateContactPatch(wheel);
    Require(dry.Valid &&
            dry.PatchAreaCm2>0.0 &&
            dry.HydroplaningSeverity==0.0 &&
            dry.AvailableLongitudinalForceN>0.0 &&
            !dry.AppliesChaosForce,
            "verified dry contact patch has finite force envelope");
    wheel.ActualWaterDepthMm=40.0;
    wheel.ActualForwardSpeedCmS=9000.0;
    const auto wet=EvaluateContactPatch(wheel);
    Require(wet.Valid &&
            wet.HydroplaningSeverity>0.0 &&
            wet.LateralGrip<dry.LateralGrip &&
            wet.AvailableLateralForceN<
                dry.AvailableLateralForceN,
            "measured deep water reduces lateral contact reserve");
    wheel.MeasuredTreadDepthMm=1.0;
    const auto wornWet=EvaluateContactPatch(wheel);
    Require(wornWet.Valid &&
            wornWet.HydroplaningOnsetSpeedCmS<
            wet.HydroplaningOnsetSpeedCmS,
            "worn tread reaches hydroplaning conditions sooner");
    wheel=Contact();
    FourPatchObservation four;
    four.Identity=RealIdentity();
    four.FrameSeconds=0.016;
    for(auto& patch:four.Wheels)patch=wheel;
    const auto aggregate=EvaluateFourContactPatches(four);
    Require(aggregate.Valid &&
            aggregate.GroundedWheels==4 &&
            aggregate.TotalContactAreaCm2>0.0,
            "four measured wheel patches aggregate consistently");
    PhysicalContactPatchSupervisor controller;
    Require(controller.Configure({}) &&
            controller.Step(four).Valid &&
            !controller.Step(four).Valid,
            "replayed tire-frame observation cannot be reused");
    four.Wheels[2].Identity.Frame--;
    Require(!EvaluateFourContactPatches(four).Valid,
            "asynchronous wheel sample invalidates whole car");
    wheel=Contact();
    wheel.MeasuredPressureKpa=
        std::numeric_limits<double>::infinity();
    Require(!EvaluateContactPatch(wheel).Valid,
            "infinite tire pressure is not physical evidence");
}
int main()
{
    RoadProfile();
    DamperRide();
    ContactPatches();
    std::cout<<"PhysicalRideAndContact OK\n";
}
