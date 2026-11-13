#include "Telemetry/TelemetryMath.h"
#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryValidation.h"
#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryTrack.h"
#include "Telemetry/TelemetryBraking.h"
#include "Telemetry/TelemetryCornering.h"
#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryOpponents.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "RaceEngineerTelemetry FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::engineering::TelemetryFrame Frame(
    int ordinal, double time, double x) noexcept
{
    using namespace latebrake::engineering;
    TelemetryFrame f;
    f.Identity.Trust = TelemetryTrust::EngineObserved;
    f.Identity.SessionToken = 12;
    f.Identity.ActorToken = 9121;
    f.Identity.FrameOrdinal = static_cast<std::uint64_t>(ordinal);
    f.Identity.DriverSlot = 0;
    f.Identity.SessionRevision = 1;
    f.Race.Phase = TelemetryPhase::Racing;
    f.Race.EnginePhysicsReady = true;
    f.Race.LocalRaceClockRunning = true;
    f.MonotonicSeconds = time;
    f.RaceClockSeconds = time;
    f.DeltaSeconds = 1.0 / 60.0;
    f.Dynamics.WorldPositionCm = {x, -1800.0, 180.0};
    f.Dynamics.WorldVelocityCmS = {600.0, 0.0, 0.0};
    f.Dynamics.WorldAccelerationCmS2 = {0.0, 0.0, 0.0};
    f.Dynamics.YawRadians = 0.0;
    f.Dynamics.EngineRpm = 3500.0;
    f.Dynamics.FuelFraction = 0.9;
    f.Dynamics.GripScale = 1.0;
    f.SpeedometerKmH = 600.0 * CmPerSecondToKmh;
    f.Environment.Surface = TelemetrySurface::Asphalt;
    f.Environment.OnPhysicalPavement = true;
    return f;
}

void TestMath()
{
    using namespace latebrake::engineering;
    Require(NearlyEqual(Length(Vector2{3.0, 4.0}), 5.0),
            "engine units and vector lengths");
    Require(NearlyEqual(Distance({0.0, 0.0}, {3.0, 4.0}), 5.0),
            "planar distance");
    Require(NearlyEqual(WrapRadians(3.0 * Pi), -Pi),
            "yaw wrapping");
    Require(NearlyEqual(BrakingDistance(1000.0, 500.0), 1000.0),
            "constant-deceleration braking estimate");
    Require(NearlyEqual(SafeStoppingSpeed(1000.0, 500.0), 1000.0),
            "inverse stopping speed");
    Require(NearlyEqual(Curvature({0.0, 0.0},
                                  {1.0, 0.0},
                                  {1.0, 1.0}), 1.4142135623730951),
            "circumcircle curvature for a right triangle");
    Require(!Finite(std::numeric_limits<double>::quiet_NaN()),
            "NaN is never qualified physics");
    Require(Clamp01(std::numeric_limits<double>::infinity()) == 0.0,
            "invalid throttle never escalates");
}

void TestFrameGateAndMotion()
{
    using namespace latebrake::engineering;
    TelemetryFrameGate gate;
    TelemetryKinematicEstimator estimator;
    TelemetryFrame initial = Frame(1, 1.0, 0.0);
    auto verdict = gate.Observe(initial);
    Require(verdict.Accepted && verdict.StartNewSegment,
            "first witnessed engine actor starts independent segment");
    Require(estimator.Observe(initial, verdict),
            "valid initial motion seeds filters");
    const auto next = Frame(2, 1.0 + 1.0 / 60.0, 10.0);
    verdict = gate.Observe(next);
    Require(verdict.Accepted && !verdict.StartNewSegment &&
            std::abs(verdict.DistanceSincePreviousCm - 10.0) < 1.0e-7,
            "continuous physical position and time accepted");
    Require(estimator.Observe(next, verdict) &&
            estimator.Snapshot().Valid &&
            estimator.TravelledDistanceCm() >= 10.0,
            "distance integration is purely observational");

    auto stale = next;
    stale.Identity.FrameOrdinal = 2;
    Require(!gate.Observe(stale).Accepted,
            "stale ordinal cannot reuse the same engine sample");
    // After a stale frame the gate intentionally invalidates old
    // temporal state. Establish a NEW accepted baseline before testing
    // that a later large gap cannot bridge the fresh segment.
    auto recovered = Frame(3, 1.05, 30.0);
    Require(gate.Observe(recovered).Accepted,
            "after stale data only a fresh segment may resume");
    auto jump = Frame(4, 2.0, 12000.0);
    Require(!gate.Observe(jump).Accepted,
            "huge time jump is rejected against new qualified baseline");
    auto corrupt = Frame(5, 2.1, 0.0);
    corrupt.Controls.Throttle =
        std::numeric_limits<double>::quiet_NaN();
    Require(!gate.Observe(corrupt).Accepted,
            "unsafe controls reject observation before filters");
    Require(gate.Snapshot().Rejected >= 3,
            "each source rejected observation is counted");
}

