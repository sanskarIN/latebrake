#include "AI/RivalFleetRiskSupervisor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "RivalFleetRiskSupervisor FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double a, double b, double eps = 1.0e-8)
{
    return std::abs(a - b) <= eps;
}

latebrake::RivalFleetTrafficSample ValidSixCars()
{
    using namespace latebrake;
    RivalFleetTrafficSample s{};
    s.SessionQualified = true;
    s.BlockerCount = MaxRecoveryObstacles;
    s.Self = {true, 1, 1001, {{-2500.0, SouthY}, 0.0}, 1100.0, 0.0};
    s.Blockers[0] = {true, 0, 1000, {{2000.0, SouthY}, 0.0}, 0.0, 0.0};
    s.Blockers[1] = {true, 2, 1002, {{2800.0, SouthY}, 0.0}, 0.0, 0.0};
    s.Blockers[2] = {true, 3, 1003, {{2800.0, 0.0}, 90.0}, 0.0, 0.0};
    s.Blockers[3] = {true, 4, 1004, {{0.0, NorthY}, 180.0}, 0.0, 0.0};
    s.Blockers[4] = {true, 5, 1005, {{-2800.0, NorthY}, -90.0}, 0.0, 0.0};
    return s;
}

void TestExactPhysicalRoster()
{
    using namespace latebrake;
    auto s = ValidSixCars();
    std::array<std::uint64_t, StartGridCapacity> tokens{};
    Require(ValidateSixCarFleet(s, tokens), "all six observed physical actors");
    Require(tokens[0] == 1000 && tokens[5] == 1005,
            "slot sorted map built from genuine actor tokens");
    Require(ForecastSixCarRisk(s).Kind == FleetRiskKind::Clear,
            "separated six-car snapshot does not invent a threat");

    auto bad = s;
    bad.BlockerCount = 4;
    Require(!ValidateSixCarFleet(bad, tokens),
            "five cars with a missing rival cannot drive");
    bad = s;
    bad.BlockerCount = 6;
    Require(!ValidateSixCarFleet(bad, tokens),
            "overflowed array count must not be indexed");
    bad = s;
    bad.SessionQualified = false;
    Require(!ForecastSixCarRisk(bad).Qualified(),
            "interrupted session cannot reuse prior traffic samples");
    bad = s;
    bad.Blockers[4].ActorToken = bad.Blockers[1].ActorToken;
    Require(!ValidateSixCarFleet(bad, tokens),
            "two real cars cannot share one actor identity");
    bad = s;
    bad.Blockers[4].Slot = bad.Blockers[1].Slot;
    Require(!ValidateSixCarFleet(bad, tokens),
            "two actor tokens cannot claim the same start-grid slot");
    bad = s;
    bad.Blockers[4].ActorToken = bad.Self.ActorToken;
    Require(!ValidateSixCarFleet(bad, tokens),
            "rival cannot masquerade as its own blocker");
    bad = s;
    bad.Blockers[3].Verified = false;
    Require(!ValidateSixCarFleet(bad, tokens),
            "physically unverified blocker is never an empty lane");
    bad = s;
    bad.Blockers[3].Pose.Position = {0.0, 0.0};
    Require(!ValidateSixCarFleet(bad, tokens),
            "unpaved middle of the circuit rejects propulsion");
    bad = s;
    bad.Self.Pose.Position = {0.0, 0.0};
    Require(!ValidateSixCarFleet(bad, tokens),
            "unpaved rival has no valid race traffic scene");
    bad = s;
    bad.Self.VelocityX = std::numeric_limits<double>::infinity();
    Require(!ValidateSixCarFleet(bad, tokens),
            "infinite self velocity fails before collision projections");
    bad = s;
    bad.Blockers[2].VelocityY =
        std::numeric_limits<double>::quiet_NaN();
    Require(!ForecastSixCarRisk(bad).Qualified(),
            "nonfinite later blocker invalidates entire road decision");
    bad = s;
    bad.Blockers[2].ActorToken = 0;
    Require(!ForecastSixCarRisk(bad).Qualified(),
            "anonymous body is not a valid physical opponent");

    // Physical inventory must not depend on sampling order.
    std::swap(s.Blockers[0], s.Blockers[4]);
    Require(ValidateSixCarFleet(s, tokens) &&
            tokens[0] == 1000 && tokens[5] == 1005,
            "reordering sample array keeps exact identity mapping");
}

