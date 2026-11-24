#pragma once
#include "Track/WorldScaleRoundedCircuit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace latebrake
{
// FUTURE 1.70-km circuit only: geometric speed reference, NOT live AI,
// tire grip, Chaos physics, record authority or a qualified game.
inline constexpr unsigned WorldScalePaceStations = 512;
inline constexpr double WorldScalePaceStepCm =
    WorldScaleCircuitLapCm / WorldScalePaceStations;
inline constexpr double WorldScalePaceMaxSpeedCmps = 6000.0;
inline constexpr double WorldScalePaceMinSpeedCmps = 600.0;
inline constexpr double WorldScalePaceLateralAccelCmps2 = 850.0;
inline constexpr double WorldScalePaceBrakeAccelCmps2 = 850.0;
inline constexpr double WorldScalePaceDriveAccelCmps2 = 380.0;
inline constexpr double WorldScalePaceRadPerDegree =
    0.01745329251994329576923690768489;

struct WorldScalePaceNode final
{
    CircuitPoint Position{};
    double DistanceCm = 0.0;
    double CurvaturePerCm = 0.0;
    double LocalLimitCmps = 0.0;
    double TargetCmps = 0.0;
};
struct WorldScalePacePlan final
{
    bool Valid = false;
    std::array<WorldScalePaceNode, WorldScalePaceStations> Nodes{};
    double EstimatedLapSeconds = 0.0; // NOT a race time or record
};

inline double WorldScaleSampleCurvature(
    double distanceCm, double halfWindowCm) noexcept
{
    if (!std::isfinite(distanceCm) || !std::isfinite(halfWindowCm) ||
        halfWindowCm <= 0.0 || halfWindowCm > 1000.0)
        return std::numeric_limits<double>::quiet_NaN();
    const auto prev = WorldScaleCenterlineAt(distanceCm - halfWindowCm);
    const auto next = WorldScaleCenterlineAt(distanceCm + halfWindowCm);
    if (!prev.Valid || !next.Valid)
        return std::numeric_limits<double>::quiet_NaN();
    const double turn = std::remainder(
        next.Pose.HeadingDegrees - prev.Pose.HeadingDegrees, 360.0);
    return turn * WorldScalePaceRadPerDegree / (2.0 * halfWindowCm);
}

// Compile once or build at circuit load, never regenerate each racing Tick.
// Speed^2 propagation enforces brakeability BEFORE turns and limits
// acceleration on exit. Sweeps explicitly traverse the closed-lap seam.
inline WorldScalePacePlan BuildWorldScalePacePlan() noexcept
{
    WorldScalePacePlan plan{};
    std::array<double, WorldScalePaceStations> speedSquared{};
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const double s = i * WorldScalePaceStepCm;
        const auto pose = WorldScaleCenterlineAt(s);
        const double kappa = std::abs(WorldScaleSampleCurvature(
            s, WorldScalePaceStepCm * 0.5));
        if (!pose.Valid || !std::isfinite(kappa) || kappa > 0.001 ||
            !std::isfinite(pose.Pose.Position.X) ||
            !std::isfinite(pose.Pose.Position.Y)) return {};
        const double localLimit =
            kappa <= 1.0e-12 ? WorldScalePaceMaxSpeedCmps :
            std::min(WorldScalePaceMaxSpeedCmps,
                     std::sqrt(WorldScalePaceLateralAccelCmps2 / kappa));
        if (!std::isfinite(localLimit) ||
            localLimit < WorldScalePaceMinSpeedCmps) return {};
        plan.Nodes[i] = {pose.Pose.Position, s, kappa,
                         localLimit, localLimit};
        speedSquared[i] = localLimit * localLimit;
    }
    const double maxBrakingDelta =
        2.0 * WorldScalePaceBrakeAccelCmps2 * WorldScalePaceStepCm;
    const double maxAccelerationDelta =
        2.0 * WorldScalePaceDriveAccelCmps2 * WorldScalePaceStepCm;
    // Repeated relaxation can only lower speeds. Four full loops account
    // for circular dependence even when the start-line lies in a bend.
    for (unsigned pass = 0; pass < 4; ++pass)
    {
        for (unsigned i = WorldScalePaceStations; i-- > 0;)
            speedSquared[i] = std::min(speedSquared[i],
                speedSquared[(i + 1) % WorldScalePaceStations] +
                    maxBrakingDelta);
        for (unsigned i = 0; i < WorldScalePaceStations; ++i)
            speedSquared[(i + 1) % WorldScalePaceStations] = std::min(
                speedSquared[(i + 1) % WorldScalePaceStations],
                speedSquared[i] + maxAccelerationDelta);
    }
    double seconds = 0.0;
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const unsigned next = (i + 1) % WorldScalePaceStations;
        const double v2 = speedSquared[i];
        if (!std::isfinite(v2) ||
            v2 < WorldScalePaceMinSpeedCmps * WorldScalePaceMinSpeedCmps ||
            v2 > speedSquared[next] + maxBrakingDelta + 0.01 ||
            speedSquared[next] > v2 + maxAccelerationDelta + 0.01 ||
            v2 > plan.Nodes[i].LocalLimitCmps *
                    plan.Nodes[i].LocalLimitCmps + 0.01)
            return {};
        plan.Nodes[i].TargetCmps = std::sqrt(v2);
        seconds += 2.0 * WorldScalePaceStepCm /
            (plan.Nodes[i].TargetCmps + std::sqrt(speedSquared[next]));
    }
    if (!std::isfinite(seconds) || seconds < 1.0 || seconds > 1200.0)
        return {};
    plan.EstimatedLapSeconds = seconds;
    plan.Valid = true;
    return plan;
}

