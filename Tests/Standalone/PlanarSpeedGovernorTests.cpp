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

    // Speed-cap correction is an extra contact-dependent arcade brake,
    // not aerodynamic drag or a kinematic velocity cap. Genuine suspended
    // motion must not be slowed by this force before the springs reattach.
    auto contactFrame = Frame(6400.0, 0.0, 6000.0);
    contactFrame.ContactTractionScale = 0.0;
    const auto flying = CalculatePlanarSpeedGovernor(contactFrame);
    Require(flying.Valid && !flying.Active &&
            flying.ForceX == 0.0 && flying.ForceY == 0.0 &&
            flying.DecelerationCmPerSecondSquared == 0.0 &&
            flying.OverspeedCmPerSecond == 0.0,
            "airborne overspeed cannot manufacture tire-like braking");
    contactFrame.ContactTractionScale = 1.0e-7;
    const auto unloaded = CalculatePlanarSpeedGovernor(contactFrame);
    Require(unloaded.Valid && !unloaded.Active &&
            unloaded.ForceX == 0.0 && unloaded.ForceY == 0.0,
            "tiny spring load cannot revive a synthetic braking impulse");

    contactFrame.ContactTractionScale = 0.45;
    const auto halfSupported = CalculatePlanarSpeedGovernor(contactFrame);
    Require(halfSupported.Valid && halfSupported.Active &&
            Near(halfSupported.DecelerationCmPerSecondSquared,
                 0.45 * PlanarGovernorArcadeMaxDeceleration) &&
            Near(halfSupported.ForceX,
                 -1100.0 * 0.45 * PlanarGovernorArcadeMaxDeceleration) &&
            halfSupported.ForceY == 0.0 &&
            halfSupported.DecelerationCmPerSecondSquared <
                overspeed.DecelerationCmPerSecondSquared,
            "two supported corners reduce cap braking exactly once");
    contactFrame.ContactTractionScale = 0.75;
    const auto threeSupported = CalculatePlanarSpeedGovernor(contactFrame);
    Require(threeSupported.Valid && threeSupported.Active &&
            Near(threeSupported.DecelerationCmPerSecondSquared,
                 0.75 * PlanarGovernorArcadeMaxDeceleration),
            "partial spring contact never accelerates overspeed braking");
    contactFrame.ContactTractionScale = 1.0;
    const auto fullySupported = CalculatePlanarSpeedGovernor(contactFrame);
    Require(fullySupported.Valid && fullySupported.Active &&
            Near(fullySupported.ForceX, overspeed.ForceX) &&
            Near(fullySupported.DecelerationCmPerSecondSquared,
                 overspeed.DecelerationCmPerSecondSquared),
            "fully loaded road retains prior calibrated arcade behavior");

    // Explicit service/opposing/reverse braking has priority over this
    // optional cap controller. Otherwise two independent brakes stack.
    auto braking = Frame(6400.0, 0.0, 6000.0);
    braking.OtherLongitudinalBrake = true;
    const auto yielding = CalculatePlanarSpeedGovernor(braking);
    Require(yielding.Valid && !yielding.Active &&
            yielding.ForceX == 0.0 && yielding.ForceY == 0.0 &&
            yielding.OverspeedCmPerSecond == 0.0,
            "deliberate braking takes planar overspeed governor priority");

    // Pure side-slip: existing 1000 acceleration and cap force point
    // in the SAME direction. Only 550 of a 1550 budget remains, not
    // the naive independent sqrt(1550^2-1000^2) allowance.
    auto pureSlide = Frame(0.0, 6400.0, 6000.0);
    pureSlide.LateralCmPerSecond = 6400.0;
    pureSlide.AppliedRightForce = -1100.0 * 1000.0;
    const auto cornering = CalculatePlanarSpeedGovernor(pureSlide);
    Require(cornering.Valid && cornering.Active &&
            Near(cornering.DecelerationCmPerSecondSquared, 550.0) &&
            cornering.ForceX == 0.0 &&
            Near(cornering.ForceY, -1100.0 * 550.0),
            "co-directional slide and cap braking share one vector budget");

    auto diagonalSlip = Frame(4800.0, 6400.0, 6500.0);
    diagonalSlip.LateralCmPerSecond = 6400.0;
    diagonalSlip.AppliedRightForce = -1100.0 * 1000.0;
    const auto diagonalGrip = CalculatePlanarSpeedGovernor(diagonalSlip);
    const double diagonalBudget = std::hypot(
        diagonalGrip.ForceX / 1100.0,
        (diagonalGrip.ForceY + diagonalSlip.AppliedRightForce) / 1100.0);
    Require(diagonalGrip.Valid && diagonalGrip.Active &&
            diagonalGrip.DecelerationCmPerSecondSquared < 650.0 &&
            Near(diagonalBudget, ArcadeCombinedGripBudget, 1.0e-5),
            "diagonal skid saturates one shared force-vector friction circle");

    auto dampedGrip = Frame(0.0, 6400.0, 6000.0);
    dampedGrip.GripScale = 0.20;
    dampedGrip.ContactTractionScale = 0.45;
    dampedGrip.LateralCmPerSecond = 6400.0;
    dampedGrip.AppliedRightForce = -1100.0 * 60.0;
    const auto limitedWet = CalculatePlanarSpeedGovernor(dampedGrip);
    const double wetBudget = ArcadeCombinedGripBudget * 0.20 * 0.45;
    Require(limitedWet.Valid && limitedWet.Active &&
            Near(limitedWet.DecelerationCmPerSecondSquared,
                 wetBudget - 60.0, 1.0e-5) &&
            limitedWet.DecelerationCmPerSecondSquared <
                PlanarGovernorArcadeMaxDeceleration * 0.45,
            "worn low-grip partial road contact limits braking once");

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
    for (double bad : {-0.01, 1.0001,
                       std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()})
    {
        s = Frame(6400.0, 0.0, 6000.0);
        s.ContactTractionScale = bad;
        const auto rejected = CalculatePlanarSpeedGovernor(s);
        Require(!rejected.Valid && !rejected.Active &&
                rejected.ForceX == 0.0 && rejected.ForceY == 0.0,
                "invalid sampled spring grip cannot authorize cap braking");
    }
    for (double bad : {-0.01, 4.001,
                       std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()})
    {
        s = Frame(6400.0, 0.0, 6000.0);
        s.GripScale = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "invalid physical grip cannot authorize cap braking");
    }
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()})
    {
        s = Frame(0.0, 6400.0, 6000.0);
        s.LateralCmPerSecond = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite true lateral speed fails closed");
        s = Frame(0.0, 6400.0, 6000.0);
        s.AppliedRightForce = bad;
        Require(!CalculatePlanarSpeedGovernor(s).Valid,
                "nonfinite delivered lateral force fails closed");
    }
    s = Frame(0.0, 6400.0, 6000.0);
    s.LateralCmPerSecond = 6400.0;
    s.AppliedRightForce = 1100.0 * 500.0;
    Require(!CalculatePlanarSpeedGovernor(s).Valid,
            "fabricated lateral slip-accelerating force fails closed");
    s = Frame(6400.0, 0.0, 6000.0);
    s.LateralCmPerSecond = 6401.5;
    Require(!CalculatePlanarSpeedGovernor(s).Valid,
            "impossible measured sideways projection fails closed");
    s = Frame(0.0, 6400.0, 6000.0);
    s.LateralCmPerSecond = 6400.0;
    s.AppliedRightForce = -1100.0 * 1700.0;
    Require(!CalculatePlanarSpeedGovernor(s).Valid,
            "fabricated force over full contact-grip limit fails closed");

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
        s.ContactTractionScale = i % 5 == 0 ? 0.0 :
            i % 5 == 1 ? 0.20 : i % 5 == 2 ? 0.45 :
            i % 5 == 3 ? 0.75 : 1.0;
        s.GripScale = 0.10 + (i % 350) * 0.01;
        s.LateralCmPerSecond = s.VelocityY; // right = world +Y
        const double shared = (s.ArcadeHandling
            ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) *
            s.GripScale * s.ContactTractionScale;
        s.AppliedRightForce = std::abs(s.LateralCmPerSecond) < 1.0e-7
            ? 0.0 : -std::copysign(0.63 * shared * s.MassKg,
                                  s.LateralCmPerSecond);
        s.OtherLongitudinalBrake = i % 17 == 0;
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
                        : PlanarGovernorSimulationMaxDeceleration) *
                            s.ContactTractionScale + 1.0e-8 &&
                    d * dt <= out.OverspeedCmPerSecond + 1.0e-8 &&
                    out.ForceX * s.VelocityX +
                    out.ForceY * s.VelocityY <= 0.0 &&
                    std::hypot(out.ForceX / s.MassKg,
                        (out.ForceY + s.AppliedRightForce) / s.MassKg) <=
                            shared + 1.0e-6 &&
                    !s.OtherLongitudinalBrake,
                    "120k samples share one grip vector and never stack braking");
        }
        else
        {
            Require(out.ForceX == 0.0 && out.ForceY == 0.0 &&
                    out.OverspeedCmPerSecond == 0.0,
                    "non-overspeed or unsupported sample never creates a force");
        }
    }
    std::cout << "Planar speed governor passed 120k independent frames.\n";
    return EXIT_SUCCESS;
}
