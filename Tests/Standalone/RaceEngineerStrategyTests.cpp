#include "Telemetry/TelemetrySessionTimeline.h"
#include "Telemetry/TelemetryLapAnalysis.h"
#include "Telemetry/TelemetrySectorAnalysis.h"
#include "Telemetry/TelemetryEventJournal.h"
#include "Telemetry/TelemetryReplayBuffer.h"
#include "Telemetry/TelemetryDeltaComparison.h"
#include "Telemetry/TelemetryVehicleHealth.h"
#include "Telemetry/TelemetryEnvironmentModel.h"
#include "Telemetry/TelemetryFuelStrategy.h"
#include "Telemetry/TelemetryPitReview.h"
#include "Telemetry/TelemetryReliability.h"
#include "Telemetry/TelemetryCoach.h"
#include "Telemetry/TelemetryRaceStrategy.h"
#include "Telemetry/TelemetryDriverComparison.h"
#include "Telemetry/TelemetryStintPerformance.h"
#include "Telemetry/TelemetryTrainingDrills.h"
#include "Telemetry/TelemetryExport.h"
#include "Telemetry/TelemetryDashboard.h"
#include "Telemetry/TelemetrySafetyAudit.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::engineering;

void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "RaceEngineerStrategy FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

TelemetryFrame MakeNativeFrame(
    int ordinal, double seconds,
    double xCm = 0.0) noexcept
{
    TelemetryFrame frame;
    frame.Identity.SessionToken = 18;
    frame.Identity.ActorToken = 1818;
    frame.Identity.FrameOrdinal =
        static_cast<std::uint64_t>(ordinal);
    frame.Identity.SessionRevision = 1;
    frame.Identity.DriverSlot = 0;
    frame.Identity.Trust = TelemetryTrust::EngineObserved;
    frame.Identity.VehicleClass = TelemetryVehicleClass::Touring;
    frame.MonotonicSeconds = seconds;
    frame.RaceClockSeconds = seconds;
    frame.DeltaSeconds = 0.05;
    frame.Race.Phase = TelemetryPhase::Racing;
    frame.Race.EnginePhysicsReady = true;
    frame.Race.LocalRaceClockRunning = true;
    frame.Race.NextGateIndex = 0;
    frame.Race.SectorIndex = 0;
    frame.Race.LapIndex = 0;
    frame.Dynamics.WorldPositionCm = {xCm, -1800.0, 150.0};
    frame.Dynamics.WorldVelocityCmS = {800.0, 0.0, 0.0};
    frame.Dynamics.FuelFraction = 0.9;
    frame.Dynamics.EngineRpm = 3300.0;
    frame.Dynamics.GripScale = 1.0;
    frame.Dynamics.EngineTemperatureCelsius = 95.0;
    frame.Dynamics.BrakeTemperatureCelsius = 280.0;
    frame.Dynamics.SuspensionCompressionCm = 20.0;
    frame.Controls.Throttle = 0.7;
    frame.Wheels.Contact.fill(true);
    frame.Wheels.NormalLoadNewtons.fill(3000.0);
    frame.Wheels.TireTemperatureCelsius.fill(90.0);
    frame.Wheels.TireWearFraction.fill(0.2);
    frame.Environment.Surface = TelemetrySurface::Asphalt;
    frame.Environment.OnPhysicalPavement = true;
    frame.Environment.RoadFrictionEstimate = 1.0;
    frame.SpeedometerKmH = 800.0 * CmPerSecondToKmh;
    return frame;
}

TelemetryKinematics MakeMotion(
    double xCm = 0.0, double speed = 800.0) noexcept
{
    TelemetryKinematics motion;
    motion.Valid = true;
    motion.PositionCm = {xCm, -1800.0};
    motion.VelocityCmS = {speed, 0.0};
    motion.SmoothedVelocityCmS = motion.VelocityCmS;
    motion.SpeedCmS = speed;
    motion.SpeedKmh = speed * CmPerSecondToKmh;
    motion.ForwardSpeedCmS = speed;
    return motion;
}

