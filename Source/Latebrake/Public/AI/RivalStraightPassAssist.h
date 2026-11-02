#pragma once

#include "AI/RivalPilot.h"
#include "Track/CircuitLayout.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace latebrake
{
// Limited, reversible straight-line passing assist for the ONE existing
// physical AI rival. This modifies a steering request only after actual
// player/rival chassis positions and velocities are qualified. The
// directional gate itself, route and scoring are NEVER offset.
struct RivalPassSample final
{
    double DeltaSeconds = 0.0;
    int NextGate = -1;
    CircuitPoint Rival{};
    CircuitPoint Player{};
    double RivalHeadingRadians = 0.0;
    double RivalVelocityX = 0.0;
    double RivalVelocityY = 0.0;
    double PlayerVelocityX = 0.0;
    double PlayerVelocityY = 0.0;
    bool Qualified = false;
    bool Paved = false;
};

struct RivalPassOutput final
{
    PilotCommand Command{};
    bool Passing = false;
    double TargetLaneOffsetCm = 0.0;
};

// Original track has 800cm paved half-width. A conservative lane center
// of 440cm keeps the body centroid inside pavement with room for the
// original ~380cm wide chassis. It cannot guarantee collision avoidance.
class RivalStraightPassAssist final
{
public:
    static constexpr double LaneOffsetCm = 440.0;
    static constexpr double MinimumCornerClearanceCm = 2150.0;
    // Only long original straights give a physical car enough runway
    // to move sideways before the original courtesy-brake envelope.
    static constexpr double StartAheadMinimumCm = 1600.0;
    static constexpr double StartAheadMaximumCm = 2400.0;
    static constexpr double MinimumPassingStraightLengthCm = 5000.0;
    static constexpr double MinimumEntryCornerRunwayCm = 4200.0;
    static constexpr double MinimumEntryContactTimeSeconds = 1.8;
    static constexpr double MaximumEntryClosingSpeedCmPerSecond = 900.0;
    static constexpr double SafetySeparationCm = 380.0;
    static constexpr double FinishBehindClearanceCm = 550.0;
    static constexpr double MaximumPassDurationSeconds = 6.0;

    void Reset() noexcept
    {
        ActiveGate = -1;
        LaneSign = 0.0;
        Elapsed = 0.0;
    }

    bool IsActive() const noexcept { return ActiveGate >= 0; }

    RivalPassOutput Apply(PilotCommand command, RivalPassSample s) noexcept
    {
        if (!s.Qualified || !s.Paved || !IsCircuitGate(s.NextGate) ||
            !std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
            s.DeltaSeconds > 0.10 ||
            !std::isfinite(s.Rival.X) || !std::isfinite(s.Rival.Y) ||
            !std::isfinite(s.Player.X) || !std::isfinite(s.Player.Y) ||
            !std::isfinite(s.RivalHeadingRadians) ||
            std::abs(s.RivalHeadingRadians) > 2.0 * std::numbers::pi ||
            !std::isfinite(s.RivalVelocityX) ||
            !std::isfinite(s.RivalVelocityY) ||
            !std::isfinite(s.PlayerVelocityX) ||
            !std::isfinite(s.PlayerVelocityY) ||
            std::hypot(s.RivalVelocityX, s.RivalVelocityY) > 10000.0 ||
            std::hypot(s.PlayerVelocityX, s.PlayerVelocityY) > 10000.0 ||
            !std::isfinite(command.Throttle) ||
            command.Throttle < 0.0 || command.Throttle > 1.0 ||
            !std::isfinite(command.Steering) ||
            std::abs(command.Steering) > 1.0)
        {
            Reset();
            return {{}, false, 0.0}; // invalid live physics brakes, no propulsion
        }

        // Existing corner, traffic or recovery authority wins. Never
        // redirect a braking rival across traffic or a checkpoint.
        if (command.Arrived || command.Brake)
        {
            Reset();
            return {command, false, 0.0};
        }

        const CircuitPoint gate = CircuitGate(s.NextGate);
        const CircuitPoint previous = CircuitGate(
            (s.NextGate + CircuitGateCount - 1) % CircuitGateCount);
        const double vx = gate.X - previous.X;
        const double vy = gate.Y - previous.Y;
        const double length = std::hypot(vx, vy);
        if (!(length > 1.0) || !std::isfinite(length))
        {
            Reset();
            return {{}, false, 0.0};
        }
        const double dx = vx / length;
        const double dy = vy / length;
        const double nx = -dy;
        const double ny = dx;
        const double toGate =
            (gate.X - s.Rival.X) * dx + (gate.Y - s.Rival.Y) * dy;
        const double rivalSide =
            (s.Rival.X - previous.X) * nx +
            (s.Rival.Y - previous.Y) * ny;
        const double playerSide =
            (s.Player.X - previous.X) * nx +
            (s.Player.Y - previous.Y) * ny;
        const double playerAhead =
            (s.Player.X - s.Rival.X) * dx +
            (s.Player.Y - s.Rival.Y) * dy;
        const double rivalForward =
            s.RivalVelocityX * dx + s.RivalVelocityY * dy;
        const double playerForward =
            s.PlayerVelocityX * dx + s.PlayerVelocityY * dy;
        const double playerSideVelocity =
            s.PlayerVelocityX * nx + s.PlayerVelocityY * ny;
        const double rivalSideVelocity =
            s.RivalVelocityX * nx + s.RivalVelocityY * ny;
        const double headingAlignment =
            std::cos(s.RivalHeadingRadians) * dx +
            std::sin(s.RivalHeadingRadians) * dy;
        if (!std::isfinite(toGate) || !std::isfinite(rivalSide) ||
            !std::isfinite(playerSide) || !std::isfinite(playerAhead) ||
            !std::isfinite(rivalForward) || !std::isfinite(playerForward) ||
            !std::isfinite(headingAlignment))
        {
            Reset();
            return {{}, false, 0.0};
        }

        // No weaving near turns or through unsupported routes. The pass
        // does not persist across a gate, restart, focus interruption
        // or a player that drives onto the passing side.
        if (ActiveGate != -1 && ActiveGate != s.NextGate)
        {
            Reset();
        }
        if (length < MinimumPassingStraightLengthCm ||
            toGate < MinimumCornerClearanceCm ||
            toGate > length + 200.0 ||
            !IsOnPavedSurface(s.Rival) ||
            !IsOnPavedSurface(s.Player) ||
            std::abs(rivalSide) > 540.0 ||
            std::abs(playerSide) > 660.0 ||
            headingAlignment < 0.94 ||
            rivalForward < 850.0 ||
            playerForward < 250.0 ||
            std::abs(playerSideVelocity) > 300.0 ||
            std::abs(rivalSideVelocity) > 600.0)
        {
            Reset();
            return {command, false, 0.0};
        }

        if (!IsActive())
        {
            const double closingSpeed = rivalForward - playerForward;
            const double emergencyRunway = std::max(
                0.0, playerAhead - SafetySeparationCm);
            // A heading change cannot be instantaneous on the actual
            // rigid-body chassis. Reject the old 900-1600cm start band:
            // it could activate the pass only to hit the downstream
            // emergency traffic brake before moving out of the lane.
            if (std::abs(rivalSide) > 175.0 ||
                std::abs(playerSide) > 330.0 ||
                playerAhead < StartAheadMinimumCm ||
                playerAhead > StartAheadMaximumCm ||
                toGate < MinimumEntryCornerRunwayCm ||
                closingSpeed < 300.0 ||
                closingSpeed > MaximumEntryClosingSpeedCmPerSecond ||
                emergencyRunway / closingSpeed <
                    MinimumEntryContactTimeSeconds)
            {
                return {command, false, 0.0};
            }
            // Choose the side OPPOSITE the player's measured location
            // on the road. Tie goes right. Do not randomly weave.
            LaneSign = playerSide >= 0.0 ? -1.0 : 1.0;
            ActiveGate = s.NextGate;
            Elapsed = 0.0;
        }

        Elapsed += s.DeltaSeconds;
        if (playerAhead <= -FinishBehindClearanceCm ||
            Elapsed >= MaximumPassDurationSeconds ||
            playerAhead > StartAheadMaximumCm + 400.0)
        {
            Reset();
            return {command, false, 0.0};
        }

        const double selectedLane = LaneSign * LaneOffsetCm;
        if (std::abs(playerSide - selectedLane) < 310.0 &&
            playerAhead > -200.0 && playerAhead < 1250.0)
        {
            // Player drifted across the selected passing lane. Yield
            // rather than commanding a speculative collision path.
            Reset();
            command.Throttle = 0.0;
            command.Brake = true;
            return {command, false, 0.0};
        }

        // A local aim point, 1000cm ahead within the fixed existing
        // paved straight, provides an actual steering change; only
        // changing the far run-through gate target barely turns a car.
        const CircuitPoint aim{
            s.Rival.X + 1000.0 * dx + (selectedLane - rivalSide) * nx,
            s.Rival.Y + 1000.0 * dy + (selectedLane - rivalSide) * ny
        };
        if (!IsOnPavedSurface(aim))
        {
            Reset();
            return {command, false, 0.0};
        }
        const double desiredHeading =
            std::atan2(aim.Y - s.Rival.Y, aim.X - s.Rival.X);
        const double error = std::atan2(
            std::sin(desiredHeading - s.RivalHeadingRadians),
            std::cos(desiredHeading - s.RivalHeadingRadians));
        if (!std::isfinite(error))
        {
            Reset();
            return {{}, false, 0.0};
        }
        command.Steering = std::clamp(error / 0.82, -0.65, 0.65);
        // No extra throttle, braking removal, saved race authority or
        // physical transforms. Existing traffic yields until clear.
        return {command, true, selectedLane};
    }

private:
    int ActiveGate = -1;
    double LaneSign = 0.0;
    double Elapsed = 0.0;
};
} // namespace latebrake
