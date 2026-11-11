#include "Racecraft/RacecraftMath.h"
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/RacecraftFrameGate.h"
#include "Racecraft/SurfaceMaterialModel.h"
#include "Racecraft/TireForceModel.h"
#include "Racecraft/BrakeHydraulics.h"
#include "Racecraft/ChassisLoadTransfer.h"
#include "Racecraft/SuspensionForceModel.h"
#include "Racecraft/SteeringKinematics.h"
#include "Racecraft/EngineTorqueModel.h"
#include "Racecraft/GearboxDynamics.h"
#include "Racecraft/DifferentialDynamics.h"
#include "Racecraft/VehicleAerodynamics.h"
#include "Racecraft/VehiclePlanarDynamics.h"
#include "Racecraft/CollisionImpulseModel.h"
#include "Racecraft/VehicleStabilityAssist.h"
#include "Racecraft/VerifiedGateCrossing.h"
#include "Racecraft/RaceClockDirector.h"
#include "Racecraft/RaceStartGridSafety.h"
#include "Racecraft/SmoothRacingLine.h"
#include "Racecraft/CornerSpeedPlanner.h"
#include "Racecraft/AIPathFollowing.h"
#include "Racecraft/PhysicalCarFollowing.h"
#include "Racecraft/AIOvertakePlanner.h"
#include "Racecraft/RaceMarshalFlags.h"
#include "Racecraft/TrackLimitsEvidence.h"
#include "Racecraft/RaceStartLaunchGuard.h"
#include "Racecraft/PitLaneOperations.h"
#include "Racecraft/TireThermalDynamics.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace latebrake::racecraft;

