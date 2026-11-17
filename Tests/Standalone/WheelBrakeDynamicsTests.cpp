#include "Vehicles/WheelBrakeDynamics.h"
#include "Vehicles/WheelAbsController.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass) { std::cerr << "WheelBrakeDynamics FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
}
int main()
{
    using namespace latebrake;
    for (int id = -1; id < 8; ++id)
    for (bool arcade : {false, true})
        Require(IsValidFourCornerBrakeProfile(BrakeProfileForVehicle(id, arcade)),
                "all original garage presets and safe fallback validate");
    auto profile = BrakeProfileForVehicle(0, true).Wheels[0];
    WheelBrakeDynamics rotor;
    Require(!rotor.Advance({1.0/600.0, 20.0, 3000.0, 1.0, -2400.0, 1.0}, profile).Valid,
            "unseeded wheel cannot invent rotation");
    Require(rotor.SeedRolling(20.0, profile), "seed modeled rolling speed");
    double previous = rotor.ModeledAngularSpeed();
    for (unsigned step = 0; step < 1200; ++step)
    {
        const auto out = rotor.Advance({1.0/600.0, 20.0, 3000.0, 1.0, -2400.0, 1.0}, profile);
        Require(out.Valid && out.ModeledRadiansPerSecond >= 0.0 &&
                out.ModeledRadiansPerSecond <= previous + 1.0e-7 &&
                out.ForceNewtons <= 0.0 && out.ForceNewtons >= -2400.0,
                "prescribed-speed forward braking is bounded without rotor reversal");
        Require(out.RotorEnergyChangeJoules + out.ContactWorkJoules + out.BrakeWorkJoules <= 1.0e-4,
                "implicit contact/brake work cannot create model energy");
        Require(out.TireLossJoules >= 0.0 && out.NumericalLossJoules >= 0.0,
                "tire and numerical energy losses are nonnegative");
        previous = out.ModeledRadiansPerSecond;
    }
    Require(previous == 0.0, "strong held brake reaches static locked rotor");
    rotor.SeedRolling(20.0, profile);
    const auto coasting = rotor.Advance({1.0/600.0, 20.0, 3000.0, 1.0, 0.0, 0.0}, profile);
    Require(coasting.Valid && coasting.ForceNewtons == 0.0 &&
            std::abs(coasting.ModeledRadiansPerSecond - 20.0/profile.RadiusMeters) < 1.0e-7,
            "zero admitted brake force cannot become tire propulsion");
    WheelBrakeDynamics reverse;
    rotor.SeedRolling(20.0, profile); reverse.SeedRolling(-20.0, profile);
    for (unsigned step = 0; step < 500; ++step)
    {
        const auto a=rotor.Advance({1.0/600.0,20.0,3000.0,1.0,-2400.0,0.4},profile);
        const auto b=reverse.Advance({1.0/600.0,-20.0,3000.0,1.0,2400.0,0.4},profile);
        Require(a.Valid && b.Valid && std::abs(a.ForceNewtons+b.ForceNewtons)<1.0e-6 &&
                std::abs(a.ModeledRadiansPerSecond+b.ModeledRadiansPerSecond)<1.0e-7,
                "reverse wheel/force trajectory mirrors forward braking");
    }
    WheelAbsController abs;
    const auto first=abs.Advance({1.0/600.0,20.0,0.0,true,true,false},profile);
    Require(first.Valid && first.Modulating && first.Phase==WheelAbsPhase::Releasing &&
            first.Pressure<1.0, "modeled lock releases service pressure");
    const auto handbrake=abs.Advance({1.0/600.0,20.0,0.0,true,true,true},profile);
    Require(handbrake.Valid && handbrake.Pressure==1.0 && !handbrake.Modulating,
            "mechanical handbrake cannot silently acquire ABS");
    const auto slow=abs.Advance({1.0/600.0,0.5,0.0,true,true,false},profile);
    Require(slow.Valid && slow.Pressure==1.0 && !slow.Modulating,
            "low-speed brake hold bypasses slip modulation");
    // A sudden load/grip loss must lower the valve ceiling immediately;
    // pressure may rebuild only within the newly admitted torque budget.
    for (double ceiling : {0.8, 0.2, 0.0, 0.01, 0.7, 1.0})
    for (unsigned step=0;step<100;++step)
    {
        const auto bounded=abs.Advance({1.0/600.0,20.0,
            20.0/profile.RadiusMeters,true,true,false,ceiling},profile);
        Require(bounded.Valid && bounded.Pressure>=0.0 && bounded.Pressure<=ceiling,
                "feed-forward cap survives abrupt grip loss and bounded recovery");
    }
    const auto zero=abs.Advance({1.0/600.0,20.0,0.0,true,true,false,0.0},profile);
    Require(zero.Valid && zero.Pressure==0.0,
            "minimum valve floor cannot override a zero admitted torque ceiling");
    for (double badCeiling : {-0.01,1.01,std::numeric_limits<double>::quiet_NaN(),
                              std::numeric_limits<double>::infinity()})
        Require(!abs.Advance({1.0/600.0,20.0,0.0,true,true,false,badCeiling},profile).Valid,
                "invalid pressure ceiling rejects rather than retaining prior pressure");
    const auto mechanical=abs.Advance({1.0/600.0,20.0,0.0,true,true,true,0.0},profile);
    Require(mechanical.Valid && mechanical.Pressure==1.0 && !mechanical.Modulating,
            "feed-forward ceiling cannot reduce deliberately mechanical rear locking");
    const auto unsupported=abs.Advance({1.0/600.0,20.0,0.0,false,true,false,0.0},profile);
    Require(unsupported.Valid && !unsupported.Modulating &&
            unsupported.Phase==WheelAbsPhase::Inactive,
            "unsupported wheel has no synthetic ABS valve activity");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()})
    {
        rotor.SeedRolling(20.0,profile);
        Require(!rotor.Advance({bad,20.0,3000.0,1.0,-2400.0,1.0},profile).Valid &&
                !rotor.HasState(), "invalid time clears rotor state");
        auto p=profile; p.RadiusMeters=bad;
        Require(!IsValidWheelBrakeProfile(p), "invalid profile rejected");
        Require(!abs.Advance({1.0/600.0,bad,0.0,true,true,false},profile).Valid,
                "corrupt ABS speed cannot retain pressure state");
    }
    rotor.SeedRolling(20.0,profile);
    Require(!rotor.Advance({1.0e-300,20.0,3000.0,1.0,-2400.0,1.0},profile).Valid,
            "subnormal dt cannot create infinite inertia rate");
    std::mt19937 rng(0x5b9701);
    std::uniform_real_distribution<double> u(0.0,1.0);
    for (unsigned frame=0;frame<100000;++frame)
    {
        auto p=profile;
        p.RadiusMeters=0.10+0.65*u(rng);
        p.InertiaKgMetersSquared=0.10+19.90*u(rng);
        p.MaximumBrakeTorqueNm=50.0+9950.0*u(rng);
        p.FrictionCoefficient=0.05+2.95*u(rng);
        p.SlipStiffness=1.0+29.0*u(rng);
        const double seed=(u(rng)-0.5)*1000.0;
        const double road=(u(rng)-0.5)*1000.0;
        const double cap=-std::copysign(u(rng)*100000.0,road);
        const double dt=1.0e-6+(1.0/600.0-1.0e-6)*u(rng);
        rotor.SeedRolling(seed,p);
        const auto out=rotor.Advance({dt,road,u(rng)*100000.0,0.05+3.95*u(rng),cap,u(rng)},p);
        Require(out.Valid && out.SolverIterations<=WheelBrakeDynamics::RootIterations &&
                std::abs(out.ForceNewtons)<=std::abs(cap)+1.0e-7 && out.ForceNewtons*road<=0.0,
                "100k valid SI rotor samples stay finite, passive and force bounded");
        const double balance=out.RotorEnergyChangeJoules+out.ContactWorkJoules+
            out.BrakeWorkJoules+out.TireLossJoules+out.NumericalLossJoules;
        const double scale=1.0+out.RotorEnergyJoules+std::abs(out.ContactWorkJoules);
        Require(std::abs(balance)<=1.0e-5*scale,
                "independent discrete energy accounting closes within root tolerance");
    }
    std::cout<<"Wheel rotor: implicit brake/contact work, lock, reverse, ABS, 100000 energy cases passed\n";
}
