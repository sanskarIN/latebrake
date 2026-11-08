#include "Vehicles/SprungContactBraking.h"
#include "Vehicles/GripLimitedBraking.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass)
    {
        std::cerr << "SprungContactBraking FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double eps = 1.0e-7)
{
    return std::abs(a - b) <= eps;
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
latebrake::ChassisGroundContactOutput Ground(
    const latebrake::SuspensionResponse& s)
{
    latebrake::ChassisGroundContactOutput coarse;
    coarse.Valid = true;
    coarse.SupportingProbes = s.ContactCount;
    return latebrake::ConstrainGroundContactToSuspension(coarse, s);
}
latebrake::SprungContactBrakeSample Sample(double speed = 3000.0)
{
    latebrake::SprungContactBrakeSample b;
    b.MassKg = 1100.0;
    b.ForwardCmPerSecond = speed;
    b.RequestedForwardForce = -std::copysign(990000.0, speed);
    b.ContactSamplesValid = true;
    b.ContactForwardCmPerSecond.fill(speed);
    return b;
}
}
int main()
{
    using namespace latebrake;
    const auto level = Loads({0.25, 0.25, 0.25, 0.25});
    const auto ground = Ground(level);
    auto b = Sample();
    auto r = CalculateSprungContactBraking(b, ground, level);
    Require(r.Valid && Near(r.TotalForwardForce, -990000.0) &&
            Near(r.WholeBodyBudgetDeceleration, 900.0),
            "flat service braking preserves accepted whole-chassis force");
    for (double f : r.ForwardForces)
        Require(Near(f, -247500.0), "four equally loaded contacts share force");
    // Independent wrench: road is 59cm BELOW the COM. A COM force has no
    // pitch moment; real road-point braking must have positive Y moment.
    double pitchMoment = 0.0;
    double yawMoment = 0.0;
    for (unsigned i = 0; i < 4; ++i)
    {
        const double y = i % 2 == 0 ? -94.0 : 94.0;
        pitchMoment += -59.0 * r.ForwardForces[i];
        yawMoment += -y * r.ForwardForces[i];
    }
    Require(Near(pitchMoment, 59.0 * 990000.0) && Near(yawMoment, 0.0),
            "balanced road forces produce braking pitch without artificial yaw");
    b = Sample(-3000.0);
    r = CalculateSprungContactBraking(b, ground, level);
    Require(r.Valid && Near(r.TotalForwardForce, 990000.0),
            "reversing braking reverses road force and pitch");
    b = Sample();
    b.RearOnly = true;
    r = CalculateSprungContactBraking(b, ground, level);
    Require(r.Valid && r.ForwardForces[0] == 0.0 &&
            r.ForwardForces[1] == 0.0 && Near(r.TotalForwardForce, -495000.0),
            "handbrake alone spends rear shares without moving front allocation");
    const auto frontOnly = Loads({0.40, 0.40, 0.0, 0.0});
    auto g = Ground(frontOnly);
    b.RequestedForwardForce *= g.ContactTractionScale;
    r = CalculateSprungContactBraking(b, g, frontOnly);
    Require(r.Valid && r.TotalForwardForce == 0.0,
            "front-only landing cannot create rear handbrake force");
    b.RearOnly = false;
    r = CalculateSprungContactBraking(b, g, frontOnly);
    Require(r.Valid && r.TotalForwardForce < 0.0 &&
            r.ForwardForces[2] == 0.0 && r.ForwardForces[3] == 0.0 &&
            Near(r.WholeBodyBudgetDeceleration, 900.0),
            "partial contact scaling is applied once, not squared");
    const auto flight = Loads({0.0, 0.0, 0.0, 0.0});
    r = CalculateSprungContactBraking(Sample(), Ground(flight), flight);
    Require(r.Valid && r.TotalForwardForce == 0.0,
            "airborne body has no synthetic brake force");
    b = Sample();
    b.ContactForwardCmPerSecond = {-100.0, 0.0, 1500.0, 6000.0};
    r = CalculateSprungContactBraking(b, ground, level);
    Require(r.Valid && r.ForwardForces[0] == 0.0 &&
            r.ForwardForces[1] == 0.0 && Near(r.ForwardForces[2], -123750.0) &&
            Near(r.ForwardForces[3], -247500.0),
            "spin suppresses energy-adding corners, fades slow contacts, never reallocates");
    auto malformed = level;
    malformed.TotalUpForce += 1.0;
    Require(!CalculateSprungContactBraking(b, ground, malformed).Valid,
            "inconsistent summed loads fail before any corner dispatch");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()})
    {
        b = Sample(); b.MassKg = bad;
        Require(!CalculateSprungContactBraking(b, ground, level).Valid, "bad mass");
        b = Sample(); b.RequestedForwardForce = bad;
        Require(!CalculateSprungContactBraking(b, ground, level).Valid, "bad force");
        b = Sample(); b.ForwardCmPerSecond = bad;
        Require(!CalculateSprungContactBraking(b, ground, level).Valid, "bad COM speed");
        for (unsigned i = 0; i < 4; ++i)
        {
            b = Sample(); b.ContactForwardCmPerSecond[i] = bad;
            Require(!CalculateSprungContactBraking(b, ground, level).Valid,
                    "every loaded corrupt sensor rejects entire plan");
        }
    }
    b = Sample(); b.ContactSamplesValid = false;
    Require(!CalculateSprungContactBraking(b, ground, level).Valid, "bad impact point");
    b = Sample(); b.RequestedForwardForce *= -1.0;
    Require(!CalculateSprungContactBraking(b, ground, level).Valid, "brakes cannot propel COM");
    b = Sample(); b.ForwardCmPerSecond = 0.0;
    Require(!CalculateSprungContactBraking(b, ground, level).Valid, "stationary nonzero request");
    b.RequestedForwardForce = 0.0;
    Require(CalculateSprungContactBraking(b, ground, level).Valid, "stationary neutral");
    auto tinyContact = ground;
    tinyContact.ContactTractionScale = 1.0e-5;
    Require(!CalculateSprungContactBraking(Sample(), tinyContact, level).Valid,
            "unscaled request cannot overflow the equivalent lateral budget");
    // Deterministic yaw/pitch rigid-body point velocities and independently
    // varied wheel loads. Verify local mechanical power and whole budget,
    // rather than merely restating the allocation formula.
    std::mt19937 rng(0x5b8a);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    for (unsigned n = 0; n < 150000; ++n)
    {
        std::array<double, 4> f{};
        for (auto& value : f) value = unit(rng) < 0.2 ? 0.0 : unit(rng) * 0.85;
        const auto s = Loads(f);
        const auto contact = Ground(s);
        const double v = (n % 2 == 0 ? 1.0 : -1.0) * (1.0 + 10000.0 * unit(rng));
        b = Sample(v);
        b.RearOnly = n % 3 == 0;
        b.RequestedForwardForce *= contact.ContactTractionScale;
        const double yaw = (unit(rng) - 0.5) * 80.0;
        const double pitch = (unit(rng) - 0.5) * 20.0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const double y = i % 2 == 0 ? -94.0 : 94.0;
            b.ContactForwardCmPerSecond[i] = v - yaw * y + pitch * -59.0;
        }
        r = CalculateSprungContactBraking(b, contact, s);
        Require(r.Valid, "valid real-shaped measured frame accepted");
        double sum = 0.0;
        double power = 0.0;
        for (unsigned i = 0; i < 4; ++i)
        {
            const double force = r.ForwardForces[i];
            sum += force;
            power += force * b.ContactForwardCmPerSecond[i];
            Require(force * b.ContactForwardCmPerSecond[i] <= 1.0e-7,
                    "each brake contact dissipates or produces zero mechanical power");
            if (f[i] == 0.0 || (b.RearOnly && i < 2))
                Require(force == 0.0, "unloaded or nonbraking axle never receives force");
        }
        Require(Near(sum, r.TotalForwardForce) && power <= 1.0e-7 &&
                std::abs(sum) <= std::abs(b.RequestedForwardForce) + 1.0e-7,
                "summed force stays within admitted force; no energy injection");
    }
    for (double fps : {30.0, 60.0, 120.0})
    {
        for (double sign : {-1.0, 1.0})
        {
            double speed = sign * 4000.0;
            double distance = 0.0;
            for (unsigned frame = 0; frame < static_cast<unsigned>(fps * 10); ++frame)
            {
                const auto brake = CalculateGripLimitedBraking({
                    1.0 / fps, speed, 1100.0, true, false, true,
                    1.0, 1.0, 1.0, 1.0});
                b = Sample(speed);
                b.RequestedForwardForce = brake.ForwardForce;
                r = CalculateSprungContactBraking(b, ground, level);
                Require(r.Valid, "continuous brake chain accepted");
                const double acceleration = r.TotalForwardForce / 1100.0;
                distance += speed / fps + 0.5 * acceleration / (fps * fps);
                speed += acceleration / fps;
                Require(speed * sign >= -1.0e-9, "brake integration never reverses");
                if (std::abs(speed) < 1.0e-8) break;
            }
            Require(std::abs(speed) < 1.0e-6 &&
                    std::abs(std::abs(distance) - 4000.0 * 4000.0 / 1800.0) < 0.6,
                    "flat stopping distance matches physical v^2/(2a) at 30/60/120 FPS");
        }
    }
    std::cout << "Sprung contact braking: forces, road moments, 150000 passive frames, FPS stopping passed\n";
}
