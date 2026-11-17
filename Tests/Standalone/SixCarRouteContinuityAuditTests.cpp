#include "AI/SixCarRouteContinuityAudit.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "SixCarRouteContinuityAudit FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Close(double a, double b, double tolerance = 1.0e-5)
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a - b) < tolerance;
}

using Stations = std::array<double, latebrake::SixCarRouteAuditCapacity>;
using Speeds = std::array<double, latebrake::SixCarRouteAuditCapacity>;

latebrake::SixCarRouteActorSample ActorAt(
    int slot, double station, double deltaSeconds, double speed)
{
    using namespace latebrake;
    const auto current = WorldScaleCenterlineAt(station);
    const auto preceding = WorldScaleCenterlineAt(
        station - 0.5 * deltaSeconds * speed);
    const auto following = WorldScaleCenterlineAt(
        station + 0.5 * deltaSeconds * speed);
    Check(current.Valid && preceding.Valid && following.Valid,
          "authored world-scale route centerline is usable");
    auto wrapped = std::fmod(station, WorldScaleCircuitLapCm);
    if (wrapped < 0.0) wrapped += WorldScaleCircuitLapCm;
    Check(station >= 0.0, "test lap totals must never be negative");
    SixCarRouteActorSample a{};
    a.GridSlot = slot;
    a.ActorIdentity = 1000 + static_cast<std::uint64_t>(slot);
    a.CompletedLaps = static_cast<unsigned>(
        std::floor(station / WorldScaleCircuitLapCm));
    a.ReportedLapProgressCm = wrapped;
    a.WorldX = current.Pose.Position.X;
    a.WorldY = current.Pose.Position.Y;
    a.VelocityX = (following.Pose.Position.X -
                   preceding.Pose.Position.X) / deltaSeconds;
    a.VelocityY = (following.Pose.Position.Y -
                   preceding.Pose.Position.Y) / deltaSeconds;
    a.QualifiedRoadContact = true;
    return a;
}

latebrake::SixCarRouteFrameSample Fleet(
    std::uint64_t ordinal, const Stations& distances,
    const Speeds& speeds, double dt)
{
    latebrake::SixCarRouteFrameSample f{};
    f.FrameOrdinal = ordinal;
    f.SessionIdentity = 42;
    f.ActiveRace = true;
    f.DeltaSeconds = dt;
    for (std::size_t i = 0; i < latebrake::SixCarRouteAuditCapacity; ++i)
    {
        f.Actors[i] = ActorAt(static_cast<int>(i),
            distances[i], dt, speeds[i]);
    }
    return f;
}

void Advance(Stations& station, const Speeds& speeds, double dt)
{
    for (std::size_t i = 0; i < station.size(); ++i)
    {
        station[i] += speeds[i] * dt;
    }
}

void NormalSixCarMotion(double fps)
{
    using namespace latebrake;
    SixCarRouteContinuityAudit audit;
    Check(audit.HasQualifiedReference(),
          "qualified source-only original circuit race pace plan exists");
    const double dt = 1.0 / fps;
    Stations distances{{15000, 16300, 17600, 18900, 20200, 21500}};
    const Speeds speeds{{2300, 2150, 2050, 1950, 1800, 1700}};
    const auto first = audit.Observe(Fleet(1, distances, speeds, dt));
    Check(first.Valid && !first.MotionContinuous &&
          first.AcceptedContinuityFrames == 0 &&
          first.ConfirmedRelativePasses == 0 &&
          first.AdvisoryOrder.front() == 5,
          "first all-real-car roster establishes only a baseline");
    for (std::uint64_t frame = 2; frame <= 90; ++frame)
    {
        Advance(distances, speeds, dt);
        auto f = Fleet(frame, distances, speeds, dt);
        // The order of the source observation array cannot be an implied
        // race ranking or silently exclude the last physical rival.
        if ((frame % 2) == 0) std::reverse(f.Actors.begin(), f.Actors.end());
        const auto out = audit.Observe(f);
        Check(out.Valid && out.MotionContinuous &&
              out.AcceptedContinuityFrames == frame - 1 &&
              out.RejectedFrames == 0 &&
              out.CarsBySlot[0].ActorIdentity == 1000 &&
              out.CarsBySlot[5].ActorIdentity == 1005 &&
              out.AdvisoryOrder.front() == 5 &&
              out.ConfirmedRelativePasses == 0,
              "six cars keep independent identities, route positions and ordering");
        Check(Close(out.ContinuousObservedSeconds,
                    static_cast<double>(frame - 1) * dt, 1.0e-5),
              "verified elapsed time must come from consecutive observed frames");
    }
}