TelemetryFrameDecision AcceptedDecision(
    double dt = 0.05) noexcept
{
    TelemetryFrameDecision decision;
    decision.Quality = TelemetryQuality::Qualified;
    decision.Reason = TelemetryReject::None;
    decision.Accepted = true;
    decision.ValidatedDeltaSeconds = dt;
    return decision;
}

void TestSessionTimeline()
{
    SessionTimeline timeline;
    auto frame = MakeNativeFrame(1, 0.05);
    frame.Race.Phase = TelemetryPhase::PreGrid;
    frame.Race.LocalRaceClockRunning = false;
    Require(timeline.Observe(frame).Accepted,
            "native pre-grid phase is recorded but never makes race green");
    frame.Identity.FrameOrdinal += 1;
    frame.MonotonicSeconds += 0.05;
    frame.Race.Phase = TelemetryPhase::Countdown;
    Require(timeline.Observe(frame).Boundary ==
            TimelineBoundary::LightsStarted,
            "verified countdown transition recorded exactly once");
    frame.Identity.FrameOrdinal += 1;
    frame.MonotonicSeconds += 0.05;
    frame.Race.Phase = TelemetryPhase::Racing;
    frame.Race.LocalRaceClockRunning = true;
    const auto green = timeline.Observe(frame);
    Require(green.Accepted &&
            green.Boundary == TimelineBoundary::RaceGreen &&
            green.RaceActive,
            "engine-supplied green transition is observational");
    frame.Identity.FrameOrdinal += 1;
    frame.MonotonicSeconds += 0.05;
    frame.Race.Phase = TelemetryPhase::Finished;
    Require(timeline.Observe(frame).Accepted,
            "native finished transition is observable");
    frame.Identity.FrameOrdinal += 1;
    frame.MonotonicSeconds += 0.05;
    frame.Race.Phase = TelemetryPhase::Racing;
    Require(!timeline.Observe(frame).Accepted,
            "finished session cannot silently return to racing");
    Require(timeline.Statistics().RejectedFrames >= 1,
            "illegal transition is auditable");
}

void TestLapsAndSectors()
{
    LapMeasurementJournal laps;
    SectorTimingObserver sectors;
    Require(laps.Configure({}) && sectors.Configure({}),
            "lap and sector settings accepted");
    TrackProjection projection;
    projection.Valid = true;
    projection.Segment = 3;
    projection.SectorIndex = 0;
    double seconds = 0.0;
    double xCm = -1000.0;
    bool verifiedSector = false;
    bool verifiedLap = false;
    for (int i = 1; i <= 230; ++i)
    {
        seconds += 0.05;
        xCm += 40.0;
        TelemetryFrame frame = MakeNativeFrame(i, seconds, xCm);
        frame.Dynamics.WorldVelocityCmS = {800.0, 0.0, 0.0};
        TelemetryKinematics motion = MakeMotion(xCm);
        if (i >= 100)
        {
            frame.Race.SectorIndex = 1;
            frame.Race.CreditedGateOrdinal = 1;
            frame.Race.VerifiedForwardGate = i == 100;
        }
        if (i == 230)
        {
            frame.Race.LapIndex = 1;
            frame.Race.VerifiedForwardGate = true;
        }
        const auto decision = AcceptedDecision();
        const auto lap = laps.Observe(frame, decision, motion);
        Require(lap.Accepted, "valid engine lap journal is read-only");
        verifiedLap = verifiedLap || lap.CompletedLap;
        const auto sector = sectors.Observe(
            frame, decision, motion, projection);
        if (i <= 100)
        {
            Require(sector.Accepted,
                    "real sector sequence accepts clock continuity");
        }
        verifiedSector = verifiedSector || sector.CompletedSplit;
        projection.DistanceAlongCm += 40.0;
    }
    Require(verifiedLap && laps.RecordedLaps() == 1 &&
            laps.LatestCompletedLap().Quality ==
                TelemetryQuality::Qualified &&
            laps.BestLapSeconds() >= 3.0,
            "recorded lap comes only from observed verified forward gate");
    Require(verifiedSector && sectors.QualifiedSplits() >= 1 &&
            sectors.SectorStats(0).Samples >= 1,
            "measured sector split keeps its separate reference");
}

