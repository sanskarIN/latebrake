#include "Telemetry/TelemetryMath.h"
#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryValidation.h"
#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryTrack.h"

#include <cmath>
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
    Require(NearlyEqual(Length({3.0, 4.0}), 5.0),
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
    auto jump = Frame(3, 2.0, 12000.0);
    Require(!gate.Observe(jump).Accepted,
            "huge time jump is rejected by frame clock");
    auto corrupt = Frame(4, 2.1, 0.0);
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
} // namespace

int main()
{
    TestMath();
    TestFrameGateAndMotion();
    TestStatisticsAndFilters();
    TestOriginalTrack();
    std::cout << "Race engineer telemetry foundations passed.\n";
    return EXIT_SUCCESS;
}