void InvalidAndIncompleteFleet()
{
    using namespace latebrake;
    SixCarRouteContinuityAudit audit;
    const double dt = 0.05;
    Stations station{{15000, 16000, 17000, 18000, 19000, 20000}};
    const Speeds speed{{2200, 2000, 2100, 2300, 2000, 1900}};
    const auto good = Fleet(1, station, speed, dt);
    Check(audit.Observe(good).Valid, "trusted six-car baseline accepted");
    unsigned rejected = 0;
    for (int variant = 0; variant < 22; ++variant)
    {
        auto f = good;
        f.FrameOrdinal = 2;
        switch (variant)
        {
        case 0: f.SessionIdentity = 0; break;
        case 1: f.FrameOrdinal = 0; break;
        case 2: f.ActiveRace = false; break;
        case 3: f.DeltaSeconds = 0.0; break;
        case 4: f.DeltaSeconds = 0.2; break;
        case 5: f.Actors[5].QualifiedRoadContact = false; break;
        case 6: f.Actors[5].ActorIdentity = 0; break;
        case 7: f.Actors[5].ActorIdentity = f.Actors[0].ActorIdentity; break;
        case 8: f.Actors[5].GridSlot = f.Actors[4].GridSlot; break;
        case 9: f.Actors[5].GridSlot = 6; break;
        case 10: f.Actors[5].GridSlot = -1; break;
        case 11: f.Actors[5].ReportedLapProgressCm = -1; break;
        case 12: f.Actors[5].ReportedLapProgressCm = WorldScaleCircuitLapCm; break;
        case 13: f.Actors[5].WorldX = std::numeric_limits<double>::infinity(); break;
        case 14: f.Actors[5].WorldY = 2000000.0; break;
        case 15: f.Actors[5].VelocityX = 20000.0; break;
        case 16: f.Actors[5].VelocityY = std::numeric_limits<double>::quiet_NaN(); break;
        case 17: f.Actors[5].CompletedLaps = SixCarRouteAuditMaxRecordedLaps + 1; break;
        case 18: f.Actors[5].ReportedLapProgressCm += 2000.0; break;
        case 19: f.Actors[5].WorldY += 5000.0; break;
        case 20: f.Actors[5].WorldX = std::numeric_limits<double>::quiet_NaN(); break;
        case 21: f.DeltaSeconds = std::numeric_limits<double>::quiet_NaN(); break;
        default: Check(false, "unhandled invalid-source case"); break;
        }
        const auto result = audit.Observe(f);
        ++rejected;
        Check(!result.Valid && !result.MotionContinuous &&
              result.ConfirmedRelativePasses == 0 &&
              result.AcceptedContinuityFrames == 0 &&
              result.RejectedFrames == rejected,
              "one bad or missing real rival invalidates entire six-car advisory");
    }
    station[0] += 100.0;
    const auto fresh = audit.Observe(Fleet(3, station, speed, dt));
    Check(fresh.Valid && !fresh.MotionContinuous &&
          fresh.AcceptedContinuityFrames == 0,
          "recovery after invalid roster cannot inherit motion seconds");
}

