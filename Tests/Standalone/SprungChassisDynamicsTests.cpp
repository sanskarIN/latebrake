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
        std::cerr << "Sprung chassis regression: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double tolerance = 1.0e-7)
{
    return std::abs(a - b) <= tolerance;
}
latebrake::SuspensionFrame Resting(double mass = 1100.0)
{
    latebrake::SuspensionFrame sample;
    sample.DeltaSeconds = 1.0 / 60.0;
    sample.MassKg = mass;
    sample.PhysicsReady = true;
    for (auto& c : sample.Corners)
    {
        c = {true, latebrake::SuspensionTargetRideDistanceCm, 1.0, 0.0};
    }
    return sample;
}
}

int main()
{
    using namespace latebrake;
    for (const double mass : {100.0, 800.0, 1100.0, 1800.0, 15000.0})
    {
        auto s = Resting(mass);
        const auto out = CalculateSprungChassis(s);
        Require(out.Valid && out.ContactCount == 4,
                "all four genuinely measured corners support the car");
        Require(Near(out.TotalUpForce,
                     mass * SuspensionGravityCmPerSecondSquared, 0.001),
                "spring force balances real gravity for any accepted mass");
        for (const auto& c : out.Corners)
        {
            Require(c.Contact && Near(c.WheelTravelCm, 0.0) &&
                    Near(c.UpForce,
                         mass * SuspensionGravityCmPerSecondSquared / 4.0,
                         0.001),
                    "each wheel independently balances a quarter of chassis weight");
        }
    }

    auto alternateGravity = Resting(1320.0);
    alternateGravity.GravityCmPerSecondSquared = 490.0;
    const auto lowGravity = CalculateSprungChassis(alternateGravity);
    Require(lowGravity.Valid &&
            Near(lowGravity.TotalUpForce, 1320.0 * 490.0, 0.001),
            "real nondefault gravity changes spring equilibrium load once");

    auto s = Resting();
    s.Corners[0].DistanceCm = 45.0;
    const auto loaded = CalculateSprungChassis(s);
    Require(loaded.Valid &&
            loaded.Corners[0].UpForce > loaded.Corners[1].UpForce &&
            loaded.Corners[0].WheelTravelCm > 0.0,
            "one physically raised wheel applies extra lift and visually compresses");
    s.Corners[0].PointVerticalSpeedCmPerSecond = 600.0;
    const auto rebound = CalculateSprungChassis(s);
    Require(rebound.Corners[0].UpForce == 0.0,
            "strong upward corner motion cannot create downward spring suction");
    s.Corners[0].PointVerticalSpeedCmPerSecond = -2000.0;
    const auto compression = CalculateSprungChassis(s);
    Require(Near(compression.Corners[0].UpForce,
                 s.MassKg * SuspensionGravityCmPerSecondSquared *
                     SuspensionMaxPerCornerWeightFactor, 0.01),
            "hard landing is bounded per corner, including damper response");
    s = Resting();
    s.Corners[0].Hit = false;
    s.Corners[1].NormalZ = 0.35;
    s.Corners[2].DistanceCm = 210.0;
    const auto edge = CalculateSprungChassis(s);
    Require(edge.Valid && edge.ContactCount == 1 &&
            edge.Corners[0].UpForce == 0.0 &&
            edge.Corners[1].UpForce == 0.0 &&
            edge.Corners[2].UpForce == 0.0 &&
            edge.Corners[3].UpForce > 0.0,
            "missing road, wall and overlong traces cannot prop up car");
    Require(Near(edge.Corners[0].WheelTravelCm,
                 SuspensionVisualMinTravelCm) &&
            Near(edge.Corners[1].WheelTravelCm,
                 SuspensionVisualMinTravelCm),
            "airborne wheel visual has bounded extension without fictitious support");

    s = Resting();
    s.Corners[0].DistanceCm = SuspensionRestDistanceCm;
    s.Corners[0].PointVerticalSpeedCmPerSecond = 0.0;
    const auto fullExtension = CalculateSprungChassis(s);
    Require(fullExtension.Corners[0].Contact &&
            fullExtension.Corners[0].UpForce == 0.0,
            "full extension cannot pull a rigid body toward the ground");
    s.Corners[0].DistanceCm = 0.0;
    const auto bottomed = CalculateSprungChassis(s);
    Require(bottomed.Corners[0].UpForce > 0.0 &&
            bottomed.Corners[0].WheelTravelCm ==
                SuspensionVisualMaxTravelCm,
            "bottomed suspension has a finite bounded response");

    s = Resting();
    s.PhysicsReady = false;
    Require(!CalculateSprungChassis(s).Valid,
            "unqualified physics cannot activate springs");
    s = Resting();
    s.DeltaSeconds = 0.11;
    Require(!CalculateSprungChassis(s).Valid,
            "hitch frame cannot submit fresh spring force");
    s = Resting();
    s.DeltaSeconds = 0.0;
    Require(!CalculateSprungChassis(s).Valid,
            "zero frame cannot submit springs");
    s = Resting();
    s.MassKg = 15001;
    Require(!CalculateSprungChassis(s).Valid,
            "unbounded physical mass is rejected");
    s = Resting();
    s.MassKg = 99;
    Require(!CalculateSprungChassis(s).Valid,
            "nonsensical light car mass is rejected");
    s = Resting();
    s.GravityCmPerSecondSquared = -1.0;
    Require(!CalculateSprungChassis(s).Valid,
            "inverted or missing downward gravity does not grant lift");
    s = Resting();
    s.GravityCmPerSecondSquared = 3000.0;
    Require(!CalculateSprungChassis(s).Valid,
            "implausible gravity cannot inject unbounded vertical impulses");
    s = Resting();
    s.GravityCmPerSecondSquared = std::numeric_limits<double>::infinity();
    Require(!CalculateSprungChassis(s).Valid,
            "nonfinite actual world gravity fails closed");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    s = Resting();
    s.Corners[2].DistanceCm = nan;
    Require(!CalculateSprungChassis(s).Valid,
            "NaN distance fails closed");
    s = Resting();
    s.Corners[1].NormalZ = 2.0;
    Require(!CalculateSprungChassis(s).Valid,
            "impossible ground normal fails closed");
    s = Resting();
    s.Corners[3].PointVerticalSpeedCmPerSecond = inf;
    Require(!CalculateSprungChassis(s).Valid,
            "infinite local contact speed fails closed");
    s = Resting();
    s.Corners[0].DistanceCm = -2.0;
    Require(!CalculateSprungChassis(s).Valid,
            "negative measured distance fails closed");

    // Regression: no accumulated force state, no fabricated contacts, no
    // huge impulses or numeric drift across realistic and extreme samples.
    for (int i = 0; i < 120000; ++i)
    {
        s = Resting(100.0 + static_cast<double>((i * 41) % 14901));
        s.DeltaSeconds = 1.0 / (30.0 + static_cast<double>(i % 110));
        const int active = i % 5;
        for (int j = 0; j < 4; ++j)
        {
            s.Corners[static_cast<unsigned>(j)].Hit = j < active;
            s.Corners[static_cast<unsigned>(j)].DistanceCm =
                static_cast<double>((i + j * 17) % 281);
            s.Corners[static_cast<unsigned>(j)].NormalZ =
                (i + j) % 9 == 0 ? 0.5 : 0.99;
            s.Corners[static_cast<unsigned>(j)].PointVerticalSpeedCmPerSecond =
                static_cast<double>(((i * 17 + j * 13) % 1801) - 900);
        }
        const auto out = CalculateSprungChassis(s);
        Require(out.Valid && out.ContactCount >= 0 &&
                out.ContactCount <= 4 && std::isfinite(out.TotalUpForce) &&
                out.TotalUpForce >= 0.0 &&
                out.TotalUpForce <= s.MassKg *
                    SuspensionGravityCmPerSecondSquared *
                    SuspensionMaxPerCornerWeightFactor * 4.0 + 0.01,
                "120k randomized frames respect strict overall impulse budget");
        int counted = 0;
        double sum = 0.0;
        for (const auto& c : out.Corners)
        {
            counted += c.Contact ? 1 : 0;
            sum += c.UpForce;
            Require(std::isfinite(c.UpForce) && c.UpForce >= 0.0 &&
                    c.UpForce <= s.MassKg *
                        SuspensionGravityCmPerSecondSquared *
                        SuspensionMaxPerCornerWeightFactor + 0.001 &&
                    c.WheelTravelCm >= SuspensionVisualMinTravelCm &&
                    c.WheelTravelCm <= SuspensionVisualMaxTravelCm,
                    "every per-corner physical force and visual offset is bounded");
        }
        Require(counted == out.ContactCount &&
                Near(out.TotalUpForce, sum, 0.02),
                "published contacts and total never drift from wheel force outputs");
    }
    std::cout << "Four-corner sprung chassis policy passed (120k frames).\n";
    return EXIT_SUCCESS;
}
