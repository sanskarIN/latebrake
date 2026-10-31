#pragma once

#include "AI/RivalPilot.h"
#include "Vehicles/ArcadeSurfaceTraction.h"
#include "Vehicles/ArcadeCollisionDamage.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// A modest, source-level pace adjustment for the physically controlled rival.
// Existing paved-surface hysteresis and accepted chassis impacts are the
// *only* sources of risk: this never predicts terrain or invents damage.
// Tire grip/ordinary corner policy, traffic braking and circuit waypoints
// remain independently authoritative.
struct RivalSurfaceDamageSample final
{
    double DeltaSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double LateralSpeedCmPerSecond = 0.0;
    double SurfaceGripMultiplier = 1.0;
    double ImpactGripMultiplier = 1.0;
    bool OffPavement = false;
    bool SurfaceReady = false;
    bool ImpactFrameReady = false;
};

class RivalSurfaceDamagePace final
{
public:
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MaxSpeedCmPerSecond = 10000.0;
    static constexpr double BrakeHoldSeconds = 0.12;

    void Reset() noexcept { RemainingBrakeSeconds = 0.0; }
    double RemainingHoldSeconds() const noexcept { return RemainingBrakeSeconds; }

    PilotCommand Apply(PilotCommand command,
                       const RivalSurfaceDamageSample& sample) noexcept
    {
        const double dt = sample.DeltaSeconds;
        const double speed = sample.PlanarSpeedCmPerSecond;
        const double lateral = sample.LateralSpeedCmPerSecond;
        const double surfaceGrip = sample.SurfaceGripMultiplier;
        const double impactGrip = sample.ImpactGripMultiplier;

        if (!sample.SurfaceReady || !sample.ImpactFrameReady ||
            !std::isfinite(dt) || dt <= 0.0 || dt > MaxFrameSeconds ||
            !std::isfinite(speed) || speed < 0.0 || speed > MaxSpeedCmPerSecond ||
            !std::isfinite(lateral) || std::abs(lateral) > speed + 5.0 ||
            !std::isfinite(surfaceGrip) ||
            (sample.OffPavement
                ? std::abs(surfaceGrip -
                    ArcadeSurfaceTraction::OffPavementGripMultiplier) > 1e-9
                : std::abs(surfaceGrip - 1.0) > 1e-9) ||
            !std::isfinite(impactGrip) ||
            impactGrip < ArcadeCollisionDamage::MinLateralGripMultiplier ||
            impactGrip > 1.0 ||
            !std::isfinite(command.Throttle) ||
            !std::isfinite(command.Steering) ||
            command.Throttle < 0.0 || command.Throttle > 1.0 ||
            std::abs(command.Steering) > 1.0)
        {
            Reset();
            return {}; // no throttle, neutral steering and service brake
        }

        if (command.Arrived || command.Brake)
        {
            Reset();
            return command; // do not weaken authoritative prior braking
        }

        const double steering = std::abs(command.Steering);
        const double slip = std::clamp(
            std::abs(lateral) / std::max(500.0, speed), 0.0, 1.0);
        // Small contact wear alone must not penalize a straight car. Severe
        // damage plus genuine turning, or confirmed dirt, raises corner risk.
        const bool damagedCorner = impactGrip <= 0.95 && steering >= 0.35;
        const bool lowGripRisk = sample.OffPavement || damagedCorner;

        if (lowGripRisk && speed >= 650.0)
        {
            const double target = sample.OffPavement
                ? std::clamp(1900.0 - 320.0 * slip - 250.0 * steering,
                             1300.0, 1900.0)
                : std::clamp(2800.0 - 3000.0 * (1.0 - impactGrip) -
                             360.0 * steering - 320.0 * slip,
                             1850.0, 2800.0);
            if (speed > target + 180.0)
            {
                RemainingBrakeSeconds = BrakeHoldSeconds;
                command.Throttle = 0.0;
                command.Brake = true;
                return command;
            }
        }

        // A finite recovery window prevents brake chatter. When the car is
        // safely slowed, do not indefinitely suppress low-speed recovery.
        if (RemainingBrakeSeconds > 0.0)
        {
            RemainingBrakeSeconds = std::max(0.0, RemainingBrakeSeconds - dt);
            if (RemainingBrakeSeconds > 0.0)
            {
                command.Throttle = 0.0;
                command.Brake = true;
                return command;
            }
        }
        if (lowGripRisk && speed >= 650.0)
        {
            // Never steer the AI away from its route or raise its motor
            // demand; normal grip/traffic policies may further reduce it.
            const double permitted = std::clamp(
                0.58 - 0.20 * steering - 0.12 * slip, 0.30, 0.58);
            command.Throttle = std::min(command.Throttle, permitted);
        }
        return command;
    }

private:
    double RemainingBrakeSeconds = 0.0;
};
} // namespace latebrake