void TestContinuousSweptGeometry()
{
    using namespace latebrake;
    auto s = ValidSixCars();
    const RecoveryObstacle self = s.Self;
    RecoveryObstacle other = s.Blockers[0];
    other.Pose.Position = {-1400.0, SouthY};
    const auto approaching = ForecastFleetContact(self, other);
    Require(approaching.Intersects &&
            approaching.FirstContactSeconds > 0.0 &&
            approaching.FirstContactSeconds < FleetForecastHorizonSeconds,
            "continuous contact time detects fast forward approach");
    const auto urgent = ForecastSixCarRisk(
        [&]() { auto x = s; x.Blockers[0] = other; return x; }());
    Require(urgent.Qualified() && urgent.Emergency() &&
            urgent.LimitingSlot == 0 &&
            urgent.ThrottleCeiling == 0.0 &&
            urgent.Kind == FleetRiskKind::SweptContact,
            "forecast prevents throttle before moving into actual car");

    other.Pose.Position = {-2200.0, SouthY};
    const auto overlap = ForecastSixCarRisk(
        [&]() { auto x = s; x.Blockers[0] = other; return x; }());
    Require(overlap.Kind == FleetRiskKind::Touching &&
            Near(overlap.TimeToContactSeconds, 0.0),
            "current padded chassis intersection is most urgent");
    other.Pose.Position = {-1400.0, -1000.0};
    other.VelocityX = 0.0;
    other.VelocityY = -2500.0;
    Require(ForecastFleetContact(self, other).Intersects,
            "crossing traffic caught between discrete rendered frames");
    other.Pose.Position = {-1400.0, -1100.0};
    other.VelocityY = 0.0;
    Require(!ForecastFleetContact(self, other).Intersects,
            "separated side-by-side lanes do not create ghost contact");
    other.Pose.Position = {-3500.0, SouthY};
    other.VelocityX = 2400.0;
    Require(ForecastSixCarRisk(
        [&]() { auto x = s; x.Blockers[0] = other; return x; }()).Kind ==
            FleetRiskKind::Clear,
            "rearward faster driver cannot force leading car into brake");
    other.Pose.Position = {-1400.0, SouthY};
    other.VelocityX = s.Self.VelocityX;
    Require(!ForecastFleetContact(self, other).Intersects,
            "equal parallel velocities never invent closing contact");

    auto y = s;
    y.Self.Pose.Position = {2800.0, -500.0};
    y.Self.Pose.HeadingDegrees = 90.0;
    y.Self.VelocityX = 0.0;
    y.Self.VelocityY = 1400.0;
    y.Blockers[0].Pose.Position = {2800.0, 500.0};
    y.Blockers[0].Pose.HeadingDegrees = 90.0;
    Require(ForecastFleetContact(y.Self, y.Blockers[0]).Intersects,
            "same collision estimate applies on north-facing east straight");
}

void TestDistanceAndPriority()
{
    using namespace latebrake;
    auto s = ValidSixCars();
    const PilotCommand moving{0.80, 0.35, false, false};
    s.Self.VelocityX = 350.0;
    s.Blockers[0].Pose.Position = {-400.0, SouthY};
    s.Blockers[0].VelocityX = 100.0;
    auto follow = ForecastSixCarRisk(s);
    Require(follow.Kind == FleetRiskKind::Following &&
            follow.ThrottleCeiling < 1.0 &&
            follow.ThrottleCeiling > 0.0 &&
            follow.LimitingSlot == 0,
            "measured closing speed makes a gradual follow cap");
    s.Blockers[0].VelocityX = 350.0;
    Require(ForecastSixCarRisk(s).Kind == FleetRiskKind::Clear,
            "matching forward speed cannot produce fake braking distance");
    s.Blockers[0].VelocityX = 100.0;
    s.Blockers[4].Pose.Position = {-2200.0, SouthY};
    auto hazard = ForecastSixCarRisk(s);
    Require(hazard.Kind == FleetRiskKind::Touching &&
            hazard.LimitingSlot == 5,
            "last physical actor emergency must override first soft cap");
    std::swap(s.Blockers[0], s.Blockers[4]);
    hazard = ForecastSixCarRisk(s);
    Require(hazard.Kind == FleetRiskKind::Touching &&
            hazard.LimitingSlot == 5,
            "threat priority independent of sampled actor order");

    s = ValidSixCars();
    s.Self.VelocityX = 900.0;
    s.Blockers[0].Pose.Position = {-1200.0, SouthY};
    s.Blockers[0].VelocityX = 0.0;
    auto predicted = ForecastSixCarRisk(s);
    Require(predicted.Emergency(),
            "close stopped traffic exceeds conservative stopping capacity");
    s.Blockers[0].Pose.Position = {2000.0, -1000.0};
    Require(ForecastSixCarRisk(s).Kind == FleetRiskKind::Clear,
            "large lateral separation cannot trigger same-lane stop demand");

    RivalFleetRiskSupervisor director;
    const FleetRiskSupervision capped =
        director.Apply(moving, ValidSixCars(), 1.0 / 60.0);
    Require(capped.Qualified && capped.Command.Throttle <= moving.Throttle &&
            Near(capped.Command.Steering, moving.Steering) &&
            !capped.Command.Brake,
            "clean forecast never adds throttle or changes steering");
}

