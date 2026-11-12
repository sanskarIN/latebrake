#include "Vehicles/SprungGripReattachment.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "SprungGripReattachment: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-8)
{
    return std::abs(a - b) <= tolerance;
}
latebrake::SuspensionFrame Flat()
{
    latebrake::SuspensionFrame s;
    s.PhysicsReady = true;
    s.MassKg = 1100.0;
    s.DeltaSeconds = 1.0 / 60.0;
    for (auto& corner : s.Corners)
    {
        corner = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    }
    return s;
}
struct Input
{
    latebrake::ChassisGroundContactOutput Ground{};
    latebrake::AxleTractionBudget Axles{};
};
Input Measure(latebrake::SuspensionFrame sample)
{
    const auto springs = latebrake::CalculateSprungChassis(sample);
    latebrake::ChassisGroundContactSample coarse;
    coarse.PhysicsReady = true;
    coarse.DeltaSeconds = sample.DeltaSeconds;
    for (unsigned i = 0; i < 4; ++i)
    {
        const auto& c = sample.Corners[i];
        coarse.Probes[i] = {c.Hit, c.DistanceCm, c.NormalZ};
    }
    auto ground = latebrake::ConstrainGroundContactToSuspension(
        latebrake::EvaluateChassisGroundContact(coarse), springs);
    return {ground, latebrake::CalculateAxleTractionBudget(ground, springs)};
}
}

