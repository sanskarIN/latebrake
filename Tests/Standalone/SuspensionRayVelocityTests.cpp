#include "Vehicles/SuspensionRayVelocity.h"

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
        std::cerr << "SuspensionRayVelocity FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b, double epsilon = 1.0e-7)
{
    return std::isfinite(a) && std::isfinite(b) && std::abs(a - b) < epsilon;
}
latebrake::SuspensionRayVelocitySample Plane(
    double gradeX, double gradeY, double vx, double vy, double extensionRate)
{
    const double normalLength = std::sqrt(1.0 + gradeX * gradeX + gradeY * gradeY);
    return {vx, vy, gradeX * vx + gradeY * vy + extensionRate,
        -gradeX / normalLength, -gradeY / normalLength, 1.0 / normalLength};
}
latebrake::SuspensionFrame Supported(double rate, double normalZ = 1.0)
{
    latebrake::SuspensionFrame frame;
    frame.DeltaSeconds = 1.0 / 60.0;
    frame.MassKg = 1100.0;
    frame.PhysicsReady = true;
    frame.ChassisUpZ = normalZ;
    for (auto& c : frame.Corners)
        c = {true, latebrake::SuspensionTargetRideDistanceCm, normalZ, rate};
    return frame;
}
} // namespace

int main()
{
    using namespace latebrake;
    // Real former policy composition: a chassis translated tangentially
    // along a 30% grade has constant ray lengths but Vz=+/-900 cm/s.
    for (double vx : {-3000.0, 3000.0})
    {
        const auto s = Plane(0.30, 0.0, vx, 0.0, 0.0);
        const auto corrected = MeasureSuspensionRayVelocity(s);
        const auto oldSprings = CalculateSprungChassis(Supported(s.PointVelocityZ, s.NormalZ));
        const auto newSprings = CalculateSprungChassis(Supported(
            corrected.DistanceSpeedCmPerSecond, s.NormalZ));
        Require(corrected.Valid && !corrected.RateLimited &&
            Near(corrected.DistanceSpeedCmPerSecond, 0.0),
            "constant-length uphill/downhill travel is not spring damping");
        Require(oldSprings.Valid && newSprings.Valid &&
            (vx > 0.0 ? oldSprings.TotalUpForce == 0.0 :
                oldSprings.TotalUpForce > 3.0 * oldSprings.WeightForce),
            "fixture reproduces old uphill unloading and downhill force saturation");
        Require(Near(newSprings.TotalUpForce, newSprings.WeightForce, 1.0e-6),
            "constant-length supported slope keeps nominal static spring loading");
    }

    for (double vz : {-1200.0, -60.0, 0.0, 60.0, 1200.0})
    {
        const auto measured = MeasureSuspensionRayVelocity(Plane(0.0, 0.0, 4000.0, -2000.0, vz));
        const auto before = CalculateSprungChassis(Supported(vz));
        const auto after = CalculateSprungChassis(Supported(measured.DistanceSpeedCmPerSecond));
        Require(measured.Valid && measured.DistanceSpeedCmPerSecond == vz &&
            before.TotalUpForce == after.TotalUpForce,
            "flat-road heave, damping and spring force stay identical");
    }
    const auto compression = MeasureSuspensionRayVelocity(Plane(0.4, -0.2, 3000.0, 1000.0, -75.0));
    const auto rebound = MeasureSuspensionRayVelocity(Plane(0.4, -0.2, 3000.0, 1000.0, 75.0));
    Require(compression.Valid && rebound.Valid &&
        Near(compression.DistanceSpeedCmPerSecond, -75.0) &&
        Near(rebound.DistanceSpeedCmPerSecond, 75.0),
        "real compression and extension remain signed on compound slopes");
    const auto compressed = CalculateSprungChassis(Supported(compression.DistanceSpeedCmPerSecond));
    const auto extended = CalculateSprungChassis(Supported(rebound.DistanceSpeedCmPerSecond));
    Require(compressed.TotalUpForce > compressed.WeightForce &&
        extended.TotalUpForce < extended.WeightForce,
        "damper still resists actual compression and rebound");

    for (const auto& normal : {SuspensionRayVelocitySample{10, 20, 30, 1, 0, 0},
             SuspensionRayVelocitySample{10, 20, 30, 0, 0, -1}})
    {
        const auto measured = MeasureSuspensionRayVelocity(normal);
        Require(measured.Valid && measured.DistanceSpeedCmPerSecond == 0.0,
            "walls and undersides cannot divide by zero or supply road damping");
        auto frame = Supported(measured.DistanceSpeedCmPerSecond);
        for (auto& c : frame.Corners) c.NormalZ = normal.NormalZ;
        const auto springs = CalculateSprungChassis(frame);
        Require(springs.Valid && springs.ContactCount == 0 && springs.TotalUpForce == 0.0,
            "steep trace observations remain rejected by the actual spring contact policy");
    }
    for (double fast : {-25000.0, 25000.0})
    {
        const auto measured = MeasureSuspensionRayVelocity(Plane(0.0, 0.0, 0.0, 0.0, fast));
        Require(measured.Valid && measured.RateLimited &&
            Near(measured.DistanceSpeedCmPerSecond, std::copysign(
                SuspensionMaximumDistanceSpeedCmPerSecond, fast)),
            "fast finite approach stays within the existing spring damping domain");
        Require(CalculateSprungChassis(Supported(measured.DistanceSpeedCmPerSecond)).Valid,
            "finite rate saturation must not create a malformed spring frame");
    }
    for (auto member : {&SuspensionRayVelocitySample::PointVelocityX,
             &SuspensionRayVelocitySample::PointVelocityY,
             &SuspensionRayVelocitySample::PointVelocityZ,
             &SuspensionRayVelocitySample::NormalX,
             &SuspensionRayVelocitySample::NormalY,
             &SuspensionRayVelocitySample::NormalZ})
    {
        for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                 std::numeric_limits<double>::infinity(),
                 -std::numeric_limits<double>::infinity()})
        {
            auto s = Plane(0.3, -0.2, 3000.0, -1000.0, 20.0);
            s.*member = bad;
            const auto measured = MeasureSuspensionRayVelocity(s);
            Require(!measured.Valid && measured.DistanceSpeedCmPerSecond == 0.0,
                "all nonfinite sensor components reject the frame");
        }
    }
    for (const auto& bad : {SuspensionRayVelocitySample{},
             SuspensionRayVelocitySample{0, 0, 0, 1, 1, 1},
             SuspensionRayVelocitySample{0, 0, 0, 0, 0, 0.5},
             SuspensionRayVelocitySample{0, 0, 0, 0, 0, 1.001}})
        Require(!MeasureSuspensionRayVelocity(bad).Valid,
            "missing, nonunit and out-of-range impact normals fail closed");
    auto absurd = Plane(-1.0, 0.0, 0.0, 0.0, 0.0);
    absurd.PointVelocityX = std::numeric_limits<double>::max();
    absurd.PointVelocityZ = std::numeric_limits<double>::max();
    Require(!MeasureSuspensionRayVelocity(absurd).Valid,
        "derived floating overflow cannot authorize spring force");

    // Independent finite-difference plane geometry, including changing
    // grades/yaw, lateral travel and corner point velocities from angular
    // body motion. This is math, not a real Chaos collision simulation.
    for (unsigned i = 0; i < 120000; ++i)
    {
        const double angle = static_cast<double>(i % 6283) / 1000.0;
        const double grade = static_cast<double>(i % 101) / 100.0;
        const double gx = grade * std::cos(angle);
        const double gy = grade * std::sin(angle);
        const double vx = static_cast<double>(static_cast<int>(i % 12001) - 6000);
        const double vy = static_cast<double>(static_cast<int>((i * 7) % 8001) - 4000);
        const double extension = static_cast<double>(static_cast<int>(i % 1001) - 500);
        auto s = Plane(gx, gy, vx, vy, extension);
        const auto measured = MeasureSuspensionRayVelocity(s);
        Require(measured.Valid && !measured.RateLimited &&
            Near(measured.DistanceSpeedCmPerSecond, extension, 1.0e-6),
            "all accepted grade/yaw combinations preserve actual spring-length rate");
        const double dt = 1.0 / (i % 3 == 0 ? 30.0 : i % 3 == 1 ? 60.0 : 120.0);
        const double x = static_cast<double>(i % 1000) - 500.0;
        const double y = static_cast<double>(i % 1400) - 700.0;
        const double z = gx * x + gy * y + 59.0;
        const double nextDistance = z + s.PointVelocityZ * dt -
            gx * (x + vx * dt) - gy * (y + vy * dt);
        Require(Near((nextDistance - 59.0) / dt,
            measured.DistanceSpeedCmPerSecond, 2.0e-6),
            "measured rate matches independent translated-plane finite differences");

        const double anchorX = i % 2 == 0 ? 122.0 : -122.0;
        const double anchorY = i % 4 < 2 ? -94.0 : 94.0;
        const double rollRate = static_cast<double>(i % 101) / 50.0 - 1.0;
        const double pitchRate = static_cast<double>(i % 71) / 35.0 - 1.0;
        const double yawRate = static_cast<double>(i % 41) / 20.0 - 1.0;
        s.PointVelocityX -= yawRate * anchorY;
        s.PointVelocityY += yawRate * anchorX;
        s.PointVelocityZ += rollRate * anchorY - pitchRate * anchorX;
        const auto corner = MeasureSuspensionRayVelocity(s);
        const double cornerDistance = z + s.PointVelocityZ * dt -
            gx * (x + s.PointVelocityX * dt) -
            gy * (y + s.PointVelocityY * dt);
        Require(corner.Valid && Near((cornerDistance - 59.0) / dt,
            corner.DistanceSpeedCmPerSecond, 2.0e-6),
            "true corner rotational velocity is retained instead of substituting COM speed");
    }
    std::cout << "SuspensionRayVelocity: all checks passed\n";
}