void TestEventJournal()
{
    EventJournal journal;
    Require(journal.Configure({}), "bounded event journal configured");
    TelemetryEvent event;
    event.Kind = TelemetryEventKind::BrakeApplication;
    event.Quality = TelemetryQuality::Qualified;
    event.MonotonicSeconds = 1.0;
    event.RaceClockSeconds = 1.0;
    event.DriverSlot = 0;
    event.RelatedSlot = -1;
    event.ActorToken = 1818;
    Require(journal.Append(
        event, EventSource::NativePhysics, 18, 1, 1),
        "native brake event accepted");
    event.MonotonicSeconds = 1.1;
    Require(journal.Append(
        event, EventSource::NativePhysics, 18, 1, 2) &&
        journal.Size() == 1 &&
        journal.AtFromNewest(0).MergeCount == 2,
        "adjacent brake observations coalesce without losing sequence");
    event.MonotonicSeconds = 3.0;
    Require(journal.Append(
        event, EventSource::NativePhysics, 18, 1, 3) &&
        journal.Size() == 2,
        "new temporally distinct event remains distinct");
    std::array<EventRecord, 4> copy{};
    Require(journal.CopyRecent(
        copy.data(), 4, EventSeverity::Informational, 0.0) == 2,
        "read-only event window can be copied without allocation");
}

void TestReplayAndDelta()
{
    static ReplayRecorder replay;
    replay.Reset();
    Require(replay.Configure({}), "fixed-capacity replay configured");
    TrackProjection track;
    track.Valid = true;
    track.Segment = 3;
    track.SectorIndex = 0;
    const auto decision = AcceptedDecision();
    for (int i = 1; i <= 100; ++i)
    {
        const double seconds = i * 0.05;
        const double x = i * 25.0;
        TelemetryFrame frame = MakeNativeFrame(i, seconds, x);
        TelemetryKinematics motion = MakeMotion(x, 500.0);
        track.DistanceAlongCm = x;
        Require(replay.Observe(
            frame, decision, motion, track).Accepted,
            "source replay records actual actor frames");
    }
    Require(replay.Size() == 100 &&
            replay.Latest().FrameOrdinal == 100,
            "bounded replay retains exact source frame identity");
    const ReplaySample mid =
        replay.SampleAt(2.525, ReplaySampleMode::Linear);
    Require(mid.Valid && !mid.NativeObserved &&
            std::abs(mid.PositionCm.X - 1262.5) < 0.1,
            "interpolated replay is never mislabelled native");
    ReplayPlaybackClock clock;
    Require(clock.Configure(1.0, 5.0) && clock.Play() &&
            clock.Advance(0.1) &&
            clock.PositionSeconds() > 1.0,
            "playback clock cannot change source race clock");

    LapDeltaReference reference;
    Require(reference.Begin(
        18, 1818, 911, 0, 1000.0, 1.0),
        "reference requires track identity");
    for (int i = 0; i <= 10; ++i)
    {
        DeltaReferencePoint point;
        point.Valid = true;
        point.NativeObserved = true;
        point.DistanceCm = i * 100.0;
        point.ElapsedSeconds = i * 1.0;
        point.SpeedCmS = 100.0;
        point.SectorIndex = 0;
        point.FrameOrdinal =
            static_cast<std::uint64_t>(i + 1);
        Require(reference.Append(point),
                "continuous distance-sampled reference");
    }
    Require(reference.Finish(10.0, 1, true),
            "only sampled native and officially qualified lap is reference");
    LiveDeltaAnalyzer analyzer;
    Require(analyzer.Configure(&reference),
            "immutable reference is not live scoring");
    const auto compared = analyzer.Compare(
        450.0, 4.8, 98.0, 0.7, 0.0,
        0.0, 10.0, 0.5, 0, 911);
    Require(compared.Qualified &&
            std::abs(compared.DeltaSeconds - 0.3) < 0.001,
            "distance-aligned lap delta reports observed loss");
    Require(!analyzer.Compare(
        400.0, 5.0, 90.0, 0.7, 0.0,
        0.0, 0.0, 0.5, 0, 911).Qualified,
            "backtracking cannot fabricate a faster delta");
}

