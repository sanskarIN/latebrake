#include "Racecraft/RacecraftMath.h"
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/CircuitGeometry.h"
#include "Racecraft/RacecraftFrameGate.h"
#include "Racecraft/SurfaceMaterialModel.h"
#include "Racecraft/TireForceModel.h"
#include "Racecraft/BrakeHydraulics.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace latebrake::racecraft;

static void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "RacecraftFoundations FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
static PhysicalVehicleFrame ValidPhysicalCar(std::uint64_t frameIndex,
                                             double seconds,
                                             double x)
{
    PhysicalVehicleFrame f;
    f.Identity.Session = 1;
    f.Identity.Actor = 100;
    f.Identity.Frame = frameIndex;
    f.Identity.Slot = 0;
    f.Identity.EngineObserved = true;
    f.Identity.BodySimulating = true;
    f.Identity.OriginalTrack = true;
    f.Chassis.PlanarPose.Position = {x, -1800.0};
    f.Chassis.WorldPositionCm = {x, -1800.0, 150.0};
    f.Chassis.VelocityCmS = {200.0, 0.0, 0.0};
    f.Race.EngineGameModeReady = true;
    f.Race.Phase = RacePhase::Green;
    f.Race.FrameDeltaSeconds = 0.05;
    f.Race.ClockSeconds = seconds;
    f.TimeSeconds = seconds;
    f.PhysicsQualified = true;
    f.ChassisOnWorldStatic = true;
    for (WheelObservation& wheel : f.Wheels)
    {
        wheel.ContactVerified = true;
        wheel.PhysicalContact = true;
        wheel.NormalLoadN = 3200.0;
        wheel.Surface = ContactSurface::Asphalt;
    }
    return f;
}
static void TestPhysicalMathAndCircuit()
{
    Require(Finite(Vec2{1.0, 2.0}), "finite vector accepted");
    Require(std::abs(Length(Vec2{3.0, 4.0}) - 5.0) < 1.e-10,
            "Euclidean plan length");
    Require(std::abs(std::abs(WrappedAngle(3.0 * Pi)) - Pi) < 1.e-10,
            "wrapped angle stable");
    Require(std::abs(Dot(Vec2{1.0, 0.0}, Vec2{0.0, 1.0})) < 1.e-10,
            "orthogonal axes");
    const SegmentProjection projection = ProjectToSegment(
        {50.0, 10.0}, {{0.0, 0.0}, {100.0, 0.0}});
    Require(projection.Valid &&
            std::abs(projection.SignedSideCm - 10.0) < 1.e-9,
            "signed road projection");
    Require(CircuitGeometryValid(), "original circuit topology is sound");
    const auto start = PoseAtDistance(0.0);
    Require(std::abs(start.Position.X - WestCenterX) < 1.e-9,
            "lap begins on authored original southern straight");
    const auto onRoad = ProjectOriginalCircuit({0.0, SouthCenterY});
    Require(onRoad.Valid && onRoad.PavedCenter &&
            onRoad.Segment == 0 &&
            onRoad.CenterlineDistanceCm < 1.e-8,
            "source track projection matches physical road");
    Require(!CenterOnOriginalRoad({0.0, 0.0}),
            "interior grass is not authenticated as track");
    Require(RectsOverlap(
        {{0,0},{150,60},0},{{120,0},{150,60},0}),
        "oriented boxes detect collision");
    Require(!RectsOverlap(
        {{0,0},{100,50},0},{{2000,0},{100,50},0}),
        "separated physical boxes do not collide");
    const BrakeEnvelope stopping =
        StoppingEnvelope(2000.0, 0.0, 800.0, 0.2);
    Require(stopping.Valid &&
            stopping.DistanceCm > stopping.BrakingDistanceCm,
            "finite speed braking includes human reaction");
}
static void TestActorAndRosterGate()
{
    RacecraftFrameGate gate;
    FrameGateSettings options;
    options.RequireWheelSensors = true;
    Require(gate.Configure(options), "wheel-qualified gate settings");
    const auto first = ValidPhysicalCar(1, 1.0, -2500.0);
    Require(ValidateVehicleFrame(first, true) == SnapshotFault::None,
            "real actor and wheel contact qualified");
    Require(gate.Observe(first).Qualified, "first physical frame admitted");
    const auto second = ValidPhysicalCar(2, 1.05, -2490.0);
    Require(gate.Observe(second).Qualified,
            "fresh actor frame matches measured speed");
    Require(!gate.Observe(second).Qualified,
            "same real frame ordinal cannot replay into controller");
    auto changed = ValidPhysicalCar(3, 1.1, -2480.0);
    changed.Identity.Actor = 404;
    Require(!gate.Observe(changed).Qualified,
            "new physical actor requires new safety epoch");
    Require(gate.Snapshot().RejectedFrames == 2,
            "gate rejection receipts exact");
    FleetFrame roster;
    roster.Session = 1;
    roster.TimeSeconds = 1.0;
    roster.Count = PhysicalFleetCapacity;
    roster.EngineRosterVerified = true;
    roster.OriginalMapVerified = true;
    for (int i = 0; i < roster.Count; ++i)
    {
        roster.Members[static_cast<std::size_t>(i)] =
            ValidPhysicalCar(1, 1.0, -2500.0 + i * 320.0);
        auto& actor = roster.Members[static_cast<std::size_t>(i)];
        actor.Identity.Actor = static_cast<std::uint64_t>(100 + i);
        actor.Identity.Slot = i;
    }
    Require(ValidateFleetFrame(roster) == SnapshotFault::None,
            "six distinct physical actors admitted");
    roster.Members[2].Identity.Actor = roster.Members[1].Identity.Actor;
    Require(ValidateFleetFrame(roster) == SnapshotFault::DuplicateActor,
            "duplicate six-car identity fails closed");
}
static void TestSurfaceAndTires()
{
    const auto wet = MaterialFor(ContactSurface::Asphalt);
    SurfaceWeather weather;
    weather.EnvironmentQualified = true;
    const auto dry = EvaluateSurfaceContact(
        wet, weather, TireCompound::Medium,
        95.0, 0.1, 220.0, 800.0, 3500.0);
    weather.Wetness = 0.85;
    weather.StandingWaterMm = 3.0;
    const auto wetOutput = EvaluateSurfaceContact(
        wet, weather, TireCompound::Medium,
        95.0, 0.1, 220.0, 800.0, 3500.0);
    Require(dry.Valid && wetOutput.Valid &&
            dry.Friction > wetOutput.Friction,
            "physical water and tire suitability reduce road grip");
    WheelObservation wheel;
    wheel.ContactVerified = true;
    wheel.PhysicalContact = true;
    wheel.Surface = ContactSurface::Asphalt;
    wheel.NormalLoadN = 3200.0;
    wheel.AngularVelocityRadS = 22.0;
    TireForceInput tire;
    tire.Wheel = wheel;
    tire.Compound = TireCompound::Medium;
    tire.Surface = dry;
    tire.WheelLongitudinalSpeedCmS = 760.0;
    tire.WheelLateralSpeedCmS = 50.0;
    tire.DeltaSeconds = 0.016;
    tire.OriginalWheelContactMeasured = true;
    TireForceIntegrator solver;
    Require(solver.Configure({}), "strict tire model settings");
    const auto force = solver.Step(tire);
    Require(force.Valid && force.Grounded &&
            force.ContactPatchLoadN > 3000.0 &&
            Finite(force.LongitudinalForceN) &&
            Finite(force.LateralForceN),
            "tire force depends on grounded wheel physics");
    tire.Wheel.ContactVerified = false;
    Require(!solver.Step(tire).Valid,
            "unverified wheel contact cannot invent tire force");
}
static void TestFourWheelBrake()
{
    FourWheelBrakeController brake;
    Require(brake.Configure({}), "brake hydraulic coefficients valid");
    BrakeHydraulicInput input;
    input.DeltaSeconds = 0.016;
    input.PedalFraction = 0.75;
    input.ForwardSpeedCmS = 1200.0;
    input.OriginalEngineWheelSignals = true;
    input.VehiclePhysicsReady = true;
    input.WheelNormalLoadsN.fill(3200.0);
    input.DiscTemperaturesC.fill(100.0);
    input.ContactVerified.fill(true);
    auto result = brake.Step(input);
    Require(result.Valid && result.TotalCommandedTorqueNm > 0.0 &&
            result.ContactWheels == 4 &&
            !result.RealHydraulicHardwareVerified,
            "source physics model cannot claim installed hardware");
    input.WheelSlipRatios[0] = -0.9;
    result = brake.Step(input);
    Require(result.Valid && result.LockedWheels >= 1,
            "locked wheel triggers hydraulic control review");
    Require(brake.Frames() == 2,
            "all qualifying controller frames counted");
}
int main()
{
    TestPhysicalMathAndCircuit();
    TestActorAndRosterGate();
    TestSurfaceAndTires();
    TestFourWheelBrake();
    std::cout << "Racecraft physical foundations passed.\n";
    return EXIT_SUCCESS;
}