struct WorldScalePaceReference final
{
    bool Valid = false;
    CircuitPose Pose{};
    double WrappedDistanceCm = 0.0;
    double TargetSpeedCmps = 0.0;
};

inline WorldScalePaceReference SampleWorldScalePace(
    const WorldScalePacePlan& plan, double s) noexcept
{
    if (!plan.Valid || !std::isfinite(s)) return {};
    double wrapped = std::fmod(s, WorldScaleCircuitLapCm);
    if (wrapped < 0.0) wrapped += WorldScaleCircuitLapCm;
    const unsigned i = std::min(
        static_cast<unsigned>(wrapped / WorldScalePaceStepCm),
        WorldScalePaceStations - 1);
    const auto pose = WorldScaleCenterlineAt(wrapped);
    if (!pose.Valid) return {};
    // Interpolate squared speed between adjacent fully qualified nodes.
    // This yields constant-distance acceleration within each interval and
    // stays CONTINUOUS at node boundaries and across the lap seam. Taking
    // the lower endpoint instead creates a false speed jump at each node,
    // causing repeated-lap inputs to depend on floating-point fmod jitter.
    const unsigned next = (i + 1) % WorldScalePaceStations;
    const double fraction = std::clamp(
        (wrapped - i * WorldScalePaceStepCm) /
        WorldScalePaceStepCm, 0.0, 1.0);
    const double startSquared =
        plan.Nodes[i].TargetCmps * plan.Nodes[i].TargetCmps;
    const double endSquared =
        plan.Nodes[next].TargetCmps * plan.Nodes[next].TargetCmps;
    const double speed = std::sqrt(
        startSquared + fraction * (endSquared - startSquared));
    if (!std::isfinite(speed)) return {};
    return {true, pose.Pose, wrapped, speed};
}

inline constexpr double WorldScaleProjectionMaxDistanceCm = 1500.0;
struct WorldScaleRouteProjection final
{
    bool Valid = false;
    double LapDistanceCm = 0.0;
    double SignedLateralCm = 0.0;
    double CentrelineSeparationCm = 0.0;
    bool InsidePhysicalCandidateRoad = false;
    double SuggestedSpeedCmps = 0.0;
};

// Source-only nearest segment geometry; DO NOT use as checkpoint or
// physics-road truth. O(512) lookup requires UE profiling before Tick use.
inline WorldScaleRouteProjection ProjectWorldScaleRoute(
    const WorldScalePacePlan& plan, CircuitPoint car) noexcept
{
    if (!plan.Valid || !std::isfinite(car.X) || !std::isfinite(car.Y) ||
        std::abs(car.X) > 1000000.0 || std::abs(car.Y) > 1000000.0)
        return {};
    double least = std::numeric_limits<double>::infinity();
    unsigned bestIndex = 0;
    double fraction = 0.0;
    double signedLateral = 0.0;
    for (unsigned i = 0; i < WorldScalePaceStations; ++i)
    {
        const auto a = plan.Nodes[i].Position;
        const auto b = plan.Nodes[(i + 1) % WorldScalePaceStations].Position;
        const double dx = b.X - a.X;
        const double dy = b.Y - a.Y;
        const double norm2 = dx * dx + dy * dy;
        if (!std::isfinite(norm2) || norm2 <= 0.0) return {};
        const double u = std::clamp(
            ((car.X - a.X) * dx + (car.Y - a.Y) * dy) / norm2, 0.0, 1.0);
        const double ex = car.X - (a.X + u * dx);
        const double ey = car.Y - (a.Y + u * dy);
        const double d2 = ex * ex + ey * ey;
        if (!std::isfinite(d2)) return {};
        if (d2 < least)
        {
            least = d2;
            bestIndex = i;
            fraction = u;
            signedLateral =
                (dx * (car.Y - a.Y) - dy * (car.X - a.X)) /
                std::sqrt(norm2);
        }
    }
    if (!std::isfinite(least) ||
        least > WorldScaleProjectionMaxDistanceCm *
                WorldScaleProjectionMaxDistanceCm)
        return {};
    double along = (bestIndex + fraction) * WorldScalePaceStepCm;
    if (along >= WorldScaleCircuitLapCm) along -= WorldScaleCircuitLapCm;
    const auto reference = SampleWorldScalePace(plan, along);
    if (!reference.Valid || !std::isfinite(signedLateral)) return {};
    return {true, along, signedLateral, std::sqrt(least),
            IsOnWorldScaleRoad(car), reference.TargetSpeedCmps};
}
} // namespace latebrake