void TestStatisticsAndFilters()
{
    using namespace latebrake::engineering;
    RollingWindow<5> values;
    for (double v : {1.0, 2.0, 3.0, 4.0, 5.0, 6.0})
    {
        Require(values.Push(v), "bounded rolling statistics accept finite");
    }
    Require(values.Size() == 5 &&
            NearlyEqual(values.Mean(), 4.0) &&
            NearlyEqual(values.Percentile(0.5), 4.0) &&
            NearlyEqual(values.Minimum(), 2.0) &&
            NearlyEqual(values.Maximum(), 6.0),
            "rolling overwrite retains latest five samples");
    RunningStatistics all;
    for (double v : {1.0, 2.0, 3.0, 4.0, 5.0})
    {
        Require(all.Observe(v), "Welford running statistics accept finite");
    }
    Require(NearlyEqual(all.Mean(), 3.0) &&
            NearlyEqual(all.Variance(), 2.5),
            "unbiased variance");
    AngularFilter yaw;
    yaw.Observe(3.1, 0.05, 0.1);
    Require(std::abs(yaw.Observe(-3.1, 0.05, 0.1)) > 3.0,
            "yaw smoothing uses shortest wrapped arc");
    HysteresisDetector signal;
    Require(signal.Configure(0.8, 0.2), "valid two-stage event thresholds");
    Require(signal.Observe(0.9) && signal.Observe(0.4) &&
            !signal.Observe(0.1),
            "hysteresis prevents rapid switching at threshold");
}

void TestOriginalTrack()
{
    using namespace latebrake::engineering;
    TrackCenterline track;
    Require(track.ConfigureOriginalCircuit() &&
            track.IsReady() && track.Segments() == 4 &&
            track.LengthCm() > 17000.0,
            "four genuine gate corner coordinates define centerline");
    const auto projection = track.Project({0.0, -1800.0}, 0.0);
    Require(projection.Valid && projection.Segment == 3 &&
            std::abs(projection.SignedLateralCm) < 0.01,
            "real south straight projects to correct forward segment");
    Require(track.ContainsCenter({0.0, -1800.0}) &&
            !track.ContainsCenter({0.0, 0.0}),
            "source centerline does not treat infield as pavement");
    TrackProgressEstimator estimator;
    Require(estimator.Configure(&track), "on-track projection continuity");
    const auto a = estimator.Observe({0.0, -1800.0}, 0.0, 1.0/60.0);
    const auto b = estimator.Observe({60.0, -1800.0}, 0.0, 1.0/60.0);
    Require(a.Accepted && b.Accepted && b.SignedTravelCm > 0.0,
            "moving forward is continuous and not scoring a lap");
}

void TestBrakePerformance()
{
    using namespace latebrake::engineering;
    BrakeEventMonitor brake;
    Require(brake.Configure({}), "brake settings validated before telemetry");
    TelemetryKinematics kinematics;
    kinematics.Valid = true;
    kinematics.LongitudinalAccelerationCmS2 = -625.0;
    kinematics.SpeedCmS = 1700.0;
    TrackProjection track;
    track.Valid = true;
    track.Segment = 3;
    track.DistanceAlongCm = 1800.0;
    auto f = Frame(1, 1.0, 0.0);
    for (int i = 0; i < 56; ++i)
    {
        const double dt = 1.0 / 60.0;
        f.MonotonicSeconds += dt;
        f.Controls.Brake = i < 30 ? 0.82 : 0.0;
        f.Controls.Steering = i < 30 ? 0.35 : 0.0;
        f.Dynamics.BrakeTemperatureCelsius = 450.0 + i * 0.3;
        f.Wheels.Contact.fill(true);
        f.Wheels.NormalLoadNewtons.fill(3500.0);
        f.Wheels.SlipRatio.fill(i < 30 ? -0.35 : 0.0);
        kinematics.SpeedCmS = 1700.0 - i * 12.0;
        track.DistanceAlongCm += kinematics.SpeedCmS * dt;
        Require(brake.Observe(f, kinematics, track, dt),
                "qualified braking frame never fabricates physics");
    }
    const auto event = brake.LastEvent();
    Require(brake.RecordedEvents() == 1 && event.Valid &&
            event.DurationSeconds > 0.5 &&
            event.PeakPedal > 0.8 &&
            event.WheelLockObserved &&
            event.LostKineticEnergyPerKg > 0.0 &&
            event.DistanceCm > 0.0,
            "brake event records physical pedal/deceleration and release");
    TelemetryZone zone;
    zone.Valid = true;
    zone.StartCm = 1000.0;
    zone.CenterCm = 4000.0;
    const BrakeZoneAssessment review =
        AssessBrakeZone(event, zone, 650.0, 0.85);
    Require(review.Valid && review.GripEstimateQualified &&
            review.GripLimitedDecelerationCmS2 < 650.0 &&
            review.PredictedStoppingDistanceCm > 0.0,
            "wear-aware braking analysis remains read-only");
    BrakeZoneHistory history;
    Require(history.Record(0, review, event) &&
            history.ZoneCount() == 1 &&
            history.MeanBrakeDecelerationG(0) >= 0.0,
            "brake-zone comparison accepts real completed event");
    auto bad = event;
    bad.Valid = false;
    Require(!history.Record(0, review, bad),
            "unfinished event cannot corrupt zone baseline");
}