void FrozenAndStalePhysics(double fps)
{
    using namespace latebrake;
    const double dt = 1.0 / fps;
    const Stations initial{{15000, 16200, 17400, 18600, 19800, 21000}};
    const Speeds speeds{{2400, 2300, 2200, 2100, 2000, 1900}};

    // A chassis pose frozen at the previous position while its
    // reported velocity still says 19m/s MUST NOT pass a 120Hz sample
    // simply because the old 180cm fixed error budget exceeded the
    // actual ~16cm of physical motion between two frames.
    SixCarRouteContinuityAudit frozen;
    Check(frozen.Observe(Fleet(1, initial, speeds, dt)).Valid,
          "trusted physical baseline required");
    auto oldPosition = Fleet(2, initial, speeds, dt);
    const auto notMoving = frozen.Observe(oldPosition);
    Check(!notMoving.Valid && notMoving.RejectedFrames == 1,
          "stationary observed chassis cannot pass as moving at any frame rate");

    // Forging route progress while remaining at the same measured
    // world pose is a separate attack. Its apparent 30/60/120Hz step
    // is BELOW the broad 220cm road-projection tolerance, but must
    // still be rejected by successive-frame kinematics.
    SixCarRouteContinuityAudit falseProgress;
    Check(falseProgress.Observe(Fleet(1, initial, speeds, dt)).Valid,
          "baseline for independent forged progress case");
    oldPosition = Fleet(2, initial, speeds, dt);
    oldPosition.Actors[5].ReportedLapProgressCm += speeds[5] * dt;
    const auto lying = falseProgress.Observe(oldPosition);
    Check(!lying.Valid && lying.RejectedFrames == 1 &&
          lying.ConfirmedRelativePasses == 0,
          "reported route distance never substitutes actual moving chassis");

    // A real world motion with a stale source lap station is also
    // inconsistent even if the stationary route claim is geometrically
    // close enough to the projected world point at this frame.
    SixCarRouteContinuityAudit staleRoute;
    Check(staleRoute.Observe(Fleet(1, initial, speeds, dt)).Valid,
          "baseline for stale route-progress case");
    Stations moved = initial;
    Advance(moved, speeds, dt);
    auto stale = Fleet(2, moved, speeds, dt);
    stale.Actors[5].ReportedLapProgressCm =
        initial[5];
    const auto missingAdvance = staleRoute.Observe(stale);
    Check(!missingAdvance.Valid &&
          missingAdvance.AcceptedContinuityFrames == 0 &&
          missingAdvance.ConfirmedRelativePasses == 0,
          "moving real body cannot claim unchanged route distance");
}

void SignedRoadMotionCannotBeForged(double fps)
{
    using namespace latebrake;
    const double dt = 1.0 / fps;
    const Stations initial{{15000, 16300, 17600, 18900, 20200, 21500}};
    const Speeds speeds{{1900, 1800, 2100, 2000, 2200, 1700}};

    // Actual physical motion is backward, with truthful reverse world
    // velocities but a forged INCREASING route station. The geometric
    // route-projection gap is under 220cm at every tested frame rate.
    // Magnitude-only travel would incorrectly treat this as forward.
    SixCarRouteContinuityAudit reverse;
    auto reverseStart = Fleet(1, initial, speeds, dt);
    reverseStart.Actors[4].VelocityX *= -1.0;
    reverseStart.Actors[4].VelocityY *= -1.0;
    Check(reverse.Observe(reverseStart).Valid,
          "reverse-speed observation can establish neutral baseline");
    Stations backward = initial;
    Advance(backward, speeds, dt);
    backward[4] = initial[4] - speeds[4] * dt;
    auto forgedForward = Fleet(2, backward, speeds, dt);
    forgedForward.Actors[4].VelocityX *= -1.0;
    forgedForward.Actors[4].VelocityY *= -1.0;
    forgedForward.Actors[4].ReportedLapProgressCm =
        initial[4] + speeds[4] * dt;
    const auto wrongWay = reverse.Observe(forgedForward);
    Check(!wrongWay.Valid && wrongWay.RejectedFrames == 1 &&
          wrongWay.AcceptedContinuityFrames == 0 &&
          wrongWay.ConfirmedRelativePasses == 0,
          "honest reverse chassis cannot invent positive forward race distance");

    // A second actor slides across the track with its velocity 90deg
    // to the independent route tangent. Its position and measured
    // velocity AGREE in XY; only the claimed forward progress is fake.
    // This isolates route-direction truth from world-motion validation.
    const auto before = WorldScaleCenterlineAt(initial[3] - 100.0);
    const auto after = WorldScaleCenterlineAt(initial[3] + 100.0);
    Check(before.Valid && after.Valid,
          "future authored track has a valid local tangent");
    const double tangentX =
        after.Pose.Position.X - before.Pose.Position.X;
    const double tangentY =
        after.Pose.Position.Y - before.Pose.Position.Y;
    const double tangentLength = std::hypot(tangentX, tangentY);
    Check(tangentLength > 1.0, "valid physical tangent has length");
    const double normalX = -tangentY / tangentLength;
    const double normalY = tangentX / tangentLength;
    SixCarRouteContinuityAudit sideways;
    auto lateralStart = Fleet(1, initial, speeds, dt);
    lateralStart.Actors[3].VelocityX = normalX * speeds[3];
    lateralStart.Actors[3].VelocityY = normalY * speeds[3];
    Check(sideways.Observe(lateralStart).Valid,
          "sideways observation can establish source baseline");
    Stations noRouteAdvance = initial;
    Advance(noRouteAdvance, speeds, dt);
    noRouteAdvance[3] = initial[3];
    auto inventedForward = Fleet(2, noRouteAdvance, speeds, dt);
    inventedForward.Actors[3].WorldX += normalX * speeds[3] * dt;
    inventedForward.Actors[3].WorldY += normalY * speeds[3] * dt;
    inventedForward.Actors[3].VelocityX = normalX * speeds[3];
    inventedForward.Actors[3].VelocityY = normalY * speeds[3];
    inventedForward.Actors[3].ReportedLapProgressCm =
        initial[3] + speeds[3] * dt;
    const auto slipFraud = sideways.Observe(inventedForward);
    Check(!slipFraud.Valid && slipFraud.RejectedFrames == 1 &&
          slipFraud.ConfirmedRelativePasses == 0,
          "real sideways slip cannot pretend to advance along the circuit");

    // Legitimate forward motion with a moderate lateral chassis slip
    // is still accepted: the signed longitudinal component agrees
    // with actual independently projected route progress.
    SixCarRouteContinuityAudit drift;
    auto first = Fleet(1, initial, speeds, dt);
    constexpr double SlipCmps = 330.0;
    first.Actors[3].VelocityX += normalX * SlipCmps;
    first.Actors[3].VelocityY += normalY * SlipCmps;
    Check(drift.Observe(first).Valid, "physical forward drift establishes baseline");
    Stations next = initial;
    Advance(next, speeds, dt);
    auto second = Fleet(2, next, speeds, dt);
    second.Actors[3].WorldX += normalX * SlipCmps * dt;
    second.Actors[3].WorldY += normalY * SlipCmps * dt;
    second.Actors[3].VelocityX += normalX * SlipCmps;
    second.Actors[3].VelocityY += normalY * SlipCmps;
    const auto realDrift = drift.Observe(second);
    Check(realDrift.Valid && realDrift.MotionContinuous &&
          realDrift.RejectedFrames == 0 &&
          realDrift.AcceptedContinuityFrames == 1,
          "independently measured moderate physical drift retains forward evidence");
}