void TestVehicleHealthAndEnvironment()
{
    VehicleHealthMonitor health;
    TrackEnvironmentMonitor weather;
    Require(health.Configure({}) && weather.Configure({}),
            "separate diagnostics have explicit valid settings");
    TelemetryFrame frame = MakeNativeFrame(1, 1.0);
    GripTelemetry grip;
    grip.Valid = true;
    grip.Confidence = 1.0;
    grip.ContactFraction = 1.0;
    grip.LoadWeightedGripFactor = 0.9;
    TrackProjection track;
    track.Valid = true;
    track.SectorIndex = 1;
    for (int i = 0; i < 24; ++i)
    {
        frame.Identity.FrameOrdinal =
            static_cast<std::uint64_t>(i + 1);
        frame.MonotonicSeconds += 0.05;
        frame.Environment.AmbientWetness = 0.5;
        Require(health.Observe(frame, grip, 0.05) &&
                weather.Observe(frame, grip, track, 0.05),
                "native vehicle heat and wet weather tracked per sector");
    }
    Require(health.Snapshot().Qualified &&
            health.AcceptedSamples() == 24,
            "native vehicle channels remain read-only and confidence gated");
    Require(weather.SectorReview(1).Qualified &&
            weather.SectorReview(1).QualifiedFrames == 24 &&
            weather.Snapshot().Wetness > 0.0,
            "sector-local conditions reflect actual sampled wetness");
    frame.Dynamics.EngineTemperatureCelsius =
        std::numeric_limits<double>::quiet_NaN();
    Require(!health.Observe(frame, grip, 0.05),
            "nonfinite engine sensor fails closed");
}

void TestFuelStintAndPit()
{
    FuelStintMonitor fuel;
    PitVisitMonitor pits;
    Require(fuel.Configure({}) && pits.Configure({}),
            "fuel and pit review remain advisory");
    double seconds = 0.0;
    for (int i = 1; i <= 240; ++i)
    {
        seconds += 0.05;
        const double x = i * 40.0;
        auto frame = MakeNativeFrame(i, seconds, x);
        frame.Dynamics.FuelFraction = 0.9 - i * 0.0001;
        frame.Race.LapIndex = i >= 120 ? 1 : 0;
        frame.Race.VerifiedForwardGate = i == 120;
        const auto motion = MakeMotion(x);
        Require(fuel.Observe(frame, AcceptedDecision(), motion),
                "fuel burn follows real changing level");
    }
    Require(fuel.History().CompletedFuelLaps == 1 &&
            fuel.History().TotalConsumedFraction > 0.0,
            "only credited lap rollover adds consumption sample");
    auto outside = MakeNativeFrame(241, 12.05, 0.0);
    outside.Environment.InPitLane = false;
    outside.Dynamics.FuelFraction = 0.72;
    TelemetryKinematics motion = MakeMotion(0.0, 800.0);
    Require(pits.Observe(outside, AcceptedDecision(), motion),
            "native outside-pit baseline accepted");
    for (int i = 0; i < 55; ++i)
    {
        auto frame = MakeNativeFrame(242 + i, 12.10 + i * 0.05, 0.0);
        frame.Environment.InPitLane = true;
        frame.Dynamics.FuelFraction =
            i >= 25 ? 0.75 : 0.72;
        motion.SpeedCmS = i < 12 ? 800.0 : 0.0;
        Require(pits.Observe(frame, AcceptedDecision(), motion),
                "pit observation can distinguish transit and service");
    }
    auto exit = MakeNativeFrame(300, 15.0, 0.0);
    exit.Environment.InPitLane = false;
    exit.Dynamics.FuelFraction = 0.75;
    motion.SpeedCmS = 800.0;
    Require(pits.Observe(exit, AcceptedDecision(), motion) &&
            pits.VisitCount() == 1,
            "observed exit records a pit visit without teleport");
}

