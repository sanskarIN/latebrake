#include "Vehicles/AxleTractionBudget.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "AxleTractionBudget regression: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 0.0001)
{
    return std::abs(a - b) <= tolerance;
}
latebrake::SuspensionFrame Standing()
{
    latebrake::SuspensionFrame s;
    s.PhysicsReady = true;
    s.DeltaSeconds = 1.0 / 60.0;
    s.MassKg = 1100.0;
    for (auto& c : s.Corners)
    {
        c = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    }
    return s;
}
latebrake::ChassisGroundContactOutput Ground(
    const latebrake::SuspensionResponse& springs)
{
    latebrake::ChassisGroundContactSample s;
    s.PhysicsReady = true;
    s.DeltaSeconds = 1.0 / 60.0;
    s.ChassisVerticalSpeedCmPerSecond = 0.0;
    // Coarse WorldStatic observations intentionally report all four
    // nearby lanes. Constrained grip MUST use actual engaged springs.
    for (auto& probe : s.Probes)
    {
        probe = {true, 59.0, 1.0};
    }
    return latebrake::ConstrainGroundContactToSuspension(
        latebrake::EvaluateChassisGroundContact(s), springs);
}
latebrake::AxleTractionBudget Measure(latebrake::SuspensionFrame s)
{
    const auto springs = latebrake::CalculateSprungChassis(s);
    return latebrake::CalculateAxleTractionBudget(Ground(springs), springs);
}
}
int main()
{
    using namespace latebrake;
    const auto fullFrame = Standing();
    const auto full = Measure(fullFrame);
    Require(full.Valid && full.RearDriveReady && full.FrontSteeringReady &&
            full.FrontWheelContacts == 2 && full.RearWheelContacts == 2 &&
            Near(full.FrontAxleLoadRatio, 1.0) &&
            Near(full.RearAxleLoadRatio, 1.0) &&
            Near(full.RearDriveTractionScale, 1.0) &&
            Near(full.FrontSteeringTractionScale, 1.0),
            "stationary fully supported car retains unchanged drive and steering");

    auto frame = Standing();
    // Front wheels both support, REAR completely airborne: engine/nitro
    // cannot push the car because the rear axle carries no actual load.
    frame.Corners[2].Hit = false;
    frame.Corners[3].Hit = false;
    const auto frontOnly = Measure(frame);
    Require(frontOnly.Valid && frontOnly.FrontWheelContacts == 2 &&
            frontOnly.RearWheelContacts == 0 &&
            Near(frontOnly.FrontSteeringTractionScale, 0.45) &&
            frontOnly.RearDriveTractionScale == 0.0 &&
            !frontOnly.RearDriveReady && frontOnly.FrontSteeringReady,
            "only front axle loaded preserves bounded yaw but rejects rear drive");

    frame = Standing();
    frame.Corners[0].Hit = false;
    frame.Corners[1].Hit = false;
    const auto rearOnly = Measure(frame);
    Require(rearOnly.Valid && rearOnly.RearWheelContacts == 2 &&
            rearOnly.FrontWheelContacts == 0 &&
            Near(rearOnly.RearDriveTractionScale, 0.45) &&
            rearOnly.FrontSteeringTractionScale == 0.0 &&
            rearOnly.RearDriveReady && !rearOnly.FrontSteeringReady,
            "rear-only support cannot steer nonexistent loaded front tires");

    frame = Standing();
    frame.Corners[1].Hit = false;
    frame.Corners[2].Hit = false;
    const auto diagonal = Measure(frame);
    Require(diagonal.Valid && diagonal.FrontWheelContacts == 1 &&
            diagonal.RearWheelContacts == 1 &&
            Near(diagonal.RearDriveTractionScale, 0.20) &&
            Near(diagonal.FrontSteeringTractionScale, 0.225) &&
            diagonal.RearDriveReady && diagonal.FrontSteeringReady,
            "diagonal wheel support uses reduced one-wheel axle budgets");

    frame = Standing();
    frame.Corners[3].Hit = false;
    const auto rearOne = Measure(frame);
    Require(rearOne.Valid && rearOne.FrontWheelContacts == 2 &&
            rearOne.RearWheelContacts == 1 &&
            Near(rearOne.RearDriveTractionScale, 0.20) &&
            Near(rearOne.FrontSteeringTractionScale, 0.75),
            "three wheels never grant a one-wheel rear axle full engine traction");

    frame = Standing();
    frame.Corners[1].Hit = false;
    const auto frontOne = Measure(frame);
    Require(frontOne.Valid && frontOne.FrontWheelContacts == 1 &&
            frontOne.RearWheelContacts == 2 &&
            Near(frontOne.RearDriveTractionScale, 0.75) &&
            Near(frontOne.FrontSteeringTractionScale, 0.225),
            "missing front tire reduces steering while both rear tires remain powered");

    frame = Standing();
    for (auto& c : frame.Corners)
    {
        c.DistanceCm = SuspensionRestDistanceCm;
    }
    const auto unloaded = Measure(frame);
    Require(unloaded.Valid && !unloaded.RearDriveReady &&
            !unloaded.FrontSteeringReady &&
            unloaded.RearDriveTractionScale == 0.0 &&
            unloaded.FrontSteeringTractionScale == 0.0,
            "four geometrically touching but zero-load springs cannot steer or drive");

    frame = Standing();
    for (auto& c : frame.Corners)
    {
        c.PointVerticalSpeedCmPerSecond = 700.0;
    }
    const auto rising = Measure(frame);
    Require(rising.Valid && !rising.RearDriveReady &&
            !rising.FrontSteeringReady,
            "upward rebound unloads both axles and all power/steering");

    frame = Standing();
    frame.Corners[2].DistanceCm = 70.0;
    frame.Corners[3].DistanceCm = 70.0;
    const auto weakRear = Measure(frame);
    Require(weakRear.Valid && weakRear.RearDriveReady &&
            weakRear.RearDriveTractionScale < weakRear.FrontSteeringTractionScale &&
            weakRear.RearDriveTractionScale > 0.0,
            "reduced rear normal load reduces actual motor authority");

    frame = Standing();
    frame.Corners[0].DistanceCm = 70.0;
    frame.Corners[1].DistanceCm = 70.0;
    const auto weakFront = Measure(frame);
    Require(weakFront.Valid &&
            weakFront.FrontSteeringTractionScale <
                weakFront.RearDriveTractionScale,
            "reduced front normal load reduces actual steering authority");

    frame = Standing();
    frame.Corners[1].Hit = false;
    frame.Corners[2].Hit = false;
    frame.Corners[3].Hit = false;
    const auto single = Measure(frame);
    Require(single.Valid && !single.RearDriveReady &&
            !single.FrontSteeringReady &&
            single.RearDriveTractionScale == 0.0 &&
            single.FrontSteeringTractionScale == 0.0,
            "one wheel cannot create playable traction from a rejected body");

    auto springs = CalculateSprungChassis(Standing());
    auto coarse = Ground(springs);
    auto invalid = coarse;
    invalid.Valid = false;
    Require(!CalculateAxleTractionBudget(invalid, springs).Valid,
            "invalid coarse ground sample rejected");
    invalid = coarse;
    invalid.SupportingProbes = 3;
    Require(!CalculateAxleTractionBudget(invalid, springs).Valid,
            "mismatched contact counts are rejected");
    invalid = coarse;
    invalid.HasDriveContact = false;
    Require(!CalculateAxleTractionBudget(invalid, springs).Valid,
            "contradictory drive metadata rejected");
    invalid = coarse;
    invalid.ContactTractionScale = std::numeric_limits<double>::quiet_NaN();
    Require(!CalculateAxleTractionBudget(invalid, springs).Valid,
            "nonfinite global grip rejected");
    invalid = coarse;
    invalid.ContactTractionScale = 1.4;
    Require(!CalculateAxleTractionBudget(invalid, springs).Valid,
            "amplifying global traction is rejected");
    springs.Corners[2].UpForce = -1.0;
    Require(!CalculateAxleTractionBudget(coarse, springs).Valid,
            "negative spring force rejected");
    springs = CalculateSprungChassis(Standing());
    springs.Corners[3].UpForce = 0.0;
    Require(!CalculateAxleTractionBudget(coarse, springs).Valid,
            "tampered force sum rejected");
    springs = CalculateSprungChassis(Standing());
    springs.Corners[0].Contact = false;
    Require(!CalculateAxleTractionBudget(coarse, springs).Valid,
            "stale individual contact qualification rejected");

    // 100k varying real spring frames, mixed one/two/three/four wheel
    // support, large bump/damper velocity and three gravity fields.
    for (int t = 0; t < 100000; ++t)
    {
        frame = Standing();
        frame.MassKg = 100.0 + (t % 14901);
        frame.GravityCmPerSecondSquared =
            t % 3 == 0 ? 490.0 : t % 3 == 1 ? 980.0 : 1600.0;
        frame.DeltaSeconds = 1.0 / (30.0 + t % 91);
        for (int i = 0; i < 4; ++i)
        {
            auto& c = frame.Corners[static_cast<unsigned>(i)];
            c.Hit = ((t * 3 + i * 7) % 13) < 8;
            c.DistanceCm = static_cast<double>((t * 19 + i * 37) % 281);
            c.NormalZ = ((t + i * 3) % 17 == 0) ? 0.25 : 1.0;
            c.PointVerticalSpeedCmPerSecond =
                static_cast<double>(((t * 17 + i * 101) % 1601) - 800);
        }
        const auto sr = CalculateSprungChassis(frame);
        const auto ground = Ground(sr);
        const auto outcome = CalculateAxleTractionBudget(ground, sr);
        Require(sr.Valid && ground.Valid && outcome.Valid,
                "valid 100k frames preserve accepted axle budget");
        Require(outcome.FrontWheelContacts + outcome.RearWheelContacts ==
                    sr.ContactCount,
                "axle wheel qualification sums to accepted spring count");
        Require(std::isfinite(outcome.FrontAxleLoadRatio) &&
                std::isfinite(outcome.RearAxleLoadRatio) &&
                outcome.FrontAxleLoadRatio >= 0.0 &&
                outcome.FrontAxleLoadRatio <= 1.0 &&
                outcome.RearAxleLoadRatio >= 0.0 &&
                outcome.RearAxleLoadRatio <= 1.0,
                "axle loading can never invent extra weight");
        Require(std::isfinite(outcome.RearDriveTractionScale) &&
                std::isfinite(outcome.FrontSteeringTractionScale) &&
                outcome.RearDriveTractionScale >= 0.0 &&
                outcome.FrontSteeringTractionScale >= 0.0 &&
                outcome.RearDriveTractionScale <=
                    ground.ContactTractionScale + 1.0e-12 &&
                outcome.FrontSteeringTractionScale <=
                    ground.ContactTractionScale + 1.0e-12,
                "new axle force multipliers never amplify the old grip budget");
        Require(!outcome.RearDriveReady ||
                    (outcome.RearWheelContacts > 0 &&
                     ground.HasDriveContact &&
                     outcome.RearDriveTractionScale > 0.0),
                "rear drive cannot engage without a loaded rear tire");
        Require(!outcome.FrontSteeringReady ||
                    (outcome.FrontWheelContacts > 0 &&
                     ground.HasDriveContact &&
                     outcome.FrontSteeringTractionScale > 0.0),
                "front steer cannot engage without a loaded front tire");
    }
    std::cout << "Measured rear-drive/front-steer axle traction passed "
                 "(100k frames).\n";
    return EXIT_SUCCESS;
}
