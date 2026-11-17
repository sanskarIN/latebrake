#include "Track/CircuitPaceAudit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass)
    {
        std::cerr << "CircuitPaceAudit FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double x, double y, double epsilon = 1.0e-6)
{
    return std::isfinite(x) && std::isfinite(y) &&
        std::abs(x - y) <= epsilon;
}

latebrake::CircuitPaceAuditSample OnCircuit(
    std::uint64_t frame, double progress,
    double interval, double nominalSpeed)
{
    using namespace latebrake;
    const double before = progress - 0.5 * interval * nominalSpeed;
    const double after = progress + 0.5 * interval * nominalSpeed;
    const auto current = WorldScaleCenterlineAt(progress);
    const auto preceding = WorldScaleCenterlineAt(before);
    const auto following = WorldScaleCenterlineAt(after);
    Require(current.Valid && preceding.Valid && following.Valid,
            "original curved world-scale road centerline must exist");
    CircuitPaceAuditSample result{};
    result.SessionIdentity = 14;
    result.ActorIdentity = 212;
    result.FrameOrdinal = frame;
    result.DeltaSeconds = interval;
    result.WorldX = current.Pose.Position.X;
    result.WorldY = current.Pose.Position.Y;
    result.WorldVelocityX = (
        following.Pose.Position.X - preceding.Pose.Position.X) / interval;
    result.WorldVelocityY = (
        following.Pose.Position.Y - preceding.Pose.Position.Y) / interval;
    result.ReportedLapDistanceCm = std::fmod(
        progress, WorldScaleCircuitLapCm);
    if (result.ReportedLapDistanceCm < 0.0)
        result.ReportedLapDistanceCm += WorldScaleCircuitLapCm;
    result.ForwardThrottle = 0.7;
    result.SteeringAxis = 0.0;
    result.ActiveHumanRace = true;
    result.QualifiedRoadSupport = true;
    return result;
}

void NormalForwardSequence()
{
    using namespace latebrake;
    CircuitPaceAudit recorder;
    Require(recorder.Plan().Valid && !recorder.Readout().Valid,
            "source-only audit owns qualified immutable planned circuit");
    const double dt = 0.05;
    double distance = 15000.0;
    const double speed = 2400.0;
    auto first = recorder.Observe(OnCircuit(1, distance, dt, speed));
    Require(first.Valid && first.RouteValid && first.OnAuthoredRoad &&
            !first.MotionContinuous &&
            Near(first.ContinuousObservedSeconds, 0.0) &&
            first.AcceptedContinuityFrames == 0 &&
            Near(first.SuggestedDecelerationCmPerSecondSquared, 0.0),
            "first physics frame only establishes a new actor baseline");
    for (std::uint64_t i = 2; i <= 40; ++i)
    {
        distance += dt * speed;
        const auto reading = recorder.Observe(
            OnCircuit(i, distance, dt, speed));
        Require(reading.Valid && reading.MotionContinuous &&
                reading.OnAuthoredRoad &&
                reading.AcceptedContinuityFrames == i - 1 &&
                reading.SuggestedDecelerationCmPerSecondSquared >= 0.0 &&
                reading.SuggestedDecelerationCmPerSecondSquared <=
                    WorldScalePaceBrakeAccelCmps2,
                "physical movement in original lane qualifies sequential evidence");
    }
    Require(Near(recorder.Readout().ContinuousObservedSeconds,
                 39.0 * dt, 1.0e-5) &&
            recorder.Readout().TimeAbovePaceSeconds >= 0.0,
            "continuous accepted time is the sum of observed adjacent intervals");
}

void MalformedAndUntrustedSamples()
{
    using namespace latebrake;
    CircuitPaceAudit recorder;
    const auto good = OnCircuit(1, 5000.0, 0.05, 2000.0);
    Require(recorder.Observe(good).Valid, "baseline must be good");
    for (int variant = 0; variant < 16; ++variant)
    {
        auto bad = good;
        bad.FrameOrdinal = 2;
        switch (variant)
        {
        case 0: bad.ActiveHumanRace = false; break;
        case 1: bad.QualifiedRoadSupport = false; break;
        case 2: bad.SessionIdentity = 0; break;
        case 3: bad.ActorIdentity = 0; break;
        case 4: bad.FrameOrdinal = 0; break;
        case 5: bad.DeltaSeconds = 0.0; break;
        case 6: bad.DeltaSeconds = 0.11; break;
        case 7: bad.DeltaSeconds =
            std::numeric_limits<double>::quiet_NaN(); break;
        case 8: bad.WorldX =
            std::numeric_limits<double>::infinity(); break;
        case 9: bad.WorldY = 1000001.0; break;
        case 10: bad.WorldVelocityX = 16000.0; break;
        case 11: bad.WorldVelocityY =
            std::numeric_limits<double>::quiet_NaN(); break;
        case 12: bad.ForwardThrottle = 1.2; break;
        case 13: bad.SteeringAxis = -1.1; break;
        case 14: bad.ReportedLapDistanceCm = -0.01; break;
        case 15: bad.ReportedLapDistanceCm = WorldScaleCircuitLapCm; break;
        default: Require(false, "unsupported mutation");
        }
        const auto out = recorder.Observe(bad);
        Require(!out.Valid && !out.MotionContinuous &&
                out.SuggestedDecelerationCmPerSecondSquared == 0.0 &&
                out.RejectedSamples >= 1,
                "bad or untrusted real physics cannot retain qualified advice");
    }
    Require(recorder.Readout().RejectedSamples == 16,
            "each rejected provenance mutation is visible to diagnostic caller");
    const auto restart = recorder.Observe(good);
    Require(restart.Valid && !restart.MotionContinuous &&
            restart.AcceptedContinuityFrames == 0,
            "valid telemetry after rejection begins new continuity baseline");
}

void RouteProgressAndIdentityGuards()
{
    using namespace latebrake;
    CircuitPaceAudit recorder;
    const double dt = 0.05;
    const double speed = 2500.0;
    const auto a = recorder.Observe(OnCircuit(1, 9000.0, dt, speed));
    Require(a.Valid && !a.MotionContinuous,
            "fresh actor cannot be credited for nonexistent predecessor");
    const auto b = recorder.Observe(OnCircuit(2, 9125.0, dt, speed));
    Require(b.Valid && b.MotionContinuous,
            "plausible continuous movement is accepted");
    auto forgedRoute = OnCircuit(3, 9250.0, dt, speed);
    forgedRoute.ReportedLapDistanceCm += 800.0;
    const auto rejected = recorder.Observe(forgedRoute);
    Require(!rejected.Valid && rejected.RejectedSamples == 1,
            "lane geometry cannot certify fabricated track progress");
    auto after = recorder.Observe(OnCircuit(4, 9375.0, dt, speed));
    Require(after.Valid && !after.MotionContinuous,
            "recovery after bad route information establishes baseline");
    auto actorSwap = OnCircuit(5, 9500.0, dt, speed);
    actorSwap.ActorIdentity = 213;
    after = recorder.Observe(actorSwap);
    Require(after.Valid && !after.MotionContinuous &&
            after.AcceptedContinuityFrames == 0 &&
            after.RejectedSamples == 0,
            "new genuine actor identity cannot borrow previous run evidence");
    auto sessionSwap = OnCircuit(6, 9625.0, dt, speed);
    sessionSwap.ActorIdentity = 213;
    sessionSwap.SessionIdentity = 15;
    after = recorder.Observe(sessionSwap);
    Require(after.Valid && !after.MotionContinuous &&
            Near(after.ContinuousObservedSeconds, 0.0),
            "new race session cannot inherit an older session's sampled time");
}

void TeleportAndUnobservedGaps()
{
    using namespace latebrake;
    CircuitPaceAudit recorder;
    const double dt = 0.05;
    const double speed = 2200.0;
    recorder.Observe(OnCircuit(1, 12000.0, dt, speed));
    auto good = recorder.Observe(OnCircuit(2, 12110.0, dt, speed));
    Require(good.MotionContinuous, "two moving physical samples form baseline");
    // A legal lane position and plausible velocity alone do not explain
    // an instantaneous jump across the track between recorded frames.
    auto jump = recorder.Observe(OnCircuit(3, 14900.0, dt, speed));
    Require(jump.Valid && !jump.MotionContinuous &&
            jump.SuggestedDecelerationCmPerSecondSquared == 0.0,
            "on-asphalt teleport cannot earn route or pace continuity");
    auto recovered = recorder.Observe(OnCircuit(4, 15010.0, dt, speed));
    Require(recovered.Valid && recovered.MotionContinuous &&
            recovered.AcceptedContinuityFrames == 1,
            "teleport restarts elapsed-time accounting from live motion");
    auto skipped = recorder.Observe(OnCircuit(6, 15230.0, dt, speed));
    Require(skipped.Valid && !skipped.MotionContinuous,
            "dropped ordinal must not claim unseen physics frames");
    recorder.Suspend();
    Require(!recorder.Readout().Valid &&
            recorder.Readout().AcceptedContinuityFrames == 0,
            "pause/focus loss/restart clears stale diagnostic state");
}

void FrameRateAndLapSeamInvariance()
{
    using namespace latebrake;
    for (const int fps : {30, 60, 120})
    {
        CircuitPaceAudit audit;
        const double dt = 1.0 / static_cast<double>(fps);
        const double velocity = 2250.0;
        const double origin = 5000.0;
        const auto baseline = audit.Observe(OnCircuit(1, origin, dt, velocity));
        Require(baseline.Valid && !baseline.MotionContinuous,
                "frame cadence baseline independent of rendering speed");
        for (int frame = 2; frame <= fps + 1; ++frame)
        {
            const double progress = origin +
                static_cast<double>(frame - 1) * dt * velocity;
            const auto response = audit.Observe(OnCircuit(
                static_cast<std::uint64_t>(frame),
                progress, dt, velocity));
            Require(response.Valid && response.MotionContinuous,
                    "30/60/120 Hz path advances from independently measured world positions");
        }
        Require(Near(audit.Readout().ContinuousObservedSeconds, 1.0, 1e-6),
                "one measured second has same total audited duration at each cadence");
    }
    const double lap = WorldScaleCircuitLapCm;
    Require(Near(CircularCircuitDistanceDifference(lap - 30.0, 20.0),
                 50.0) &&
            Near(SignedCircuitAdvance(lap - 30.0, 20.0), 50.0),
            "full-lap seam uses circular route progress rather than huge jumps");
    Require(std::isnan(CircularCircuitDistanceDifference(
                lap, 20.0)) &&
            std::isnan(SignedCircuitAdvance(-1.0, 20.0)),
            "invalid lap distance never becomes a valid wraparound observation");
}
} // namespace

int main()
{
    NormalForwardSequence();
    MalformedAndUntrustedSamples();
    RouteProgressAndIdentityGuards();
    TeleportAndUnobservedGaps();
    FrameRateAndLapSeamInvariance();
    std::cout << "Circuit pace audit: genuine source continuity and fail-closed QA passed\n";
    return EXIT_SUCCESS;
}