void TestReliabilityCoachingAndStrategy()
{
    TelemetryReliabilityMonitor reliability;
    Require(reliability.Configure({}),
            "sensor quality settings are bounded");
    ReliabilityInput input;
    input.Available.fill(true);
    input.NativeVerified.fill(true);
    input.SessionToken = 18;
    input.ActorToken = 1818;
    input.SessionRevision = 1;
    input.DeltaSeconds = 0.05;
    for (int i = 1; i <= 25; ++i)
    {
        input.MonotonicSeconds = i * 0.05;
        reliability.Observe(input);
    }
    Require(reliability.Snapshot().OverallConfidence > 0.7,
            "sustained native samples qualify sensor chain");
    RaceEngineerCoach coach;
    Require(coach.Configure({}),
            "advisory coach settings accepted");
    CoachingFrameInput source;
    source.SourceFrame = MakeNativeFrame(1, 2.0);
    source.NativeObserved = true;
    source.EvidenceConfidence = 1.0;
    source.MonotonicSeconds = 2.0;
    source.Grip.Valid = true;
    source.Grip.State = GripState::Sliding;
    source.Grip.MaximumCombinedSaturation = 1.5;
    Require(coach.Observe(source) &&
            coach.ActiveTips() > 0 &&
            coach.PrimaryTip().Evidence.Topic ==
                CoachingTopic::ReduceExcessiveSlip,
            "safety feedback outranks performance without drive control");
    RaceStrategyReview review;
    Require(review.Configure({}),
            "race strategy cannot command physical service");
    StrategyInputs strategy;
    strategy.SessionToken = 18;
    strategy.MonotonicSeconds = 10.0;
    strategy.NativeObserved = true;
    strategy.RaceActive = true;
    strategy.PhysicalPitServiceAvailable = true;
    strategy.RemainingLaps = 5.0;
    strategy.ReferenceRaceLapSeconds = 90.0;
    strategy.CurrentTireWear = 0.2;
    strategy.CurrentDamageFraction = 0.1;
    strategy.FuelStint.Qualified = true;
    strategy.FuelStint.MeanFuelFractionPerLap = 0.03;
    strategy.FuelStint.WorstLapFuelConsumption = 0.04;
    strategy.FuelStint.LatestFuelFraction = 0.65;
    strategy.Health.Qualified = true;
    strategy.Health.CombinedConfidence = 1.0;
    strategy.Reliability.OverallConfidence = 1.0;
    Require(review.Review(strategy) &&
            review.Snapshot().CandidateCount > 0 &&
            review.Snapshot().AdvisoryOnly,
            "qualified measured strategy evaluates non-authoritative alternatives");
}