int main()
{
    using namespace latebrake;
    SprungGripReattachment filter;
    const auto flat = Measure(Flat());
    Check(flat.Ground.Valid && flat.Axles.Valid &&
          Near(flat.Ground.ContactTractionScale, 1.0),
          "real flat four-corner source sample is mandatory");

    // A cold car cannot instantly apply full motor/yaw force on a lucky
    // trace; brake/lateral grip must retain its real physical ceiling.
    // A settled countdown restores both axle authorities.
    for (int i = 1; i <= 10; ++i)
    {
        const auto r = filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles);
        Check(r.Valid && Near(r.Ground.ContactTractionScale, 1.0) &&
              Near(r.Axles.RearDriveTractionScale, i * 0.1) &&
              Near(r.Axles.FrontSteeringTractionScale, i * 0.1),
              "cold adhesion must rise at a bounded, deterministic rate");
    }
    const auto steady = filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles);
    Check(steady.Valid && Near(steady.Ground.ContactTractionScale, 1.0),
          "flat equilibrium must keep the existing full-force baseline");

    // A complete three-second 3/2/1 countdown keeps measuring physical
    // spring contact but cannot command propulsion. With stable contact,
    // the first real GO sample should already have full traction authority.
    filter.Reset();
    for (int gridFrame = 0; gridFrame < 180; ++gridFrame)
    {
        const auto grid = filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles);
        Check(grid.Valid, "pre-GO spring grip sample must remain valid");
        Check(grid.Axles.RearDriveTractionScale <=
                  flat.Axles.RearDriveTractionScale &&
              grid.Axles.FrontSteeringTractionScale <=
                  flat.Axles.FrontSteeringTractionScale,
              "countdown cannot exceed measured spring axle grip");
    }
    const auto firstGreen = filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles);
    Check(firstGreen.Valid &&
          Near(firstGreen.Axles.RearDriveTractionScale, 1.0) &&
          Near(firstGreen.Axles.FrontSteeringTractionScale, 1.0),
          "settled 3-second grid must avoid artificial motor/yaw lag on GO");

    // Front axle over empty pavement: motor authority is immediately zero,
    // even though front steering and whole-body support can remain.
    auto frontOnly = Flat();
    frontOnly.Corners[2].Hit = false;
    frontOnly.Corners[3].Hit = false;
    const auto front = Measure(frontOnly);
    Check(front.Ground.Valid && front.Axles.Valid &&
          !front.Axles.RearDriveReady, "front-only source sample");
    const auto frontResult = filter.Advance(1.0 / 60.0, front.Ground, front.Axles);
    Check(frontResult.Valid &&
          Near(frontResult.Axles.RearDriveTractionScale, 0.0) &&
          !frontResult.Axles.RearDriveReady &&
          frontResult.Axles.FrontSteeringReady &&
          frontResult.Ground.ContactTractionScale <=
              front.Ground.ContactTractionScale,
          "loss of rear contact must kill torque on the same frame");

    const auto returned = filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles);
    Check(returned.Valid &&
          returned.Axles.RearDriveTractionScale <= 0.1 + 1.0e-8 &&
          returned.Axles.RearDriveTractionScale <=
              returned.Ground.ContactTractionScale,
          "rear axle reattachment must ramp independently");

    // Airborne chassis cannot keep road grip or pseudo-drive through a
    // stored filter value. No ghost steering during a one-corner support.
    auto air = Flat();
    for (auto& c : air.Corners) c.Hit = false;
    const auto airborne = Measure(air);
    const auto noGrip = filter.Advance(1.0 / 60.0,
        airborne.Ground, airborne.Axles);
    Check(noGrip.Valid && Near(noGrip.Ground.ContactTractionScale, 0.0) &&
          Near(noGrip.Axles.FrontSteeringTractionScale, 0.0) &&
          Near(noGrip.Axles.RearDriveTractionScale, 0.0) &&
          !noGrip.Ground.HasDriveContact,
          "airborne must immediately clear all previously earned grip");

    // Recovery, pause, focus loss and invalid input must reset stored gain.
    filter.Reset();
    Check(Near(filter.Advance(1.0 / 30.0, flat.Ground, flat.Axles)
               .Axles.RearDriveTractionScale, 0.2),
          "30 fps rise is bounded by actual frame duration");
    auto invalid = filter.Advance(
        std::numeric_limits<double>::infinity(), flat.Ground, flat.Axles);
    Check(!invalid.Valid, "nonfinite frame duration must fail closed");
    Check(Near(filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles)
               .Axles.RearDriveTractionScale, 0.1),
          "reject must erase stored grip before resuming");

    auto forged = flat;
    forged.Axles.RearDriveTractionScale = 2.0;
    Check(!filter.Advance(1.0 / 60.0, forged.Ground, forged.Axles).Valid,
          "invalid input must not bypass measured whole-body grip");
    Check(Near(filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles)
               .Axles.RearDriveTractionScale, 0.1),
          "rejection must reset response memory");
    Check(!filter.Advance(0.11, flat.Ground, flat.Axles).Valid,
          "physics hitch may not authorize stale reattachment");

    // Corrupt metadata can be finite yet physically impossible. A load
    // ratio of zero cannot authorize real axle torque, and whole-body
    // force still obeys the same contact-count ceiling as road sampling.
    for (const double invalidRatio : {
             -0.01, 1.01, std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::infinity()})
    {
        auto bogus = flat;
        bogus.Axles.FrontAxleLoadRatio = invalidRatio;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "invalid front axle load ratio must fail closed");
        bogus = flat;
        bogus.Axles.RearAxleLoadRatio = invalidRatio;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "invalid rear axle load ratio must fail closed");
    }
    {
        auto bogus = flat;
        bogus.Axles.RearAxleLoadRatio = 0.2;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "rear motor cannot exceed claimed measured spring load");
        bogus = flat;
        bogus.Axles.FrontAxleLoadRatio = 0.2;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "front yaw cannot exceed claimed measured spring load");
        bogus = front;
        bogus.Ground.ContactTractionScale = 0.70;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "two physical contacts cannot claim three-wheel grip");
        bogus = flat;
        bogus.Axles.RearWheelContacts = 1;
        bogus.Axles.FrontWheelContacts = 3;
        Check(!filter.Advance(1.0 / 60.0, bogus.Ground, bogus.Axles).Valid,
              "impossible front wheel count must not authorize steering");
    }
    Check(Near(filter.Advance(1.0 / 60.0, flat.Ground, flat.Axles)
               .Axles.RearDriveTractionScale, 0.1),
          "failed metadata observation must clear previous axle grip");

    // Seeded repeated detach/reattach frames: never exceed current physical
    // observations, never lag a loss, and never outpace the 6/s rise budget.
    unsigned seed = 0x9e3779b9u;
    double lastRear = 0.0;
    double lastFront = 0.0;
    filter.Reset();
    for (int index = 0; index < 100000; ++index)
    {
        seed = seed * 1664525u + 1013904223u;
        auto observed = Flat();
        for (unsigned wheel = 0; wheel < 4; ++wheel)
        {
            if (((seed >> (wheel * 7)) & 3u) == 0u)
            {
                observed.Corners[wheel].Hit = false;
            }
        }
        const auto input = Measure(observed);
        const double dt = (index % 3 == 0) ? 1.0 / 30.0 :
                          (index % 3 == 1) ? 1.0 / 60.0 : 1.0 / 120.0;
        const auto r = filter.Advance(dt, input.Ground, input.Axles);
        Check(r.Valid && r.Ground.ContactTractionScale <=
                  input.Ground.ContactTractionScale + 1.0e-8 &&
              r.Axles.RearDriveTractionScale <=
                  input.Axles.RearDriveTractionScale + 1.0e-8 &&
              r.Axles.FrontSteeringTractionScale <=
                  input.Axles.FrontSteeringTractionScale + 1.0e-8 &&
              r.Axles.RearDriveTractionScale <= lastRear +
                  GripReattachmentRisePerSecond * dt + 1.0e-8 &&
              r.Axles.FrontSteeringTractionScale <= lastFront +
                  GripReattachmentRisePerSecond * dt + 1.0e-8,
              "100k frames must stay below physical and rate ceilings");
        lastRear = r.Axles.RearDriveTractionScale;
        lastFront = r.Axles.FrontSteeringTractionScale;
    }
    std::cout << "Suspension traction fast-loss / bounded-reattachment "
                 "regressions passed.\n";
    return EXIT_SUCCESS;
}
