#include "AI/RivalBrakeCapacityPace.h"
#include "AI/RivalCornerBrakeEnvelope.h"
#include "Vehicles/FourCornerWheelBrakes.h"
#include "Vehicles/GripLimitedBraking.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass) { std::cerr << "RivalWheelBrakeApproach FAIL: " << why << '\n'; std::exit(EXIT_FAILURE); }
}
struct Condition
{
    double Grip;
    double Heat;
    std::array<double, 4> Loads;
};
struct Observation
{
    double EntrySpeed;
    unsigned BrakeFrames;
};
Observation Approach(int vehicle, bool arcade, double fps, const Condition& condition)
{
    using namespace latebrake;
    constexpr double Mass = 1100.0;
    const double dt = 1.0 / fps;
    const auto profile = BrakeProfileForVehicle(vehicle, arcade);
    SuspensionResponse springs;
    springs.Valid = true; springs.WeightForce = Mass * 980.0;
    for (unsigned i = 0; i < 4; ++i)
    {
        auto& c = springs.Corners[i]; c.Contact = condition.Loads[i] > 0.0;
        c.UpForce = springs.WeightForce * condition.Loads[i];
        springs.TotalUpForce += c.UpForce;
        if (c.Contact) ++springs.ContactCount;
    }
    ChassisGroundContactOutput initial;
    initial.Valid = true; initial.SupportingProbes = springs.ContactCount;
    const auto ground = ConstrainGroundContactToSuspension(initial, springs);
    FourCornerWheelBrakes brakes;
    RivalBrakeCapacityPace pace;
    ServiceBrakeCapabilityLedger ledger;
    double speed = 1550.0, remaining = 5300.0;
    unsigned brakeFrames = 0;
    for (unsigned frame = 0; frame < static_cast<unsigned>(fps * 20.0); ++frame)
    {
        ServiceBrakeCapabilitySample sample;
        sample.MassKg = Mass; sample.GripScale = condition.Grip;
        sample.HeatScale = condition.Heat; sample.Arcade = arcade;
        sample.RoadSamplesValid = true; sample.ForwardCmPerSecond = speed;
        sample.RoadForwardCmPerSecond.fill(speed);
        sample.NormalZ.fill(1.0); sample.ForwardProjection.fill(1.0);
        const auto capability = EstimateServiceBrakeCapability(sample, ground, springs, profile);
        Require(ledger.Publish(capability, dt, 0), "synthetic accepted flat source frame publishes capability");
        const auto guarded = pace.Apply({0.85, 0.0, false, false}, {dt, 0, true, ledger.Read()});
        Require(guarded.Valid, "fresh synthetic source identities stay accepted");
        const auto gate = CircuitGate(0);
        const auto forecast = ApplyRivalCornerBrakeEnvelope(guarded.Command, {
            0, {gate.X - remaining, gate.Y}, speed, condition.Grip, 1.0, 1.0,
            condition.Heat, arcade, true, true,
            guarded.DecelerationCeilingCmPerSecondSquared, 0.20 + dt});
        Require(forecast.Valid, "contact-aware actual gate forecast accepts flat approach");
        double acceleration = 400.0 * forecast.Command.Throttle - 8.0;
        if (forecast.Command.Brake)
        {
            ++brakeFrames;
            const auto body = CalculateGripLimitedBraking({dt, speed, Mass, true, false,
                arcade, condition.Grip, 1.0, 1.0, condition.Heat});
            std::array<double, 4> road{speed, speed, speed, speed};
            const auto admitted = CalculateSprungContactBraking({Mass, speed,
                body.ForwardForce * ground.ContactTractionScale, false, true, road}, ground, springs);
            FourCornerWheelBrakeSample wheel;
            wheel.DeltaSeconds = dt; wheel.MassKg = Mass; wheel.GripScale = condition.Grip;
            wheel.ServiceHeatScale = condition.Heat; wheel.ServiceBrake = true;
            wheel.PhysicsReady = true; wheel.RoadSpeedCmPerSecond = road;
            wheel.RoadNormalZ.fill(1.0);
            const auto plan = brakes.Propose(wheel, profile, ground, springs, admitted);
            Require(plan.Valid && plan.Brakes.TotalForwardForce <= 0.0,
                    "same bounded rotor path supplies only passive accepted braking");
            acceleration = plan.Brakes.TotalForwardForce / Mass - 8.0;
            if (plan.Receipt) Require(brakes.CommitDeliveredFrame(plan.Receipt), "toy force delivery settles once");
        }
        else brakes.Reset();
        const double stepDistance = speed * dt + 0.5 * acceleration * dt * dt;
        Require(stepDistance > 0.0, "corner approach remains forward without artificial teleports");
        remaining -= stepDistance;
        speed = std::max(0.0, speed + acceleration * dt);
        if (remaining <= RivalCornerRunThroughBufferCm) return {speed, brakeFrames};
    }
    Require(false, "bounded flat approach reaches the gate buffer within twenty simulated seconds");
    return {};
}
}
int main()
{
    constexpr std::array<Condition, 3> conditions{{
        {1.0, 1.0, {0.25, 0.25, 0.25, 0.25}},
        {0.45, 0.4, {0.25, 0.25, 0.25, 0.25}},
        {0.7, 0.65, {0.35, 0.35, 0.0, 0.0}}
    }};
    unsigned runs = 0;
    for (int car = 0; car < 6; ++car)
    for (bool arcade : {true, false})
    for (const auto& condition : conditions)
    {
        double reference = 0.0;
        for (double fps : {120.0, 60.0, 30.0})
        {
            const auto result = Approach(car, arcade, fps, condition);
            Require(result.BrakeFrames > 0 && result.EntrySpeed <= 1810.0,
                    "capacity-limited controller slows to the synthetic entry envelope in every condition");
            if (fps == 120.0) reference = result.EntrySpeed;
            else Require(std::abs(result.EntrySpeed - reference) < 55.0,
                    "synthetic actual-rotor approach remains within 0.55 m/s across render frame rates");
            ++runs;
        }
    }
    Require(runs == 108, "all six profiles, two modes, three road/load states and three frame rates ran");
    std::cout << "108 prescribed flat rival/wheel-brake approach scenarios passed; not Chaos runtime evidence\n";
}
