#pragma once

#include "AI/RivalPilot.h"
#include "Track/WorldScaleRacePacePlan.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// FUTURE-CIRCUIT advisory controller, built only from the candidate
// 1.70-km physical-road source plan and actual measured car pose.
// No actor, Chaos force, controller possession, checkpoints or saves.
// It must NOT be activated by the original four-strip GameMode.
inline constexpr double WorldScalePilotMaxFrameSeconds = 0.10;
inline constexpr double WorldScalePilotMaxObservedSpeedCmps = 10000.0;
inline constexpr double WorldScalePilotMinLookaheadCm = 650.0;
inline constexpr double WorldScalePilotMaxLookaheadCm = 1700.0;
inline constexpr double WorldScalePilotMaxDiscontinuitySlackCm = 500.0;
inline constexpr double WorldScalePilotRadiansPi = 3.14159265358979323846;

struct WorldScalePilotSample final
{
    CircuitPoint Position{};
    double HeadingRadians = 0.0; // real body forward yaw, not camera yaw
    double FlatSpeedCmps = 0.0; // measured Chaos horizontal speed magnitude
    double ForwardSpeedCmps = 0.0; // signed true body-forward velocity
    double DeltaSeconds = 0.0;
    bool PhysicsReady = false;
    bool RaceGreen = false;
};

struct WorldScalePilotDecision final
{
    bool Valid = false;
    PilotCommand Command{};  // default fail-safe is brake with zero throttle
    double WrappedLapDistanceCm = 0.0;
    double SignedCrossTrackCm = 0.0;
    double LookaheadCm = 0.0;
    double SpeedReferenceCmps = 0.0;
    double HeadingErrorRadians = 0.0;
};

// One per physical rival / race attempt. State only prevents a teleport or
// arbitrary waypoint jump from being mistaken for continuous travel. This
// is NOT official checkpoint progress or anti-cheat authentication.
class WorldScaleCandidateRivalPilot final
{
public:
    void Reset() noexcept
    {
        HasPrevious = false;
        LastPosition = {};
        LastLapDistanceCm = 0.0;
    }

    WorldScalePilotDecision Step(
        const WorldScalePacePlan& plan,
        const WorldScalePilotSample& sample) noexcept
    {
        if (!plan.Valid || !sample.PhysicsReady || !sample.RaceGreen ||
            !std::isfinite(sample.Position.X) ||
            !std::isfinite(sample.Position.Y) ||
            !std::isfinite(sample.HeadingRadians) ||
            std::abs(sample.HeadingRadians) > 1000000.0 ||
            !std::isfinite(sample.FlatSpeedCmps) ||
            sample.FlatSpeedCmps < 0.0 ||
            sample.FlatSpeedCmps > WorldScalePilotMaxObservedSpeedCmps ||
            !std::isfinite(sample.ForwardSpeedCmps) ||
            std::abs(sample.ForwardSpeedCmps) >
                sample.FlatSpeedCmps + 0.1 ||
            !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > WorldScalePilotMaxFrameSeconds)
        {
            Reset();
            return {};
        }

        // Never read old rectangular pavement or treat nearest centerline
        // distance as certified tire contact. The planned 9-m road union is
        // a necessary (but insufficient for Unreal physics) eligibility.
        const auto road = ProjectWorldScaleRoute(plan, sample.Position);
        if (!road.Valid || !road.InsidePhysicalCandidateRoad)
        {
            Reset();
            return {};
        }

        if (HasPrevious)
        {
            const double dx = sample.Position.X - LastPosition.X;
            const double dy = sample.Position.Y - LastPosition.Y;
            const double movedCm = std::hypot(dx, dy);
            const double maxTravelCm = std::max(
                WorldScalePilotMaxDiscontinuitySlackCm,
                2.0 * sample.FlatSpeedCmps * sample.DeltaSeconds +
                    WorldScalePilotMaxDiscontinuitySlackCm);
            double lapDelta = std::remainder(
                road.LapDistanceCm - LastLapDistanceCm,
                WorldScaleCircuitLapCm);
            if (!std::isfinite(movedCm) || !std::isfinite(lapDelta) ||
                movedCm > maxTravelCm ||
                std::abs(lapDelta) > maxTravelCm +
                    WorldScalePaceStepCm)
            {
                Reset();
                return {};
            }
        }

        const double lookahead = std::clamp(
            450.0 + 0.22 * sample.FlatSpeedCmps,
            WorldScalePilotMinLookaheadCm, WorldScalePilotMaxLookaheadCm);
        const auto target = SampleWorldScalePace(
            plan, road.LapDistanceCm + lookahead);
        if (!target.Valid)
        {
            Reset();
            return {};
        }

        const double dx = target.Pose.Position.X - sample.Position.X;
        const double dy = target.Pose.Position.Y - sample.Position.Y;
        if (!std::isfinite(dx) || !std::isfinite(dy) ||
            std::hypot(dx, dy) < 1.0)
        {
            Reset();
            return {};
        }
        const double desiredHeading = std::atan2(dy, dx);
        const double error = std::remainder(
            desiredHeading - sample.HeadingRadians,
            2.0 * WorldScalePilotRadiansPi);
        const double steer = std::clamp(error / 0.65, -1.0, 1.0);
        const double speedLimit = std::min(
            road.SuggestedSpeedCmps, target.TargetSpeedCmps);
        if (!std::isfinite(error) || !std::isfinite(steer) ||
            !std::isfinite(speedLimit) ||
            speedLimit < WorldScalePaceMinSpeedCmps ||
            speedLimit > WorldScalePaceMaxSpeedCmps)
        {
            Reset();
            return {};
        }

        // A car facing backwards, spinning or exceeding a curvature pace
        // reference must not be given throttle. This is advisory only;
        // authentic collision braking authority remains with Unreal.
        const bool backwards = sample.ForwardSpeedCmps < -50.0;
        // Never accelerate a stationary car facing the wrong direction.
        // Higher-speed moderate misalignment also needs a brake demand;
        // future Unreal recovery must handle a blocked zero-speed rival.
        const bool badlyMisaligned = std::abs(error) > 2.2 ||
            (std::abs(error) > 0.8 && sample.FlatSpeedCmps > 1300.0);
        const bool brake = backwards || badlyMisaligned ||
                           sample.FlatSpeedCmps > speedLimit + 120.0;
        const double throttle = !brake &&
            sample.FlatSpeedCmps + 120.0 < speedLimit ? 0.85 : 0.0;

        // Save only independently accepted same-session source evidence;
        // never expose it as a verified gate/lap or race timing update.
        HasPrevious = true;
        LastPosition = sample.Position;
        LastLapDistanceCm = road.LapDistanceCm;
        return {
            true, {throttle, steer, brake, false},
            road.LapDistanceCm, road.SignedLateralCm,
            lookahead, speedLimit, error
        };
    }

private:
    bool HasPrevious = false;
    CircuitPoint LastPosition{};
    double LastLapDistanceCm = 0.0;
};
} // namespace latebrake
