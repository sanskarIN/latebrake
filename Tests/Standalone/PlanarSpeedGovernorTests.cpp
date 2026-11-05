#include "Vehicles/PlanarSpeedGovernor.h"

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
        std::cerr << "PlanarSpeedGovernor FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x, double y, double epsilon = 1.0e-7)
{
    return std::isfinite(x) && std::isfinite(y) &&
        std::abs(x - y) <= epsilon;
}
latebrake::PlanarSpeedGovernorSample Frame(
    double velocityX, double velocityY, double cap,
    bool arcade = true, double dt = 1.0 / 60.0)
{
    return {dt, velocityX, velocityY, 1100.0, cap, arcade};
}
double Integrate(double initialSpeed, double cap, int fps)
{
    using namespace latebrake;
    double speed = initialSpeed;
    const double dt = 1.0 / fps;
    for (int step = 0; step < 8 * fps; ++step)
    {
        const auto out = CalculatePlanarSpeedGovernor(
            Frame(speed, 0.0, cap, true, dt));
        Require(out.Valid && out.Active &&
                out.ForceX <= 0.0 && out.ForceY == 0.0,
                "overspeed regulation uses finite opposite planar force");
        const double next = speed + out.ForceX / 1100.0 * dt;
        Require(next <= speed &&
                next >= cap - 1.0e-8,
                "governor alone never accelerates or snaps under speed cap");
        speed = next;
    }
    return speed;
}
} // namespace