void TestDriverAndStint()
{
    DriverPerformanceRecord a;
    a.Qualified = true;
    a.EngineObserved = true;
    a.SessionToken = 18;
    a.ActorToken = 100;
    a.TrackFingerprint = 911;
    a.VehicleClass = TelemetryVehicleClass::Touring;
    a.DriverSlot = 0;
    a.CompletedLaps = 3;
    a.ValidTimedLaps = 3;
    a.SectorCount = 4;
    a.TrackLengthCm = 18000.0;
    a.BestLapSeconds = 90.0;
    a.MeanLapSeconds = 92.0;
    a.TheoreticalBestLapSeconds = 88.0;
    a.MeanCornerScore = 80.0;
    a.MeanPeakSpeedKmh = 110.0;
    a.Confidence = 1.0;
    for (int i = 0; i < 4; ++i)
    {
        a.SectorSamples[static_cast<std::size_t>(i)] = 3;
        a.BestSectorSeconds[static_cast<std::size_t>(i)] = 22.0;
        a.MeanSectorSeconds[static_cast<std::size_t>(i)] = 23.0;
    }
    auto b = a;
    b.ActorToken = 101;
    b.DriverSlot = 1;
    b.BestLapSeconds = 91.0;
    b.MeanLapSeconds = 93.0;
    const auto comparison = CompareDrivers(a, b);
    Require(comparison.Qualified &&
            comparison.BestLapAdvantage ==
                ComparisonAdvantage::DriverA &&
            comparison.SharedSectors == 4,
            "same circuit/class and measured laps are comparable");
    DriverComparisonJournal history;
    Require(history.Observe(comparison, 18000.0) &&
            history.Snapshot().DriverAWins == 1,
            "driver observations cannot affect actual race rank");
    StintPaceMonitor stint;
    Require(stint.Configure({}),
            "stint trend requires valid thresholds");
    for (int i = 0; i < 4; ++i)
    {
        StintLapSample sample;
        sample.Qualified = true;
        sample.NativeMeasured = true;
        sample.VerifiedForwardFinish = true;
        sample.LapOrdinal = i;
        sample.SessionToken = 18;
        sample.ActorToken = 100;
        sample.LapTimeSeconds = 90.0 + i * 0.5;
        sample.DistanceCm = 18000.0;
        sample.ConsistencyConfidence = 1.0;
        sample.FuelAtStart = 0.9 - i * 0.04;
        sample.FuelAtFinish = sample.FuelAtStart - 0.04;
        sample.TireWearAtStart = 0.2 + i * 0.02;
        sample.TireWearAtFinish =
            sample.TireWearAtStart + 0.02;
        Require(stint.Observe(sample),
                "verified independent laps populate bounded trend");
    }
    Require(stint.QualifiedLaps() == 4 &&
            stint.Snapshot().PaceSlopeSecondsPerLap > 0.4,
            "time trend estimates observed rather than simulated lap pace");
}

void TestTrainingAndExport()
{
    TrainingDrillTracker drill;
    DrillDefinition definition;
    definition.Kind = DrillKind::SmoothThrottle;
    definition.Metric = DrillMetric::LowerIsBetter;
    definition.Reference = 0.5;
    definition.Target = 0.1;
    definition.Tolerance = 0.01;
    definition.MinimumObservationSeconds = 0.5;
    definition.MaximumWindowSeconds = 120.0;
    definition.RequiredAttempts = 2;
    definition.Enabled = true;
    Require(drill.Configure(definition),
            "safe training goal is finite and advisory only");
    DrillSample sample;
    sample.Kind = DrillKind::SmoothThrottle;
    sample.MetricValue = 0.05;
    sample.Confidence = 1.0;
    sample.DeltaSeconds = 0.05;
    sample.NativeObserved = true;
    sample.Valid = true;
    for (int i = 1; i <= 120; ++i)
    {
        sample.MonotonicSeconds = i * 0.05;
        Require(drill.Observe(sample),
                "safe, verified training sample progresses the drill");
    }
    Require(drill.Achieved() &&
            drill.Snapshot().SuccessfulAttempts >= 2,
            "sustained repeated skill practice qualifies drill target");
    char buffer[4096]{};
    TelemetryExportSession exportSession;
    Require(exportSession.Begin(
        buffer, static_cast<int>(sizeof(buffer)),
        ExportFormat::CsvReplay),
            "bounded, caller-owned CSV destination");
    ReplaySample replay;
    replay.Valid = true;
    replay.NativeObserved = true;
    replay.MonotonicSeconds = 1.0;
    replay.SpeedCmS = 1000.0;
    replay.LapIndex = 0;
    replay.SectorIndex = 0;
    Require(exportSession.AddReplay(replay),
            "native replay row serializes without filesystem access");
    const auto report = exportSession.Finish();
    Require(report.Qualified && report.RowsWritten == 1 &&
            report.BytesWritten > 100,
            "finalized export preserves explicit source-only metadata");
}