void TeleportsAndLapFraud()
{
    using namespace latebrake;
    const double dt = 0.05;
    const Speeds speed{{2000, 2000, 2000, 2000, 2000, 2000}};
    const Stations initial{{20000, 21200, 22400, 23600, 24800, 26000}};
    for (int mutation = 0; mutation < 5; ++mutation)
    {
        SixCarRouteContinuityAudit audit;
        Stations position = initial;
        Check(audit.Observe(Fleet(1, position, speed, dt)).Valid,
              "baseline is valid prior to hostile route mutation");
        Advance(position, speed, dt);
        auto second = Fleet(2, position, speed, dt);
        switch (mutation)
        {
        case 0: second.Actors[4].WorldX += 1200.0; break;
        case 1: second.Actors[4].ReportedLapProgressCm += 1000.0; break;
        case 2: ++second.Actors[4].CompletedLaps; break;
        case 3: second.Actors[4].VelocityX = 14999.0; break;
        case 4: second.Actors[4].ReportedLapProgressCm =
                    initial[4] - 800.0; break;
        default: Check(false, "invalid attack matrix"); break;
        }
        const auto bad = audit.Observe(second);
        Check(!bad.Valid && bad.RejectedFrames == 1 &&
              bad.AcceptedContinuityFrames == 0 &&
              bad.ConfirmedRelativePasses == 0,
              "physical displacement, speed or reported lap fraud cannot earn passes");
        const auto restored = audit.Observe(Fleet(3, position, speed, dt));
        Check(restored.Valid && !restored.MotionContinuous,
              "after a failed frame, all six identities must re-baseline");
    }
}