void TestCornerPerformance()
{
    using namespace latebrake::engineering;
    CornerEventObserver observer;
    Require(observer.Configure({}), "valid corner observation settings");
    auto frame = Frame(1, 1.0, 0.0);
    TelemetryKinematics motion;
    motion.Valid = true;
    TrackProjection track;
    track.Valid = true;
    track.Segment = 0;
    track.DistanceAlongCm = 200.0;
    for (int i = 0; i < 75; ++i)
    {
        const bool turning = i < 44;
        frame.MonotonicSeconds += 1.0 / 60.0;
        frame.Controls.Steering = turning ? 0.68 : 0.0;
        frame.Controls.Brake = i < 16 ? 0.28 : 0.0;
        frame.Controls.Throttle = i > 38 ? 0.60 : 0.0;
        motion.SpeedCmS = i < 33 ?
            1200.0 - i * 9.0 : 900.0 + (i - 33) * 10.0;
        motion.YawRateRadS = turning ? 0.45 : 0.0;
        motion.LateralAccelerationCmS2 = turning ? 650.0 : 0.0;
        motion.SlipAngleRadians = turning ? 0.14 : 0.01;
        track.CurvaturePerCm = turning ? 0.00020 : 0.0;
        track.DistanceAlongCm += motion.SpeedCmS / 60.0;
        Require(observer.Observe(frame, motion, track, 1.0 / 60.0),
                "measured corner entry, apex, exit frames accepted");
    }
    const CornerEventRecord event = observer.LastEvent();
    Require(observer.QualifiedEvents() >= 1 && event.Valid &&
            event.Direction == CornerDirection::Left &&
            event.ApexObserved && event.DurationSeconds >= 0.4 &&
            event.CompositeReviewScore >= 0.0 &&
            event.CompositeReviewScore <= 100.0,
            "observed corner is qualified without modifying steering");
    CornerHistory history;
    Require(history.Record(0, event) &&
            history.SampleCount(0) == 1 &&
            history.AverageApexSpeed(0) > 0.0,
            "best and average corner reviews preserve distinct samples");
}

void TestGripAndTireLoad()
{
    using namespace latebrake::engineering;
    auto f = Frame(1, 1.0, 0.0);
    f.Wheels.Contact.fill(true);
    f.Wheels.NormalLoadNewtons.fill(3200.0);
    f.Wheels.AngularSpeedRadS.fill(50.0);
    f.Wheels.TireTemperatureCelsius.fill(105.0);
    f.Wheels.TireWearFraction.fill(0.2);
    f.Wheels.SlipRatio.fill(0.98);
    f.Wheels.SlipAngleRadians.fill(0.43);
    TelemetryKinematics motion;
    motion.Valid = true;
    motion.SpeedCmS = 1450.0;
    motion.LongitudinalAccelerationCmS2 = 480.0;
    motion.LateralAccelerationCmS2 = 650.0;
    const auto grip = EvaluateGripTelemetry(f, motion, {});
    Require(grip.Valid && grip.ContactFraction == 1.0 &&
            grip.MaximumCombinedSaturation > 1.0 &&
            grip.State == GripState::Sliding &&
            grip.LoadWeightedGripFactor > 0.0 &&
            grip.EstimatedRequiredFriction > 0.0,
            "four engine-observed tires produce a traction diagnostic");
    GripTelemetryMonitor monitor;
    Require(monitor.Configure({}), "valid grip sensor thresholds");
    for (int i = 0; i < 24; ++i)
    {
        f.MonotonicSeconds += 1.0 / 60.0;
        Require(monitor.Observe(f, motion, 1.0 / 60.0),
                "bounded slip/heat monitoring rejects no valid samples");
    }
    Require(monitor.History().SlidingFrames == 24 &&
            monitor.SlidingEventCount() == 1 &&
            monitor.FractionOfTimeInSlide() > 0.99,
            "grip metrics accumulate duration not guessed physics");
    f.Wheels.NormalLoadNewtons.fill(0.0);
    Require(!monitor.Observe(f, motion, 1.0 / 60.0),
            "unsupported wheels are not estimated as grounded tires");
}

