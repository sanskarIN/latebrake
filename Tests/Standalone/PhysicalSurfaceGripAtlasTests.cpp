#include "Racecraft/PhysicalSurfaceGripAtlas.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace latebrake::racecraft;

static void Check(bool pass,const char* reason)
{
    if (!pass)
    {
        std::cerr<<"PhysicalSurfaceGripAtlas FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static GripContactFrame MeasuredGripFrame()
{
    GripContactFrame frame;
    frame.Identity.Session=93;
    frame.Identity.SessionRevision=2;
    frame.Identity.Actor=678;
    frame.Identity.Slot=1;
    frame.Identity.Frame=1;
    frame.Identity.EngineObserved=true;
    frame.Identity.BodySimulating=true;
    frame.Identity.OriginalTrack=true;
    frame.DistanceAlongOriginalLapCm=15.0;
    frame.RaceClockSeconds=1.0;
    frame.DeltaSeconds=0.016;
    frame.MeasuredForwardSpeedCmS=2800.0;
    frame.RoadFrictionScale=1.0;
    frame.ActualWetnessFraction=0.0;
    frame.MeasurementConfidence=0.98;
    frame.MeasuredMaterial=ContactSurface::Asphalt;
    frame.EngineRaceRunning=true;
    frame.OriginalMapPhysicsReady=true;
    frame.RoadGeometryVerified=true;
    frame.OriginalSurfaceContact=true;
    frame.RealFrictionSensorReady=true;
    for(auto& wheel:frame.Wheels)
    {
        wheel.PhysicalContact=true;
        wheel.ContactVerified=true;
        wheel.NormalLoadN=3200.0;
        wheel.Surface=ContactSurface::Asphalt;
        wheel.SurfaceGrip=1.15;
        wheel.SlipRatio=0.02;
        wheel.SlipAngleRadians=0.03;
    }
    return frame;
}
static void MoveForward(GripContactFrame& frame)
{
    ++frame.Identity.Frame;
    frame.RaceClockSeconds+=frame.DeltaSeconds;
}
static CornerForecastObservation RealCornerObservation()
{
    CornerForecastObservation sample;
    sample.Identity=MeasuredGripFrame().Identity;
    sample.Identity.Frame=5;
    sample.VehicleVelocityCmS={2800.0,0.0};
    sample.MassKg=1300.0;
    sample.DeltaSeconds=0.016;
    sample.ProposedThrottle=0.7;
    for(auto& wheel:sample.Wheels)
    {
        wheel.ContactVerified=true;
        wheel.PhysicalContact=true;
        wheel.NormalLoadN=3200.0;
        wheel.Surface=ContactSurface::Asphalt;
        wheel.SurfaceGrip=1.15;
    }
    AeroObservation air;
    air.VehicleVelocityCmS=sample.VehicleVelocityCmS;
    air.DeltaSeconds=0.016;
    air.OriginalVehicleObserved=true;
    air.AeroBodyPhysicallyPresent=true;
    sample.MeasuredAero=CalculateAerodynamicForces(air,{},0.0);
    sample.KnotCount=3;
    for(int i=0;i<3;++i)
    {
        auto& knot=sample.Route[
            static_cast<std::size_t>(i)];
        knot.AlongRouteCm=20.0*i;
        knot.MeasuredFrictionScale=1.0;
        knot.OriginalAsphaltSample=true;
        knot.ActualGeometryQualified=true;
        knot.RealFrictionQualified=true;
        knot.DistanceToBarrierCm=600.0;
    }
    sample.Route[1].CurvaturePerCm=0.0002;
    sample.EngineRaceGreen=true;
    sample.OriginalCircuitConfirmed=true;
    sample.ExistingAeroCalculatedFromPhysics=true;
    return sample;
}
static void ProvenPhysicalCoverage()
{
    GripAtlasSettings settings;
    PhysicalSurfaceGripAtlas atlas;
    Check(atlas.Configure(settings) &&
          atlas.BeginSession(93,2),
          "original circuit atlas begins for a verified session");
    auto frame=MeasuredGripFrame();
    Check(!atlas.Query(15.0,1.0).Valid,
          "no extrapolation before real wheel observations");
    for(int i=0;i<3;++i)
    {
        const auto accepted=atlas.Record(frame);
        Check(accepted.Qualified &&
              accepted.CellIndex==0 &&
              !accepted.AppliedToChaos &&
              accepted.NewGrip>0.0,
              "real four-wheel traction evidence counted once");
        MoveForward(frame);
    }
    const auto cell=atlas.Query(25.0,frame.RaceClockSeconds);
    Check(cell.Valid && cell.ReadyForPlanning &&
          cell.Samples==3 &&
          cell.ProvenOriginalRoad &&
          cell.PhysicsObserved &&
          cell.MeasuredGripScale>1.0 &&
          cell.Confidence>=settings.MinimumSensorConfidence,
          "three samples qualify one original asphalt cell");
    const auto provenAhead=atlas.ReviewRoadAhead(
        15.0,120.0,frame.RaceClockSeconds);
    Check(provenAhead.Valid &&
          provenAhead.FullRoadEvidence &&
          provenAhead.VerifiedCells==1 &&
          provenAhead.UnknownCells==0 &&
          provenAhead.NeverInferredUnobservedGrip,
          "short lookahead within measured physical road is fully proven");
    const double boundary=settings.CircuitLengthCm/GripAtlasCells;
    const auto unobservedAhead=atlas.ReviewRoadAhead(
        boundary-25.0,125.0,frame.RaceClockSeconds);
    Check(unobservedAhead.Valid &&
          !unobservedAhead.FullRoadEvidence &&
          unobservedAhead.VerifiedCells==1 &&
          unobservedAhead.UnknownCells>=1 &&
          unobservedAhead.UnknownAheadCm>0.0,
          "lookahead crossing an unmeasured cell reports exact missing coverage");
    const auto progress=atlas.Progress(frame.RaceClockSeconds);
    Check(progress.ConfirmedCells==1 &&
          progress.FreshCells==1 &&
          progress.PhysicalCoverageFraction==1.0/GripAtlasCells &&
          progress.CoverageIsMeasuredOnly &&
          progress.QualifiedMeasurements==3,
          "only actually visited original circuit coverage is credited");
    const double unvisited=
        settings.CircuitLengthCm*0.75;
    Check(!atlas.Query(unvisited,frame.RaceClockSeconds).Valid,
          "unsampled future circuit cannot inherit invented friction");
    Check(!atlas.Query(15.0,
          frame.RaceClockSeconds+settings.MaximumSensorAgeSeconds+1.0).Valid,
          "old physical friction cannot indefinitely plan a lap");
    auto target=RealCornerObservation();
    Check(atlas.ApplyToRoute(
              target,15.0,frame.RaceClockSeconds),
          "all route knots inside a measured cell accept grip binding");
    for(int i=0;i<target.KnotCount;++i)
        Check(target.Route[
              static_cast<std::size_t>(i)].
                MeasuredFrictionScale<=1.0,
              "historic evidence cannot inflate fresh route grip");
    Check(ForecastPhysicalCornerGrip(target).Valid,
          "measured per-cell grip feeds existing corner forecast");
    auto unsupported=target;
    unsupported.Route[2].AlongRouteCm=
        settings.CircuitLengthCm*0.75;
    const double prior=unsupported.Route[0].
        MeasuredFrictionScale;
    Check(!atlas.ApplyToRoute(
            unsupported,15.0,frame.RaceClockSeconds) &&
          unsupported.Route[0].MeasuredFrictionScale==prior,
          "missing road measurements reject entire route atomically");
}
static void RejectForgedOrStale()
{
    PhysicalSurfaceGripAtlas atlas;
    Check(atlas.Configure({}) &&
          atlas.BeginSession(93,2),
          "verified session configured");
    auto frame=MeasuredGripFrame();
    Check(atlas.Record(frame).Qualified,
          "first sample accepted");
    Check(atlas.Record(frame).Fault==
          GripRecordFault::DuplicateFrame,
          "duplicate actor frame is rejected");
    MoveForward(frame);
    ++frame.Identity.Actor;
    Check(atlas.Record(frame).Fault==
          GripRecordFault::WrongActor,
          "different physical actor cannot poison the same grip atlas");
    --frame.Identity.Actor;
    frame.Identity.Session=94;
    Check(atlas.Record(frame).Fault==
          GripRecordFault::MissingSession,
          "cross-session sample rejected");
    frame.Identity.Session=93;
    frame.Wheels[2].PhysicalContact=false;
    Check(!atlas.Record(frame).Qualified,
          "missing physics contact invalidates road friction");
    frame.Wheels[2].PhysicalContact=true;
    frame.OriginalSurfaceContact=false;
    Check(!atlas.Record(frame).Qualified,
          "unverified asphalt geometry rejected");
    frame.OriginalSurfaceContact=true;
    frame.MeasuredForwardSpeedCmS=
        std::numeric_limits<double>::quiet_NaN();
    Check(!atlas.Record(frame).Qualified,
          "NaN chassis velocity rejected");
    frame.MeasuredForwardSpeedCmS=2800.0;
    frame.RaceClockSeconds+=1.0;
    Check(atlas.Record(frame).Fault==
          GripRecordFault::BadSessionClock,
          "stale clock gap rejected rather than interpolated");
    Check(atlas.RejectedFrames()>=6,
          "invalid frames recorded separately from production coverage");
}
static void WetnessAndBraking()
{
    PhysicalSurfaceGripAtlas atlas;
    Check(atlas.Configure({}) &&
          atlas.BeginSession(93,2),
          "wetness atlas configured");
    auto frame=MeasuredGripFrame();
    for(int i=0;i<3;++i)
    {
        Check(atlas.Record(frame).Qualified,
              "dry baseline real contact");
        MoveForward(frame);
    }
    const auto dry=atlas.Query(15.0,frame.RaceClockSeconds);
    const auto before=RealCornerObservation();
    const auto baseline=ForecastPhysicalCornerGrip(before);
    Check(dry.Valid && baseline.Valid,
          "baseline can produce a corner speed");
    frame.MeasuredMaterial=ContactSurface::WetAsphalt;
    frame.ActualWetnessFraction=0.8;
    frame.RoadFrictionScale=0.65;
    for(auto& wheel:frame.Wheels)
        wheel.SurfaceGrip=0.68;
    bool firstTransition=false;
    for(int i=0;i<12;++i)
    {
        const auto result=atlas.Record(frame);
        Check(result.Qualified,
              "actual wet readings are not invented");
        if(i==0)firstTransition=result.ChangedWetnessClass;
        MoveForward(frame);
    }
    const auto wet=atlas.Query(15.0,frame.RaceClockSeconds);
    Check(firstTransition && wet.Valid &&
          wet.ReadyForPlanning &&
          wet.StandingWaterHazard &&
          wet.MeasuredGripScale<dry.MeasuredGripScale,
          "observed wet road reduces projected physical grip");
    const auto wetRoadAhead=atlas.ReviewRoadAhead(
        15.0,100.0,frame.RaceClockSeconds);
    Check(wetRoadAhead.Valid &&
          wetRoadAhead.FullRoadEvidence &&
          wetRoadAhead.WetHazardSeen &&
          wetRoadAhead.HazardCells==1 &&
          wetRoadAhead.NearestHazardCell==0 &&
          wetRoadAhead.NearestPhysicalHazardCm==0.0,
          "physical wet-road warning is grounded in a measured cell");
    const auto progress=atlas.Progress(frame.RaceClockSeconds);
    Check(progress.RealWetTransitions>=1 &&
          progress.ConfirmedCells==1,
          "wet transition is counted but not mistaken for new coverage");
    auto corner=RealCornerObservation();
    Check(atlas.ApplyToRoute(corner,15.0,
            frame.RaceClockSeconds),
          "wet road evidence overlays the original circuit route");
    const auto predicted=ForecastPhysicalCornerGrip(corner);
    Check(predicted.Valid &&
          predicted.PlannedEntrySpeedCmS<=
              baseline.PlannedEntrySpeedCmS,
          "wet grip atlas never makes a previously safer corner faster");
    Check(atlas.BeginSession(94,0) &&
          !atlas.Query(15.0,frame.RaceClockSeconds).Valid,
          "new session clears all prior grip telemetry");
}
int main()
{
    ProvenPhysicalCoverage();
    RejectForgedOrStale();
    WetnessAndBraking();
    std::cout<<"PhysicalSurfaceGripAtlas OK\n";
}