void EpochIdentityAndFrameGaps()
{
    using namespace latebrake;
    SixCarRouteContinuityAudit audit;
    const double dt = 0.05;
    Stations position{{15000, 16500, 18000, 19500, 21000, 22500}};
    const Speeds speed{{1800, 1800, 1800, 1800, 1800, 1800}};
    Check(audit.Observe(Fleet(1, position, speed, dt)).Valid,
          "epoch identity baseline exists");
    Advance(position, speed, dt);
    Check(audit.Observe(Fleet(2, position, speed, dt)).MotionContinuous,
          "second qualifying frame is continuous");
    Advance(position, speed, dt);
    auto missing = audit.Observe(Fleet(4, position, speed, dt));
    Check(missing.Valid && !missing.MotionContinuous &&
          missing.AcceptedContinuityFrames == 0 &&
          missing.ConfirmedRelativePasses == 0,
          "dropped frame cannot create unseen race telemetry");
    Advance(position, speed, dt);
    auto replaced = Fleet(5, position, speed, dt);
    replaced.Actors[3].ActorIdentity = 9000003;
    missing = audit.Observe(replaced);
    Check(missing.Valid && !missing.MotionContinuous &&
          missing.AcceptedContinuityFrames == 0,
          "new actual car may form baseline but inherits no previous passes");
    Advance(position, speed, dt);
    auto session = Fleet(6, position, speed, dt);
    session.SessionIdentity = 43;
    missing = audit.Observe(session);
    Check(missing.Valid && !missing.MotionContinuous &&
          missing.AcceptedContinuityFrames == 0 &&
          missing.ContinuousObservedSeconds == 0.0,
          "new race session cannot borrow older actor continuity");
    audit.Suspend();
    Check(!audit.Readout().Valid &&
          audit.Readout().AcceptedContinuityFrames == 0,
          "pause and focus loss explicitly discard all session evidence");
}

void FullLapBoundary()
{
    using namespace latebrake;
    const double dt = 0.05;
    const double lap = WorldScaleCircuitLapCm;
    Stations position{{lap-610, lap-510, lap-410,
                        lap-310, lap-210, lap-110}};
    const Speeds speeds{{2000, 2000, 2000, 2000, 2000, 2000}};
    SixCarRouteContinuityAudit audit;
    Check(audit.Observe(Fleet(1, position, speeds, dt)).Valid,
          "near-seam source observation accepted");
    for (std::uint64_t frame = 2; frame <= 10; ++frame)
    {
        Advance(position, speeds, dt);
        const auto out = audit.Observe(Fleet(frame, position, speeds, dt));
        Check(out.Valid && out.MotionContinuous &&
              out.RejectedFrames == 0 &&
              out.AcceptedContinuityFrames == frame - 1,
              "real lap count and wrapped route distance remain continuous at seam");
    }
    Check(audit.Readout().CarsBySlot[5].TotalReportedProgressCm > lap,
          "unwrapped physical-route observations can continue onto next lap");
}

void PassHysteresis(double fps)
{
    using namespace latebrake;
    SixCarRouteContinuityAudit audit;
    const double dt = 1.0 / fps;
    Stations position{{15500, 15750, 18100, 20100, 22100, 24100}};
    const Speeds speeds{{3200, 1400, 1400, 1400, 1400, 1400}};
    Check(audit.Observe(Fleet(1, position, speeds, dt)).Valid,
          "two cars have initial coherent physical gap");
    unsigned recorded = 0;
    for (std::uint64_t frame = 2; frame <= 40; ++frame)
    {
        Advance(position, speeds, dt);
        const auto out = audit.Observe(Fleet(frame, position, speeds, dt));
        Check(out.Valid && out.MotionContinuous &&
              out.ConfirmedRelativePasses <= 1,
              "continuously measured pairwise reversal is at most once per frame");
        recorded += out.ConfirmedRelativePasses;
    }
    Check(recorded == 1,
          "one real ordered overtake must cross a 35cm hysteresis only once");
}

} // namespace

int main()
{
    static_assert(latebrake::SixCarRouteAuditCapacity == 6);
    static_assert(latebrake::SixCarRouteAuditPassHysteresisCm > 0.0);
    for (double fps : {30.0, 60.0, 120.0})
    {
        NormalSixCarMotion(fps);
        FrozenAndStalePhysics(fps);
        SignedRoadMotionCannotBeForged(fps);
        PassHysteresis(fps);
    }
    InvalidAndIncompleteFleet();
    TeleportsAndLapFraud();
    EpochIdentityAndFrameGaps();
    FullLapBoundary();
    std::cout
        << "Six-car route/provenance source audit: 30/60/120Hz, six actors, "
        << "permuted roster, lap seam, hysteresis, identity, and safety "
        << "regressions passed; NOT Unreal physics/gameplay evidence\n";
}