void TestTemporalHoldAndReacquisition()
{
    using namespace latebrake;
    RivalFleetRiskSupervisor director;
    const PilotCommand drive{0.85, -0.27, false, false};
    auto safe = ValidSixCars();
    auto first = director.Apply(drive, safe, 1.0 / 60.0);
    Require(first.Qualified && !first.HoldingBrake &&
            Near(first.Command.Throttle, drive.Throttle) &&
            director.QualifiedFrames() == 1,
            "first complete physical roster can drive without fake hold");
    safe.Blockers[0].Pose.Position = {-1400.0, SouthY};
    auto incident = director.Apply(drive, safe, 1.0 / 60.0);
    Require(incident.Qualified && incident.HoldingBrake &&
            incident.Command.Brake && incident.Command.Throttle == 0.0 &&
            incident.Forecast.Emergency() &&
            director.EmergencyFrames() == 1,
            "measured collision starts real-time emergency hold");
    safe.Blockers[0].Pose.Position = {2000.0, SouthY};
    for (int frame = 0; frame < 10; ++frame)
    {
        const auto held = director.Apply(drive, safe, 0.02);
        Require(held.HoldingBrake && held.Command.Brake &&
                held.Command.Throttle == 0.0,
                "rapidly cleared hazard must not release braking instantly");
    }
    Require(director.HoldSecondsRemaining() > 0.0,
            "brake hold measured in seconds, not an arbitrary frame count");
    FleetRiskSupervision released;
    for (int frame = 0; frame < 40; ++frame)
    {
        released = director.Apply(drive, safe, 0.02);
    }
    Require(released.Qualified && !released.HoldingBrake &&
            !released.Command.Brake &&
            released.Command.Throttle > 0.0 &&
            released.Command.Throttle <= drive.Throttle &&
            released.Command.Steering == drive.Steering,
            "post-contact release ramps motor only after verified clearance");

    const auto replaced = safe;
    auto swapped = replaced;
    swapped.Blockers[4].ActorToken = 70005;
    auto newActor = director.Apply(drive, swapped, 0.016);
    Require(newActor.Qualified && newActor.RosterChanged &&
            newActor.HoldingBrake && newActor.Command.Throttle == 0.0 &&
            newActor.Command.Brake,
            "replaced Chaos actor cannot inherit stale clear-road authority");
    auto stillHeld = director.Apply(drive, swapped, 0.016);
    Require(!stillHeld.RosterChanged && stillHeld.HoldingBrake,
            "actor replacement is a bounded stable reacquisition hold");
    director.Reset();
    Require(!director.HasPhysicalRoster() &&
            director.HoldSecondsRemaining() == 0.0 &&
            director.QualifiedFrames() == 0,
            "explicit restart invalidates old fleet identities and timers");

    director.Apply(drive, safe, 0.016);
    const auto rejected = director.Apply(
        drive, safe, std::numeric_limits<double>::quiet_NaN());
    Require(!rejected.Qualified && rejected.Command.Brake &&
            !director.HasPhysicalRoster(),
            "invalid frame duration fails closed and clears memory");
    director.Apply(drive, safe, 0.016);
    safe.BlockerCount = 4;
    Require(!director.Apply(drive, safe, 0.016).Qualified &&
            !director.HasPhysicalRoster(),
            "missing sixth car invalidates every previous clear forecast");
}

