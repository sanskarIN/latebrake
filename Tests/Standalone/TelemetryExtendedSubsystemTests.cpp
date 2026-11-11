#include "Telemetry/TelemetryRaceLineProfile.h"
#include "Telemetry/TelemetryCornerSequence.h"
#include "Telemetry/TelemetryFuelStint.h"
#include "Telemetry/TelemetryTireStint.h"
#include "Telemetry/TelemetryWeatherTrend.h"
#include "Telemetry/TelemetryPitReview.h"
#include "Telemetry/TelemetryVehicleBalance.h"
#include "Telemetry/TelemetrySuspensionReview.h"
#include "Telemetry/TelemetryTransmissionReview.h"
#include "Telemetry/TelemetryIncidentJournal.h"
#include "Telemetry/TelemetryFleetGapReview.h"
#include "Telemetry/TelemetryDriverCoach.h"
#include "Telemetry/TelemetrySessionIntegrity.h"
#include "Telemetry/TelemetryReplayBookmarks.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake::engineering;

void Require(bool pass, const char* reason)
{
    if (!pass)
    {
        std::cerr << "TelemetryExtendedSubsystem FAIL: "
                  << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

TelemetryFrame Frame(std::uint64_t ordinal,
                     double seconds, double x) noexcept
{
    TelemetryFrame f;
    f.Identity.SessionToken = 100;
    f.Identity.ActorToken = 200;
    f.Identity.FrameOrdinal = ordinal;
    f.Identity.DriverSlot = 0;
    f.Identity.SessionRevision = 1;
    f.Identity.Trust = TelemetryTrust::EngineObserved;
    f.Race.Phase = TelemetryPhase::Racing;
    f.Race.EnginePhysicsReady = true;
    f.Race.LocalRaceClockRunning = true;
    f.MonotonicSeconds = seconds;
    f.RaceClockSeconds = seconds;
    f.DeltaSeconds = 0.05;
    f.Dynamics.WorldPositionCm = {x, -1800.0, 160.0};
    f.Dynamics.WorldVelocityCmS = {200.0, 0.0, 0.0};
    f.Dynamics.FuelFraction = 0.9;
    f.Dynamics.GripScale = 1.0;
    f.Dynamics.EngineRpm = 2400.0;
    f.Wheels.Contact.fill(true);
    f.Wheels.NormalLoadNewtons.fill(3000.0);
    f.Wheels.TireTemperatureCelsius.fill(80.0);
    f.Environment.OnPhysicalPavement = true;
    return f;
}

void TestRaceLineAndCornerSequence()
{
    RaceLineProfiler<32> line;
    RaceLineSettings settings;
    settings.SegmentLengthCm = 250.0;
    Require(line.Configure(3000.0, settings),
            "strict fixed-size racing line");
    for (int i = 0; i < 14; ++i)
    {
        RaceLineSample s;
        s.TrackDistanceCm = 50.0 + i * 20.0;
        s.LateralOffsetCm = i * 1.5;
        s.RoadHalfWidthCm = 800.0;
        s.SpeedCmS = 1400.0;
        s.Sector = 0;
        s.DeltaSeconds = 0.05;
        s.OnOriginalPavement = true;
        s.CollisionFree = true;
        s.EngineQualified = true;
        Require(line.Observe(s),
                "real padded line position feeds advisory map");
    }
    Require(line.Segment(0).Valid &&
            line.Summarize().Qualified &&
            line.Summarize().MeanAbsoluteLateralCm >= 0.0,
            "line consistency is numerical not a steering command");

    CornerEventRecord c;
    c.Valid = true;
    c.EntryTimeSeconds = 10.0;
    c.ApexTimeSeconds = 11.0;
    c.ExitTimeSeconds = 12.0;
    c.DurationSeconds = 2.0;
    c.EntrySpeedCmS = 1400.0;
    c.ApexSpeedCmS = 1000.0;
    c.ExitSpeedCmS = 1300.0;
    c.EntryAtTrackCm = 1200.0;
    c.ApexAtTrackCm = 1600.0;
    c.ExitAtTrackCm = 1900.0;
    c.DirectionReliable = true;
    c.ApexObserved = true;
    BrakeEventRecord b;
    b.Valid = true;
    b.StartedSeconds = 8.0;
    b.EndedSeconds = 9.0;
    b.DurationSeconds = 1.0;
    b.StartedAtTrackCm = 700.0;
    b.EndedAtTrackCm = 1100.0;
    b.DistanceCm = 400.0;
    const CornerSequenceEntry entry =
        ReviewCornerEntry(c, b, 0, 1, true, {});
    Require(entry.Qualified &&
            entry.BrakeLinked &&
            entry.ReviewScore > 0.0,
            "corner sequence links source-apex and observed brake");
    CornerSequenceJournal journal;
    Require(journal.Configure({}) &&
            journal.BeginSession(100, 200, 0),
            "driver/session identity pins corner history");
    Require(journal.Observe(c, b, 100, 200, 0, 1, true),
            "first corner context accepted");
    c.EntryTimeSeconds = 20.0;
    c.ApexTimeSeconds = 21.0;
    c.ExitTimeSeconds = 22.0;
    b.EndedSeconds = 19.0;
    Require(journal.Observe(c, b, 100, 200, 0, 2, true),
            "second distinct corner preserves lap-local history");
    Require(journal.Compare().Complete &&
            journal.Compare().AdvisoryOnly,
            "two physical corner observations remain non-authoritative");
}

void TestFuelTireAndWeather()
{
    FuelStintMonitor fuel;
    Require(fuel.Configure({}), "valid physical fuel channel");
    for (int i = 0; i < 22; ++i)
    {
        FuelSample s;
        s.TimeSeconds = static_cast<double>(i);
        s.FuelFraction = 0.9 - i * 0.001;
        s.TrackDistanceCm = i * 200.0;
        s.Throttle = 0.55;
        s.SpeedCmS = 200.0;
        s.CompletedLaps = i / 7;
        s.DriverSlot = 0;
        s.Session = 100;
        s.Actor = 200;
        s.PhysicalSensor = true;
        s.EngineQualified = true;
        Require(fuel.Observe(s),
                "physical fuel depletion and lap transitions");
    }
    Require(fuel.Snapshot().PhysicalSensorTrusted &&
            fuel.Snapshot().ConsumedLiters > 0.0 &&
            fuel.Snapshot().CompletedFuelLaps > 0 &&
            !fuel.Snapshot().AuthoritativeFuelLevel,
            "fuel estimates retain source provenance");
    TireStintMonitor tire;
    Require(tire.Configure({}), "verified four wheel tire channel");
    for (int i = 0; i < 25; ++i)
    {
        TireSensorFrame s;
        s.TimeSeconds = 1.0 + i * 0.05;
        s.TemperatureC.fill(94.0);
        s.WearFraction.fill(i * 0.001);
        s.NormalLoadN.fill(3000.0);
        s.SlipRatio.fill(0.15);
        s.Contact.fill(true);
        s.CompletedLaps = i / 8;
        s.DriverSlot = 0;
        s.Session = 100;
        s.Actor = 200;
        s.PhysicalSensorsReady = true;
        s.EngineQualified = true;
        Require(tire.Observe(s),
                "real four tire wear frames accumulate without reversal");
    }
    Require(tire.Snapshot().Valid &&
            tire.Snapshot().CompletedLaps >= 2 &&
            tire.Snapshot().Front.Valid &&
            tire.Snapshot().Rear.Valid,
            "tire risk requires four wheel observations");
    TrackWeatherMonitor weather;
    Require(weather.Configure({}), "strict environment sensor policy");
    for (int i = 0; i < 16; ++i)
    {
        TrackEnvironmentSample s;
        s.TimeSeconds = i * 1.0;
        s.WetnessFraction = 0.2 + i * 0.01;
        s.SourceEngineQualified = true;
        s.PhysicallyMeasuredWetness = true;
        s.PhysicallyMeasuredFriction = true;
        Require(weather.Observe(s),
                "wetness trajectory grounded in source sensors");
    }
    Require(weather.Snapshot().Valid &&
            weather.Snapshot().EnoughHistory &&
            weather.Snapshot().CurrentWetness > 0.2 &&
            !weather.Snapshot().SafeToRecommendTireChange,
            "weather trends never authorize pit or race control");
    PitSessionEvidence evidence;
    evidence.Fuel = fuel.Snapshot();
    evidence.Tire = tire.Snapshot();
    evidence.Weather = weather.Snapshot();
    evidence.TrackLengthCm = 16000.0;
    evidence.MeanLapSeconds = 64.0;
    evidence.PitLaneTransitSeconds = 30.0;
    evidence.StationaryServiceSeconds = 3.0;
    evidence.ExpectedFuelRefillSeconds = 8.0;
    evidence.ExpectedTireChangeSeconds = 9.0;
    evidence.ExpectedRepairSeconds = 12.0;
    evidence.RemainingRaceSeconds = 360.0;
    evidence.RemainingLaps = 5;
    evidence.RacePosition = 3;
    evidence.TotalDrivers = 6;
    evidence.EngineQualified = true;
    evidence.PitLaneExists = true;
    evidence.ServiceAllowed = true;
    evidence.ReliableClock = true;
    const PitReviewResult pit = EvaluatePitReview(evidence);
    Require(pit.Valid && pit.EvaluatedScenarios >= 2 &&
            !pit.IssuedPitCommand && pit.AdvisoryOnly,
            "pit strategy compares options without triggering any service");
}

void TestHandlingSuspensionAndTransmission()
{
    BalanceObservation b;
    b.TimeSeconds = 2.0;
    b.SpeedCmS = 1000.0;
    b.Steering = 0.30;
    b.YawRateRadiansSecond = 0.5;
    b.LateralAccelerationG = 0.50;
    b.GripScale = 1.0;
    b.OnOriginalRoad = true;
    b.OriginalPhysicsReady = true;
    b.EngineQualified = true;
    b.PhysicalWheelContact.fill(true);
    b.WheelNormalLoadsN.fill(3200.0);
    b.TireSlipAnglesRadians.fill(0.10);
    b.TireSlipRatios.fill(0.06);
    const HandlingBalanceReview review =
        EvaluateHandlingBalance(b);
    Require(review.Valid && review.Front.Valid &&
            review.Rear.Valid &&
            review.Classification == HandlingBalance::Stable,
            "measured normal loads produce independent axle assessment");
    VehicleBalanceMonitor balance;
    Require(balance.Configure({}) &&
            balance.Observe(b, 0.05) &&
            balance.History().Valid &&
            balance.History().AcceptedFrames == 1,
            "vehicle balance aggregates source physics without control");
    SuspensionEventMonitor suspension;
    Require(suspension.Configure({}), "four-wheel suspension data gate");
    SuspensionSample s;
    s.TimeSeconds = 1.0;
    s.CompressionCm.fill(8.0);
    s.NormalLoadNewtons.fill(3200.0);
    s.Contact.fill(true);
    s.FourWheelSuspensionAvailable = true;
    s.OriginalEngineObserved = true;
    Require(suspension.Observe(s),
            "first genuine wheel travel sample");
    s.TimeSeconds += 0.05;
    Require(suspension.Observe(s) &&
            suspension.Snapshot().Valid &&
            suspension.QualifiedFrames() == 2,
            "stable suspension sample has bounded travel derivatives");
    TransmissionShiftMonitor transmission;
    Require(transmission.Configure({}),
            "gear review uses physical engine thresholds");
    DrivetrainSample d;
    d.TimeSeconds = 1.0;
    d.EngineRpm = 5800.0;
    d.Gear = 2;
    d.Throttle = 0.5;
    d.EnginePhysicalReadout = true;
    d.SourceQualified = true;
    d.TransmissionEngaged = true;
    Require(transmission.Observe(d),
            "first drivetrain sample establishes gear baseline");
    d.TimeSeconds += 0.05;
    d.Gear = 3;
    d.TransmissionEngaged = false;
    d.EngineRpm = 4600.0;
    Require(transmission.Observe(d),
            "observed physical gear change begins transition");
    d.TimeSeconds += 0.10;
    d.TransmissionEngaged = true;
    Require(transmission.Observe(d) &&
            transmission.StoredShifts() == 1 &&
            transmission.Snapshot().LastShift.Valid,
            "engine gear transition finalized by actual engagement");
}

void TestIncidentsFleetCoachingAndAudit()
{
    IncidentJournal incidents;
    Require(incidents.Configure({}), "incident evidence severity policy");
    IncidentObservation event;
    event.Kind = IncidentKind::WallContact;
    event.MonotonicSeconds = 10.0;
    event.TrackDistanceCm = 1200.0;
    event.SpeedCmS = 800.0;
    event.RelativeImpactSpeedCmS = 600.0;
    event.ChassisDamageBefore = 0.05;
    event.ChassisDamageAfter = 0.14;
    event.DurationSeconds = 0.2;
    event.LapIndex = 0;
    event.SectorIndex = 1;
    event.OwnSlot = 0;
    event.Session = 100;
    event.OwnActor = 200;
    event.PhysicalContactConfirmed = true;
    event.EngineQualified = true;
    Require(incidents.Observe(event),
            "real wall contact stored as non-official review");
    Require(!incidents.Observe(event) &&
            incidents.Snapshot().DuplicateSuppressed == 1,
            "repeated physics events do not inflate contact statistics");
    Require(incidents.Snapshot().WallContacts == 1 &&
            !incidents.Snapshot().OfficialRacePenaltyAuthorized,
            "no incident review may grant official penalty");
    std::array<FleetProgressSample, 6> fleet{};
    for (int i = 0; i < 6; ++i)
    {
        FleetProgressSample sample;
        sample.ActorToken = static_cast<std::uint64_t>(200 + i);
        sample.SessionToken = 100;
        sample.MonotonicSeconds = 10.0;
        sample.UnwrappedDistanceCm = 8000.0 - i * 500.0;
        sample.TrackDistanceCm = 1500.0 - i * 100.0;
        sample.SpeedCmS = 900.0;
        sample.GridSlot = i;
        sample.LapIndex = 1;
        sample.ReportedRacePosition = i + 1;
        sample.SourceEngineQualified = true;
        sample.PhysicalRosterMember = true;
        sample.VerifiedForwardProgress = true;
        fleet[static_cast<std::size_t>(i)] = sample;
    }
    const FleetGapSnapshot gaps =
        EvaluateFleetGaps(fleet.data(), 6, 16000.0);
    Require(gaps.Valid && gaps.QualifiedDrivers == 6 &&
            gaps.SlotByAnalyticalRank[0] == 0 &&
            gaps.BySlot[5].GapToLeaderSeconds > 0.0 &&
            !gaps.OfficialClassificationChanged,
            "exact six physical progress members determine analytical gaps");
    FleetGapHistory gapHistory;
    Require(gapHistory.Configure(5) &&
            gapHistory.Observe(gaps) &&
            gapHistory.Snapshot().Valid,
            "rival gap history stores observed positions");
    CoachingEvidence coaching;
    coaching.TimeSeconds = 11.0;
    coaching.DriverSlot = 0;
    coaching.LapIndex = 1;
    coaching.ChassisDamageFraction = 0.75;
    coaching.EngineQualified = true;
    coaching.PhysicalSensorsReady = true;
    coaching.OriginalTrackVerified = true;
    const CoachingCandidate suggestion =
        EvaluateCoachingEvidence(coaching);
    Require(suggestion.Valid &&
            suggestion.Topic ==
                CoachingTopic::ReviewPhysicalDamage &&
            !suggestion.CommandsVehicle,
            "physical damage generates human-review advice only");
    DriverCoachingJournal hints;
    Require(hints.Configure({}) &&
            hints.Observe(coaching) &&
            hints.History().AdviceOnly &&
            !hints.History().AnyVehicleCommands,
            "coaching cannot change physical motor/steering requests");
    TelemetrySessionIntegrityMonitor integrity;
    Require(integrity.Configure({}),
            "session clock and actor identity audit settings");
    const TelemetryFrame first = Frame(1, 1.0, 100.0);
    Require(integrity.Observe(first).Qualified,
            "first original engine observation accepted");
    const TelemetryFrame second = Frame(2, 1.05, 110.0);
    Require(integrity.Observe(second).Qualified,
            "monotonic physically plausible position accepted");
    Require(!integrity.Observe(second).Qualified &&
            integrity.Snapshot().RejectedFrames == 1,
            "same frame ordinal is never consumed twice");
    ReplayBookmarkIndex bookmarks;
    Require(bookmarks.Configure({}),
            "safe replay evidence index");
    const ReplayBookmark mark = MakeBookmark(
        ReplayBookmarkKind::WallContact,
        second, 0.4, 0.8, 2000.0, 600.0);
    Require(mark.Valid && bookmarks.Add(mark) &&
            bookmarks.StoredBookmarks() == 1,
            "verified physical event admits immutable replay marker");
    const ReplayClipWindow clip =
        bookmarks.BuildClipWindow(0, 0);
    Require(clip.TimeRangeQualified &&
            clip.CriticalBookmarks == 1 &&
            !clip.ExportedMovie,
            "replay clip is only source time metadata, not a video");
}
}

int main()
{
    TestRaceLineAndCornerSequence();
    TestFuelTireAndWeather();
    TestHandlingSuspensionAndTransmission();
    TestIncidentsFleetCoachingAndAudit();
    std::cout << "Extended source-only race engineering systems passed.\n";
    return EXIT_SUCCESS;
}
