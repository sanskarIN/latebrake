#include "Racecraft/RaceTelemetrySensorCoherence.h"
#include "Racecraft/RivalSectorPaceLearning.h"
#include "Racecraft/PhysicalIncidentSourceReview.h"
#include "Racecraft/PhysicalVisibilitySafetyEnvelope.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace latebrake::racecraft;

static void Check(bool valid,const char* why)
{
    if(!valid)
    {
        std::cerr<<"PhysicalRaceOperationsEvidence FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
static RaceIdentity RealActor(int slot=1)
{
    RaceIdentity r;
    r.Session=4001;
    r.SessionRevision=2;
    r.Actor=5000+static_cast<std::uint64_t>(slot);
    r.Frame=17;
    r.Slot=slot;
    r.EngineObserved=true;
    r.BodySimulating=true;
    r.OriginalTrack=true;
    return r;
}
static void PitEntryEvidence()
{
    PhysicalPitApproachFrame frame;
    frame.Identity=RealActor();
    frame.CarVelocityCmS={3000.0,0.0};
    frame.DistanceUntilPitEntryCm=900.0;
    frame.DistanceUntilPitBoxCm=8000.0;
    frame.DistanceUntilPitExitCm=12000.0;
    frame.PhysicalWallClearanceCm=400.0;
    frame.ActualGripG=1.0;
    frame.ClockSeconds=5.0;
    frame.DeltaSeconds=0.016;
    frame.DriverThrottle=0.8;
    frame.ActualChaosBodyReady=true;
    frame.OriginalPitMapReady=true;
    frame.EnginePitEntryTrigger=true;
    frame.PitLaneOpenedByRaceDirector=true;
    frame.EngineRaceRunning=true;
    const auto entry=EvaluatePhysicalPitApproach(frame);
    Check(entry.Valid &&
          entry.Phase==PitApproachPhase::BrakeToLimit &&
          entry.CommandThrottle<=frame.DriverThrottle &&
          !entry.ActualUnrealCommandIssued,
          "real pit entry requires observed deceleration before speed gate");
    PhysicalPitApproachSupervisor supervisor;
    Check(supervisor.Configure({}) &&
          supervisor.Step(frame).Valid &&
          !supervisor.Step(frame).Valid,
          "pit entry cannot reuse a physical actor frame");
    frame.OriginalPitMapReady=false;
    Check(!EvaluatePhysicalPitApproach(frame).Valid,
          "unauthored pit cannot grant a pit route");
}
static void PitCrewEvidence()
{
    PitCrewServiceFrame frame;
    frame.Identity=RealActor();
    frame.PhysicsClockSeconds=5.0;
    frame.DeltaSeconds=0.016;
    frame.ActualBrakeFraction=1.0;
    frame.CurrentVehicleSpeedCmS=0.0;
    frame.CrewAreaClearanceCm=220.0;
    frame.MeasuredJackSupportLoadN=3500.0;
    frame.JackHeightCm=15.0;
    frame.PitBoxComponentToken=18;
    frame.ReservationToken=22;
    frame.OriginalPitFacilityVerified=true;
    frame.ReservationAuthorizedByRaceDirector=true;
    frame.ActualServiceCrewPresent=true;
    frame.EngineVehicleBodyQualified=true;
    frame.PitLaneCollisionEnabled=true;
    frame.RaceSessionActive=true;
    frame.EngineFuelProbeQualified=true;
    for(std::size_t i=0;i<PhysicalWheelCount;++i)
    {
        auto& w=frame.Wheels[i];
        w.WheelComponentToken=700+i;
        w.OriginalChaosWheelComponent=true;
        w.CrewSensorConfidence=1.0;
        w.MeasuredTirePressureKpa=210.0;
    }
    const auto before=EvaluatePhysicalPitService(frame);
    Check(before.Valid &&
          before.Phase==PitCrewPhase::JackUp &&
          !before.PhysicalReleaseClear &&
          !before.ActualUnrealVehicleModified,
          "unperformed pit tasks do not become completed work");
    for(auto& done:frame.CrewTaskAcknowledged)done=true;
    for(auto& w:frame.Wheels)
    {
        w.ActualWheelRemoved=true;
        w.ActualNewWheelInstalled=true;
        w.WheelTorqueVerified=true;
        w.MeasuredWheelBoltTorqueNm=120.0;
    }
    frame.JackHeightCm=0.0;
    frame.MeasuredJackSupportLoadN=0.0;
    frame.PitReleaseAuthorized=true;
    const auto after=EvaluatePhysicalPitService(frame);
    Check(after.Valid &&
          after.Phase==PitCrewPhase::ReadyForRelease &&
          after.PhysicalReleaseClear &&
          after.WheelsTorqueQualified==4 &&
          !after.OfficialRaceResultModified,
          "physical wheel service and lowered jack allow review only");
    frame.Wheels[2].OriginalChaosWheelComponent=false;
    Check(!EvaluatePhysicalPitService(frame).Valid,
          "fabricated replacement wheel cannot permit pit release");
}
static void RoadWaterEvidence()
{
    RoadWaterFrame sample;
    sample.Identity=RealActor();
    sample.ProbeCount=1;
    sample.RaceClockSeconds=5.0;
    sample.DeltaSeconds=0.016;
    sample.ActualVehicleSpeedCmS=7000.0;
    sample.ActualWheelTreadFraction=0.4;
    sample.EngineRaceRunning=true;
    sample.OriginalCircuitVerified=true;
    sample.RealEnvironmentSensorReady=true;
    auto& probe=sample.Probes[0];
    probe.Identity=sample.Identity;
    probe.CellIndex=0;
    probe.OriginalRoadComponent=10;
    probe.PhysicalDrainComponent=11;
    probe.PhysicsClockSeconds=5.0;
    probe.RainSensorClockSeconds=5.0;
    probe.WaterSensorClockSeconds=5.0;
    probe.MeasuredRainMmHour=40.0;
    probe.MeasuredWaterDepthMm=14.0;
    probe.MeasuredRoadFriction=0.7;
    probe.MeasuredDryFriction=1.2;
    const auto wet=EvaluateMeasuredRoadHydrology(sample);
    Check(wet.Valid &&
          wet.VerifiedProbes==1 &&
          wet.WetCells==1 &&
          wet.MaximumPhysicalWaterMm==14.0 &&
          wet.NoUnrealWeatherModified,
          "measured water film produces conservative physical hazard");
    RoadHydrologySupervisor tracker;
    Check(tracker.Configure({}) &&
          tracker.Step(sample).Valid &&
          !tracker.Step(sample).Valid,
          "duplicate water sensor frame is not new physical evidence");
    probe.RealWaterProbeReady=false;
    Check(!EvaluateMeasuredRoadHydrology(sample).Valid,
          "unverified wet road sensor cannot synthesize water film");
}
static void SectorTimingEvidence()
{
    SectorTimingParameters p;
    SectorTimingCourse course;
    course.GateCount=4;
    course.CircuitLengthCm=p.CircuitLengthCm;
    course.OriginalRaceMapVerified=true;
    course.RealGateComponentsConfirmed=true;
    for(int i=0;i<course.GateCount;++i)
    {
        auto& g=course.Gates[static_cast<std::size_t>(i)];
        g.OriginalGateComponent=400+i;
        g.GateOrdinal=i;
        g.SectorIndex=i;
        g.OriginalGateCenterCm={5000.0*i,0.0};
        g.GateForwardNormal={1.0,0.0};
        g.DistanceAlongLapCm=5000.0*i;
        g.EngineClockSeconds=2.0;
    }
    Check(ValidSectorTimingCourse(course,p),
          "four original physical gates form an ordered timing course");
    PhysicalSectorTimingSupervisor supervisor;
    Check(supervisor.Configure(course,p),
          "verified course accepted for read-only clock review");
    SectorTimingCrossing crossing;
    crossing.Identity=RealActor();
    crossing.GateOrdinal=0;
    crossing.PreviousPositionCm={-10.0,0.0};
    crossing.CurrentPositionCm={10.0,0.0};
    crossing.MeasuredVelocityCmS={1000.0,0.0};
    crossing.PreviousRaceClockSeconds=1.984;
    crossing.RaceClockSeconds=2.0;
    crossing.DeltaSeconds=0.016;
    crossing.EngineRaceGreen=true;
    crossing.ChaosBodyQualified=true;
    crossing.OriginalMapConfirmed=true;
    crossing.CollisionTriggerQualified=true;
    Check(supervisor.Submit(crossing)==SectorTimingFault::None &&
          supervisor.NextGateOrdinal()==1 &&
          supervisor.CandidateLaps()==0,
          "real forward gate crossing advances review, not official score");
    crossing.Identity.Frame++;
    crossing.CurrentPositionCm={-20.0,0.0};
    Check(supervisor.Submit(crossing)!=SectorTimingFault::None,
          "wrong next gate cannot be used to claim a lap");
}
static RaceTelemetryPhysicalFrame Telemetry()
{
    RaceTelemetryPhysicalFrame frame;
    frame.Identity=RealActor();
    frame.MeasuredVelocityCmS={2400.0,0.0};
    frame.MeasuredAccelerationCmS2={20.0,0.0};
    frame.PhysicsClockSeconds=5.0;
    frame.DeltaSeconds=0.016;
    frame.ReportedSpeedCmS=2400.0;
    frame.MeasuredRoadGrip=1.0;
    frame.MeasuredRoadHalfWidthCm=800.0;
    frame.MeasuredChassisMassKg=1320.0;
    frame.EngineRaceGreen=true;
    frame.OriginalTrackVerified=true;
    frame.OriginalMapCollisionQualified=true;
    frame.ChaosBodySimulating=true;
    frame.EngineInputAuthorityObserved=true;
    for(auto& wheel:frame.Wheels)
    {
        wheel.EngineWheelComponentVerified=true;
        wheel.Wheel.PhysicalContact=true;
        wheel.Wheel.ContactVerified=true;
        wheel.Wheel.NormalLoadN=3300.0;
        wheel.MeasuredHubSpeedCmS=2400.0;
        wheel.SourceClockSeconds=5.0;
    }
    return frame;
}
static void TelemetryEvidence()
{
    const auto frame=Telemetry();
    const auto report=AuditPhysicalTelemetry(frame);
    Check(report.Valid &&
          report.Quality==SensorFrameQuality::Qualified &&
          report.VerifiedWheels==4 &&
          !report.GameplayAuthorityIssued,
          "real four-wheel telemetry is read only");
    RaceTelemetrySourceWindow window;
    Check(window.Configure({}) &&
          window.Record(frame).Valid &&
          !window.Record(frame).Valid &&
          window.FramesRejected()==1,
          "sensor ring rejects replayed chassis source frame");
    const auto summary=window.Summarize();
    Check(summary.Valid && summary.FramesInWindow==1 &&
          summary.SourceOnly,
          "telemetry report references measured physics only");
}
static void RivalPaceEvidence()
{
    RivalPaceSensorFrame s;
    s.Identity=RealActor();
    s.RaceClockSeconds=4.0;
    s.DeltaSeconds=0.016;
    s.OriginalTrackDistanceCm=150.0;
    s.MeasuredVehicleSpeedCmS=2600.0;
    s.MeasuredRoadGripG=1.0;
    s.MeasuredRoadHalfWidthCm=800.0;
    s.CurrentThrottle=0.5;
    s.OriginalMapConfirmed=true;
    s.EngineRaceGreen=true;
    s.ChaosWheelObservationsQualified=true;
    s.ActualRivalActor=true;
    s.TrafficRosterPhysical=true;
    s.OriginalRouteMeasured=true;
    RivalSectorPaceSupervisor controller;
    Check(controller.Configure({}),
          "sensor-based rival pace parameters validated");
    const auto first=controller.Step(s);
    Check(first.Valid && first.SourceOnly &&
          first.CommandThrottle<=s.CurrentThrottle &&
          !first.OriginalRaceClockUnchanged==false,
          "unproven history cannot invent acceleration");
    Check(!controller.Step(s).Valid &&
          controller.Rejections()==1,
          "replayed speed sample cannot train sector pace");
}
static void IncidentEvidence()
{
    PhysicalIncidentFrame sample;
    sample.Identity=RealActor();
    sample.PreImpactVelocityCmS={3200.0,0.0,0.0};
    sample.PostImpactVelocityCmS={2400.0,0.0,0.0};
    sample.MeasuredVehicleMassKg=1320.0;
    sample.MeasuredWheelContactCount=4.0;
    sample.PhysicsClockSeconds=5.0;
    sample.DeltaSeconds=0.016;
    sample.RaceRunning=true;
    sample.OriginalCircuitVerified=true;
    sample.ChaosVehicleBodySimulating=true;
    sample.ChassisDamageSensorQualified=true;
    sample.WheelContactSensorsQualified=true;
    sample.ContactCount=1;
    auto& contact=sample.Contacts[0];
    contact.Identity=sample.Identity;
    contact.ContactEventToken=1;
    contact.OriginalCollisionComponent=123;
    contact.ContactNormal={1.0,0.0,0.0};
    contact.ImpulseNSeconds={8000.0,0.0,0.0};
    contact.RelativeContactVelocityCmS={-700.0,0.0,0.0};
    contact.PhysicsClockSeconds=5.0;
    contact.Class=IncidentSourceClass::WallImpact;
    contact.OriginalCollisionBodyObserved=true;
    contact.ActualChaosContactReported=true;
    contact.OriginalCircuitProven=true;
    const auto incident=ReviewPhysicalIncident(sample);
    Check(incident.Valid && incident.PhysicalIncidentDetected &&
          incident.OriginalBarrierContacts==1 &&
          !incident.ImmediateStopRecommended &&
          incident.OfficialPenaltyUntouched,
          "real barrier impulse supports diagnostics, not race penalties");
    sample.Contacts[0].ActualChaosContactReported=false;
    Check(!ReviewPhysicalIncident(sample).Valid,
          "fictional collision events cannot create physical damage evidence");
}
static void VisibilityEvidence()
{
    PhysicalVisibilityFrame frame;
    frame.Identity=RealActor();
    frame.RayCount=4;
    frame.VelocityCmS={3400.0,0.0};
    frame.CurrentRoadGripG=1.0;
    frame.MeasuredRoadHalfWidthCm=800.0;
    frame.PhysicsClockSeconds=5.0;
    frame.DeltaSeconds=0.016;
    frame.CurrentThrottle=0.7;
    frame.OriginalMapCollisionReady=true;
    frame.RealVisibilitySensorsReady=true;
    frame.ChaosVehicleBodyQualified=true;
    frame.RaceGreen=true;
    for(int i=0;i<frame.RayCount;++i)
    {
        auto& ray=frame.Rays[static_cast<std::size_t>(i)];
        ray.Identity=frame.Identity;
        ray.RayDirectionWorld={1.0,0.0,0.0};
        ray.MaximumTraceDistanceCm=7000.0;
        ray.HitDistanceCm=7000.0;
        ray.SourceClockSeconds=5.0;
        ray.RealTracePerformed=true;
        ray.ChaosSceneQuerySucceeded=true;
        ray.LineOfSightQualified=true;
    }
    const auto clear=EvaluatePhysicalVisibility(frame);
    Check(clear.Valid && clear.VerifiedPhysicalRays==4 &&
          clear.SourceOnly && !clear.EmergencyStopRequired,
          "real collision rays permit a measured visibility estimate");
    frame.Rays[0].HitRecorded=true;
    frame.Rays[0].Kind=VisibilityObstacleKind::OriginalBarrier;
    frame.Rays[0].OriginalCollisionComponent=60;
    frame.Rays[0].HitDistanceCm=350.0;
    const auto barrier=EvaluatePhysicalVisibility(frame);
    Check(barrier.Valid && barrier.BarriersSeen==1 &&
          barrier.EmergencyStopRequired &&
          barrier.ProposedThrottle==0.0,
          "short actual barrier sight forces braking");
    frame.Rays[2].LineOfSightQualified=false;
    Check(!EvaluatePhysicalVisibility(frame).Valid,
          "failed Chaos scene query invalidates forward visibility");
}
int main()
{
    PitEntryEvidence();
    PitCrewEvidence();
    RoadWaterEvidence();
    SectorTimingEvidence();
    TelemetryEvidence();
    RivalPaceEvidence();
    IncidentEvidence();
    VisibilityEvidence();
    std::cout<<"PhysicalRaceOperationsEvidence OK\n";
}
