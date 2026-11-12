#include "AI/RivalBrakeCapacityPace.h"
#include "AI/RivalCornerBrakeEnvelope.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass) { std::cerr << "RivalBrakeCapacityPace FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
latebrake::AcceptedBrakeCapability Frame(std::uint64_t seq, double deceleration)
{
    latebrake::AcceptedBrakeCapability f;
    f.Valid = true; f.Sequence = seq; f.TeleportRevision = 5; f.FrameSeconds = 1.0/60.0;
    f.Estimate.Valid = true; f.Estimate.Supported = true; f.Estimate.LoadedMask = 15;
    f.Estimate.NominalDecelerationCmPerSecondSquared = 900.0;
    f.Estimate.PlanningDecelerationCmPerSecondSquared = deceleration;
    for (auto& c : f.Estimate.Corners)
    {
        c.Loaded = true; c.AdmittedForceNewtons = 2475.0;
        c.NormalEstimateNewtons = 2695.0; c.PlanningForceNewtons = deceleration * 2.75;
    }
    return f;
}
}
int main()
{
    using namespace latebrake;
    const PilotCommand drive{0.85, 0.3, false, false};
    RivalBrakeCapacityPace policy;
    auto result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(1, 600.0)});
    Require(result.Valid && result.CapacityAvailable &&
            result.DecelerationCeilingCmPerSecondSquared == 300.0 &&
            result.Command.Throttle == drive.Throttle && result.Command.Steering == drive.Steering,
            "first accepted capacity starts with a conservative bootstrap and preserves route steering");
    for (std::uint64_t seq = 2; seq <= 61; ++seq)
        result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(seq, 600.0)});
    Require(std::abs(result.DecelerationCeilingCmPerSecondSquared - 600.0) < 1.0e-7,
            "continuous fresh observations recover only at the bounded per-second rate");
    result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(62, 90.0)});
    Require(result.Valid && result.DecelerationCeilingCmPerSecondSquared == 90.0,
            "a sudden load/grip decline immediately lowers corner braking authority");
    result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(63, 600.0)});
    Require(result.DecelerationCeilingCmPerSecondSquared == 96.0,
            "one good crest landing frame cannot restore the old high forecast");
    result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(63, 600.0)});
    Require(!result.Valid && result.Command.Brake && result.Command.Throttle == 0.0,
            "same accepted source frame cannot be consumed twice");
    result = policy.Apply(drive, {1.0/60.0, 5, true, Frame(63, 600.0)});
    Require(!result.Valid, "reject/reset cannot make the rejected receipt reusable");
    policy.Reset();
    Require(!policy.Apply(drive, {1.0/60.0, 5, true, Frame(62, 600.0)}).Valid,
            "restart of transient confidence does not reopen older identities");
    Require(!policy.Apply(drive, {1.0/60.0, 6, true, Frame(64, 600.0)}).Valid,
            "pre-teleport source snapshot cannot feed a post-teleport control frame");
    Require(!policy.Apply(drive, {0.2, 5, true, Frame(65, 600.0)}).Valid,
            "frame hitch cannot accumulate confident recovery time");
    auto zero = Frame(66, 0.0);
    result = policy.Apply(drive, {1.0/60.0, 5, true, zero});
    Require(result.Valid && !result.CapacityAvailable && result.Command.Brake &&
            result.Command.Throttle == 0.0, "zero estimated capacity cannot authorize more propulsion");
    const auto corner = CircuitGate(0);
    RivalCornerBrakeSample envelope;
    envelope.NextGate = 0; envelope.Position = {corner.X - 3000.0, corner.Y};
    envelope.PlanarSpeedCmPerSecond = 2500.0; envelope.PriorPhysicsQualified = true;
    const auto original = ApplyRivalCornerBrakeEnvelope(drive, envelope);
    envelope.UseContactCapacityCeiling = true;
    envelope.ContactDecelerationCeiling = 120.0; envelope.ReactionSeconds = 0.22;
    const auto limited = ApplyRivalCornerBrakeEnvelope(drive, envelope);
    Require(original.Valid && limited.Valid && limited.Command.Brake &&
            limited.Command.Throttle == 0.0 &&
            limited.BrakeEnvelopeSpeedCmPerSecond < original.BrakeEnvelopeSpeedCmPerSecond,
            "real contact capacity reduces target speed before the same actual turn");
    const double runway = 3000.0 - 550.0 - 2500.0 * 0.22;
    Require(std::abs(limited.BrakeEnvelopeSpeedCmPerSecond -
            std::sqrt(1550.0*1550.0 + 2.0*120.0*runway)) < 1.0e-7,
            "capacity envelope matches independent stopping-distance and reaction-distance reference");
    envelope.ContactDecelerationCeiling = 0.0;
    const auto blocked = ApplyRivalCornerBrakeEnvelope(drive, envelope);
    Require(blocked.Valid && blocked.Command.Brake && blocked.Command.Throttle == 0.0,
            "supported but zero-capacity forecast fails to a brake command");
    std::cout << "Rival brake capacity: source identity, immediate loss, recovery rate and analytic envelope passed\n";
}