void TestRouteAuthorityAndFrameRates()
{
    using namespace latebrake;
    const auto original = ValidSixCars();
    const PilotCommand corner{0.0, 0.63, true, false};
    RivalFleetRiskSupervisor director;
    auto out = director.Apply(corner, original, 1.0 / 60.0);
    Require(out.Qualified && out.Command.Brake &&
            Near(out.Command.Steering, corner.Steering) &&
            out.Command.Throttle == 0.0,
            "existing route corner braking remains authoritative");
    const PilotCommand arrived{0.0, 0.0, false, true};
    out = director.Apply(arrived, original, 1.0 / 60.0);
    Require(out.Qualified && out.Command.Arrived &&
            !director.HasPhysicalRoster(),
            "route arrival resets old traffic memory without scoring");
    const PilotCommand invalid{-1.0, 0.2, false, false};
    Require(!director.Apply(invalid, original, 1.0 / 60.0).Qualified,
            "negative invalid route throttle never qualifies");
    Require(!director.Apply({0.4,
         std::numeric_limits<double>::infinity(), false, false},
         original, 1.0 / 60.0).Qualified,
            "nonfinite steer cannot propagate to Unreal controls");

    // Same physical scenario across variable frame rates: held time is
    // expressed in seconds, never render ticks. Finite bounded work.
    for (int fps : {30, 60, 120})
    {
        director.Reset();
        const double dt = 1.0 / static_cast<double>(fps);
        const PilotCommand drive{0.9, -0.11, false, false};
        auto danger = original;
        danger.Blockers[0].Pose.Position = {-1400.0, SouthY};
        auto emergency = director.Apply(drive, danger, dt);
        Require(emergency.HoldingBrake && emergency.Command.Throttle == 0.0,
                "emergency cannot disappear on any supported frame rate");
        int heldFrames = 0;
        for (int i = 0; i < 2 * fps; ++i)
        {
            auto clear = director.Apply(drive, original, dt);
            if (clear.HoldingBrake) ++heldFrames;
            Require(clear.Qualified && clear.Command.Throttle <=
                    drive.Throttle + 1.0e-12,
                    "temporal supervisor never invents extra torque");
        }
        const double heldSeconds = heldFrames * dt;
        Require(heldSeconds >= FleetRiskEmergencyHoldSeconds - dt * 2.0 &&
                heldSeconds <= FleetRiskEmergencyHoldSeconds + dt,
                "30/60/120 FPS maintains consistent emergency hold duration");
    }
}

void TestDeterministicPerturbations()
{
    using namespace latebrake;
    // Bounded repeatable sweep of physically plausible six-car scenes.
    // This catches NaN leakage and action amplification under varied
    // closing speeds, road positions, lateral offsets and actor ordering.
    std::uint32_t seed = 0x574F524BU;
    const auto random32 = [&seed]() -> std::uint32_t
    {
        seed = seed * 1664525U + 1013904223U;
        return seed;
    };
    RivalFleetRiskSupervisor director;
    for (int i = 0; i < 30000; ++i)
    {
        auto scene = ValidSixCars();
        const double speed = static_cast<double>(random32() % 4200);
        const double gap = 500.0 + static_cast<double>(random32() % 5900);
        const double lateral =
            static_cast<double>(static_cast<int>(random32() % 901) - 450);
        scene.Self.VelocityX = speed;
        scene.Blockers[0].Pose.Position = {
            -2500.0 + gap, SouthY + lateral};
        scene.Blockers[0].VelocityX =
            static_cast<double>(random32() % 1800);
        if (i % 2 == 0)
        {
            std::swap(scene.Blockers[0], scene.Blockers[4]);
        }
        const double steer =
            static_cast<double>(static_cast<int>(random32() % 2001) -
                                1000) / 1000.0;
        const PilotCommand planned{0.78, steer, false, false};
        const double dt = 1.0 / static_cast<double>(
            (i % 3 == 0) ? 30 : (i % 3 == 1) ? 60 : 120);
        const auto risk = ForecastSixCarRisk(scene);
        const auto result = director.Apply(planned, scene, dt);
        if (!risk.Qualified())
        {
            Require(!result.Qualified && result.Command.Brake &&
                    result.Command.Throttle == 0.0,
                    "invalid random road scene fails closed");
            continue;
        }
        Require(result.Qualified && std::isfinite(result.Command.Throttle) &&
                std::isfinite(result.Command.Steering) &&
                result.Command.Throttle >= 0.0 &&
                result.Command.Throttle <= planned.Throttle &&
                result.Command.Steering == planned.Steering,
                "all qualified scenarios stay finite and brake-only");
        if (risk.Emergency())
        {
            Require(result.Command.Brake &&
                    result.Command.Throttle == 0.0,
                    "every swept emergency scenario brakes immediately");
        }
    }
}
} // namespace

int main()
{
    TestExactPhysicalRoster();
    TestContinuousSweptGeometry();
    TestDistanceAndPriority();
    TestTemporalHoldAndReacquisition();
    TestRouteAuthorityAndFrameRates();
    TestDeterministicPerturbations();
    std::cout << "Six physical rival-traffic forecast, roster and 30k"
                 " temporal safety sweeps passed.\n";
    return EXIT_SUCCESS;
}
