#include "Vehicles/SprungChassisDynamics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

// Isolated small-angle, rigid-box heave + pitch + roll *reference*.
// The production CalculateSprungChassis function supplies every wheel force.
// The integrator/inertias here are illustrative, NOT the Unreal Chaos solver,
// actual collision shape, tire model, physics substep or installed-game QA.
namespace
{
constexpr double Pi = 3.14159265358979323846;
constexpr std::array<double, 4> WheelX{122.0, 122.0, -122.0, -122.0};
constexpr std::array<double, 4> WheelY{-94.0, 94.0, -94.0, 94.0};
constexpr double BoxLengthCm = 380.0;
constexpr double BoxWidthCm = 180.0;
constexpr double BoxHeightCm = 65.0;

void Require(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "SprungChassisAttitudeRecovery FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Pose
{
    double HeightCm = 59.0;
    double VerticalSpeedCmPerSecond = 0.0;
    double PitchRadians = 0.0;
    double PitchRateRadiansPerSecond = 0.0;
    double RollRadians = 0.0;
    double RollRateRadiansPerSecond = 0.0;
};

struct Result
{
    Pose Final;
    double FirstPitchTorque = 0.0;
    double FirstRollTorque = 0.0;
    double MaximumAbsPitchDegrees = 0.0;
    double MaximumAbsRollDegrees = 0.0;
    double MinimumCenterHeightCm = 1000000.0;
    int SupportedFrames = 0;
};

Result Integrate(int fps, double massKg, double initialHeight,
                 double initialPitchDegrees, double initialRollDegrees,
                 bool frontLeftShortBump = false)
{
    using namespace latebrake;
    const double stepSeconds = 1.0 / static_cast<double>(fps);
    const double gravity = SuspensionGravityCmPerSecondSquared;
    // Approximate rectangular-solid moments for *testing only*. No
    // claim that a scaled Unreal mesh has these Chaos inertial tensors.
    const double inertiaRoll = massKg *
        (BoxWidthCm * BoxWidthCm + BoxHeightCm * BoxHeightCm) / 12.0;
    const double inertiaPitch = massKg *
        (BoxLengthCm * BoxLengthCm + BoxHeightCm * BoxHeightCm) / 12.0;

    Pose pose;
    pose.HeightCm = initialHeight;
    pose.PitchRadians = initialPitchDegrees * Pi / 180.0;
    pose.RollRadians = initialRollDegrees * Pi / 180.0;
    Result result;

    for (int step = 0; step < fps * 6; ++step)
    {
        SuspensionFrame frame;
        frame.PhysicsReady = true;
        frame.DeltaSeconds = stepSeconds;
        frame.MassKg = massKg;
        frame.GravityCmPerSecondSquared = gravity;
        for (unsigned index = 0; index < frame.Corners.size(); ++index)
        {
            // Small-angle world-vertical corner offsets: positive pitch
            // lowers the nose; positive roll lowers the left side.
            const double roadHeight = frontLeftShortBump &&
                    index == 0 && stepSeconds * step < 0.35 ? 10.0 : 0.0;
            const double rayDistance = pose.HeightCm -
                pose.PitchRadians * WheelX[index] +
                pose.RollRadians * WheelY[index] - roadHeight;
            const double pointSpeed = pose.VerticalSpeedCmPerSecond -
                pose.PitchRateRadiansPerSecond * WheelX[index] +
                pose.RollRateRadiansPerSecond * WheelY[index];
            const bool rayHit = rayDistance >= 0.0 &&
                rayDistance <= SuspensionMaxTraceDistanceCm;
            frame.Corners[index] = {
                rayHit, rayHit ? rayDistance : 0.0,
                rayHit ? 1.0 : 0.0, pointSpeed
            };
        }

        // Use the ACTUAL production spring/anti-roll policy, not a
        // reimplementation of its force calculation.
        const auto response = CalculateSprungChassis(frame);
        Require(response.Valid, "source spring policy rejected valid numerical frame");
        if (response.ContactCount > 0)
        {
            ++result.SupportedFrames;
        }
        Require(std::isfinite(response.TotalUpForce) &&
            response.TotalUpForce >= 0.0 &&
            response.TotalUpForce <= massKg * gravity *
                SuspensionMaxPerCornerWeightFactor * 4.0 + 0.01,
            "unbounded or nonfinite total spring force");

        double rollTorque = 0.0;
        double pitchTorque = 0.0;
        double summedForce = 0.0;
        for (unsigned index = 0; index < response.Corners.size(); ++index)
        {
            const double force = response.Corners[index].UpForce;
            Require(std::isfinite(force) && force >= 0.0,
                    "spring provided an invalid physical corner force");
            summedForce += force;
            rollTorque += WheelY[index] * force;
            pitchTorque -= WheelX[index] * force;
        }
        Require(std::abs(summedForce - response.TotalUpForce) < 0.01,
                "anti-roll budget changed net vertical lift");
        if (step == 0)
        {
            result.FirstRollTorque = rollTorque;
            result.FirstPitchTorque = pitchTorque;
        }

        // Semi-implicit Euler on a simplified rigid box. It deliberately
        // does not emulate Unreal's solver or resolve world collisions.
        pose.VerticalSpeedCmPerSecond +=
            (summedForce / massKg - gravity) * stepSeconds;
        pose.RollRateRadiansPerSecond +=
            rollTorque / inertiaRoll * stepSeconds;
        pose.PitchRateRadiansPerSecond +=
            pitchTorque / inertiaPitch * stepSeconds;
        pose.HeightCm += pose.VerticalSpeedCmPerSecond * stepSeconds;
        pose.RollRadians += pose.RollRateRadiansPerSecond * stepSeconds;
        pose.PitchRadians += pose.PitchRateRadiansPerSecond * stepSeconds;

        Require(std::isfinite(pose.HeightCm) &&
                std::isfinite(pose.VerticalSpeedCmPerSecond) &&
                std::isfinite(pose.RollRadians) &&
                std::isfinite(pose.PitchRadians) &&
                std::isfinite(pose.RollRateRadiansPerSecond) &&
                std::isfinite(pose.PitchRateRadiansPerSecond),
                "attitude reference diverged");
        result.MinimumCenterHeightCm = std::min(
            result.MinimumCenterHeightCm, pose.HeightCm);
        result.MaximumAbsRollDegrees = std::max(
            result.MaximumAbsRollDegrees,
            std::abs(pose.RollRadians * 180.0 / Pi));
        result.MaximumAbsPitchDegrees = std::max(
            result.MaximumAbsPitchDegrees,
            std::abs(pose.PitchRadians * 180.0 / Pi));
    }

    result.Final = pose;
    return result;
}

void CheckRest(const Result& result)
{
    using namespace latebrake;
    Require(result.SupportedFrames > 60,
            "reference did not engage its real production spring policy");
    Require(std::abs(result.Final.HeightCm -
        SuspensionTargetRideDistanceCm) < 0.75,
        "settled chassis center must return near target spring ride height");
    Require(std::abs(result.Final.VerticalSpeedCmPerSecond) < 5.0,
        "vertical support should not develop a perpetual bounce");
    Require(std::abs(result.Final.RollRadians * 180.0 / Pi) < 0.25 &&
            std::abs(result.Final.PitchRadians * 180.0 / Pi) < 0.25,
        "spring and anti-roll response must recover near-level attitude");
}
}

int main()
{
    // Reversed signed pitch/roll, initial extra ride height, different
    // masses and frame rates must share a bounded 6-second settle.
    for (const int fps : {30, 60, 120})
    {
        for (const double mass : {800.0, 1100.0, 1800.0})
        {
            for (const double direction : {-1.0, 1.0})
            {
                const auto tilt = Integrate(
                    fps, mass, 59.0, direction * 4.0, direction * 6.0);
                CheckRest(tilt);
                Require(tilt.FirstPitchTorque * direction < 0.0 &&
                        tilt.FirstRollTorque * direction < 0.0,
                        "initial spring moments must resist signed body pitch and roll");
                const auto elevated = Integrate(
                    fps, mass, 80.0, direction * 4.0, direction * 6.0);
                CheckRest(elevated);
                Require(tilt.MinimumCenterHeightCm > 36.0 &&
                        elevated.MinimumCenterHeightCm > 36.0,
                        "small-angle reference must not collapse its center onto road");
            }
        }

        // A single 10cm front-left road-height excursion should first
        // disturb BOTH axes, then lose its transient and settle. This is
        // NOT a real cooked curb or verified Chaos tire-ground collision.
        const auto bump = Integrate(fps, 1100.0, 59.0, 0.0, 0.0, true);
        CheckRest(bump);
        Require(bump.MaximumAbsRollDegrees > 0.20 &&
                bump.MaximumAbsPitchDegrees > 0.20,
                "single-corner geometry must produce both roll and pitch response");
    }

    std::cout << "Isolated 4-corner heave/pitch/roll attitude "
                 "and short bump regression passed (30/60/120 FPS).\n";
    return EXIT_SUCCESS;
}
