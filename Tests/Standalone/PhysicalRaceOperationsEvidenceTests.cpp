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
    frame.OriginalVehicleStationary=true;
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
        w.WheelRemovalClockSeconds=1.0;
        w.WheelInstallationClockSeconds=2.0;
        w.MeasuredWheelBoltTorqueNm=120.0;
    }
    frame.JackHeightCm=0.0;
    frame.MeasuredJackSupportLoadN=0.0;
    frame.PitReleaseAuthorized=true;
    const auto after=EvaluatePhysicalPitService(frame);
    Check(after.Valid &&
          after.Phase==PitCrewPhase::SafetyInspection &&
          !after.PhysicalReleaseClear &&
          after.WheelsActuallyReplaced==4 &&
          after.WheelsTorqueQualified==4 &&
          after.TasksPhysicallyComplete==after.TasksRequired &&
          !after.OfficialRaceResultModified,
          "one pit snapshot cannot prove chronological jack and wheel work");
    auto missingPhysicalRemoval=frame;
    missingPhysicalRemoval.Wheels[0].ActualWheelRemoved=false;
    Check(!EvaluatePhysicalPitService(missingPhysicalRemoval).Valid,
          "acknowledged tire change without actual removal is rejected");
    missingPhysicalRemoval=frame;
    missingPhysicalRemoval.Wheels[0].MeasuredTirePressureKpa=75.0;
    const auto lowPressure=EvaluatePhysicalPitService(missingPhysicalRemoval);
    Check(lowPressure.Valid &&
          lowPressure.WheelsActuallyReplaced<4 &&
          !lowPressure.PhysicalReleaseClear,
          "unsafe fitted tire pressure cannot certify a replaced wheel");
    missingPhysicalRemoval=frame;
    missingPhysicalRemoval.Wheels[2].OriginalChaosWheelComponent=false;
    Check(!EvaluatePhysicalPitService(missingPhysicalRemoval).Valid,
          "fabricated replacement wheel cannot permit pit release");

    // Real release requires SEPARATE authentic engine frames in the
    // correct physical service order. Invented all-true task snapshots
    // and instantaneous jack lowering never certify an installed tire.
    auto stage=frame;
    stage.Identity.Frame=100;
    stage.PhysicsClockSeconds=100.0;
    stage.JackHeightCm=0.0;
    stage.MeasuredJackSupportLoadN=0.0;
    stage.PitReleaseAuthorized=true; // premature attempt is recorded
    stage.CrewTaskAcknowledged.fill(false);
    for(auto& wheel:stage.Wheels)
    {
        wheel.ActualWheelRemoved=false;
        wheel.ActualNewWheelInstalled=false;
        wheel.WheelTorqueVerified=false;
        wheel.WheelRemovalClockSeconds=0.0;
        wheel.WheelInstallationClockSeconds=0.0;
        wheel.MeasuredWheelBoltTorqueNm=0.0;
    }
    PhysicalPitCrewSupervisor supervisor;
    Check(supervisor.Configure({}),
          "physical pit service state machine configured");
    const auto initial=supervisor.Step(stage);
    Check(initial.Valid &&
          initial.Phase==PitCrewPhase::StationaryVerified &&
          !initial.PhysicalReleaseClear &&
          supervisor.UnsafeReleaseAttempts()==1,
          "pit service begins only with stationary car and jack down");
    stage.PitReleaseAuthorized=false;
    auto next=[&](double clockSeconds)
    {
        ++stage.Identity.Frame;
        stage.PhysicsClockSeconds=clockSeconds;
        return supervisor.Step(stage);
    };
    stage.CrewTaskAcknowledged[
        static_cast<std::size_t>(PitCrewTask::Lift)]=true;
    stage.JackHeightCm=15.0;
    stage.MeasuredJackSupportLoadN=3500.0;
    Check(next(101.0).Phase==PitCrewPhase::JackUp,
          "jack must be physically supported after reservation");
    for(int i=0;i<PhysicalWheelCount;++i)
    {
        auto& wheel=stage.Wheels[static_cast<std::size_t>(i)];
        wheel.ActualWheelRemoved=true;
        wheel.WheelRemovalClockSeconds=101.5;
        stage.CrewTaskAcknowledged[static_cast<std::size_t>(
            static_cast<int>(PitCrewTask::RemoveFrontLeft)+i)]=true;
    }
    const auto removed=next(102.0);
    Check(removed.Valid &&
          removed.Phase==PitCrewPhase::RemoveWheels &&
          !removed.PhysicalReleaseClear,
          "all four physically removed wheels precede new installation");
    for(int i=0;i<PhysicalWheelCount;++i)
    {
        auto& wheel=stage.Wheels[static_cast<std::size_t>(i)];
        wheel.ActualNewWheelInstalled=true;
        wheel.WheelInstallationClockSeconds=103.0;
        stage.CrewTaskAcknowledged[static_cast<std::size_t>(
            static_cast<int>(PitCrewTask::InstallFrontLeft)+i)]=true;
    }
    const auto installed=next(104.0);
    Check(installed.Valid &&
          installed.Phase==PitCrewPhase::FitWheels &&
          installed.WheelsActuallyReplaced==4 &&
          !installed.PhysicalReleaseClear,
          "four fitted original-Chaos wheels need later torque checks");
    for(auto& wheel:stage.Wheels)
    {
        wheel.WheelTorqueVerified=true;
        wheel.MeasuredWheelBoltTorqueNm=120.0;
    }
    stage.CrewTaskAcknowledged[static_cast<std::size_t>(
        PitCrewTask::InspectWheelTorque)]=true;
    const auto torqued=next(105.0);
    Check(torqued.Valid &&
          torqued.Phase==PitCrewPhase::SafetyInspection &&
          torqued.WheelsTorqueQualified==4 &&
          !torqued.PhysicalReleaseClear,
          "wheel torque qualification must occur after fitment");
    stage.JackHeightCm=0.0;
    stage.MeasuredJackSupportLoadN=0.0;
    stage.CrewTaskAcknowledged[static_cast<std::size_t>(
        PitCrewTask::ReleaseJack)]=true;
    const auto lowered=next(106.0);
    Check(lowered.Valid &&
          lowered.Phase==PitCrewPhase::JackDown &&
          !lowered.PhysicalReleaseClear,
          "physically lowered jack must be observed separately");
    stage.PitReleaseAuthorized=true;
    const auto released=next(107.0);
    Check(released.Valid &&
          released.Phase==PitCrewPhase::ReadyForRelease &&
          released.PhysicalReleaseClear &&
          released.TotalElapsedSeconds==7.0 &&
          !released.ActualUnrealVehicleModified &&
          !released.OfficialRaceResultModified &&
          supervisor.CertifiedReleaseFrames()==1,
          "only chronological original-wheel service can certify release");
    const auto replay=supervisor.Step(stage);
    Check(!replay.Valid && supervisor.Rejections()==1,
          "the same physical pit frame cannot release twice");
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
        g.OriginalGateCenterCm={62500.0*i,0.0};
        g.GateForwardNormal={1.0,0.0};
        g.DistanceAlongLapCm=62500.0*i;
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

    // A lap is NOT complete at the fourth gate: the car must cross
    // start/finish a second time, with four qualified segment splits.
    PhysicalSectorTimingSupervisor complete;
    Check(complete.Configure(course,p),
          "complete-lap test course qualifies original gates");
    for(int crossingIndex=0;crossingIndex<=4;++crossingIndex)
    {
        const int gateIndex=crossingIndex%course.GateCount;
        const auto& authored=course.Gates[
            static_cast<std::size_t>(gateIndex)];
        auto event=crossing;
        event.Identity.Frame=
            static_cast<std::uint64_t>(17+crossingIndex);
        event.GateOrdinal=gateIndex;
        event.RaceClockSeconds=2.0+20.0*crossingIndex;
        event.PreviousRaceClockSeconds=
            event.RaceClockSeconds-0.016;
        event.DeltaSeconds=0.016;
        event.PreviousPositionCm={
            authored.OriginalGateCenterCm.X-10.0,0.0};
        event.CurrentPositionCm={
            authored.OriginalGateCenterCm.X+10.0,0.0};
        event.DistanceAlongLapCm=authored.DistanceAlongLapCm;
        event.UnwrappedDistanceCm=62500.0*crossingIndex;
        Check(complete.Submit(event)==SectorTimingFault::None,
              "only sequential authentic original-sector crossings count");
        if(crossingIndex==3)
            Check(complete.CandidateLaps()==0 &&
                  complete.NextGateOrdinal()==0,
                  "last intermediate gate cannot complete an entire lap");
    }
    const auto lap=complete.Snapshot();
    Check(complete.CandidateLaps()==1 &&
          lap.Valid &&
          lap.CompletedSectorCount==PhysicalSectorCount &&
          lap.AllPhysicalGatesVisited &&
          lap.TotalVerifiedDistanceCm==p.CircuitLengthCm &&
          lap.ProposedLapTimeSeconds==80.0 &&
          !lap.ActualEngineLapCreditIssued,
          "second original start/finish crossing qualifies full measured lap");
    for(const auto& split:lap.Sectors)
        Check(split.Valid &&
              split.DistanceCompletedCm==62500.0 &&
              split.SectorDurationSeconds==20.0,
              "each four-sector split must use real gated distance and time");

    // A lap which teleports through the missing last segment cannot
    // receive a candidate split even when it crosses the gate plane.
    PhysicalSectorTimingSupervisor incomplete;
    Check(incomplete.Configure(course,p),
          "incomplete-lap negative fixture configures");
    for(int crossingIndex=0;crossingIndex<=4;++crossingIndex)
    {
        const int gateIndex=crossingIndex%course.GateCount;
        const auto& authored=course.Gates[
            static_cast<std::size_t>(gateIndex)];
        auto event=crossing;
        event.Identity.Frame=
            static_cast<std::uint64_t>(100+crossingIndex);
        event.GateOrdinal=gateIndex;
        event.RaceClockSeconds=100.0+20.0*crossingIndex;
        event.PreviousRaceClockSeconds=
            event.RaceClockSeconds-0.016;
        event.PreviousPositionCm={
            authored.OriginalGateCenterCm.X-10.0,0.0};
        event.CurrentPositionCm={
            authored.OriginalGateCenterCm.X+10.0,0.0};
        event.DistanceAlongLapCm=authored.DistanceAlongLapCm;
        event.UnwrappedDistanceCm=62500.0*crossingIndex;
        if(crossingIndex==4)
            event.UnwrappedDistanceCm=240000.0;
        const auto fault=incomplete.Submit(event);
        if(crossingIndex<4)
            Check(fault==SectorTimingFault::None,
                  "physical intermediate gates count without lap credit");
        else
            Check(fault!=SectorTimingFault::None &&
                  incomplete.CandidateLaps()==0,
                  "missing road distance must reject false finish crossing");
    }
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
          first.OriginalRaceClockUnchanged,
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
    frame.VelocityCmS={2000.0,0.0};
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
