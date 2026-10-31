#pragma once

#include "AI/RivalPilot.h"
#include "Vehicles/ArcadeEngineHeat.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Advisory AI throttle adjustment based only on the real rival's LAST
// qualified engine-heat reading. It cannot steer, brake, award gates or
// grant motor force, and is not a real engine cooling simulation.
struct RivalEngineHeatPaceSample final
{
    double FrameSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double EngineHeatFraction = ArcadeEngineHeat::IdleHeat;
    bool HeatSampleReady = false;
    bool PhysicsReady = false;
};

inline PilotCommand ModerateRivalEngineHeatPace(
    PilotCommand command, const RivalEngineHeatPaceSample& sample) noexcept
{
    constexpr double MaxFrameSeconds = 0.10;
    constexpr double MaxPlanarSpeed = 10000.0;
    constexpr double MinimumCruiseSpeed = 1200.0;
    constexpr double CoolingStartHeat = 0.70;
    constexpr double MaximumThrottle = 0.85;
    constexpr double MinimumHotThrottle = 0.30;

    if (!sample.PhysicsReady ||
        !std::isfinite(sample.FrameSeconds) ||
        sample.FrameSeconds <= 0.0 ||
        sample.FrameSeconds > MaxFrameSeconds ||
        !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
        sample.PlanarSpeedCmPerSecond < 0.0 ||
        sample.PlanarSpeedCmPerSecond > MaxPlanarSpeed ||
        !std::isfinite(command.Throttle) ||
        command.Throttle < 0.0 || command.Throttle > 1.0 ||
        !std::isfinite(command.Steering) ||
        std::abs(command.Steering) > 1.0 ||
        !std::isfinite(sample.EngineHeatFraction) ||
        sample.EngineHeatFraction < ArcadeEngineHeat::IdleHeat ||
        sample.EngineHeatFraction > 1.0)
    {
        // Malformed physical/control inputs must not carry motor demand
        // into an unqualified AI frame. PilotCommand's default brakes.
        return {};
    }

    // A just-created, paused or recovered pawn may not have a prior
    // qualified thermal reading. Do not invent engine heat or block a
    // valid launch merely because the meter has no live sample yet.
    if (!sample.HeatSampleReady || command.Arrived || command.Brake ||
        command.Throttle <= 0.0 ||
        sample.PlanarSpeedCmPerSecond < MinimumCruiseSpeed ||
        sample.EngineHeatFraction <= CoolingStartHeat)
    {
        return command;
    }

    // No added service braking: at normal road speeds, release some
    // propulsion demand so airflow can cool an overheating rival.
    // The underlying shared car applies its own independent heat derate
    // AFTER the AI controller, so every existing boost remains bounded.
    const double heatExcess = std::clamp(
        (sample.EngineHeatFraction - CoolingStartHeat) /
            (1.0 - CoolingStartHeat),
        0.0, 1.0);
    const double allowed = std::clamp(
        MaximumThrottle -
            (MaximumThrottle - MinimumHotThrottle) * heatExcess,
        MinimumHotThrottle, MaximumThrottle);
    command.Throttle = std::min(command.Throttle, allowed);
    return command;
}
} // namespace latebrake
