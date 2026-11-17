#include "Vehicles/AxleTractionBudget.h"
#include "Vehicles/SprungChassisDynamics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Suspension mounting regression: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double x, double y, double tolerance = 0.002)
{
    return std::isfinite(x) && std::isfinite(y) &&
           std::abs(x - y) <= tolerance;
}

latebrake::SuspensionFrame LevelFrame()
{
    latebrake::SuspensionFrame frame{};
    frame.PhysicsReady = true;
    frame.DeltaSeconds = 1.0 / 60.0;
    frame.MassKg = 1100.0;
    frame.GravityCmPerSecondSquared = 980.0;
    frame.ChassisUpZ = 1.0;
    for (auto& corner : frame.Corners)
    {
        corner = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    }
    return frame;
}

latebrake::ChassisGroundContactOutput FourRealRayHits()
{
    latebrake::ChassisGroundContactSample frame{};
    frame.PhysicsReady = true;
    frame.DeltaSeconds = 1.0 / 60.0;
    for (auto& probe : frame.Probes)
    {
        probe = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0};
    }
    return latebrake::EvaluateChassisGroundContact(frame);
}
}

int main()
{
    using namespace latebrake;
    auto frame = LevelFrame();
    const double weight = frame.MassKg * frame.GravityCmPerSecondSquared;
    const auto level = CalculateSprungChassis(frame);
    Require(level.Valid && level.ContactCount == 4 &&
            Near(level.TotalUpForce, weight) &&
            Near(SuspensionMountLoadScale(1.0), 1.0),
            "normal chassis orientation retains exact four-corner equilibrium");

    frame.ChassisUpZ = SuspensionFullMountUpZ;
    const auto stillSupported = CalculateSprungChassis(frame);
    Require(stillSupported.Valid && stillSupported.ContactCount == 4 &&
            Near(stillSupported.TotalUpForce, level.TotalUpForce),
            "60-degree body tilt retains full measurable vertical support");
    frame.ChassisUpZ = (SuspensionNoMountUpZ + SuspensionFullMountUpZ) * 0.5;
    const auto fading = CalculateSprungChassis(frame);
    Require(fading.Valid && fading.ContactCount == 4 &&
            Near(fading.TotalUpForce, weight * 0.5),
            "midway tilt smoothly fades total actual spring load by half");
    for (unsigned index = 0; index < 4; ++index)
    {
        Require(Near(fading.Corners[index].UpForce,
                     level.Corners[index].UpForce * 0.5),
                "every real corner receives same smooth mounting attenuation");
    }
    const auto coarse = FourRealRayHits();
    Require(coarse.Valid && coarse.SupportingProbes == 4,
            "fixture has four geometric pavement hits even on inverted roof");
    const auto reduced = ConstrainGroundContactToSuspension(coarse, fading);
    const auto axle = CalculateAxleTractionBudget(reduced, fading);
    Require(reduced.Valid && Near(reduced.ContactTractionScale, 0.5) &&
            axle.Valid && Near(axle.RearDriveTractionScale, 0.5) &&
            Near(axle.FrontSteeringTractionScale, 0.5),
            "reduced underside load must reduce front steering and rear motor grip");

    for (double up : {SuspensionNoMountUpZ, 0.0, -0.25, -0.95, -1.0})
    {
        frame.ChassisUpZ = up;
        const auto unsupported = CalculateSprungChassis(frame);
        Require(unsupported.Valid && unsupported.ContactCount == 0 &&
                unsupported.TotalUpForce == 0.0 &&
                unsupported.AntiRollTransfer[0] == 0.0 &&
                unsupported.AntiRollTransfer[1] == 0.0,
                "lying on side/roof cannot make upside-down springs carry weight");
        for (const auto& c : unsupported.Corners)
        {
            Require(!c.Contact && c.UpForce == 0.0 &&
                    c.WheelTravelCm == SuspensionVisualMinTravelCm,
                    "sideways or inverted visual tire droops with no load");
        }
        const auto denied = ConstrainGroundContactToSuspension(coarse, unsupported);
        const auto deniedAxle = CalculateAxleTractionBudget(denied, unsupported);
        Require(denied.Valid && !denied.HasDriveContact &&
                denied.ContactTractionScale == 0.0 &&
                deniedAxle.Valid && !deniedAxle.RearDriveReady &&
                !deniedAxle.FrontSteeringReady,
                "all four pavement hits cannot authorize upside-down engine/grip");
    }

    frame.ChassisUpZ = std::numeric_limits<double>::quiet_NaN();
    Require(!CalculateSprungChassis(frame).Valid,
            "NaN physical up basis must fail closed");
    frame.ChassisUpZ = std::numeric_limits<double>::infinity();
    Require(!CalculateSprungChassis(frame).Valid,
            "infinite physical up basis must fail closed");
    frame.ChassisUpZ = 1.0001;
    Require(!CalculateSprungChassis(frame).Valid,
            "impossible >1 up-axis direction must not be accepted");
    frame.ChassisUpZ = -1.0001;
    Require(!CalculateSprungChassis(frame).Valid,
            "impossible <-1 up-axis direction must not be accepted");

    // Uneven springs/anti-roll are also attenuated by exactly one shared
    // mounting factor: their relative per-axle transfer is preserved, but
    // no fake vertical force appears when the car rolls toward its roof.
    frame = LevelFrame();
    frame.Corners[0].DistanceCm = 57.0;
    frame.Corners[1].DistanceCm = 61.0;
    frame.Corners[2].DistanceCm = 62.0;
    frame.Corners[3].DistanceCm = 56.0;
    frame.Corners[0].RayDistanceSpeedCmPerSecond = -25.0;
    frame.Corners[1].RayDistanceSpeedCmPerSecond = 25.0;
    const auto unevenLevel = CalculateSprungChassis(frame);
    Require(unevenLevel.Valid && unevenLevel.AntiRollTransfer[0] > 0.0 &&
            unevenLevel.AntiRollTransfer[1] < 0.0,
            "uneven left-right contact must retain both independent anti-roll axles");
    frame.ChassisUpZ = (SuspensionNoMountUpZ + SuspensionFullMountUpZ) * 0.5;
    const auto unevenHalf = CalculateSprungChassis(frame);
    Require(unevenHalf.Valid &&
            Near(unevenHalf.TotalUpForce, unevenLevel.TotalUpForce * 0.5) &&
            Near(unevenHalf.AntiRollTransfer[0],
                 unevenLevel.AntiRollTransfer[0] * 0.5) &&
            Near(unevenHalf.AntiRollTransfer[1],
                 unevenLevel.AntiRollTransfer[1] * 0.5),
            "anti-roll redistribution must fade proportionally with actual spring load");
    for (unsigned j = 0; j < 4; ++j)
    {
        Require(Near(unevenHalf.Corners[j].UpForce,
                     unevenLevel.Corners[j].UpForce * 0.5),
                "asymmetric four-corner forces must preserve ratios during roll fade");
    }

    // Stress real attitude changes with different road load and local
    // corner speeds; every accepted spring is bounded by its own fading
    // ceiling, and no fully inverted pose can produce traction.
    for (int i = 0; i < 120000; ++i)
    {
        frame = LevelFrame();
        frame.ChassisUpZ = -1.0 + 2.0 * (i % 1001) / 1000.0;
        frame.MassKg = 100.0 + ((i * 37) % 14901);
        for (unsigned j = 0; j < 4; ++j)
        {
            auto& c = frame.Corners[j];
            c.DistanceCm = 35.0 + ((i + static_cast<int>(j) * 19) % 48);
            c.RayDistanceSpeedCmPerSecond =
                ((i * 13 + static_cast<int>(j) * 7) % 801) - 400;
            c.Hit = (i + static_cast<int>(j)) % 7 != 0;
        }
        const auto out = CalculateSprungChassis(frame);
        const double scale = SuspensionMountLoadScale(frame.ChassisUpZ);
        Require(out.Valid && out.ContactCount >= 0 && out.ContactCount <= 4 &&
                std::isfinite(out.TotalUpForce) &&
                out.TotalUpForce >= 0.0 &&
                out.TotalUpForce <=
                    frame.MassKg * frame.GravityCmPerSecondSquared *
                    SuspensionMaxPerCornerWeightFactor * 4.0 * scale + 0.01,
                "120k changing attitude samples cannot exceed scaled weight budget");
        double summed = 0.0;
        for (const auto& corner : out.Corners)
        {
            summed += corner.UpForce;
            Require(corner.UpForce >= 0.0 &&
                    corner.UpForce <= out.WeightForce *
                        SuspensionMaxPerCornerWeightFactor * scale + 0.01,
                    "a single unloading corner cannot exceed mount-attitude cap");
        }
        Require(Near(summed, out.TotalUpForce),
                "force sum remains consistent with all four measured wheel loads");
        if (scale == 0.0)
        {
            Require(out.ContactCount == 0 && out.TotalUpForce == 0.0,
                    "even four geometric rays cannot prop an inverted chassis up");
        }
    }
    std::cout << "Suspension mount attitude passed (120k frames; no inverted grip).\n";
    return EXIT_SUCCESS;
}
