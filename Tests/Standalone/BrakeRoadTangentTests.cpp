#include "Vehicles/BrakeRoadTangent.h"
#include "Vehicles/SprungContactBraking.h"
#include "Vehicles/GripLimitedBraking.h"
#include "Vehicles/LateralGripBudget.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "BrakeRoadTangent FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
struct Vec
{
    double x, y, z;
    Vec operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec operator+(Vec v) const { return {x + v.x, y + v.y, z + v.z}; }
    double Dot(Vec v) const { return x * v.x + y * v.y + z * v.z; }
    Vec Cross(Vec v) const { return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x}; }
    double Length() const { return std::sqrt(Dot(*this)); }
};
bool Near(double a, double b, double eps = 1.0e-8) { return std::abs(a - b) < eps; }
Vec Rotate(Vec v, double yaw)
{
    return {std::cos(yaw) * v.x - std::sin(yaw) * v.y,
            std::sin(yaw) * v.x + std::cos(yaw) * v.y, v.z};
}
latebrake::BrakeRoadTangentOutput Tangent(Vec f, Vec n, Vec v)
{
    return latebrake::MeasureBrakeRoadTangent({f.x, f.y, n.x, n.y, n.z, v.x, v.y, v.z});
}
latebrake::SuspensionResponse Loads(std::array<double, 4> fractions)
{
    latebrake::SuspensionResponse s;
    s.Valid = true;
    s.WeightForce = 1100.0 * 980.0;
    for (unsigned i = 0; i < 4; ++i)
    {
        s.Corners[i].Contact = fractions[i] > 0.0;
        s.Corners[i].UpForce = fractions[i] * s.WeightForce;
        s.TotalUpForce += s.Corners[i].UpForce;
        if (s.Corners[i].Contact) ++s.ContactCount;
    }
    return s;
}
latebrake::ChassisGroundContactOutput Ground(const latebrake::SuspensionResponse& s)
{
    latebrake::ChassisGroundContactOutput g;
    g.Valid = true;
    g.SupportingProbes = s.ContactCount;
    return latebrake::ConstrainGroundContactToSuspension(g, s);
}
}
int main()
{
    using namespace latebrake;
    const Vec forward{1.0, 0.0, 0.0};
    const Vec flat{0.0, 0.0, 1.0};
    const auto level = Tangent(forward, flat, {3000.0, 250.0, 700.0});
    Require(level.Valid && level.DirectionX == 1.0 && level.DirectionY == 0.0 &&
            level.DirectionZ == 0.0 && level.LongitudinalCmPerSecond == 3000.0,
            "level brake ignores lateral motion and heave exactly");
    const Vec uphill{-0.6, 0.0, 0.8};
    const auto up = Tangent(forward, uphill, {2400.0, 0.0, 1800.0});
    Require(up.Valid && Near(up.DirectionX, 0.8) && Near(up.DirectionZ, 0.6) &&
            Near(up.LongitudinalCmPerSecond, 3000.0),
            "uphill longitudinal force follows measured plane instead of pushing into it");
    const auto down = Tangent(forward, {0.6, 0.0, 0.8}, {2400.0, 0.0, -1800.0});
    Require(down.Valid && Near(down.DirectionZ, -0.6) &&
            Near(down.LongitudinalCmPerSecond, 3000.0), "downhill tangent has correct vertical sign");
    const auto bank = Tangent(forward, {0.0, 0.6, 0.8}, {3000.0, 0.0, 2000.0});
    Require(bank.Valid && bank.DirectionX == 1.0 && bank.DirectionY == 0.0 &&
            bank.DirectionZ == 0.0, "pure banking never invents longitudinal grade");
    const auto normalMotion = Tangent(forward, uphill, uphill * 5000.0);
    Require(normalMotion.Valid && std::abs(normalMotion.LongitudinalCmPerSecond) < 1.0e-9,
            "road-normal bounce cannot count as longitudinal tire motion");
    Require(std::abs((forward * -990000.0).Dot(uphill)) > 500000.0,
            "old horizontal brake has a substantial forbidden normal component");
    Require(std::abs((Vec{up.DirectionX, up.DirectionY, up.DirectionZ} * -990000.0).Dot(uphill)) < 1.0e-8,
            "new brake has no road-normal force component");
    for (const double bad : {std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity()})
    {
        for (unsigned field = 0; field < 8; ++field)
        {
            std::array<double, 8> a{1.0, 0.0, 0.0, 0.0, 1.0, 3000.0, 0.0, 0.0};
            a[field] = bad;
            Require(!MeasureBrakeRoadTangent({a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]}).Valid,
                    "every malformed sensor field rejects before force dispatch");
        }
    }
    Require(!Tangent({0.0, 0.0, 0.0}, flat, {}).Valid, "undefined heading rejected");
    Require(!Tangent({1.0, 0.1, 0.0}, flat, {}).Valid, "nonunit heading rejected");
    Require(!Tangent(forward, {0.0, 0.0, 0.9}, {}).Valid, "nonunit normal rejected");
    Require(!Tangent(forward, {0.8, 0.0, 0.6}, {}).Valid, "wall/unsupported steep road rejected");
    Require(!Tangent(forward, {0.0, 0.0, -1.0}, {}).Valid, "underside rejected");
    Require(!Tangent(forward, flat, {100001.0, 0.0, 0.0}).Valid, "extreme sensor speed rejected");
    const auto normalized = Tangent({0.9999, 0.0, 0.0}, flat, {3000.0, 0.0, 0.0});
    Require(normalized.Valid && normalized.DirectionX == 1.0,
            "accepted small heading sensor error does not scale brake strength");
    auto springs = Loads({0.25, 0.25, 0.25, 0.25});
    const auto ground = Ground(springs);
    SprungContactBrakeSample b;
    b.MassKg = 1100.0;
    b.ForwardCmPerSecond = 2400.0;
    b.RequestedForwardForce = -990000.0;
    b.ContactSamplesValid = true;
    b.ContactForwardCmPerSecond.fill(3000.0);
    b.PlanarForwardScales.fill(0.8);
    auto brake = CalculateSprungContactBraking(b, ground, springs);
    Require(brake.Valid && Near(brake.TotalForwardForce, -792000.0) &&
            Near(brake.TotalBrakeForceMagnitude, 990000.0) &&
            Near(brake.WholeBodyBudgetDeceleration, 900.0),
            "grip spends full 3D force while telemetry reports actual horizontal projection");
    b.PlanarForwardScales[3] = 0.1;
    Require(!CalculateSprungContactBraking(b, ground, springs).Valid,
            "invalid road projection rejects the complete plan");

    // Independent cross-product plane intersection, yaw-coordinate changes,
    // and full vector power/friction checks across different corner planes.
    std::mt19937 rng(0xb8a97);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    for (unsigned frame = 0; frame < 120000; ++frame)
    {
        const double yaw = unit(rng) * 6.283185307179586;
        const Vec f{std::cos(yaw), std::sin(yaw), 0.0};
        const Vec right{-f.y, f.x, 0.0};
        std::array<double, 4> loads{};
        for (auto& load : loads) load = unit(rng) < 0.15 ? 0.0 : unit(rng) * 0.85;
        springs = Loads(loads);
        const auto g = Ground(springs);
        b = {};
        b.MassKg = 1100.0;
        b.ForwardCmPerSecond = (frame % 2 == 0 ? 1.0 : -1.0) * (100.0 + unit(rng) * 8000.0);
        b.RequestedForwardForce = -std::copysign(990000.0 * g.ContactTractionScale, b.ForwardCmPerSecond);
        b.ContactSamplesValid = true;
        b.RearOnly = frame % 4 == 0;
        std::array<Vec, 4> directions{}, velocities{}, normals{};
        for (unsigned i = 0; i < 4; ++i)
        {
            const double nz = 0.68 + unit(rng) * 0.32;
            const double bearing = unit(rng) * 6.283185307179586;
            const double horizontal = std::sqrt(1.0 - nz * nz);
            normals[i] = {horizontal * std::cos(bearing), horizontal * std::sin(bearing), nz};
            velocities[i] = {30000.0 * (unit(rng) - 0.5), 30000.0 * (unit(rng) - 0.5),
                             10000.0 * (unit(rng) - 0.5)};
            const auto t = Tangent(f, normals[i], velocities[i]);
            Require(t.Valid, "valid grade/bank measured frame accepted");
            directions[i] = {t.DirectionX, t.DirectionY, t.DirectionZ};
            Vec reference = right.Cross(normals[i]);
            reference = reference * (1.0 / reference.Length());
            Require((directions[i] + reference * -1.0).Length() < 1.0e-9 &&
                    std::abs(directions[i].Dot(normals[i])) < 1.0e-9 &&
                    std::abs(directions[i].Dot(right)) < 1.0e-9 &&
                    Near(directions[i].Length(), 1.0),
                    "independent cross product agrees: unit, tangent, no sideways bypass");
            const double rotation = unit(rng) * 6.283185307179586;
            const auto rotated = Tangent(Rotate(f, rotation), Rotate(normals[i], rotation),
                                          Rotate(velocities[i], rotation));
            const Vec expected = Rotate(directions[i], rotation);
            Require(rotated.Valid && Near(rotated.DirectionX, expected.x) &&
                    Near(rotated.DirectionY, expected.y) && Near(rotated.DirectionZ, expected.z) &&
                    Near(rotated.LongitudinalCmPerSecond, t.LongitudinalCmPerSecond, 1.0e-7),
                    "world yaw rotation does not change policy or measured speed");
            b.ContactForwardCmPerSecond[i] = t.LongitudinalCmPerSecond;
            b.PlanarForwardScales[i] = t.PlanarForwardScale;
        }
        brake = CalculateSprungContactBraking(b, g, springs);
        Require(brake.Valid, "consistent road tangent allocation accepted");
        Vec total{0.0, 0.0, 0.0};
        double magnitudeSum = 0.0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const Vec force = directions[i] * brake.TangentForces[i];
            total = total + force;
            magnitudeSum += force.Length();
            Require(force.Dot(velocities[i]) <= 1.0e-5 &&
                    std::abs(force.Dot(normals[i])) < 1.0e-7,
                    "each delivered brake is passive and has zero road-normal component");
        }
        Require(Near(total.Dot(f), brake.TotalForwardForce, 1.0e-6) &&
                Near(magnitudeSum, brake.TotalBrakeForceMagnitude, 1.0e-6) &&
                magnitudeSum <= std::abs(b.RequestedForwardForce) + 1.0e-6,
                "telemetry projection and admitted magnitude agree with actual 3D forces");
        const auto lateral = CalculateLateralGripForce({
            1.0 / 60.0, 1800.0, 1100.0, 4.0, 1.0,
            brake.WholeBodyBudgetDeceleration, true, false});
        Require(lateral.Valid, "full 3D braking feeds original lateral budget");
        const Vec combined = total + right * (lateral.RightForce * g.ContactTractionScale);
        Require(combined.Length() <= 1100.0 * ArcadeCombinedGripBudget * g.ContactTractionScale + 1.0e-5,
                "grade/bank force cannot exceed shared three-dimensional grip circle");
    }
    springs = Loads({0.25, 0.25, 0.25, 0.25});
    for (double fps : {30.0, 60.0, 120.0})
    for (double nz : {0.68, 0.8, 1.0})
    for (double sign : {-1.0, 1.0})
    {
        const Vec n{-std::sqrt(1.0 - nz * nz), 0.0, nz};
        const auto t = Tangent(forward, n, {});
        const Vec direction{t.DirectionX, t.DirectionY, t.DirectionZ};
        double speed = sign * 4000.0, distance = 0.0;
        for (unsigned frame = 0; frame < static_cast<unsigned>(fps * 12); ++frame)
        {
            const auto request = CalculateGripLimitedBraking({
                1.0 / fps, speed * t.PlanarForwardScale, 1100.0, true, false, true,
                1.0, 1.0, 1.0, 1.0});
            b = {};
            b.MassKg = 1100.0; b.ContactSamplesValid = true;
            b.ForwardCmPerSecond = speed * t.PlanarForwardScale;
            b.RequestedForwardForce = request.ForwardForce;
            b.ContactForwardCmPerSecond.fill(speed);
            b.PlanarForwardScales.fill(t.PlanarForwardScale);
            brake = CalculateSprungContactBraking(b, ground, springs);
            const Vec force = direction * (brake.TangentForces[0] + brake.TangentForces[1] +
                                          brake.TangentForces[2] + brake.TangentForces[3]);
            const double acceleration = force.Dot(direction) / 1100.0;
            distance += speed / fps + 0.5 * acceleration / (fps * fps);
            speed += acceleration / fps;
            Require(brake.Valid && speed * sign >= -1.0e-8,
                    "isolated tangent braking cannot reverse motion at 30/60/120 FPS");
            if (std::abs(speed) < 1.0e-8) break;
        }
        // No gravity/spring/contact solver here: isolated longitudinal
        // deceleration must agree with v^2/(2a), including the near-stop cap.
        Require(std::abs(speed) < 1.0e-6 &&
                std::abs(std::abs(distance) - 4000.0 * 4000.0 / 1800.0) < 2.0,
                "isolated plane stopping agrees within 2cm of analytic constant deceleration");
    }
    std::cout << "Road-tangent brakes: 120000 mixed-plane frames, independent geometry/power/grip, FPS stopping passed\n";
}
