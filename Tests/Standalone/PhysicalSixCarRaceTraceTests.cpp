#include "Racecraft/PhysicalRaceReplayEvents.h"
#include "Racecraft/PhysicalSixCarCameraRig.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace latebrake::racecraft;
static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "PhysicalSixCarRaceTrace FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
static RivalLaneActor RealActor(int slot, double x, double y=0.0)
{
    RivalLaneActor a;
    a.Identity.Session=700;
    a.Identity.SessionRevision=1;
    a.Identity.Actor=1000+static_cast<std::uint64_t>(slot);
    a.Identity.Frame=1;
    a.Identity.Slot=slot;
    a.Identity.EngineObserved=true;
    a.Identity.BodySimulating=true;
    a.Identity.OriginalTrack=true;
    a.Pose.Position={x,y};
    a.VelocityCmS={slot==2?2800.0:3200.0,0.0};
    a.PhysicsClockSeconds=10.0;
    a.RoadHalfWidthCm=800.0;
    a.SensorConfidence=1.0;
    a.OriginalChassisVerified=true;
    a.OriginalPavementVerified=true;
    a.LiveChaosVelocityQualified=true;
    return a;
}
static RivalLaneTrafficFrame CompleteSixCars()
{
    RivalLaneTrafficFrame f;
    f.Self=RealActor(1,0.0);
    int i=0;
    for(int slot=0;slot<PhysicalFleetCapacity;++slot)
    {
        if(slot==1)continue;
        f.Others[static_cast<std::size_t>(i++)]=
            RealActor(slot,-10000.0-slot*1800.0);
    }
    f.Others[1]=RealActor(2,2000.0);
    f.OtherCount=RivalTrafficCount;
    f.DeltaSeconds=0.016;
    f.CurrentThrottle=0.7;
    f.OriginalCircuitConfirmed=true;
    f.SixCarRosterAuthenticated=true;
    f.RaceGreen=true;
    f.OriginalTrackCollisionReady=true;
    return f;
}
static void RealNextFrame(RivalLaneTrafficFrame& f)
{
    f.Self.Pose.Position=f.Self.Pose.Position+
        f.Self.VelocityCmS*f.DeltaSeconds;
    ++f.Self.Identity.Frame;
    f.Self.PhysicsClockSeconds+=f.DeltaSeconds;
    for(auto& rival:f.Others)
    {
        rival.Pose.Position=rival.Pose.Position+
            rival.VelocityCmS*f.DeltaSeconds;
        ++rival.Identity.Frame;
        rival.PhysicsClockSeconds+=f.DeltaSeconds;
    }
}
static void DirectPhysicalEvidence()
{
    auto frame=CompleteSixCars();
    Check(ValidPhysicalTraceSettings({}),
          "default trace settings are physically bounded");
    auto physical=ReviewPhysicalTraceFrame(frame);
    Check(physical.Valid &&
          physical.ActorRoster[1]==1001 &&
          physical.ActorRoster[2]==1002 &&
          physical.Pairs.size()==PhysicalTracePairCount &&
          physical.NearestVehicleSeparationCm>0.0 &&
          !physical.ActualEngineActuationOccurred &&
          !physical.OfficialRaceResultModified,
          "six actual chassis identities form one unmodified review frame");
    auto duplicate=frame;
    duplicate.Others[2].Identity.Actor=
        duplicate.Others[1].Identity.Actor;
    Check(!ReviewPhysicalTraceFrame(duplicate).Valid,
          "duplicated physical rival may not enter replay telemetry");
    duplicate=frame;
    --duplicate.Others[3].Identity.Frame;
    Check(!ReviewPhysicalTraceFrame(duplicate).Valid,
          "out-of-epoch rival sample never certifies a six-car frame");
    duplicate=frame;
    duplicate.Others[4].SensorConfidence=0.3;
    Check(!ReviewPhysicalTraceFrame(duplicate).Valid,
          "low-confidence chassis source cannot be stored as physical");
    auto a=CapturePhysicalTraceActor(frame.Self);
    auto b=CapturePhysicalTraceActor(frame.Others[1]);
    b.Pose.Position={500.0,0.0};
    const auto closing=ReviewPhysicalTracePair(a,b,{});
    Check(closing.Valid && closing.FollowingGapHazard &&
          closing.LongitudinalClearanceCm==60.0 &&
          closing.ClosingSpeedCmS==400.0 &&
          closing.TimeToContactSeconds>0.0,
          "real relative chassis motion drives following risk, not scripts");
    b.Pose.Position={200.0,0.0};
    Check(ReviewPhysicalTracePair(a,b,{}).MeasuredOverlap,
          "actual chassis overlap is preserved as a diagnostic hazard");
}
static void ContinuousReplay()
{
    auto f=CompleteSixCars();
    PhysicalSixCarRaceTrace trace;
    Check(trace.Configure({}) &&
          trace.Append(f)==PhysicalTraceFault::None &&
          trace.Size()==1 && trace.AcceptedFrames()==1,
          "a qualified original physical six-car frame starts bounded trace");
    Check(!trace.SampleReadOnly(10.0).Valid &&
          !trace.ReviewInterval().Valid,
          "one physical frame never creates imaginary interpolation");
    const auto before=f;
    RealNextFrame(f);
    Check(trace.Append(f)==PhysicalTraceFault::None &&
          trace.Size()==2,
          "real consecutive engine frames permit read-only replay");
    const auto mid=trace.SampleReadOnly(10.008);
    Check(mid.Valid && mid.Interpolated &&
          mid.UnmodifiedEnginePhysics &&
          mid.UnmodifiedOfficialRaceResults &&
          mid.EarlierFrame==1 && mid.LaterFrame==2 &&
          std::abs(mid.Actors[1].Pose.Position.X-25.6)<0.0001 &&
          mid.ActorRoster[2]==1002,
          "interpolated replay is temporal presentation only");
    const auto interval=trace.ReviewInterval();
    Check(interval.Valid && interval.RosterContinuous &&
          interval.EngineFramesContinuous &&
          std::abs(interval.DurationSeconds-0.016)<0.00001 &&
          interval.ContactFrames==0,
          "reviewed interval is attached to two real physics frames");
    Check(!trace.SampleReadOnly(11.0).Valid &&
          trace.FindFrame(1)!=nullptr &&
          trace.FindFrame(999)==nullptr,
          "no out-of-range fabricated playback or phantom source frame");
    auto repeated=f;
    Check(trace.Append(repeated)==PhysicalTraceFault::ReplayFrame &&
          trace.Size()==2 && trace.RejectedFrames()==1,
          "replayed original engine frame cannot inflate trace length");
    auto jumped=f;
    RealNextFrame(jumped);
    RealNextFrame(jumped);
    Check(trace.Append(jumped)==PhysicalTraceFault::MissingFrame &&
          trace.Size()==2,
          "dropped physical frame cannot be silently interpolated");
    auto teleport=f;
    RealNextFrame(teleport);
    teleport.Others[0].Pose.Position.X+=5000.0;
    Check(trace.Append(teleport)==PhysicalTraceFault::DiscontinuousMovement &&
          trace.Size()==2,
          "unobserved 50m rival teleport invalidates replay continuity");
    auto incorrectClock=f;
    RealNextFrame(incorrectClock);
    for(auto& car:incorrectClock.Others)
        car.PhysicsClockSeconds+=1.0;
    incorrectClock.Self.PhysicsClockSeconds+=1.0;
    Check(trace.Append(incorrectClock)==PhysicalTraceFault::ClockMismatch,
          "physics clock gap cannot masquerade as one 16ms game frame");
    const auto precise=InterpolatePhysicalTrace(
        ReviewPhysicalTraceFrame(before),
        ReviewPhysicalTraceFrame(f),10.016);
    Check(precise.Valid && !precise.Interpolated &&
          precise.SourceAlpha==1.0,
          "exact later sample returns measured position without extrapolation");
}
static void RosterAndSessionIsolation()
{
    auto f=CompleteSixCars();
    PhysicalSixCarRaceTrace trace;
    Check(trace.Configure({}) &&
          trace.Append(f)==PhysicalTraceFault::None,
          "original roster begins a live trace");
    RealNextFrame(f);
    f.Others[2].Identity.Actor+=9000;
    Check(trace.Append(f)==PhysicalTraceFault::LostRoster &&
          trace.Size()==0 && trace.RejectedFrames()==1 &&
          trace.ResetCount()==1,
          "roster swap clears all old evidence and fails this frame closed");
    Check(trace.Append(f)==PhysicalTraceFault::None &&
          trace.Size()==1,
          "re-qualified roster can initiate a new independent segment");
    RealNextFrame(f);
    f.Self.Identity.SessionRevision=2;
    for(auto& other:f.Others)
        other.Identity.SessionRevision=2;
    Check(trace.Append(f)==PhysicalTraceFault::None &&
          trace.Size()==1 && trace.ResetCount()==2,
          "new race epoch resets old temporal evidence without false physics");
}
static void BoundedWrapAndFaults()
{
    auto f=CompleteSixCars();
    PhysicalSixCarRaceTrace trace;
    Check(trace.Configure({}) &&
          trace.Append(f)==PhysicalTraceFault::None,
          "bounded ring replay starts from real six-car evidence");
    for(int k=0;k<140;++k)
    {
        RealNextFrame(f);
        Check(trace.Append(f)==PhysicalTraceFault::None,
              "all 140 real physics frames remain valid in a bounded ring");
    }
    Check(trace.Size()==PhysicalTraceCapacity &&
          trace.AcceptedFrames()==141 &&
          trace.FindFrame(1)==nullptr &&
          trace.FindFrame(141)!=nullptr &&
          trace.OldestFirst(0)->Identity.Frame==14 &&
          trace.OldestFirst(127)->Identity.Frame==141 &&
          trace.OldestFirst(128)==nullptr &&
          trace.ReviewInterval().Valid,
          "capacity wraps without duplication, allocation or missing events");
    const auto mutating=trace.SampleReadOnly(f.Self.PhysicsClockSeconds-0.008);
    Check(mutating.Valid && mutating.Interpolated &&
          !trace.Latest().ActualEngineActuationOccurred,
          "read-only ring playback cannot issue Unreal physics commands");
    PhysicalTraceSettings settings;
    settings.MinimumSensorConfidence=1.1;
    Check(!trace.Configure(settings),
          "nonphysical sensor confidence settings fail configuration");
}
static PhysicalReplayArchiveBytes ArchiveFixture()
{
    auto frame=CompleteSixCars();
    PhysicalSixCarRaceTrace trace;
    Check(trace.Configure({}) &&
          trace.Append(frame)==PhysicalTraceFault::None,
          "physical replay export must start with real six-car trace");
    for(int k=0;k<8;++k)
    {
        RealNextFrame(frame);
        Check(trace.Append(frame)==PhysicalTraceFault::None,
              "all pre-export engine frames must be physically qualified");
    }
    return ExportPhysicalReplayArchive(trace);
}
static void RepairTestChecksum(std::vector<std::uint8_t>& bytes)
{
    Check(bytes.size()>=4,"test archive needs trailing CRC32");
    const auto body=std::span<const std::uint8_t>(
        bytes.data(),bytes.size()-4);
    const std::uint32_t checksum=PhysicalReplayChecksum(body);
    for(int i=0;i<4;++i)
        bytes[bytes.size()-4+static_cast<std::size_t>(i)]=
            static_cast<std::uint8_t>(checksum>>(8*i));
}
static void ArchiveRoundTrip()
{
    auto encoded=ArchiveFixture();
    Check(encoded.Valid &&
          encoded.Fault==PhysicalReplayArchiveFault::None &&
          encoded.Version==PhysicalReplayVersion &&
          encoded.RecordedFrames==9 &&
          encoded.Bytes.size()<PhysicalReplayMaxBytes &&
          !encoded.Bytes.empty(),
          "nine exact six-car physics frames fit a small versioned archive");
    PhysicalReplayArchiveReader reader;
    Check(reader.Import(encoded.Bytes)==PhysicalReplayArchiveFault::None &&
          reader.Size()==9 && reader.SourceSession()==700 &&
          reader.SourceRevision()==1 &&
          reader.SourceSlot()==1 &&
          reader.Checksum()==encoded.Checksum,
          "read-only archive reconstructs source provenance without authority");
    const auto view=reader.Preview(10.024);
    Check(view.Valid && view.ImportIsUntrusted &&
          !view.MayDriveChaosBody &&
          !view.MayCreditRaceResults &&
          view.EarlierFrame==2 && view.LaterFrame==3 &&
          std::abs(view.Actors[1].PositionCm.X-76.8)<0.10 &&
          view.Actors[2].Actor==1002,
          "camera-only interpolation remains bounded and carries no actuation");
    Check(!reader.Preview(9.0).Valid &&
          !reader.Preview(12.0).Valid &&
          reader.Get(1000)==nullptr &&
          reader.Get(8)!=nullptr,
          "outside archived time span no fabricated replay frame is returned");
    const auto reexport=ArchiveFixture();
    Check(encoded.Bytes==reexport.Bytes,
          "same original physics samples encode deterministic identical bytes");
}
static void RejectMalformedArchives()
{
    const auto source=ArchiveFixture();
    Check(source.Valid,"invalid archive fixture must not mask decode failure");
    auto expectFault=[&](std::vector<std::uint8_t> mutated,
                         PhysicalReplayArchiveFault expected,
                         const char* message)
    {
        PhysicalReplayArchiveReader reader;
        const auto observed=reader.Import(mutated);
        if(observed!=expected)
            std::cerr << "Archive fault expected=" << static_cast<int>(expected)
                      << " got=" << static_cast<int>(observed) << '\n';
        Check(observed==expected &&
              reader.Size()==0 &&
              !reader.Preview(10.0).Valid,
              message);
    };
    auto bad=source.Bytes;
    bad[20]^=0x80;
    expectFault(bad,PhysicalReplayArchiveFault::ChecksumMismatch,
          "single-bit corrupted archive fails checksum before any replay");
    bad=source.Bytes;
    bad[0]=0;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::InvalidMagic,
          "wrong magic cannot impersonate LateBrake replay format");
    bad=source.Bytes;
    bad[4]=2;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::InvalidVersion,
          "unknown future replay schema is rejected, never guessed");
    bad=source.Bytes;
    bad[6]=5;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::UnsupportedHeader,
          "an archive claiming fewer than six source cars is invalid");
    bad=source.Bytes;
    bad[8]=0xff;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::UnsupportedHeader,
          "unbounded archive frame count cannot allocate a phantom fleet");
    bad=source.Bytes;
    // Locate the actual first-frame actor token, never an unrelated byte.
    // For slot 0 the identity is 1000, encoded in 8 little-endian bytes.
    constexpr std::array<std::uint8_t,8> recordedFirstActor{
        0xe8U,0x03U,0U,0U,0U,0U,0U,0U};
    constexpr std::size_t archiveHeaderBytes=73;
    const std::size_t recordBytes=(source.Bytes.size()-4-
        archiveHeaderBytes)/source.RecordedFrames;
    std::size_t actorOffset=bad.size();
    for(std::size_t i=archiveHeaderBytes;
        i+recordedFirstActor.size()<=
            archiveHeaderBytes+recordBytes;++i)
    {
        if(std::equal(recordedFirstActor.begin(),
                      recordedFirstActor.end(),bad.begin()+
                          static_cast<std::ptrdiff_t>(i)))
        {
            actorOffset=i;
            break;
        }
    }
    Check(actorOffset<archiveHeaderBytes+recordBytes,
          "first recorded physical actor token exists in wire payload");
    bad[actorOffset]^=0x40;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::InvalidIdentity,
          "serialized chassis identity must match fixed six-slot roster");
    bad=source.Bytes;
    // A record is exactly 28 + 6*(8 + 10*4) = 316 bytes.
    // The second recorded source frame must be the successor of frame 1.
    bad[archiveHeaderBytes+recordBytes]=1;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::TemporalDiscontinuity,
          "non-monotonic imported source frame cannot be interpolated");
    bad=source.Bytes;
    bad.pop_back();
    {
        PhysicalReplayArchiveReader reader;
        Check(reader.Import(bad)!=PhysicalReplayArchiveFault::None &&
              reader.Size()==0,
              "truncated archive cannot publish partially decoded physical data");
    }
    bad=source.Bytes;
    bad.insert(bad.end()-4,0x7a);
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::TrailingBytes,
          "extra authenticated payload cannot silently alter replay schema");
    PhysicalReplayArchiveReader reader;
    Check(reader.Import(source.Bytes)==PhysicalReplayArchiveFault::None,
          "valid import before rejection has actual source frames");
    bad=source.Bytes;
    bad[20]^=0x02;
    Check(reader.Import(bad)==PhysicalReplayArchiveFault::ChecksumMismatch &&
          reader.Size()==0 &&
          !reader.Preview(10.024).Valid,
          "bad later import clears all previously decoded replay authority");
}
static PhysicalCameraFrame CameraFrame(const RivalLaneTrafficFrame& traffic)
{
    PhysicalCameraFrame frame;
    frame.Fleet=ReviewPhysicalTraceFrame(traffic);
    frame.OriginalCircuitCollisionReady=true;
    frame.SpectatorCameraPermitted=true;
    for(int slot=0;slot<PhysicalFleetCapacity;++slot)
    {
        const auto index=static_cast<std::size_t>(slot);
        const auto& actor=frame.Fleet.Actors[index];
        auto& sensor=frame.Chassis[index];
        sensor.Identity=actor.Identity;
        sensor.ChaosBodyComponentToken=4000+
            static_cast<std::uint64_t>(slot);
        sensor.PositionCm={actor.Pose.Position.X,
            actor.Pose.Position.Y,150.0};
        sensor.VelocityCmS={actor.VelocityCmS.X,
            actor.VelocityCmS.Y,0.0};
        sensor.YawRadians=actor.Pose.HeadingRadians;
        sensor.RoadHeightCm=0.0;
        sensor.SensorClockSeconds=frame.Fleet.PhysicsClockSeconds;
        sensor.ActualEngineTransformObserved=true;
        sensor.OriginalCollisionBody=true;
        sensor.OriginalTrackSurfaceObserved=true;
    }
    return frame;
}
static void PhysicalCameraShotReview()
{
    auto traffic=CompleteSixCars();
    const auto frame=CameraFrame(traffic);
    Check(ValidCameraFrame(frame),
          "all six camera chassis match exact real traffic frame");
    for(int kind=0;kind<5;++kind)
    {
        const auto shot=static_cast<PhysicalCameraShot>(kind);
        const auto camera=PlanPhysicalCamera(frame,1,shot);
        Check(camera.Valid &&
              camera.Fault==PhysicalCameraFault::None &&
              camera.FocusSlot==1 &&
              camera.VerifiedRivalCount==5 &&
              camera.EyeGroundClearanceCm>=55.0 &&
              camera.FieldOfViewDegrees>=43.0 &&
              camera.FieldOfViewDegrees<=91.0 &&
              !camera.UnrealCameraCommandIssued &&
              !camera.VehicleControlsModified &&
              !camera.OfficialRaceClockModified,
              "camera composition is physically bounded and read-only");
    }
    const auto chase=PlanPhysicalCamera(
        frame,1,PhysicalCameraShot::ChaseNear);
    Check(chase.Valid &&
          std::abs(chase.EyeCm.X+620.0)<0.001 &&
          std::abs(chase.EyeCm.Z-370.0)<0.001,
          "near chase camera uses actual chassis pose and authored offset");
    Check(!PlanPhysicalCamera(frame,6,
          PhysicalCameraShot::ChaseNear).Valid,
          "out-of-roster camera slot cannot spawn phantom car");
    Check(!PlanPhysicalCamera(frame,1,
          PhysicalCameraShot::Trackside).Valid,
          "an invented broadcast marker cannot impersonate original world");
    auto marker=frame;
    marker.Trackside.PositionCm={2000.0,3000.0,800.0};
    marker.Trackside.LookAtCm={0.0,0.0,0.0};
    marker.Trackside.OriginalCameraMarkerToken=191;
    marker.Trackside.OriginalTrackCameraMarker=true;
    const auto trackside=PlanPhysicalCamera(
        marker,1,PhysicalCameraShot::Trackside);
    Check(trackside.Valid &&
          trackside.EyeCm.X==2000.0 &&
          trackside.EyeCm.Y==3000.0,
          "original trackside camera comes only from authenticated marker");
    auto invalid=frame;
    invalid.Chassis[2].ChaosBodyComponentToken=
        invalid.Chassis[1].ChaosBodyComponentToken;
    Check(!ValidCameraFrame(invalid,{}),
          "duplicate original Chaos camera chassis tokens are rejected");
    invalid=frame;
    invalid.Chassis[3].SensorClockSeconds+=0.5;
    Check(!ValidCameraFrame(invalid,{}),
          "misaligned real chassis timestamp cannot update spectator view");
    invalid=frame;
    invalid.Chassis[4].PositionCm.Y+=30.0;
    Check(!ValidCameraFrame(invalid,{}),
          "camera sensor cannot spoof a different world body location");
}
static void OriginalCameraCollisionAndState()
{
    auto traffic=CompleteSixCars();
    auto frame=CameraFrame(traffic);
    frame.ObstacleCount=1;
    auto& wall=frame.Obstacles[0];
    wall.BoundsMinimumCm={-470.0,-180.0,0.0};
    wall.BoundsMaximumCm={-320.0,180.0,800.0};
    wall.OriginalCollisionComponentToken=5001;
    wall.ActualEngineObstacleObserved=true;
    wall.OriginalTrackGeometry=true;
    const auto clipped=PlanPhysicalCamera(
        frame,1,PhysicalCameraShot::ChaseNear);
    Check(clipped.Valid &&
          clipped.ObstructionDistanceCm<620.0 &&
          clipped.EyeCm.X>-620.0 &&
          clipped.EyeCm.X<0.0 &&
          clipped.MeasuredObstacleCount==1,
          "original world obstacle shortens unsafe chase-camera ray");
    wall.BoundsMinimumCm={-100.0,-180.0,0.0};
    wall.BoundsMaximumCm={-30.0,180.0,800.0};
    const auto blocked=PlanPhysicalCamera(
        frame,1,PhysicalCameraShot::ChaseNear);
    Check(!blocked.Valid &&
          blocked.Fault==PhysicalCameraFault::CameraOccluded,
          "camera cannot hide inside near-chassis physical wall");
    wall.OriginalTrackGeometry=false;
    Check(!ValidCameraFrame(frame,{}),
          "unknown authored obstacle cannot certify collision avoidance");
    frame=CameraFrame(traffic);
    PhysicalSixCarCameraRig controller;
    Check(controller.Configure({}),
          "physical six-car camera supervisor can configure");
    const auto first=controller.Step(
        frame,1,PhysicalCameraShot::ChaseNear);
    Check(first.Valid && controller.Accepted()==1,
          "real six-car fleet frame starts camera-supervisor state");
    Check(!controller.Step(frame,1,
            PhysicalCameraShot::ChaseNear).Valid &&
          controller.Rejected()==1,
          "same observed frame cannot advance moving camera twice");
    RealNextFrame(traffic);
    frame=CameraFrame(traffic);
    const auto next=controller.Step(
        frame,1,PhysicalCameraShot::ChaseFar);
    Check(next.Valid && controller.Changes()==1 &&
          controller.Accepted()==2 &&
          !next.UnrealCameraCommandIssued,
          "camera shot change does not change source collision or race state");
    RealNextFrame(traffic);
    frame=CameraFrame(traffic);
    const auto smooth=controller.Step(
        frame,1,PhysicalCameraShot::ChaseFar);
    Check(smooth.Valid &&
          smooth.EyeCm.X>next.EyeCm.X &&
          smooth.EyeCm.X<frame.Chassis[1].PositionCm.X,
          "continuous actual engine frames smooth bounded spectator camera");
    RealNextFrame(traffic);
    traffic.Others[2].Identity.Actor+=9000;
    frame=CameraFrame(traffic);
    Check(!controller.Step(frame,1,
              PhysicalCameraShot::ChaseFar).Valid &&
          controller.Rejected()==2,
          "changed original six-car roster revokes smooth camera session");
}
static void PhysicalReplayEventTimeline()
{
    auto traffic=CompleteSixCars();
    traffic.Others[1].Pose.Position={450.0,0.0};
    PhysicalReplayEventRecorder reporter;
    Check(reporter.Configure({}),
          "read-only physical race incident observer configures");
    auto observed=ReviewPhysicalTraceFrame(traffic);
    Check(observed.Valid &&
          reporter.Append(observed)==PhysicalReplayEventFault::None &&
          reporter.Accepted()==1 &&
          !reporter.HasActivePhysicalContact(1,2),
          "close physical gap is not falsely reported as current collision");
    const auto initial=reporter.Snapshot();
    Check(initial.FollowGapEvents>=1 &&
          initial.CollisionRiskEvents>=1 &&
          initial.ContactsBegan==0 &&
          initial.OfficialRaceControlUntouched,
          "measured closing gap emits only spectator diagnostic events");
    Check(reporter.Append(observed)==PhysicalReplayEventFault::StaleFrame &&
          reporter.Rejected()==1,
          "replayed physical frame cannot duplicate collision notifications");
    for(int k=0;k<145;++k)
    {
        RealNextFrame(traffic);
        observed=ReviewPhysicalTraceFrame(traffic);
        Check(observed.Valid &&
              reporter.Append(observed)==PhysicalReplayEventFault::None,
              "continuous measured six-car traffic preserves event sequence");
    }
    const auto after=reporter.Snapshot();
    Check(after.ContactsBegan==1 &&
          after.ContactsEnded==1 &&
          after.FollowGapEvents>=1 &&
          after.AcceptedFrames==146 &&
          !reporter.HasActivePhysicalContact(1,2),
          "real overtaking creates one contact-start and one physical separation");
    int observedContacts=0;
    for(std::size_t i=0;i<reporter.Size();++i)
    {
        const auto* event=reporter.OldestFirst(i);
        Check(event && event->Valid &&
              event->FirstActor!=0 && event->SecondActor!=0 &&
              !event->OfficialPenaltyIssued &&
              !event->EngineCollisionEventFired &&
              !event->ActualVehicleControlModified,
              "replay event retains real physical slots with zero race authority");
        if(event->Kind==PhysicalReplayEventKind::ContactBegan ||
           event->Kind==PhysicalReplayEventKind::ContactEnded)
            ++observedContacts;
    }
    Check(observedContacts==2,
          "a single measured collision episode cannot create repeated alerts");
    RealNextFrame(traffic);
    traffic.Others[1].Identity.Actor+=9000;
    observed=ReviewPhysicalTraceFrame(traffic);
    Check(observed.Valid &&
          reporter.Append(observed)==
              PhysicalReplayEventFault::InconsistentRoster &&
          reporter.Size()==0 &&
          reporter.Snapshot().RosterResets==1,
          "changed physical racer invalidates all previous event identity");
    Check(reporter.Append(observed)==PhysicalReplayEventFault::None &&
          reporter.Size()==0,
          "after roster reauthentication the new event epoch starts empty");
}
static void NearMissAndDiscontinuityEvents()
{
    auto traffic=CompleteSixCars();
    traffic.Others[1].Pose.Position={450.0,200.0};
    PhysicalReplayEventRecorder events;
    Check(events.Configure({}),
          "near-miss event review has valid settings");
    const auto sample=ReviewPhysicalTraceFrame(traffic);
    Check(sample.Valid &&
          events.Append(sample)==PhysicalReplayEventFault::None &&
          events.Snapshot().ClosePassEvents==1 &&
          events.Snapshot().ContactsBegan==0,
          "close adjacent verified chassis produces near-miss, not collision");
    RealNextFrame(traffic);
    auto later=ReviewPhysicalTraceFrame(traffic);
    Check(events.Append(later)==PhysicalReplayEventFault::None &&
          events.Snapshot().ClosePassEvents==1,
          "one continuous near miss is deduplicated across real physics frames");
    RealNextFrame(traffic);
    traffic.Others[2].Pose.Position.X+=2500.0;
    later=ReviewPhysicalTraceFrame(traffic);
    Check(later.Valid &&
          events.Append(later)==
              PhysicalReplayEventFault::LostPhysicsContinuity &&
          events.Rejected()==1,
          "unsupported physical position teleport cannot create replay event");
    PhysicalReplayEventSettings invalid;
    invalid.MinimumConfidence=2.0;
    Check(!events.Configure(invalid),
          "nonphysical event sensor confidence is rejected");
}
int main()
{
    DirectPhysicalEvidence();
    ContinuousReplay();
    RosterAndSessionIsolation();
    BoundedWrapAndFaults();
    ArchiveRoundTrip();
    RejectMalformedArchives();
    PhysicalCameraShotReview();
    OriginalCameraCollisionAndState();
    PhysicalReplayEventTimeline();
    NearMissAndDiscontinuityEvents();
    std::cout << "PhysicalSixCarRaceTrace OK\n";
}