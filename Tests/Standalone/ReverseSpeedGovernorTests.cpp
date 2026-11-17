#include "Vehicles/ReverseSpeedGovernor.h"
#include "Vehicles/LateralGripBudget.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes, const char* why)
{
    if (!yes)
    {
        std::cerr << "ReverseSpeedGovernor FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double eps = 1.0e-7)
{
    return std::isfinite(a) && std::isfinite(b) &&
        std::abs(a - b) <= eps;
}
latebrake::ReverseSpeedGovernorSample Sample(
    double vx, double vy, double fx=1.0, double fy=0.0,
    bool arcade=true, double dt=1.0/60.0)
{
    return {dt, vx, vy, fx, fy, 1100.0, -1.0,
            1.0, 1.0, 1.0, arcade, false};
}
double Settle(int fps)
{
    using namespace latebrake;
    double longitudinal = -3800.0;
    const double dt=1.0 / static_cast<double>(fps);
    const double cap=MaximumReverseSpeedCmPerSecond(true);
    for(int n=0;n<12*fps;++n)
    {
        const auto o=CalculateReverseSpeedGovernor(
            Sample(longitudinal, 0.0, 1.0, 0.0, true, dt));
        Require(o.Valid && o.ReverseMotorBlocked,
                "reverse thrust is blocked at and beyond speed target");
        if (o.Active)
        {
            Require(o.ForceX > 0.0 && o.ForceY == 0.0,
                    "governor applies finite forward force, not reverse thrust");
        }
        const double next=longitudinal+o.ForceX/1100.0*dt;
        Require(next>=longitudinal && next<=-cap+1.0e-6,
                "governor alone never inverts reverse travel or crosses target");
        longitudinal=next;
    }
    return longitudinal;
}
} // namespace

int main()
{
    using namespace latebrake;
    const auto idle=CalculateReverseSpeedGovernor(Sample(0.0, 0.0));
    Require(idle.Valid && !idle.Active && !idle.ReverseMotorBlocked &&
            Near(idle.ForceX,0.0) && Near(idle.ForceY,0.0),
            "resting car has zero invented force");
    for (double speed : {-700.0, -1300.0, 1000.0, 6900.0})
    {
        const auto r=CalculateReverseSpeedGovernor(Sample(speed, 130.0));
        Require(r.Valid && !r.Active && !r.ReverseMotorBlocked &&
                r.ForceX==0.0 && r.ForceY==0.0,
                "below reverse target and forward cruising are untouched");
    }
    const auto nearCap=CalculateReverseSpeedGovernor(Sample(-1349.0, 300.0));
    Require(nearCap.Valid && !nearCap.Active && nearCap.ReverseMotorBlocked,
            "close to reverse target only requested extra reverse motor is blocked");

    const auto fast=CalculateReverseSpeedGovernor(Sample(-3800.0, 750.0));
    Require(fast.Valid && fast.Active && fast.ReverseMotorBlocked &&
            Near(fast.ExcessReverseCmPerSecond, 2450.0) &&
            Near(fast.DecelerationCmPerSecondSquared,850.0) &&
            Near(fast.ForceX,1100.0*850.0) && fast.ForceY==0.0,
            "deep overspeed applies bounded, mass-scaled, forward-only force");
    const auto sim=CalculateReverseSpeedGovernor(
        Sample(-3800.0, 750.0, 1.0, 0.0, false));
    Require(sim.Valid && sim.Active &&
            Near(sim.DecelerationCmPerSecondSquared,650.0),
            "simulation mode has separate reverse deceleration ceiling");

    auto sideways=Sample(1000.0,-3800.0,0.0,2.0);
    const auto slide=CalculateReverseSpeedGovernor(sideways);
    Require(slide.Valid && slide.Active && slide.ForceX==0.0 &&
            slide.ForceY>0.0 && Near(slide.ExcessReverseCmPerSecond,2450.0),
            "yaw-aligned force cannot remove lateral drifting world X");

    auto wet=Sample(-3800.0,500.0);
    wet.GripScale=0.5;
    wet.RearTractionScale=0.45;
    wet.ContactTractionScale=0.75;
    const auto lowGrip=CalculateReverseSpeedGovernor(wet);
    Require(lowGrip.Valid && lowGrip.Active &&
            Near(lowGrip.DecelerationCmPerSecondSquared,850.0*0.5*0.45) &&
            Near(lowGrip.WholeBodyBudgetDeceleration,
                 lowGrip.DecelerationCmPerSecondSquared/0.75),
            "surface and observed rear support scale once, sharing lateral budget");

    auto noRoad=Sample(-4800.0,0.0);
    noRoad.RearTractionScale=0.0;
    noRoad.ContactTractionScale=0.0;
    const auto airborne=CalculateReverseSpeedGovernor(noRoad);
    Require(airborne.Valid && !airborne.Active && airborne.ReverseMotorBlocked &&
            airborne.ForceX==0.0 && airborne.ForceY==0.0,
            "airborne overspeed gets no imaginary tire braking force");
    for (int brake=0; brake<2;++brake)
    {
        auto sample=Sample(-3800.0,0.0);
        sample.OtherLongitudinalBrake=true;
        const auto out=CalculateReverseSpeedGovernor(sample);
        Require(out.Valid && !out.Active && out.ForceX==0.0 &&
                out.ForceY==0.0,
                "service and opposing-throttle braking take priority");
    }

    auto atThreshold=Sample(-1350.01,0.0);
    const auto gentle=CalculateReverseSpeedGovernor(atThreshold);
    Require(gentle.Valid && gentle.Active &&
            gentle.DecelerationCmPerSecondSquared<0.1,
            "overspeed starts continuously without a velocity snap");
    for (const int fps : {30,60,120})
    {
        const double end=Settle(fps);
        Require(end <= -1350.0 && end > -1350.001,
                "30/60/120 FPS finite-force settling converges to target");
    }
    Require(std::abs(Settle(30)-Settle(120))<0.001,
            "reverse overspeed force is frame-convergent");

    auto invalid=Sample(-2500.0,0.0);
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for (const double bad : {nan, std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity()})
    {
        invalid=Sample(-2500.0,0.0); invalid.ForwardX=bad;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "nonfinite heading fails closed");
        invalid=Sample(-2500.0,0.0); invalid.VelocityY=bad;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "nonfinite true velocity fails closed");
        invalid=Sample(-2500.0,0.0); invalid.MassKg=bad;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "nonfinite chassis mass fails closed");
        invalid=Sample(-2500.0,0.0); invalid.GripScale=bad;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "nonfinite grip fails closed");
        invalid=Sample(-2500.0,0.0); invalid.DeltaSeconds=bad;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "nonfinite timestep fails closed");
    }
    for (double badDt : {0.0,-0.001,0.100001})
    {
        invalid=Sample(-2500.0,0.0); invalid.DeltaSeconds=badDt;
        Require(!CalculateReverseSpeedGovernor(invalid).Valid,
                "unqualified physics frame cannot submit a reverse force");
    }
    invalid=Sample(-2500.0,0.0,0.0,0.0);
    Require(!CalculateReverseSpeedGovernor(invalid).Valid,
            "degenerate chassis heading cannot invent forward direction");
    invalid=Sample(-2500.0,0.0); invalid.RearTractionScale=0.8;
    invalid.ContactTractionScale=0.5;
    Require(!CalculateReverseSpeedGovernor(invalid).Valid,
            "rear wheel force cannot exceed whole-body support");
    invalid=Sample(-15001.0,0.0);
    Require(!CalculateReverseSpeedGovernor(invalid).Valid,
            "absurd measured speed cannot authorize output");

    constexpr double pi=3.14159265358979323846;
    for(int n=0;n<100000;++n)
    {
        const double angle=(n%720)*pi/360.0;
        const double fx=std::cos(angle),fy=std::sin(angle);
        const double along=-6000.0+(n%12001);
        const double lateral=-4000.0+((n*17)%8001);
        const double dt=1.0/(n%3==0?30.0:n%3==1?60.0:120.0);
        auto s=Sample(along*fx-lateral*fy,along*fy+lateral*fx,
                      fx,fy,n%2==0,dt);
        s.MassKg=100.0+(n%14901);
        s.ContactTractionScale=n%3==0?0.45:n%3==1?0.75:1.0;
        s.RearTractionScale=s.ContactTractionScale*(n%11==0?0.0:0.8);
        s.GripScale=0.10+(n%250)*0.01;
        s.OtherLongitudinalBrake=(n%7==0);
        s.Throttle=(n%5==0?0.4:-0.9);
        const auto out=CalculateReverseSpeedGovernor(s);
        Require(out.Valid && std::isfinite(out.ForceX) &&
                std::isfinite(out.ForceY) &&
                std::isfinite(out.WholeBodyBudgetDeceleration),
                "100k independent contact/heading/slide inputs stay finite");
        if(out.Active)
        {
            const double dot=out.ForceX*s.VelocityX+out.ForceY*s.VelocityY;
            const double cross=out.ForceX*fy-out.ForceY*fx;
            Require(dot<=1.0e-6 &&
                    std::abs(cross)<1.0e-4 &&
                    out.DecelerationCmPerSecondSquared<=
                        850.0*2.0+1.0e-6 &&
                    out.DecelerationCmPerSecondSquared*dt<=
                        out.ExcessReverseCmPerSecond+1.0e-6 &&
                    out.WholeBodyBudgetDeceleration>=0.0,
                    "force only brakes actual reverse axis, never sideways or upward");
        }
        else
        {
            Require(out.ForceX==0.0 && out.ForceY==0.0,
                    "non-active reverse governor never changes motion");
        }
    }
    // End-to-end FORCE COMPOSITION on the same physical rigid body.
    // The older governor tests check the pure policy, but not whether its
    // resulting braking leaves the same tire-grip budget as sideways skid.
    // These are independent source vectors, not Chaos collision impulses.
    for (int n = 0; n < 60000; ++n)
    {
        const double angle = (n % 720) * pi / 360.0;
        const double fx = std::cos(angle), fy = std::sin(angle);
        const double along = -1200.0 - static_cast<double>(n % 6001);
        const double lateral = static_cast<double>((n * 47) % 9001) - 4500.0;
        const double dt = 1.0 / (n % 3 == 0 ? 30.0 :
                                 n % 3 == 1 ? 60.0 : 120.0);
        const bool arcade = n % 2 == 0;
        auto s = Sample(along * fx - lateral * fy,
                        along * fy + lateral * fx, fx, fy, arcade, dt);
        s.MassKg = 100.0 + (n % 14901);
        s.GripScale = 0.10 + static_cast<double>(n % 300) * 0.01;
        s.ContactTractionScale = n % 4 == 0 ? 0.0 :
            n % 4 == 1 ? 0.45 : n % 4 == 2 ? 0.75 : 1.0;
        s.RearTractionScale = s.ContactTractionScale *
            (n % 9 == 0 ? 0.0 : n % 9 == 1 ? 0.35 : 0.85);
        s.OtherLongitudinalBrake = n % 13 == 0;
        const auto reverse = CalculateReverseSpeedGovernor(s);
        Require(reverse.Valid, "force composition reverse sample must be valid");
        const auto skid = CalculateLateralGripForce({
            dt, lateral, s.MassKg,
            arcade ? 8.5 : 4.25, s.GripScale,
            reverse.WholeBodyBudgetDeceleration,
            arcade, false
        });
        Require(skid.Valid, "same-frame skid budget must remain valid");
        const double actualReverseAcceleration =
            std::hypot(reverse.ForceX, reverse.ForceY) / s.MassKg;
        const double actualLateralAcceleration =
            std::abs(skid.RightForce) * s.ContactTractionScale / s.MassKg;
        const double combinedMagnitude =
            std::hypot(actualReverseAcceleration, actualLateralAcceleration);
        const double sharedGrip =
            (arcade ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) *
            s.GripScale * s.ContactTractionScale;
        Require(std::isfinite(combinedMagnitude) &&
                combinedMagnitude <= sharedGrip + 1.0e-7,
                "60k live-force compositions never exceed shared friction circle");
        if (s.RearTractionScale <= 0.0)
        {
            Require(!reverse.Active &&
                    reverse.ForceX == 0.0 && reverse.ForceY == 0.0,
                    "unsupported chassis cannot get ghost reverse-braking force");
        }
        if (reverse.Active)
        {
            const double worldForwardForce =
                reverse.ForceX * fx + reverse.ForceY * fy;
            const double worldLateralForce =
                -reverse.ForceX * fy + reverse.ForceY * fx;
            Require(worldForwardForce > 0.0 &&
                    std::abs(worldLateralForce) <= 1.0e-6,
                    "reverse overspeed never injects sideways force or thrust");
        }
    }
    std::cout<<"ReverseSpeedGovernor passed 100k policy + 60k shared-grip source frames.\n";
    return EXIT_SUCCESS;
}