void TestDashboardAndEvidence()
{
    RaceDashboardModel dashboard;
    Require(dashboard.Configure({}),
            "snapshot selector has bounded display settings");
    DashboardSources input;
    input.Frame = MakeNativeFrame(1, 1.0);
    input.SessionToken = 18;
    input.ActorToken = 1818;
    input.SnapshotSeconds = 1.0;
    input.NativeObservation = true;
    input.Kinematics = MakeMotion(0.0);
    input.Reliability.OverallConfidence = 1.0;
    input.Reliability.PhysicalSensorConfidence = 1.0;
    input.Reliability.TimingConfidence = 1.0;
    input.Reliability.DriverInputConfidence = 1.0;
    Require(dashboard.Update(input) &&
            dashboard.CanDisplayNativeRaceView() &&
            dashboard.Snapshot().AdvisoryOnly,
            "dashboard only exposes input-bound view, not official game UI");
    TelemetryEvidenceReview evidence;
    Require(evidence.Configure({}),
            "provenance audit has explicit thresholds");
    EvidenceInputs observed;
    observed.Frame = input.Frame;
    observed.NativeGate = AcceptedDecision();
    observed.Motion = input.Kinematics;
    observed.Reliability = input.Reliability;
    observed.Reliability.Qualified = true;
    observed.Timeline.Accepted = true;
    observed.MonotonicSeconds = 1.0;
    observed.ContinuousNativeSeconds = 1.5;
    observed.SameOriginSession = true;
    observed.SameOriginActor = true;
    const auto receipt = evidence.Review(observed);
    Require(receipt.ProvenanceBound &&
            receipt.RaceTelemetryQualified &&
            !receipt.ShippingEvidenceComplete &&
            !receipt.InstalledGameVerified &&
            receipt.AdvisoryOnly,
            "passing source frame tests never fabricates engine or installer proof");
}

void TestFailureInputs()
{
    BoundedTextWriter writer;
    char small[8]{};
    Require(writer.Begin(small, 8) &&
            !writer.Append("oversized-content"),
            "writer reports overflow and never writes past buffer");
    const auto invalid = MakeNativeFrame(1, 1.0);
    auto stale = invalid;
    stale.Identity.Trust = TelemetryTrust::Missing;
    SessionTimeline timeline;
    Require(!timeline.Observe(stale).Accepted,
            "unverified actor cannot enter native race timeline");
    EventJournal journal;
    TelemetryEvent event;
    event.Kind = TelemetryEventKind::WallContact;
    event.DriverSlot = 0;
    event.ActorToken = 0;
    event.MonotonicSeconds = 1.0;
    Require(!journal.Append(
        event, EventSource::NativePhysics, 18, 1, 1),
            "events without actual source actor never look qualified");
    ReplaySample sample;
    sample.Valid = true;
    sample.NativeObserved = false;
    Require(!ValidReplaySample(sample, ReplaySettings{}),
            "simulation of replay never impersonates live engine data");
}
} // namespace

int main()
{
    TestSessionTimeline();
    TestLapsAndSectors();
    TestEventJournal();
    TestReplayAndDelta();
    TestVehicleHealthAndEnvironment();
    TestFuelStintAndPit();
    TestReliabilityCoachingAndStrategy();
    TestDriverAndStint();
    TestTrainingAndExport();
    TestDashboardAndEvidence();
    TestFailureInputs();
    std::cout << "19 race engineering extensions: native provenance, "
                 "laps, sectors, replay, strategy and safe export OK\n";
    return EXIT_SUCCESS;
}