int main()
{
    using namespace latebrake;
    for (double speed : {0.0, 20.0, 2500.0, 6000.0})
    {
        const auto out = CalculatePlanarSpeedGovernor(
            Frame(speed, 0.0, 6000.0));
        Require(out.Valid && !out.Active && out.ForceX == 0.0 &&
                out.ForceY == 0.0 && out.OverspeedCmPerSecond == 0.0,
                "at and below active cap no invented force is applied");
    }
    const auto overspeed = CalculatePlanarSpeedGovernor(
        Frame(6400.0, 0.0, 6000.0));
    Require(overspeed.Valid && overspeed.Active &&
            Near(overspeed.DecelerationCmPerSecondSquared,
                 PlanarGovernorArcadeMaxDeceleration) &&
            Near(overspeed.ForceX, -1100.0 * 650.0) &&
            overspeed.ForceY == 0.0 &&
            Near(overspeed.OverspeedCmPerSecond, 400.0),
            "large overspeed is capped at finite mass-scaled arcade deceleration");

    const auto simulation = CalculatePlanarSpeedGovernor(
        Frame(6400.0, 0.0, 6000.0, false));
    Require(simulation.Valid && simulation.Active &&
            Near(simulation.DecelerationCmPerSecondSquared,
                 PlanarGovernorSimulationMaxDeceleration),
            "Simulation Inspired applies its separately bounded force");

    const auto slight = CalculatePlanarSpeedGovernor(
        Frame(6000.1, 0.0, 6000.0));
    Require(slight.Valid && slight.Active &&
            Near(slight.DecelerationCmPerSecondSquared, 0.3, 1.0e-5),
            "speed cap force begins continuously, not as hard velocity snap");

    const auto leftSlide = CalculatePlanarSpeedGovernor(
        Frame(0.0, 6800.0, 6000.0));
    Require(leftSlide.Valid && leftSlide.Active &&
            leftSlide.ForceX == 0.0 && leftSlide.ForceY < 0.0,
            "pure lateral drift remains an XY-only resisting force");
    const auto reverse = CalculatePlanarSpeedGovernor(
        Frame(-7000.0, 0.0, 6000.0));
    Require(reverse.Valid && reverse.Active && reverse.ForceX > 0.0,
            "negative motion is slowed toward zero instead of given free thrust");
    const auto diagonal = CalculatePlanarSpeedGovernor(
        Frame(4800.0, 6400.0, 6500.0));
    Require(diagonal.Valid && diagonal.Active &&
            Near(diagonal.ForceX * 6400.0 -
                     diagonal.ForceY * 4800.0, 0.0, 0.01) &&
            diagonal.ForceX * 4800.0 + diagonal.ForceY * 6400.0 < 0.0,
            "diagonal force directly opposes actual velocity without yaw impulse");

    // A nitro cap can fall while the car is still traveling at the
    // former higher cap. No instant chassis SetPhysicsLinearVelocity is
    // allowed: 30/60/120 FPS force integration should converge instead.
    for (const int fps : {30, 60, 120})
    {
        const auto boostHeld = CalculatePlanarSpeedGovernor(
            Frame(7200.0, 0.0, 7800.0, true, 1.0 / fps));
        Require(boostHeld.Valid && !boostHeld.Active,
                "qualified higher nitro cap allows intended vehicle speed");
        const double final = Integrate(7200.0, 6500.0, fps);
        Require(final >= 6500.0 && final < 6500.02,
                "nitro-cap release decelerates smoothly back toward normal limit");
    }
    Require(std::abs(Integrate(7200.0, 6500.0, 30) -
                     Integrate(7200.0, 6500.0, 120)) < 0.02,
            "30/120 FPS source integration reaches matching bounded terminal speed");

    auto s = Frame(6000.0, 0.0, 6500.0);
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                      -std::numeric_limits<double>::infinity()})
    {
        s.VelocityX = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite motion cannot authorize force");
        s = Frame(6000.0, 0.0, 6500.0);
        s.MassKg = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite body mass fails closed");
        s = Frame(6000.0, 0.0, 6500.0);
        s.SpeedCapCmPerSecond = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite boost cap fails closed");
        s = Frame(6000.0, 0.0, 6500.0);
        s.DeltaSeconds = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite physics time fails closed");
        s = Frame(6000.0, 0.0, 6500.0);
    }
    for (double dt : {0.0, -0.02, 0.100001})
    {
        s = Frame(6000.0, 0.0, 6500.0, true, dt);
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "unqualified frame duration cannot generate speed correction");
    }
    for (double cap : {1.0, 499.0, 20001.0})
    {
        s = Frame(6000.0, 0.0, cap);
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "impossible cap must never authorize a corrective force");
    }
    Require(!CalculatePlanarSpeedGovernor(
        Frame(15001.0, 0.0, 6000.0)).Valid,
        "same bound as real road load rejects absurd Chaos speeds");

    // Vary diagonal angles, frame step, mass and rapidly changing cap
    // for 120k independent production-policy samples. Forces must be
    // finite, never aligned with motion and never exceed the mass-scaled
    // deceleration ceiling, even across arcade/sim profile changes.
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < 120000; ++i)
    {
        const double angle = (i % 720) * pi / 360.0;
        const double speed = 10.0 + (i % 14900);
        const double dt = 1.0 / (i % 3 == 0 ? 30.0 :
                                  i % 3 == 1 ? 60.0 : 120.0);
        s = {dt, speed * std::cos(angle), speed * std::sin(angle),
             100.0 + (i % 14901), 500.0 + (i % 13001), i % 2 == 0};
        const auto out = CalculatePlanarSpeedGovernor(s);
        Require(out.Valid &&
                std::isfinite(out.ForceX) && std::isfinite(out.ForceY),
                "120k finite valid frames retain finite force outputs");
        if (out.Active)
        {
            const double d = out.DecelerationCmPerSecondSquared;
            Require(d > 0.0 &&
                    d <= (s.ArcadeHandling
                        ? PlanarGovernorArcadeMaxDeceleration
                        : PlanarGovernorSimulationMaxDeceleration) + 1.0e-8 &&
                    d * dt <= out.OverspeedCmPerSecond + 1.0e-8 &&
                    out.ForceX * s.VelocityX +
                    out.ForceY * s.VelocityY <= 0.0,
                    "opposing deceleration cannot exceed cap or reverse direction");
        }
        else
        {
            Require(out.ForceX == 0.0 && out.ForceY == 0.0 &&
                    out.OverspeedCmPerSecond == 0.0,
                    "non-overspeed sample never creates a force");
        }
    }
    std::cout << "Planar speed governor passed 120k independent frames.\n";
    return EXIT_SUCCESS;
}
