#include "Racecraft/PhysicalSixCarReplayArchive.h"
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
        Check(reader.Import(mutated)==expected &&
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
    // Header is 69 bytes, then 28-byte frame prefix; first real Actor ID.
    bad[69+28]^=0x40;
    RepairTestChecksum(bad);
    expectFault(bad,PhysicalReplayArchiveFault::InvalidIdentity,
          "serialized chassis identity must match fixed six-slot roster");
    bad=source.Bytes;
    // A record is exactly 28 + 6*(8 + 10*4) = 316 bytes.
    // The second recorded source frame must be the successor of frame 1.
    bad[69+316]=1;
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
int main()
{
    DirectPhysicalEvidence();
    ContinuousReplay();
    RosterAndSessionIsolation();
    BoundedWrapAndFaults();
    ArchiveRoundTrip();
    RejectMalformedArchives();
    std::cout << "PhysicalSixCarRaceTrace OK\n";
}