static void Check(bool pass,const char* reason)
{
    if (!pass)
    {
        std::cerr<<"RacecraftExtended FAIL: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static PhysicalVehicleFrame RealActor(int slot=1,double x=-1500.0)
{
    PhysicalVehicleFrame c;
    c.Identity.Session=101;
    c.Identity.Actor=200+static_cast<std::uint64_t>(slot);
    c.Identity.Slot=slot;
    c.Identity.Frame=10;
    c.Identity.EngineObserved=true;
    c.Identity.BodySimulating=true;
    c.Identity.OriginalTrack=true;
    c.PhysicsQualified=true;
    c.ChassisOnWorldStatic=true;
    c.Chassis.PlanarPose.Position={x,-1800.0};
    c.Chassis.WorldPositionCm={x,-1800.0,145.0};
    c.Chassis.VelocityCmS={1500.0,0.0,0.0};
    c.Track.OnOriginalPavement=true;
    c.Race.EngineGameModeReady=true;
    c.Race.Phase=RacePhase::Green;
    c.Race.ClockSeconds=5.0;
    c.Race.FrameDeltaSeconds=0.016;
    c.TimeSeconds=5.0;
    for (auto& w:c.Wheels)
    {
        w.ContactVerified=true;
        w.PhysicalContact=true;
        w.NormalLoadN=3200.0;
        w.Surface=ContactSurface::Asphalt;
    }
    return c;
}
static void TestRealRoadAndPlans()
{
    const SmoothLineGeometry track=BuildOriginalSmoothLine();
    Check(track.Valid && track.Closed && track.Continuous,
          "eight original straights and corners join continuously");
    Check(track.Pieces.size()==8 &&
          track.TotalLengthCm>10000.0,
          "bounded physical circuit line");
    for (int i=0;i<512;++i)
    {
        const SmoothLineSample sample=SampleSmoothRacingLine(
            track,track.TotalLengthCm*i/512.0);
        Check(sample.Valid && sample.InsideAuthoredRoad,
              "sampled route never leaves original asphalt slabs");
    }
    const auto quality=InspectSmoothRoute(track);
    Check(quality.Valid &&
          quality.FullyInsideOriginalAsphalt,
          "route audit traverses full verified surface");
    const auto projected=ProjectToSmoothRacingLine(
        track,{-1200.0,-1800.0},0.0);
    Check(projected.Qualified && projected.OnPavement,
          "physical starting road projects to line");
    SpeedPlanSource source;
    source.Geometry=track;
    source.StartingDistanceCm=projected.TrackDistanceCm;
    source.ObservedSpeedCmS=1800.0;
    source.MeasuredGripG=1.1;
    source.PhysicalRoadReady=true;
    source.PhysicalTiresReady=true;
    source.PhysicalBrakesReady=true;
    source.RaceGreen=true;
    const SpeedPlanResult plan=BuildCornerSpeedPlan(source);
    Check(plan.Valid && plan.Count>=10 &&
          plan.MaximumPlannedSpeedCmS>0.0,
          "grip-aware backward/forward speed planning");
    const auto advice=ReviewUpcomingSpeed(plan,1900.0,1.0);
    Check(advice.Valid &&
          !advice.ActualVehicleControlled,
          "speed lookahead has no direct Unreal steering authority");
    AIPathInput ai;
    ai.Car=RealActor();
    ai.Course=track;
    ai.SpeedPlan=plan;
    ai.DeltaSeconds=0.016;
    ai.OriginalEnginePhysicsQualified=true;
    ai.EngineRaceRunning=true;
    const auto planned=PlanPhysicalRivalPath(ai);
    Check(planned.Valid &&
          !planned.HasUnrealControlAuthority &&
          planned.ProposedThrottle>=0.0 &&
          planned.ProposedThrottle<=1.0,
          "physical AI plan is bounded and engine-neutral");
}
static void TestVehicleEngineering()
{
    LoadTransferInput load;
    load.DeltaSeconds=0.016;
    load.BodySimulating=true;
    load.OriginalRoadContactMeasured=true;
    const auto loads=EvaluateWheelLoadTransfer(load);
    Check(loads.Valid && loads.LoadConserved &&
          loads.SupportingWheels==4,
          "actual chassis mass and load-transfer conservation");
    FourCornerSuspension dampers;
    Check(dampers.Configure({},{}),
          "front and rear suspension parameters");
    SuspensionFrameInput s;
    s.DeltaSeconds=0.016;
    s.BodyPhysicsReady=true;
    s.EngineWheelSensorsReady=true;
    s.CompressionCm.fill(12.0);
    s.NormalLoadN.fill(3200.0);
    s.ContactVerified.fill(true);
    Check(dampers.Step(s,268.0,160.0,158.0).Valid,
          "four-corner spring and dampers");
    SteeringObservation steering;
    steering.DriverRequest=0.2;
    steering.SpeedCmS=1300.0;
    steering.DeltaSeconds=0.016;
    steering.PhysicalSteeringReady=true;
    steering.EngineQualified=true;
    steering.OriginalTrack=true;
    const auto rack=EvaluateSteering(steering,{},0.0);
    Check(rack.Valid && rack.AckermannQualified,
          "bounded steering rack and Ackermann angles");
    EngineTorqueEvaluator engine;
    Check(engine.Configure(DefaultTouringEngine()),
          "nine-point touring engine curve");
    EngineFrameInput input;
    input.DeltaSeconds=0.016;
    input.PedalThrottle=0.7;
    input.ExternalEngineSpeedRpm=4000.0;
    input.MeasuredEngineTemperatureC=95.0;
    input.OriginalEngineObservation=true;
    input.EnginePhysicalReadoutAvailable=true;
    Check(engine.Step(input).Valid,
          "physical RPM / throttle engine torque");
    GearboxController gearbox;
    Check(gearbox.Configure({}),
          "automatic and manual gear validation");
    GearboxObservation transmission;
    transmission.DeltaSeconds=0.016;
    transmission.EngineRpm=4000.0;
    transmission.EngineTorqueNm=200.0;
    transmission.ObservedGear=2;
    transmission.RequestedGear=2;
    transmission.EngineRunning=true;
    transmission.PhysicalGearReadoutReady=true;
    transmission.EngineQualified=true;
    Check(gearbox.Step(transmission).Valid,
          "physical gear observation and wheel-torque estimate");
    DifferentialController differential;
    Check(differential.Configure({}),
          "clutch plate LSD parameter validation");
    DifferentialInput diff;
    diff.DeltaSeconds=0.016;
    diff.InputTorqueNm=350.0;
    diff.LeftNormalLoadN=3200.0;
    diff.RightNormalLoadN=3200.0;
    diff.PhysicalWheelSpeedVerified=true;
    diff.OriginalEngineQualified=true;
    Check(differential.Step(diff).Valid,
          "load-dependent differential torque split");
    ActiveAerodynamicController aero;
    Check(aero.Configure({}),
          "drag and downforce model parameters");
    AeroObservation air;
    air.DeltaSeconds=0.016;
    air.VehicleVelocityCmS={3000.0,0.0};
    air.OriginalVehicleObserved=true;
    air.AeroBodyPhysicallyPresent=true;
    const auto force=aero.Step(air);
    Check(force.Valid &&
          force.TotalDownforceN>=0.0 &&
          !force.AuthoritativeChaosForce,
          "real-input air drag does not claim Chaos forces");
}
static void TestTrafficSafetyAndEvidence()
{
    FollowingObservation input;
    input.Self.Identity=RealActor(1).Identity;
    input.Self.Pose={{-1700.0,-1800.0},0.0};
    input.Self.VelocityCmS={2000.0,0.0};
    input.OpponentCount=1;
    input.Opponents[0]=input.Self;
    input.Opponents[0].Identity=RealActor(0).Identity;
    input.Opponents[0].Pose.Position={-650.0,-1800.0};
    input.Opponents[0].VelocityCmS={300.0,0.0};
    input.DeltaSeconds=0.016;
    input.ProposedThrottle=0.85;
    input.EnginePhysicsQualified=true;
    input.RaceRunning=true;
    const auto closingThreat=EvaluateFollowingThreat(
        input.Self,input.Opponents[0],{});
    Check(closingThreat.Valid &&
          std::abs(closingThreat.RelativeForwardSpeedCmS+1700.0)<1.0e-9 &&
          std::abs(closingThreat.ClosingSpeedCmS-1700.0)<1.0e-9 &&
          closingThreat.PhysicallyClosing,
          "slower leading car has negative relative speed and positive closing speed");
    auto fasterLeader=input.Opponents[0];
    fasterLeader.VelocityCmS={2600.0,0.0};
    const auto openingThreat=EvaluateFollowingThreat(
        input.Self,fasterLeader,{});
    Check(openingThreat.Valid &&
          std::abs(openingThreat.RelativeForwardSpeedCmS-600.0)<1.0e-9 &&
          openingThreat.ClosingSpeedCmS==0.0 &&
          !openingThreat.PhysicallyClosing,
          "faster leading car has positive relative speed and no closing risk");
    const auto follow=RestrictPhysicalCarFollowing(input);
    Check(follow.Valid &&
          follow.ProposedThrottle<=input.ProposedThrottle &&
          follow.VerifiedOpponents==1,
          "one actual slower car limits rival throttle");
    input.Opponents[0].Identity.Actor=
        input.Self.Identity.Actor;
    Check(!RestrictPhysicalCarFollowing(input).Valid,
          "fabricated duplicate actor is rejected");
    PhysicalRaceMarshal marshal;
    Check(marshal.Configure({}) &&
          marshal.BeginSession(101,0),
          "session-bound race marshal");
    FlagZone zone;
    zone.Flag=RaceFlag::LocalYellow;
    zone.Source=FlagSource::OriginalRaceDirector;
    zone.Session=101;
    zone.EventOrdinal=1;
    zone.BeginTrackDistanceCm=1000.0;
    zone.EndTrackDistanceCm=3000.0;
    zone.ExpiresAtRaceClockSeconds=25.0;
    zone.OriginalCircuitConfirmed=true;
    zone.RaceDirectorAuthorized=true;
    zone.PhysicalIncidentValidated=true;
    Check(marshal.SubmitFlag(zone),
          "real-incident yellow zone accepted");
    const auto flags=marshal.At(1500.0,1);
    Check(flags.Valid && flags.NoOvertake &&
          flags.ThrottleCeiling<1.0 &&
          !flags.ActualRaceDirectorModified,
          "yellow restriction is read-only");
    TrackLimitObservation limits;
    limits.Identity=RealActor().Identity;
    limits.ChassisPose={{-1400.0,-1800.0},0.0};
    limits.PlanarVelocityCmS={1500.0,0.0};
    limits.DeltaSeconds=0.016;
    limits.ActiveRaceClockSeconds=5.0;
    limits.WheelContactVerified.fill(true);
    limits.WheelContactPositionCm.fill({-1400.0,-1800.0});
    TrackLimitsMonitor limitMonitor;
    Check(limitMonitor.Configure({}),
          "track-limit observation policy");
    Check(limitMonitor.Observe(limits).Valid &&
          limitMonitor.OffroadEpisodes()==0,
          "legitimate asphalt is not a penalty");
    PitLaneObservation pit;
    pit.Car=RealActor(1,0.0);
    pit.Car.Chassis.PlanarPose.Position={0.0,-1300.0};
    pit.Car.Track.OnOriginalPavement=true;
    pit.DeltaSeconds=0.016;
    pit.MeasuredPlanarSpeedCmS=180.0;
    pit.OriginalPitBayCollisionVerified=true;
    pit.PitLanePhysicallyPresent=true;
    const auto reviewed=EvaluatePitLaneApproach(pit);
    Check(reviewed.Valid && reviewed.InOriginalServiceBay &&
          !reviewed.TrueRacePitRulesApplied,
          "original pit bay speed and service eligibility review");
}
static void TestContactAndThermalIntegrity()
{
    CollisionObservation collision;
    collision.A.ActorToken=1;
    collision.B.ActorToken=2;
    collision.A.RealRigidBodyVerified=true;
    collision.B.RealRigidBodyVerified=true;
    collision.A.PositionCm={0.0,0.0};
    collision.B.PositionCm={400.0,0.0};
    collision.A.VelocityCmS={500.0,0.0};
    collision.B.VelocityCmS={0.0,0.0};
    collision.WorldContactPointCm={200.0,0.0};
    collision.WorldContactNormal={1.0,0.0};
    collision.DeltaSeconds=0.016;
    collision.OriginalEngineContactReported=true;
    const auto result=ResolveMeasuredCollision(collision);
    Check(result.Valid && result.EngineContactVerified &&
          !result.PhysicallyResolvedByChaos,
          "source contact impulse review is not a Chaos impulse");
    TireThermalInput thermal;
    thermal.Wheel.ContactVerified=true;
    thermal.Wheel.PhysicalContact=true;
    thermal.Wheel.NormalLoadN=3200.0;
    thermal.Forces.Valid=true;
    thermal.Compound=TireCompound::Medium;
    thermal.DeltaSeconds=0.016;
    thermal.PhysicalContactConfirmed=true;
    thermal.OriginalEngineTireSensorReady=true;
    const auto heat=IntegrateTireThermal(
        thermal,TireThermalState{});
    Check(heat.Valid && heat.WheelSensorTrusted &&
          !heat.AppliedToActualTire &&
          heat.TreadDepthRemainingMm>0.0,
          "core/surface tire temperature and tread review");
    const auto invalid=IntegrateTireThermal(
        TireThermalInput{},TireThermalState{});
    Check(!invalid.Valid,
          "missing physical tire sensors fail closed");
}
int main()
{
    TestRealRoadAndPlans();
    TestVehicleEngineering();
    TestTrafficSafetyAndEvidence();
    TestContactAndThermalIntegrity();
    std::cout<<"Extended physical racecraft tests passed.\n";
    return EXIT_SUCCESS;
}