void TestSixCarOpponentReview()
{
    using namespace latebrake::engineering;
    OpponentObservation self;
    self.Slot = 0;
    self.ActorToken = 1200;
    self.PositionCm = {-2000.0, -1800.0};
    self.VelocityCmS = {800.0, 0.0};
    self.EngineVerified = true;
    self.OnPhysicalTrack = true;
    std::array<OpponentObservation, 5> others{};
    for (int i = 0; i < 5; ++i)
    {
        OpponentObservation other;
        other.Slot = i + 1;
        other.ActorToken = static_cast<std::uint64_t>(1201 + i);
        other.PositionCm = {6500.0 + i * 300.0, 1800.0};
        other.EngineVerified = true;
        other.OnPhysicalTrack = true;
        others[static_cast<std::size_t>(i)] = other;
    }
    others[0].PositionCm = {-1000.0, -1800.0};
    others[0].VelocityCmS = {-250.0, 0.0};
    const auto encounter =
        EvaluateOpponentEncounter(self, others[0], {});
    Require(encounter.Qualified &&
            encounter.IsAhead &&
            encounter.PredictedContact &&
            encounter.TimeToContactSeconds > 0.0 &&
            encounter.TimeToContactSeconds < 1.5,
            "measured closing trajectory predicts imminent proximity");
    FleetTelemetryMonitor fleet;
    Require(fleet.Configure({}), "bounded source-only six-car observation");
    Require(fleet.Observe(self, others.data(), 5, 42, 1.0, 1.0 / 60.0),
            "exact six distinct verified physical identities accepted");
    auto snapshot = fleet.Snapshot();
    Require(snapshot.Valid && snapshot.CompletePhysicalRoster &&
            snapshot.QualifiedOpponents == 5 &&
            snapshot.MostUrgentSlot == 1 &&
            snapshot.PredictedContacts >= 1,
            "five separate identities keep their own risk and history");
    std::swap(others[0], others[4]);
    Require(fleet.Observe(self, others.data(), 5, 42, 1.05, 0.05) &&
            fleet.Snapshot().MostUrgentSlot == 1,
            "physical actor order cannot change closest real opponent");
    others[2].ActorToken = others[3].ActorToken;
    Require(!fleet.Observe(self, others.data(), 5, 42, 1.10, 0.05),
            "duplicate Chaos actor identities reject complete fleet review");
    Require(fleet.RejectedObservations() >= 1,
            "invalid fleet attempt is audited, never silently omitted");
}

void TestLargeDeterministicFrameSweep()
{
    using namespace latebrake::engineering;
    TelemetryFrameGate gate;
    TelemetryKinematicEstimator estimator;
    std::uint32_t seed = 0xA5723U;
    const auto random32 = [&seed]() noexcept
    {
        seed = seed * 1664525U + 1013904223U;
        return seed;
    };
    double x = 0.0;
    double time = 0.0;
    constexpr double dt = 1.0 / 60.0;
    for (int i = 1; i <= 30000; ++i)
    {
        const double speed = 400.0 +
            static_cast<double>(random32() % 301U);
        x += speed * dt;
        time += dt;
        TelemetryFrame f = Frame(i, time, x);
        f.Dynamics.WorldVelocityCmS = {speed, 0.0, 0.0};
        f.SpeedometerKmH = speed * CmPerSecondToKmh;
        const auto decision = gate.Observe(f);
        Require(decision.Accepted && decision.Reason ==
                    TelemetryReject::None,
                "bounded deterministic real-frame motion remains finite");
        Require(estimator.Observe(f, decision),
                "review-only state estimates survive 30k frames");
        const auto output = estimator.Snapshot();
        Require(output.Valid && std::isfinite(output.DistanceCm) &&
                output.DistanceCm >= 0.0 &&
                output.SpeedCmS >= 0.0 &&
                output.SpeedCmS <= MaximumPlanarSpeedCmS,
                "no NaN, overflow or speculative motor command");
    }
    Require(gate.Snapshot().Accepted == 30000 &&
            estimator.QualifiedFrames() == 30000,
            "30k independent snapshots preserve exact sample count");
}
} // namespace

int main()
{
    TestMath();
    TestFrameGateAndMotion();
    TestStatisticsAndFilters();
    TestOriginalTrack();
    TestBrakePerformance();
    TestCornerPerformance();
    TestGripAndTireLoad();
    TestSixCarOpponentReview();
    TestLargeDeterministicFrameSweep();
    std::cout << "Race engineering analysis and deterministic replays passed.\n";
    return EXIT_SUCCESS;
}
